/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#if ENABLE(CSS_FILTERS)

#import "PlatformCAFilters.h"

#import "BlockExceptions.h"
#import "FloatConversion.h"
#import "LengthFunctions.h" // This is a layering violation.
#import "PlatformCALayer.h"
#import <QuartzCore/QuartzCore.h>

using namespace WebCore;

#if USE_CA_FILTERS
struct CAColorMatrix
{
    float m11, m12, m13, m14, m15;
    float m21, m22, m23, m24, m25;
    float m31, m32, m33, m34, m35;
    float m41, m42, m43, m44, m45;
};

typedef struct CAColorMatrix CAColorMatrix;

@interface NSValue (Details)
+ (NSValue *)valueWithCAColorMatrix:(CAColorMatrix)t;
@end

@interface CAFilter : NSObject <NSCopying, NSMutableCopying, NSCoding>
@end

@interface CAFilter (Details)
@property(copy) NSString *name;
+ (CAFilter *)filterWithType:(NSString *)type;
@end

extern NSString * const kCAFilterColorMatrix;
extern NSString * const kCAFilterColorMonochrome;
extern NSString * const kCAFilterColorHueRotate;
extern NSString * const kCAFilterColorSaturate;
extern NSString * const kCAFilterGaussianBlur;
#endif

// FIXME: Should share these values with FilterEffectRenderer::build() (https://bugs.webkit.org/show_bug.cgi?id=76008).
static double sepiaFullConstants[3][3] = {
    { 0.393, 0.769, 0.189 },
    { 0.349, 0.686, 0.168 },
    { 0.272, 0.534, 0.131 }
};

static double sepiaNoneConstants[3][3] = {
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 }
};

void PlatformCAFilters::setFiltersOnLayer(PlatformCALayer* platformCALayer, const FilterOperations& filters)
{
    CALayer* layer = platformCALayer->platformLayer();

    if (!filters.size()) {
        BEGIN_BLOCK_OBJC_EXCEPTIONS
        [layer setFilters:nil];
        // FIXME: this adds shadow properties to the layer even when it had none.
        [layer setShadowOffset:CGSizeZero];
        [layer setShadowColor:nil];
        [layer setShadowRadius:0];
        [layer setShadowOpacity:0];
        END_BLOCK_OBJC_EXCEPTIONS
        return;
    }
    
    // Assume filtersCanBeComposited was called and it returned true.
    ASSERT(platformCALayer->filtersCanBeComposited(filters));
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS
    
    RetainPtr<NSMutableArray> array = adoptNS([[NSMutableArray alloc] init]);
    
    for (unsigned i = 0; i < filters.size(); ++i) {
        String filterName = String::format("filter_%d", i);
        const FilterOperation* filterOperation = filters.at(i);
        switch (filterOperation->getOperationType()) {
        case FilterOperation::DROP_SHADOW: {
            // FIXME: For now assume drop shadow is the last filter, put it on the layer.
            // <rdar://problem/10959969> Handle case where drop-shadow is not the last filter.
            const DropShadowFilterOperation* op = static_cast<const DropShadowFilterOperation*>(filterOperation);
            [layer setShadowOffset:CGSizeMake(op->x(), op->y())];

            CGFloat components[4];
            op->color().getRGBA(components[0], components[1], components[2], components[3]);
            RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
            RetainPtr<CGColorRef> color = adoptCF(CGColorCreate(colorSpace.get(), components));
            [layer setShadowColor:color.get()];
            [layer setShadowRadius:op->stdDeviation()];
            [layer setShadowOpacity:1];
            break;
        }
#if USE_CA_FILTERS
        case FilterOperation::GRAYSCALE: {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(filterOperation);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterColorMonochrome];
            [filter setValue:[NSNumber numberWithFloat:op->amount()] forKey:@"inputAmount"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::SEPIA: {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(filterOperation);
            RetainPtr<NSValue> colorMatrixValue = PlatformCAFilters::colorMatrixValueForFilter(*op);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterColorMatrix];
            [filter setValue:colorMatrixValue.get() forKey:@"inputColorMatrix"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::SATURATE: {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(filterOperation);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterColorSaturate];
            [filter setValue:[NSNumber numberWithFloat:op->amount()] forKey:@"inputAmount"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::HUE_ROTATE: {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(filterOperation);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterColorHueRotate];
            [filter setValue:[NSNumber numberWithFloat:deg2rad(op->amount())] forKey:@"inputAngle"];
            [filter setName:@"hueRotate"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::INVERT: {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(filterOperation);
            RetainPtr<NSValue> colorMatrixValue = PlatformCAFilters::colorMatrixValueForFilter(*op);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterColorMatrix];
            [filter setValue:colorMatrixValue.get() forKey:@"inputColorMatrix"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::OPACITY: {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(filterOperation);
            RetainPtr<NSValue> colorMatrixValue = PlatformCAFilters::colorMatrixValueForFilter(*op);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterColorMatrix];
            [filter setValue:colorMatrixValue.get() forKey:@"inputColorMatrix"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::BRIGHTNESS: {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(filterOperation);
            RetainPtr<NSValue> colorMatrixValue = PlatformCAFilters::colorMatrixValueForFilter(*op);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterColorMatrix];
            [filter setValue:colorMatrixValue.get() forKey:@"inputColorMatrix"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::CONTRAST: {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(filterOperation);
            RetainPtr<NSValue> colorMatrixValue = PlatformCAFilters::colorMatrixValueForFilter(*op);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterColorMatrix];
            [filter setValue:colorMatrixValue.get() forKey:@"inputColorMatrix"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::BLUR: {
            const BlurFilterOperation* op = static_cast<const BlurFilterOperation*>(filterOperation);
            CAFilter *filter = [CAFilter filterWithType:kCAFilterGaussianBlur];
            [filter setValue:[NSNumber numberWithFloat:floatValueForLength(op->stdDeviation(), 0)] forKey:@"inputRadius"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
#else
        case FilterOperation::GRAYSCALE: {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIColorMonochrome"];
            [filter setDefaults];
            [filter setValue:[NSNumber numberWithFloat:op->amount()] forKey:@"inputIntensity"];
            [filter setValue:[CIColor colorWithRed:0.67 green:0.67 blue:0.67] forKey:@"inputColor"]; // Color derived empirically to match zero saturation levels.
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::SEPIA: {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIColorMatrix"];
            [filter setDefaults];

            double t = op->amount();
            t = std::min(std::max(0.0, t), 1.0);
            // FIXME: results don't match the software filter.
            [filter setValue:[CIVector vectorWithX:WebCore::blend(sepiaNoneConstants[0][0], sepiaFullConstants[0][0], t)
                                                 Y:WebCore::blend(sepiaNoneConstants[0][1], sepiaFullConstants[0][1], t)
                                                 Z:WebCore::blend(sepiaNoneConstants[0][2], sepiaFullConstants[0][2], t) W:0] forKey:@"inputRVector"];
            [filter setValue:[CIVector vectorWithX:WebCore::blend(sepiaNoneConstants[1][0], sepiaFullConstants[1][0], t)
                                                 Y:WebCore::blend(sepiaNoneConstants[1][1], sepiaFullConstants[1][1], t)
                                                 Z:WebCore::blend(sepiaNoneConstants[1][2], sepiaFullConstants[1][2], t) W:0] forKey:@"inputGVector"];
            [filter setValue:[CIVector vectorWithX:WebCore::blend(sepiaNoneConstants[2][0], sepiaFullConstants[2][0], t)
                                                 Y:WebCore::blend(sepiaNoneConstants[2][1], sepiaFullConstants[2][1], t)
                                                 Z:WebCore::blend(sepiaNoneConstants[2][2], sepiaFullConstants[2][2], t) W:0] forKey:@"inputBVector"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::SATURATE: {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIColorControls"];
            [filter setDefaults];
            [filter setValue:[NSNumber numberWithFloat:op->amount()] forKey:@"inputSaturation"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::HUE_ROTATE: {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIHueAdjust"];
            [filter setDefaults];

            [filter setValue:[NSNumber numberWithFloat:deg2rad(op->amount())] forKey:@"inputAngle"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::INVERT: {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIColorMatrix"];
            [filter setDefaults];

            double multiplier = 1 - op->amount() * 2;

            // FIXME: the results of this filter look wrong.
            [filter setValue:[CIVector vectorWithX:multiplier Y:0 Z:0 W:0] forKey:@"inputRVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:multiplier Z:0 W:0] forKey:@"inputGVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:0 Z:multiplier W:0] forKey:@"inputBVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:0 Z:0 W:1] forKey:@"inputAVector"];
            [filter setValue:[CIVector vectorWithX:op->amount() Y:op->amount() Z:op->amount() W:0] forKey:@"inputBiasVector"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::OPACITY: {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIColorMatrix"];
            [filter setDefaults];

            [filter setValue:[CIVector vectorWithX:1 Y:0 Z:0 W:0] forKey:@"inputRVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:1 Z:0 W:0] forKey:@"inputGVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:0 Z:1 W:0] forKey:@"inputBVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:0 Z:0 W:op->amount()] forKey:@"inputAVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:0 Z:0 W:0] forKey:@"inputBiasVector"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::BRIGHTNESS: {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIColorMatrix"];
            [filter setDefaults];
            double amount = op->amount();

            [filter setValue:[CIVector vectorWithX:amount Y:0 Z:0 W:0] forKey:@"inputRVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:amount Z:0 W:0] forKey:@"inputGVector"];
            [filter setValue:[CIVector vectorWithX:0 Y:0 Z:amount W:0] forKey:@"inputBVector"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::CONTRAST: {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIColorControls"];
            [filter setDefaults];
            [filter setValue:[NSNumber numberWithFloat:op->amount()] forKey:@"inputContrast"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
        case FilterOperation::BLUR: {
            // FIXME: For now we ignore stdDeviationY.
            const BlurFilterOperation* op = static_cast<const BlurFilterOperation*>(filterOperation);
            CIFilter* filter = [CIFilter filterWithName:@"CIGaussianBlur"];
            [filter setDefaults];
            [filter setValue:[NSNumber numberWithFloat:floatValueForLength(op->stdDeviation(), 0)] forKey:@"inputRadius"];
            [filter setName:filterName];
            [array.get() addObject:filter];
            break;
        }
#endif
        case FilterOperation::PASSTHROUGH:
            break;
        default:
            ASSERT(0);
            break;
        }
    }

    if ([array.get() count] > 0)
        [layer setFilters:array.get()];
    
    END_BLOCK_OBJC_EXCEPTIONS
}

RetainPtr<NSValue> PlatformCAFilters::filterValueForOperation(const FilterOperation* operation, int internalFilterPropertyIndex)
{
#if USE_CA_FILTERS
    UNUSED_PARAM(internalFilterPropertyIndex);
#endif
    FilterOperation::OperationType type = operation->getOperationType();
    RetainPtr<id> value;
    
    switch (type) {
    case FilterOperation::GRAYSCALE: {
        // CIFilter: inputIntensity
        // CAFilter: inputAmount
        double amount = 0;
        if (!operation->isDefault()) {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
            amount = op->amount();
        }
        value = [NSNumber numberWithDouble:amount];
        break;
    }
    case FilterOperation::SEPIA: {
#if USE_CA_FILTERS
        // CAFilter: inputColorMatrix
        const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
        value = PlatformCAFilters::colorMatrixValueForFilter(*op);
#else
        // CIFilter: inputRVector, inputGVector, inputBVector
        double amount = 0;
        if (!operation->isDefault()) {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
            amount = op->amount();
        }

        CIVector* rowVector = 0;
        switch (internalFilterPropertyIndex) {
        case 0: rowVector = [[CIVector alloc] initWithX:WebCore::blend(sepiaNoneConstants[0][0], sepiaFullConstants[0][0], amount)
                                                      Y:WebCore::blend(sepiaNoneConstants[0][1], sepiaFullConstants[0][1], amount)
                                                      Z:WebCore::blend(sepiaNoneConstants[0][2], sepiaFullConstants[0][2], amount) W:0]; break; // inputRVector
        case 1: rowVector = [[CIVector alloc] initWithX:WebCore::blend(sepiaNoneConstants[1][0], sepiaFullConstants[1][0], amount)
                                                      Y:WebCore::blend(sepiaNoneConstants[1][1], sepiaFullConstants[1][1], amount)
                                                      Z:WebCore::blend(sepiaNoneConstants[1][2], sepiaFullConstants[1][2], amount) W:0]; break; // inputGVector
        case 2: rowVector = [[CIVector alloc] initWithX:WebCore::blend(sepiaNoneConstants[2][0], sepiaFullConstants[2][0], amount)
                                                      Y:WebCore::blend(sepiaNoneConstants[2][1], sepiaFullConstants[2][1], amount)
                                                      Z:WebCore::blend(sepiaNoneConstants[2][2], sepiaFullConstants[2][2], amount) W:0]; break; // inputBVector
        }
        value = adoptNS(rowVector);
#endif
        break;
    }
    case FilterOperation::SATURATE: {
        // CIFilter: inputSaturation
        // CAFilter: inputAmount
        double amount = 1;
        if (!operation->isDefault()) {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
            amount = op->amount();
        }
        
        value = [NSNumber numberWithDouble:amount];
        break;
    }
    case FilterOperation::HUE_ROTATE: {
        // Hue rotate CIFilter: inputAngle
        // Hue rotate CAFilter: inputAngle
        double amount = 0;
        if (!operation->isDefault()) {
            const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
            amount = op->amount();
        }
        amount = deg2rad(amount);
        value = [NSNumber numberWithDouble:amount];
        break;
    }
    case FilterOperation::INVERT: {
#if USE_CA_FILTERS
        // CAFilter: inputColorMatrix
        const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
        value = PlatformCAFilters::colorMatrixValueForFilter(*op);
#else
        // CIFilter: inputRVector, inputGVector, inputBVector, inputBiasVector
        double amount = 0;
        if (!operation->isDefault()) {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(operation);
            amount = op->amount();
        }

        double multiplier = 1 - amount * 2;

        // The color matrix animation for invert does a scale of each color component by a value that goes from 
        // 1 (when amount is 0) to -1 (when amount is 1). Then the color values are offset by amount. This has the
        // effect of performing the operation: c' = c * -1 + 1, which inverts the color.
        CIVector* rowVector = 0;
        switch (internalFilterPropertyIndex) {
        case 0: rowVector = [[CIVector alloc] initWithX:multiplier Y:0 Z:0 W:0]; break; // inputRVector
        case 1: rowVector = [[CIVector alloc] initWithX:0 Y:multiplier Z:0 W:0]; break; // inputGVector
        case 2: rowVector = [[CIVector alloc] initWithX:0 Y:0 Z:multiplier W:0]; break; // inputBVector
        case 3: rowVector = [[CIVector alloc] initWithX:amount Y:amount Z:amount W:0]; break; // inputBiasVector
        }
        value = adoptNS(rowVector);
#endif
        break;
    }
    case FilterOperation::OPACITY: {
#if USE_CA_FILTERS
        // Opacity CAFilter: inputColorMatrix
        const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
        value = PlatformCAFilters::colorMatrixValueForFilter(*op);
#else
        // Opacity CIFilter: inputAVector
        double amount = 1;
        if (!operation->isDefault()) {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(operation);
            amount = op->amount();
        }
        
        value = adoptNS([[CIVector alloc] initWithX:0 Y:0 Z:0 W:amount]);
#endif
        break;
    }
    
    case FilterOperation::BRIGHTNESS: {
#if USE_CA_FILTERS
        // Brightness CAFilter: inputColorMatrix
        const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
        value = PlatformCAFilters::colorMatrixValueForFilter(*op);
#else
        // Brightness CIFilter: inputColorMatrix
        double amount = 1;
        if (!operation->isDefault()) {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(operation);
            amount = op->amount();
        }
        
        CIVector* rowVector = 0;
        switch (internalFilterPropertyIndex) {
        case 0: rowVector = [[CIVector alloc] initWithX:amount Y:0 Z:0 W:0]; break; // inputRVector
        case 1: rowVector = [[CIVector alloc] initWithX:0 Y:amount Z:0 W:0]; break; // inputGVector
        case 2: rowVector = [[CIVector alloc] initWithX:0 Y:0 Z:amount W:0]; break; // inputBVector
        }
        value = adoptNS(rowVector);
#endif
        break;
    }

    case FilterOperation::CONTRAST: {
#if USE_CA_FILTERS
        // Contrast CAFilter: inputColorMatrix
        const BasicColorMatrixFilterOperation* op = static_cast<const BasicColorMatrixFilterOperation*>(operation);
        value = PlatformCAFilters::colorMatrixValueForFilter(*op);
#else
        // Contrast CIFilter: inputContrast
        double amount = 1;
        if (!operation->isDefault()) {
            const BasicComponentTransferFilterOperation* op = static_cast<const BasicComponentTransferFilterOperation*>(operation);
            amount = op->amount();
        }

        value = [NSNumber numberWithDouble:amount];
#endif
        break;
    }
    case FilterOperation::BLUR: {
        // CIFilter: inputRadius
        // CAFilter: inputRadius
        double amount = 0;

        if (!operation->isDefault()) {
            const BlurFilterOperation* op = static_cast<const BlurFilterOperation*>(operation);
            amount = floatValueForLength(op->stdDeviation(), 0);
        }
        
        value = [NSNumber numberWithDouble:amount];
        break;
    }
    default:
        break;
    }
    
    return value;
}

#if USE_CA_FILTERS
RetainPtr<NSValue> PlatformCAFilters::colorMatrixValueForFilter(const FilterOperation& filterOperation)
{
    switch (filterOperation.getOperationType()) {
    case FilterOperation::SEPIA: {
        const BasicColorMatrixFilterOperation& op = static_cast<const BasicColorMatrixFilterOperation&>(filterOperation);
        double t = op.amount();
        t = std::min(std::max(0.0, t), 1.0);
        CAColorMatrix colorMatrix = {
            static_cast<float>(WebCore::blend(sepiaNoneConstants[0][0], sepiaFullConstants[0][0], t)),
            static_cast<float>(WebCore::blend(sepiaNoneConstants[0][1], sepiaFullConstants[0][1], t)),
            static_cast<float>(WebCore::blend(sepiaNoneConstants[0][2], sepiaFullConstants[0][2], t)), 0, 0,
            
            static_cast<float>(WebCore::blend(sepiaNoneConstants[1][0], sepiaFullConstants[1][0], t)),
            static_cast<float>(WebCore::blend(sepiaNoneConstants[1][1], sepiaFullConstants[1][1], t)),
            static_cast<float>(WebCore::blend(sepiaNoneConstants[1][2], sepiaFullConstants[1][2], t)), 0, 0,
            
            static_cast<float>(WebCore::blend(sepiaNoneConstants[2][0], sepiaFullConstants[2][0], t)),
            static_cast<float>(WebCore::blend(sepiaNoneConstants[2][1], sepiaFullConstants[2][1], t)),
            static_cast<float>(WebCore::blend(sepiaNoneConstants[2][2], sepiaFullConstants[2][2], t)), 0, 0,
            0, 0, 0, 1, 0
        };
        return [NSValue valueWithCAColorMatrix:colorMatrix];
    }
    case FilterOperation::INVERT: {
        const BasicComponentTransferFilterOperation& op = static_cast<const BasicComponentTransferFilterOperation&>(filterOperation);
        float amount = op.amount();
        if (op.isDefault())
            amount = 0;

        float multiplier = 1 - amount * 2;
        CAColorMatrix colorMatrix = {
            multiplier, 0, 0, 0, amount,
            0, multiplier, 0, 0, amount,
            0, 0, multiplier, 0, amount,
            0, 0, 0, 1, 0
        };
        return [NSValue valueWithCAColorMatrix:colorMatrix];
    }
    case FilterOperation::OPACITY: {
        const BasicComponentTransferFilterOperation& op = static_cast<const BasicComponentTransferFilterOperation&>(filterOperation);
        float amount = op.amount();
        if (op.isDefault())
            amount = 1;

        CAColorMatrix colorMatrix = {
            1, 0, 0, 0, 0,
            0, 1, 0, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 0, amount, 0
        };
        return [NSValue valueWithCAColorMatrix:colorMatrix];
    }
    case FilterOperation::CONTRAST: {
        const BasicComponentTransferFilterOperation& op = static_cast<const BasicComponentTransferFilterOperation&>(filterOperation);
        float amount = op.amount();
        if (op.isDefault())
            amount = 1;

        float intercept = -0.5 * amount + 0.5;
        CAColorMatrix colorMatrix = {
            amount, 0, 0, 0, intercept,
            0, amount, 0, 0, intercept,
            0, 0, amount, 0, intercept,
            0, 0, 0, 1, 0
        };
        return [NSValue valueWithCAColorMatrix:colorMatrix];
    }
    case FilterOperation::BRIGHTNESS: {
        const BasicComponentTransferFilterOperation& op = static_cast<const BasicComponentTransferFilterOperation&>(filterOperation);
        float amount = op.amount();
        if (op.isDefault())
            amount = 1;

        CAColorMatrix colorMatrix = {
            amount, 0, 0, 0, 0,
            0, amount, 0, 0, 0,
            0, 0, amount, 0, 0,
            0, 0, 0, 1, 0
        };
        return [NSValue valueWithCAColorMatrix:colorMatrix];
    }
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}
#endif

int PlatformCAFilters::numAnimatedFilterProperties(FilterOperation::OperationType type)
{
#if USE_CA_FILTERS
    switch (type) {
    case FilterOperation::GRAYSCALE: return 1;
    case FilterOperation::SEPIA: return 1;
    case FilterOperation::SATURATE: return 1;
    case FilterOperation::HUE_ROTATE: return 1;
    case FilterOperation::INVERT: return 1;
    case FilterOperation::OPACITY: return 1;
    case FilterOperation::BRIGHTNESS: return 1;
    case FilterOperation::CONTRAST: return 1;
    case FilterOperation::BLUR: return 1;
    default: return 0;
    }
#else
    switch (type) {
    case FilterOperation::GRAYSCALE: return 1;
    case FilterOperation::SEPIA: return 3;
    case FilterOperation::SATURATE: return 1;
    case FilterOperation::HUE_ROTATE: return 1;
    case FilterOperation::INVERT: return 4;
    case FilterOperation::OPACITY: return 1;
    case FilterOperation::BRIGHTNESS: return 3;
    case FilterOperation::CONTRAST: return 1;
    case FilterOperation::BLUR: return 1;
    default: return 0;
    }
#endif
}

const char* PlatformCAFilters::animatedFilterPropertyName(FilterOperation::OperationType type, int internalFilterPropertyIndex)
{
#if USE_CA_FILTERS
    UNUSED_PARAM(internalFilterPropertyIndex);
    switch (type) {
    case FilterOperation::GRAYSCALE: return "inputAmount";
    case FilterOperation::SEPIA:return "inputColorMatrix";
    case FilterOperation::SATURATE: return "inputAmount";
    case FilterOperation::HUE_ROTATE: return "inputAngle";
    case FilterOperation::INVERT: return "inputColorMatrix";
    case FilterOperation::OPACITY: return "inputColorMatrix";
    case FilterOperation::BRIGHTNESS: return "inputColorMatrix";
    case FilterOperation::CONTRAST: return "inputColorMatrix";
    case FilterOperation::BLUR: return "inputRadius";
    default: return "";
    }
#else
    switch (type) {
    case FilterOperation::GRAYSCALE: return "inputIntensity";
    case FilterOperation::SEPIA:
    case FilterOperation::BRIGHTNESS:
        switch (internalFilterPropertyIndex) {
        case 0: return "inputRVector";
        case 1: return "inputGVector";
        case 2: return "inputBVector";
        default: return "";
        }
    case FilterOperation::SATURATE: return "inputSaturation";
    case FilterOperation::HUE_ROTATE: return "inputAngle";
    case FilterOperation::INVERT:
        switch (internalFilterPropertyIndex) {
        case 0: return "inputRVector";
        case 1: return "inputGVector";
        case 2: return "inputBVector";
        case 3: return "inputBiasVector";
        default: return "";
        }
    case FilterOperation::OPACITY: return "inputAVector";
    case FilterOperation::CONTRAST: return "inputContrast";
    case FilterOperation::BLUR: return "inputRadius";
    default: return "";
    }
#endif
}

#endif // ENABLE(CSS_FILTERS)
#endif // USE(ACCELERATED_COMPOSITING)
