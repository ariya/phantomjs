/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#import "PlatformCALayer.h"

#import "AnimationUtilities.h"
#import "BlockExceptions.h"
#import "GraphicsContext.h"
#import "GraphicsLayerCA.h"
#import "LengthFunctions.h"
#import "PlatformCAFilters.h"
#import "SoftLinking.h"
#import "TiledBacking.h"
#import "WebLayer.h"
#import "WebTiledLayer.h"
#import "WebTiledBackingLayer.h"
#import <objc/objc-auto.h>
#import <objc/runtime.h>
#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/QuartzCore.h>
#import <wtf/CurrentTime.h>
#import <wtf/MathExtras.h>
#import <wtf/RetainPtr.h>

SOFT_LINK_FRAMEWORK_OPTIONAL(AVFoundation)
SOFT_LINK_CLASS(AVFoundation, AVPlayerLayer)

using std::min;
using std::max;

using namespace WebCore;

// This value must be the same as in PlatformCAAnimationMac.mm
static NSString * const WKNonZeroBeginTimeFlag = @"WKPlatformCAAnimationNonZeroBeginTimeFlag";

static double mediaTimeToCurrentTime(CFTimeInterval t)
{
    return WTF::currentTime() + t - CACurrentMediaTime();
}

// Delegate for animationDidStart callback
@interface WebAnimationDelegate : NSObject {
    PlatformCALayer* m_owner;
}

- (void)animationDidStart:(CAAnimation *)anim;
- (void)setOwner:(PlatformCALayer*)owner;

@end

@implementation WebAnimationDelegate

- (void)animationDidStart:(CAAnimation *)animation
{
    // hasNonZeroBeginTime is stored in a key in the animation
    bool hasNonZeroBeginTime = [[animation valueForKey:WKNonZeroBeginTimeFlag] boolValue];
    CFTimeInterval startTime;

    if (hasNonZeroBeginTime) {
        // We don't know what time CA used to commit the animation, so just use the current time
        // (even though this will be slightly off).
        startTime = mediaTimeToCurrentTime(CACurrentMediaTime());
    } else
        startTime = mediaTimeToCurrentTime([animation beginTime]);

    if (m_owner)
        m_owner->animationStarted(startTime);
}

- (void)setOwner:(PlatformCALayer*)owner
{
    m_owner = owner;
}

@end

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
@interface CATiledLayer(GraphicsLayerCAPrivate)
- (void)displayInRect:(CGRect)r levelOfDetail:(int)lod options:(NSDictionary *)dict;
- (BOOL)canDrawConcurrently;
- (void)setCanDrawConcurrently:(BOOL)flag;
@end
#endif

@interface CALayer(Private)
- (void)setContentsChanged;
#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
- (void)setAcceleratesDrawing:(BOOL)flag;
- (BOOL)acceleratesDrawing;
#endif
@end

static NSString * const platformCALayerPointer = @"WKPlatformCALayer";

bool PlatformCALayer::isValueFunctionSupported()
{
    static bool sHaveValueFunction = [CAPropertyAnimation instancesRespondToSelector:@selector(setValueFunction:)];
    return sHaveValueFunction;
}

void PlatformCALayer::setOwner(PlatformCALayerClient* owner)
{
    m_owner = owner;
    
    // Change the delegate's owner if needed
    if (m_delegate)
        [static_cast<WebAnimationDelegate*>(m_delegate.get()) setOwner:this];        
}

static NSDictionary* nullActionsDictionary()
{
    NSNull* nullValue = [NSNull null];
    NSDictionary* actions = [NSDictionary dictionaryWithObjectsAndKeys:
                             nullValue, @"anchorPoint",
                             nullValue, @"anchorPointZ",
                             nullValue, @"bounds",
                             nullValue, @"contents",
                             nullValue, @"contentsRect",
                             nullValue, @"opacity",
                             nullValue, @"position",
                             nullValue, @"shadowColor",
                             nullValue, @"sublayerTransform",
                             nullValue, @"sublayers",
                             nullValue, @"transform",
                             nullValue, @"zPosition",
                             nil];
    return actions;
}

static NSString* toCAFilterType(PlatformCALayer::FilterType type)
{
    switch (type) {
    case PlatformCALayer::Linear: return kCAFilterLinear;
    case PlatformCALayer::Nearest: return kCAFilterNearest;
    case PlatformCALayer::Trilinear: return kCAFilterTrilinear;
    default: return 0;
    }
}

PassRefPtr<PlatformCALayer> PlatformCALayer::create(LayerType layerType, PlatformCALayerClient* owner)
{
    return adoptRef(new PlatformCALayer(layerType, 0, owner));
}

PassRefPtr<PlatformCALayer> PlatformCALayer::create(void* platformLayer, PlatformCALayerClient* owner)
{
    return adoptRef(new PlatformCALayer(LayerTypeCustom, static_cast<PlatformLayer*>(platformLayer), owner));
}

PlatformCALayer::PlatformCALayer(LayerType layerType, PlatformLayer* layer, PlatformCALayerClient* owner)
    : m_owner(owner)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    if (layer) {
        if ([layer isKindOfClass:getAVPlayerLayerClass()])
            m_layerType = LayerTypeAVPlayerLayer;
        else
            m_layerType = LayerTypeCustom;
        m_layer = layer;
    } else {
        m_layerType = layerType;
    
        Class layerClass = Nil;
        switch (layerType) {
            case LayerTypeLayer:
            case LayerTypeRootLayer:
                layerClass = [CALayer class];
                break;
            case LayerTypeWebLayer:
                layerClass = [WebLayer class];
                break;
            case LayerTypeTransformLayer:
                layerClass = [CATransformLayer class];
                break;
            case LayerTypeWebTiledLayer:
                layerClass = [WebTiledLayer class];
                break;
            case LayerTypeTiledBackingLayer:
            case LayerTypePageTiledBackingLayer:
                layerClass = [WebTiledBackingLayer class];
                break;
            case LayerTypeAVPlayerLayer:
                layerClass = getAVPlayerLayerClass();
                break;
            case LayerTypeCustom:
                break;
        }

        if (layerClass)
            m_layer = adoptNS([[layerClass alloc] init]);
    }
    
    // Save a pointer to 'this' in the CALayer
    [m_layer.get() setValue:[NSValue valueWithPointer:this] forKey:platformCALayerPointer];
    
    // Clear all the implicit animations on the CALayer
    [m_layer.get() setStyle:[NSDictionary dictionaryWithObject:nullActionsDictionary() forKey:@"actions"]];
    
    // If this is a TiledLayer, set some initial values
    if (m_layerType == LayerTypeWebTiledLayer) {
        WebTiledLayer* tiledLayer = static_cast<WebTiledLayer*>(m_layer.get());
        [tiledLayer setTileSize:CGSizeMake(GraphicsLayerCA::kTiledLayerTileSize, GraphicsLayerCA::kTiledLayerTileSize)];
        [tiledLayer setLevelsOfDetail:1];
        [tiledLayer setLevelsOfDetailBias:0];
        [tiledLayer setContentsGravity:@"bottomLeft"];
    }
    
    if (usesTiledBackingLayer()) {
        m_customSublayers = adoptPtr(new PlatformCALayerList(1));
        CALayer* tileCacheTileContainerLayer = [static_cast<WebTiledBackingLayer *>(m_layer.get()) tileContainerLayer];
        (*m_customSublayers)[0] = PlatformCALayer::create(tileCacheTileContainerLayer, 0);
    }
    
    END_BLOCK_OBJC_EXCEPTIONS
}

PassRefPtr<PlatformCALayer> PlatformCALayer::clone(PlatformCALayerClient* owner) const
{
    LayerType type;
    switch (layerType()) {
    case LayerTypeTransformLayer:
        type = LayerTypeTransformLayer;
        break;
    case LayerTypeAVPlayerLayer:
        type = LayerTypeAVPlayerLayer;
        break;
    case LayerTypeLayer:
    default:
        type = LayerTypeLayer;
        break;
    };
    RefPtr<PlatformCALayer> newLayer = PlatformCALayer::create(type, owner);

    newLayer->setPosition(position());
    newLayer->setBounds(bounds());
    newLayer->setAnchorPoint(anchorPoint());
    newLayer->setTransform(transform());
    newLayer->setSublayerTransform(sublayerTransform());
    newLayer->setContents(contents());
    newLayer->setMasksToBounds(masksToBounds());
    newLayer->setDoubleSided(isDoubleSided());
    newLayer->setOpaque(isOpaque());
    newLayer->setBackgroundColor(backgroundColor());
    newLayer->setContentsScale(contentsScale());
#if ENABLE(CSS_FILTERS)
    newLayer->copyFiltersFrom(this);
#endif

    if (type == LayerTypeAVPlayerLayer) {
        AVPlayerLayer* destinationPlayerLayer = newLayer->playerLayer();
        AVPlayerLayer* sourcePlayerLayer = playerLayer();
        dispatch_async(dispatch_get_main_queue(), ^{
            [destinationPlayerLayer setPlayer:[sourcePlayerLayer player]];
        });
    }

    return newLayer;
}

PlatformCALayer::~PlatformCALayer()
{
    [m_layer.get() setValue:nil forKey:platformCALayerPointer];

    // Clear the owner, which also clears it in the delegate to prevent attempts 
    // to use the GraphicsLayerCA after it has been destroyed.
    setOwner(0);
    
    // Remove the owner pointer from the delegate in case there is a pending animationStarted event.
    [static_cast<WebAnimationDelegate*>(m_delegate.get()) setOwner:nil];

    if (usesTiledBackingLayer())
        [static_cast<WebTiledBackingLayer *>(m_layer.get()) invalidate];
}

PlatformCALayer* PlatformCALayer::platformCALayer(void* platformLayer)
{
    if (!platformLayer)
        return 0;
        
    // Pointer to PlatformCALayer is kept in a key of the CALayer
    PlatformCALayer* platformCALayer = nil;
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    platformCALayer = static_cast<PlatformCALayer*>([[static_cast<CALayer*>(platformLayer) valueForKey:platformCALayerPointer] pointerValue]);
    END_BLOCK_OBJC_EXCEPTIONS
    return platformCALayer;
}

PlatformLayer* PlatformCALayer::platformLayer() const
{
    return m_layer.get();
}

void PlatformCALayer::animationStarted(CFTimeInterval beginTime)
{
    if (m_owner)
        m_owner->platformCALayerAnimationStarted(beginTime);
}

void PlatformCALayer::setNeedsDisplay(const FloatRect* dirtyRect)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    if (dirtyRect)
        [m_layer.get() setNeedsDisplayInRect:*dirtyRect];
    else
        [m_layer.get() setNeedsDisplay];
    END_BLOCK_OBJC_EXCEPTIONS
}
    
void PlatformCALayer::setContentsChanged()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setContentsChanged];
    END_BLOCK_OBJC_EXCEPTIONS
}

PlatformCALayer* PlatformCALayer::superlayer() const
{
    return platformCALayer([m_layer.get() superlayer]);
}

void PlatformCALayer::removeFromSuperlayer()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() removeFromSuperlayer];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::setSublayers(const PlatformCALayerList& list)
{
    // Short circuiting here not only avoids the allocation of sublayers, but avoids <rdar://problem/7390716> (see below)
    if (list.size() == 0) {
        removeAllSublayers();
        return;
    }
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    NSMutableArray* sublayers = [[NSMutableArray alloc] init];
    for (size_t i = 0; i < list.size(); ++i)
        [sublayers addObject:list[i]->m_layer.get()];
        
    [m_layer.get() setSublayers:sublayers];
    [sublayers release];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::removeAllSublayers()
{
    // Workaround for <rdar://problem/7390716>: -[CALayer setSublayers:] crashes if sublayers is an empty array, or nil, under GC.
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    if (objc_collectingEnabled())
        while ([[m_layer.get() sublayers] count])
            [[[m_layer.get() sublayers] objectAtIndex:0] removeFromSuperlayer];
    else
        [m_layer.get() setSublayers:nil];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::appendSublayer(PlatformCALayer* layer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    ASSERT(m_layer != layer->m_layer);
    [m_layer.get() addSublayer:layer->m_layer.get()];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::insertSublayer(PlatformCALayer* layer, size_t index)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    ASSERT(m_layer != layer->m_layer);
    [m_layer.get() insertSublayer:layer->m_layer.get() atIndex:index];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::replaceSublayer(PlatformCALayer* reference, PlatformCALayer* layer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    ASSERT(m_layer != layer->m_layer);
    [m_layer.get() replaceSublayer:reference->m_layer.get() with:layer->m_layer.get()];
    END_BLOCK_OBJC_EXCEPTIONS
}

size_t PlatformCALayer::sublayerCount() const
{
    return [[m_layer.get() sublayers] count];
}

void PlatformCALayer::adoptSublayers(PlatformCALayer* source)
{
    // Workaround for <rdar://problem/7390716>: -[CALayer setSublayers:] crashes if sublayers is an empty array, or nil, under GC.
    NSArray* sublayers = [source->m_layer.get() sublayers];
    
    if (objc_collectingEnabled() && ![sublayers count]) {
        BEGIN_BLOCK_OBJC_EXCEPTIONS
        while ([[m_layer.get() sublayers] count])
            [[[m_layer.get() sublayers] objectAtIndex:0] removeFromSuperlayer];
        END_BLOCK_OBJC_EXCEPTIONS
        return;
    }
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setSublayers:sublayers];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::addAnimationForKey(const String& key, PlatformCAAnimation* animation)
{
    // Add the delegate
    if (!m_delegate) {
        WebAnimationDelegate* webAnimationDelegate = [[WebAnimationDelegate alloc] init];
        m_delegate = adoptNS(webAnimationDelegate);
        [webAnimationDelegate setOwner:this];
    }
    
    CAPropertyAnimation* propertyAnimation = static_cast<CAPropertyAnimation*>(animation->platformAnimation());
    if (![propertyAnimation delegate])
        [propertyAnimation setDelegate:static_cast<id>(m_delegate.get())];
     
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() addAnimation:animation->m_animation.get() forKey:key];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::removeAnimationForKey(const String& key)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() removeAnimationForKey:key];
    END_BLOCK_OBJC_EXCEPTIONS
}

PassRefPtr<PlatformCAAnimation> PlatformCALayer::animationForKey(const String& key)
{
    CAPropertyAnimation* propertyAnimation = static_cast<CAPropertyAnimation*>([m_layer.get() animationForKey:key]);
    if (!propertyAnimation)
        return 0;
    return PlatformCAAnimation::create(propertyAnimation);
}

PlatformCALayer* PlatformCALayer::mask() const
{
    return platformCALayer([m_layer.get() mask]);
}

void PlatformCALayer::setMask(PlatformCALayer* layer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setMask:layer ? layer->platformLayer() : 0];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayer::isOpaque() const
{
    return [m_layer.get() isOpaque];
}

void PlatformCALayer::setOpaque(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setOpaque:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

FloatRect PlatformCALayer::bounds() const
{
    return [m_layer.get() bounds];
}

void PlatformCALayer::setBounds(const FloatRect& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setBounds:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

FloatPoint3D PlatformCALayer::position() const
{
    CGPoint point = [m_layer.get() position];
    return FloatPoint3D(point.x, point.y, [m_layer.get() zPosition]);
}

void PlatformCALayer::setPosition(const FloatPoint3D& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setPosition:CGPointMake(value.x(), value.y())];
    [m_layer.get() setZPosition:value.z()];
    END_BLOCK_OBJC_EXCEPTIONS
}

FloatPoint3D PlatformCALayer::anchorPoint() const
{
    CGPoint point = [m_layer.get() anchorPoint];
    float z = 0;
    z = [m_layer.get() anchorPointZ];
    return FloatPoint3D(point.x, point.y, z);
}

void PlatformCALayer::setAnchorPoint(const FloatPoint3D& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setAnchorPoint:CGPointMake(value.x(), value.y())];
    [m_layer.get() setAnchorPointZ:value.z()];
    END_BLOCK_OBJC_EXCEPTIONS
}

TransformationMatrix PlatformCALayer::transform() const
{
    return [m_layer.get() transform];
}

void PlatformCALayer::setTransform(const TransformationMatrix& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setTransform:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

TransformationMatrix PlatformCALayer::sublayerTransform() const
{
    return [m_layer.get() sublayerTransform];
}

void PlatformCALayer::setSublayerTransform(const TransformationMatrix& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setSublayerTransform:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

TransformationMatrix PlatformCALayer::contentsTransform() const
{
    // FIXME: This function can be removed.
    return TransformationMatrix();
}

void PlatformCALayer::setContentsTransform(const TransformationMatrix& value)
{
    // FIXME: This function can be removed.
    UNUSED_PARAM(value);
}

bool PlatformCALayer::isHidden() const
{
    return [m_layer.get() isHidden];
}

void PlatformCALayer::setHidden(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setHidden:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayer::isGeometryFlipped() const
{
    return [m_layer.get() isGeometryFlipped];
}

void PlatformCALayer::setGeometryFlipped(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setGeometryFlipped:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayer::isDoubleSided() const
{
    return [m_layer.get() isDoubleSided];
}

void PlatformCALayer::setDoubleSided(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setDoubleSided:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayer::masksToBounds() const
{
    return [m_layer.get() masksToBounds];
}

void PlatformCALayer::setMasksToBounds(bool value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setMasksToBounds:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayer::acceleratesDrawing() const
{
#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    return [m_layer.get() acceleratesDrawing];
#else
    return false;
#endif
}

void PlatformCALayer::setAcceleratesDrawing(bool acceleratesDrawing)
{
#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setAcceleratesDrawing:acceleratesDrawing];
    END_BLOCK_OBJC_EXCEPTIONS
#else
    UNUSED_PARAM(acceleratesDrawing);
#endif
}

CFTypeRef PlatformCALayer::contents() const
{
    return [m_layer.get() contents];
}

void PlatformCALayer::setContents(CFTypeRef value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setContents:static_cast<id>(const_cast<void*>(value))];
    END_BLOCK_OBJC_EXCEPTIONS
}

FloatRect PlatformCALayer::contentsRect() const
{
    return [m_layer.get() contentsRect];
}

void PlatformCALayer::setContentsRect(const FloatRect& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setContentsRect:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::setMinificationFilter(FilterType value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setMinificationFilter:toCAFilterType(value)];
    END_BLOCK_OBJC_EXCEPTIONS
}

void PlatformCALayer::setMagnificationFilter(FilterType value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setMagnificationFilter:toCAFilterType(value)];
    END_BLOCK_OBJC_EXCEPTIONS
}

Color PlatformCALayer::backgroundColor() const
{
    return [m_layer.get() backgroundColor];
}

void PlatformCALayer::setBackgroundColor(const Color& value)
{
    CGFloat components[4];
    value.getRGBA(components[0], components[1], components[2], components[3]);

    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGColorRef> color = adoptCF(CGColorCreate(colorSpace.get(), components));

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setBackgroundColor:color.get()];
    END_BLOCK_OBJC_EXCEPTIONS
}

float PlatformCALayer::borderWidth() const
{
    return [m_layer.get() borderWidth];
}

void PlatformCALayer::setBorderWidth(float value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setBorderWidth:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

Color PlatformCALayer::borderColor() const
{
    return [m_layer.get() borderColor];
}

void PlatformCALayer::setBorderColor(const Color& value)
{
    CGFloat components[4];
    value.getRGBA(components[0], components[1], components[2], components[3]);

    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    RetainPtr<CGColorRef> color = adoptCF(CGColorCreate(colorSpace.get(), components));

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setBorderColor:color.get()];
    END_BLOCK_OBJC_EXCEPTIONS
}

float PlatformCALayer::opacity() const
{
    return [m_layer.get() opacity];
}

void PlatformCALayer::setOpacity(float value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setOpacity:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

#if ENABLE(CSS_FILTERS)
void PlatformCALayer::setFilters(const FilterOperations& filters)
{
    PlatformCAFilters::setFiltersOnLayer(this, filters);
}

void PlatformCALayer::copyFiltersFrom(const PlatformCALayer* sourceLayer)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setFilters:[sourceLayer->platformLayer() filters]];
    END_BLOCK_OBJC_EXCEPTIONS
}

bool PlatformCALayer::filtersCanBeComposited(const FilterOperations& filters)
{
    // Return false if there are no filters to avoid needless work
    if (!filters.size())
        return false;
        
    for (unsigned i = 0; i < filters.size(); ++i) {
        const FilterOperation* filterOperation = filters.at(i);
        switch (filterOperation->getOperationType()) {
        case FilterOperation::REFERENCE:
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
        case FilterOperation::VALIDATED_CUSTOM:
#endif
            return false;
        case FilterOperation::DROP_SHADOW:
            // FIXME: For now we can only handle drop-shadow is if it's last in the list
            if (i < (filters.size() - 1))
                return false;
            break;
        default:
            break;
        }
    }
    
    return true;
}
#endif

String PlatformCALayer::name() const
{
    return [m_layer.get() name];
}

void PlatformCALayer::setName(const String& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setName:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

FloatRect PlatformCALayer::frame() const
{
    return [m_layer.get() frame];
}

void PlatformCALayer::setFrame(const FloatRect& value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setFrame:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

float PlatformCALayer::speed() const
{
    return [m_layer.get() speed];
}

void PlatformCALayer::setSpeed(float value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setSpeed:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

CFTimeInterval PlatformCALayer::timeOffset() const
{
    return [m_layer.get() timeOffset];
}

void PlatformCALayer::setTimeOffset(CFTimeInterval value)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setTimeOffset:value];
    END_BLOCK_OBJC_EXCEPTIONS
}

float PlatformCALayer::contentsScale() const
{
#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    return [m_layer.get() contentsScale];
#else
    return 1;
#endif
}

void PlatformCALayer::setContentsScale(float value)
{
#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    [m_layer.get() setContentsScale:value];
    END_BLOCK_OBJC_EXCEPTIONS
#else
    UNUSED_PARAM(value);
#endif
}

TiledBacking* PlatformCALayer::tiledBacking()
{
    if (!usesTiledBackingLayer())
        return 0;

    WebTiledBackingLayer *tiledBackingLayer = static_cast<WebTiledBackingLayer *>(m_layer.get());
    return [tiledBackingLayer tiledBacking];
}

#if PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
void PlatformCALayer::synchronouslyDisplayTilesInRect(const FloatRect& rect)
{
    if (m_layerType != LayerTypeWebTiledLayer)
        return;

    WebTiledLayer *tiledLayer = static_cast<WebTiledLayer*>(m_layer.get());

    BEGIN_BLOCK_OBJC_EXCEPTIONS
    BOOL oldCanDrawConcurrently = [tiledLayer canDrawConcurrently];
    [tiledLayer setCanDrawConcurrently:NO];
    [tiledLayer displayInRect:rect levelOfDetail:0 options:nil];
    [tiledLayer setCanDrawConcurrently:oldCanDrawConcurrently];
    END_BLOCK_OBJC_EXCEPTIONS
}
#endif

AVPlayerLayer* PlatformCALayer::playerLayer() const
{
    ASSERT([m_layer.get() isKindOfClass:getAVPlayerLayerClass()]);
    return (AVPlayerLayer*)m_layer.get();
}

#endif // USE(ACCELERATED_COMPOSITING)
