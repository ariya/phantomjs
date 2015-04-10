/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "GraphicsLayer.h"

#include "FloatPoint.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "LayoutRect.h"
#include "RotateTransformOperation.h"
#include "TextStream.h"
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

namespace WebCore {

typedef HashMap<const GraphicsLayer*, Vector<FloatRect> > RepaintMap;
static RepaintMap& repaintRectMap()
{
    DEFINE_STATIC_LOCAL(RepaintMap, map, ());
    return map;
}

void KeyframeValueList::insert(PassOwnPtr<const AnimationValue> value)
{
    for (size_t i = 0; i < m_values.size(); ++i) {
        const AnimationValue* curValue = m_values[i].get();
        if (curValue->keyTime() == value->keyTime()) {
            ASSERT_NOT_REACHED();
            // insert after
            m_values.insert(i + 1, value);
            return;
        }
        if (curValue->keyTime() > value->keyTime()) {
            // insert before
            m_values.insert(i, value);
            return;
        }
    }
    
    m_values.append(value);
}

GraphicsLayer::GraphicsLayer(GraphicsLayerClient* client)
    : m_client(client)
    , m_anchorPoint(0.5f, 0.5f, 0)
    , m_opacity(1)
    , m_zPosition(0)
    , m_contentsOpaque(false)
    , m_preserves3D(false)
    , m_backfaceVisibility(true)
    , m_usingTiledBacking(false)
    , m_masksToBounds(false)
    , m_drawsContent(false)
    , m_contentsVisible(true)
    , m_acceleratesDrawing(false)
    , m_maintainsPixelAlignment(false)
    , m_appliesPageScale(false)
    , m_showDebugBorder(false)
    , m_showRepaintCounter(false)
    , m_paintingPhase(GraphicsLayerPaintAllWithOverflowClip)
    , m_contentsOrientation(CompositingCoordinatesTopDown)
    , m_parent(0)
    , m_maskLayer(0)
    , m_replicaLayer(0)
    , m_replicatedLayer(0)
    , m_repaintCount(0)
{
#ifndef NDEBUG
    if (m_client)
        m_client->verifyNotPainting();
#endif
}

GraphicsLayer::~GraphicsLayer()
{
    resetTrackedRepaints();
    ASSERT(!m_parent); // willBeDestroyed should have been called already.
}

void GraphicsLayer::willBeDestroyed()
{
#ifndef NDEBUG
    if (m_client)
        m_client->verifyNotPainting();
#endif

    if (m_replicaLayer)
        m_replicaLayer->setReplicatedLayer(0);

    if (m_replicatedLayer)
        m_replicatedLayer->setReplicatedByLayer(0);

    removeAllChildren();
    removeFromParent();
}

void GraphicsLayer::setParent(GraphicsLayer* layer)
{
    ASSERT(!layer || !layer->hasAncestor(this));
    m_parent = layer;
}

bool GraphicsLayer::hasAncestor(GraphicsLayer* ancestor) const
{
    for (GraphicsLayer* curr = parent(); curr; curr = curr->parent()) {
        if (curr == ancestor)
            return true;
    }
    
    return false;
}

bool GraphicsLayer::setChildren(const Vector<GraphicsLayer*>& newChildren)
{
    // If the contents of the arrays are the same, nothing to do.
    if (newChildren == m_children)
        return false;

    removeAllChildren();
    
    size_t listSize = newChildren.size();
    for (size_t i = 0; i < listSize; ++i)
        addChild(newChildren[i]);
    
    return true;
}

void GraphicsLayer::addChild(GraphicsLayer* childLayer)
{
    ASSERT(childLayer != this);
    
    if (childLayer->parent())
        childLayer->removeFromParent();

    childLayer->setParent(this);
    m_children.append(childLayer);
}

void GraphicsLayer::addChildAtIndex(GraphicsLayer* childLayer, int index)
{
    ASSERT(childLayer != this);

    if (childLayer->parent())
        childLayer->removeFromParent();

    childLayer->setParent(this);
    m_children.insert(index, childLayer);
}

void GraphicsLayer::addChildBelow(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    ASSERT(childLayer != this);
    childLayer->removeFromParent();

    bool found = false;
    for (unsigned i = 0; i < m_children.size(); i++) {
        if (sibling == m_children[i]) {
            m_children.insert(i, childLayer);
            found = true;
            break;
        }
    }

    childLayer->setParent(this);

    if (!found)
        m_children.append(childLayer);
}

void GraphicsLayer::addChildAbove(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    childLayer->removeFromParent();
    ASSERT(childLayer != this);

    bool found = false;
    for (unsigned i = 0; i < m_children.size(); i++) {
        if (sibling == m_children[i]) {
            m_children.insert(i+1, childLayer);
            found = true;
            break;
        }
    }

    childLayer->setParent(this);

    if (!found)
        m_children.append(childLayer);
}

bool GraphicsLayer::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    ASSERT(!newChild->parent());
    bool found = false;
    for (unsigned i = 0; i < m_children.size(); i++) {
        if (oldChild == m_children[i]) {
            m_children[i] = newChild;
            found = true;
            break;
        }
    }
    if (found) {
        oldChild->setParent(0);

        newChild->removeFromParent();
        newChild->setParent(this);
        return true;
    }
    return false;
}

void GraphicsLayer::removeAllChildren()
{
    while (m_children.size()) {
        GraphicsLayer* curLayer = m_children[0];
        ASSERT(curLayer->parent());
        curLayer->removeFromParent();
    }
}

void GraphicsLayer::removeFromParent()
{
    if (m_parent) {
        unsigned i;
        for (i = 0; i < m_parent->m_children.size(); i++) {
            if (this == m_parent->m_children[i]) {
                m_parent->m_children.remove(i);
                break;
            }
        }

        setParent(0);
    }
}

void GraphicsLayer::noteDeviceOrPageScaleFactorChangedIncludingDescendants()
{
    deviceOrPageScaleFactorChanged();

    if (m_maskLayer)
        m_maskLayer->deviceOrPageScaleFactorChanged();

    if (m_replicaLayer)
        m_replicaLayer->noteDeviceOrPageScaleFactorChangedIncludingDescendants();

    const Vector<GraphicsLayer*>& childLayers = children();
    size_t numChildren = childLayers.size();
    for (size_t i = 0; i < numChildren; ++i)
        childLayers[i]->noteDeviceOrPageScaleFactorChangedIncludingDescendants();
}

void GraphicsLayer::setReplicatedByLayer(GraphicsLayer* layer)
{
    if (m_replicaLayer == layer)
        return;

    if (m_replicaLayer)
        m_replicaLayer->setReplicatedLayer(0);

    if (layer)
        layer->setReplicatedLayer(this);

    m_replicaLayer = layer;
}

void GraphicsLayer::setOffsetFromRenderer(const IntSize& offset, ShouldSetNeedsDisplay shouldSetNeedsDisplay)
{
    if (offset == m_offsetFromRenderer)
        return;

    m_offsetFromRenderer = offset;

    // If the compositing layer offset changes, we need to repaint.
    if (shouldSetNeedsDisplay == SetNeedsDisplay)
        setNeedsDisplay();
}

void GraphicsLayer::setBackgroundColor(const Color& color)
{
    m_backgroundColor = color;
}

void GraphicsLayer::paintGraphicsLayerContents(GraphicsContext& context, const IntRect& clip)
{
    if (m_client) {
        IntSize offset = offsetFromRenderer();
        context.translate(-offset);

        IntRect clipRect(clip);
        clipRect.move(offset);

        m_client->paintContents(this, context, m_paintingPhase, clipRect);
    }
}

String GraphicsLayer::animationNameForTransition(AnimatedPropertyID property)
{
    // | is not a valid identifier character in CSS, so this can never conflict with a keyframe identifier.
    StringBuilder id;
    id.appendLiteral("-|transition");
    id.appendNumber(static_cast<int>(property));
    id.append('-');
    return id.toString();
}

void GraphicsLayer::suspendAnimations(double)
{
}

void GraphicsLayer::resumeAnimations()
{
}

void GraphicsLayer::getDebugBorderInfo(Color& color, float& width) const
{
    if (drawsContent()) {
        if (m_usingTiledBacking) {
            color = Color(255, 128, 0, 128); // tiled layer: orange
            width = 2;
            return;
        }

        color = Color(0, 128, 32, 128); // normal layer: green
        width = 2;
        return;
    }
    
    if (masksToBounds()) {
        color = Color(128, 255, 255, 48); // masking layer: pale blue
        width = 20;
        return;
    }
        
    color = Color(255, 255, 0, 192); // container: yellow
    width = 2;
}

void GraphicsLayer::updateDebugIndicators()
{
    if (!isShowingDebugBorder())
        return;

    Color borderColor;
    float width = 0;
    getDebugBorderInfo(borderColor, width);
    setDebugBorder(borderColor, width);
}

void GraphicsLayer::setZPosition(float position)
{
    m_zPosition = position;
}

float GraphicsLayer::accumulatedOpacity() const
{
    if (!preserves3D())
        return 1;
        
    return m_opacity * (parent() ? parent()->accumulatedOpacity() : 1);
}

void GraphicsLayer::distributeOpacity(float accumulatedOpacity)
{
    // If this is a transform layer we need to distribute our opacity to all our children
    
    // Incoming accumulatedOpacity is the contribution from our parent(s). We mutiply this by our own
    // opacity to get the total contribution
    accumulatedOpacity *= m_opacity;
    
    setOpacityInternal(accumulatedOpacity);
    
    if (preserves3D()) {
        size_t numChildren = children().size();
        for (size_t i = 0; i < numChildren; ++i)
            children()[i]->distributeOpacity(accumulatedOpacity);
    }
}

#if ENABLE(CSS_FILTERS)
static inline const FilterOperations& filterOperationsAt(const KeyframeValueList& valueList, size_t index)
{
    return static_cast<const FilterAnimationValue&>(valueList.at(index)).value();
}

int GraphicsLayer::validateFilterOperations(const KeyframeValueList& valueList)
{
    ASSERT(valueList.property() == AnimatedPropertyWebkitFilter);

    if (valueList.size() < 2)
        return -1;

    // Empty filters match anything, so find the first non-empty entry as the reference
    size_t firstIndex = 0;
    for ( ; firstIndex < valueList.size(); ++firstIndex) {
        if (!filterOperationsAt(valueList, firstIndex).operations().isEmpty())
            break;
    }

    if (firstIndex >= valueList.size())
        return -1;

    const FilterOperations& firstVal = filterOperationsAt(valueList, firstIndex);
    
    for (size_t i = firstIndex + 1; i < valueList.size(); ++i) {
        const FilterOperations& val = filterOperationsAt(valueList, i);
        
        // An emtpy filter list matches anything.
        if (val.operations().isEmpty())
            continue;
        
        if (!firstVal.operationsMatch(val))
            return -1;
    }
    
    return firstIndex;
}
#endif

// An "invalid" list is one whose functions don't match, and therefore has to be animated as a Matrix
// The hasBigRotation flag will always return false if isValid is false. Otherwise hasBigRotation is 
// true if the rotation between any two keyframes is >= 180 degrees.

static inline const TransformOperations& operationsAt(const KeyframeValueList& valueList, size_t index)
{
    return static_cast<const TransformAnimationValue&>(valueList.at(index)).value();
}

int GraphicsLayer::validateTransformOperations(const KeyframeValueList& valueList, bool& hasBigRotation)
{
    ASSERT(valueList.property() == AnimatedPropertyWebkitTransform);

    hasBigRotation = false;
    
    if (valueList.size() < 2)
        return -1;
    
    // Empty transforms match anything, so find the first non-empty entry as the reference.
    size_t firstIndex = 0;
    for ( ; firstIndex < valueList.size(); ++firstIndex) {
        if (!operationsAt(valueList, firstIndex).operations().isEmpty())
            break;
    }
    
    if (firstIndex >= valueList.size())
        return -1;
        
    const TransformOperations& firstVal = operationsAt(valueList, firstIndex);
    
    // See if the keyframes are valid.
    for (size_t i = firstIndex + 1; i < valueList.size(); ++i) {
        const TransformOperations& val = operationsAt(valueList, i);
        
        // An empty transform list matches anything.
        if (val.operations().isEmpty())
            continue;
            
        if (!firstVal.operationsMatch(val))
            return -1;
    }

    // Keyframes are valid, check for big rotations.    
    double lastRotAngle = 0.0;
    double maxRotAngle = -1.0;
        
    for (size_t j = 0; j < firstVal.operations().size(); ++j) {
        TransformOperation::OperationType type = firstVal.operations().at(j)->getOperationType();
        
        // if this is a rotation entry, we need to see if any angle differences are >= 180 deg
        if (type == TransformOperation::ROTATE_X ||
            type == TransformOperation::ROTATE_Y ||
            type == TransformOperation::ROTATE_Z ||
            type == TransformOperation::ROTATE_3D) {
            lastRotAngle = static_cast<RotateTransformOperation*>(firstVal.operations().at(j).get())->angle();
            
            if (maxRotAngle < 0)
                maxRotAngle = fabs(lastRotAngle);
            
            for (size_t i = firstIndex + 1; i < valueList.size(); ++i) {
                const TransformOperations& val = operationsAt(valueList, i);
                double rotAngle = val.operations().isEmpty() ? 0 : (static_cast<RotateTransformOperation*>(val.operations().at(j).get())->angle());
                double diffAngle = fabs(rotAngle - lastRotAngle);
                if (diffAngle > maxRotAngle)
                    maxRotAngle = diffAngle;
                lastRotAngle = rotAngle;
            }
        }
    }
    
    hasBigRotation = maxRotAngle >= 180.0;
    
    return firstIndex;
}

double GraphicsLayer::backingStoreMemoryEstimate() const
{
    if (!drawsContent())
        return 0;
    
    // Effects of page and device scale are ignored; subclasses should override to take these into account.
    return static_cast<double>(4 * size().width()) * size().height();
}

void GraphicsLayer::resetTrackedRepaints()
{
    repaintRectMap().remove(this);
}

void GraphicsLayer::addRepaintRect(const FloatRect& repaintRect)
{
    if (m_client->isTrackingRepaints()) {
        FloatRect largestRepaintRect(FloatPoint(), m_size);
        largestRepaintRect.intersect(repaintRect);
        RepaintMap::iterator repaintIt = repaintRectMap().find(this);
        if (repaintIt == repaintRectMap().end()) {
            Vector<FloatRect> repaintRects;
            repaintRects.append(largestRepaintRect);
            repaintRectMap().set(this, repaintRects);
        } else {
            Vector<FloatRect>& repaintRects = repaintIt->value;
            repaintRects.append(largestRepaintRect);
        }
    }
}

void GraphicsLayer::writeIndent(TextStream& ts, int indent)
{
    for (int i = 0; i != indent; ++i)
        ts << "  ";
}

void GraphicsLayer::dumpLayer(TextStream& ts, int indent, LayerTreeAsTextBehavior behavior) const
{
    writeIndent(ts, indent);
    ts << "(" << "GraphicsLayer";

    if (behavior & LayerTreeAsTextDebug) {
        ts << " " << static_cast<void*>(const_cast<GraphicsLayer*>(this));
        ts << " \"" << m_name << "\"";
    }

    ts << "\n";
    dumpProperties(ts, indent, behavior);
    writeIndent(ts, indent);
    ts << ")\n";
}

void GraphicsLayer::dumpProperties(TextStream& ts, int indent, LayerTreeAsTextBehavior behavior) const
{
    if (m_position != FloatPoint()) {
        writeIndent(ts, indent + 1);
        ts << "(position " << m_position.x() << " " << m_position.y() << ")\n";
    }

    if (m_boundsOrigin != FloatPoint()) {
        writeIndent(ts, indent + 1);
        ts << "(bounds origin " << m_boundsOrigin.x() << " " << m_boundsOrigin.y() << ")\n";
    }

    if (m_anchorPoint != FloatPoint3D(0.5f, 0.5f, 0)) {
        writeIndent(ts, indent + 1);
        ts << "(anchor " << m_anchorPoint.x() << " " << m_anchorPoint.y() << ")\n";
    }

    if (m_size != IntSize()) {
        writeIndent(ts, indent + 1);
        ts << "(bounds " << m_size.width() << " " << m_size.height() << ")\n";
    }

    if (m_opacity != 1) {
        writeIndent(ts, indent + 1);
        ts << "(opacity " << m_opacity << ")\n";
    }
    
    if (m_usingTiledBacking) {
        writeIndent(ts, indent + 1);
        ts << "(usingTiledLayer " << m_usingTiledBacking << ")\n";
    }

    if (m_contentsOpaque) {
        writeIndent(ts, indent + 1);
        ts << "(contentsOpaque " << m_contentsOpaque << ")\n";
    }

    if (m_preserves3D) {
        writeIndent(ts, indent + 1);
        ts << "(preserves3D " << m_preserves3D << ")\n";
    }

    if (m_drawsContent) {
        writeIndent(ts, indent + 1);
        ts << "(drawsContent " << m_drawsContent << ")\n";
    }

    if (!m_contentsVisible) {
        writeIndent(ts, indent + 1);
        ts << "(contentsVisible " << m_contentsVisible << ")\n";
    }

    if (!m_backfaceVisibility) {
        writeIndent(ts, indent + 1);
        ts << "(backfaceVisibility " << (m_backfaceVisibility ? "visible" : "hidden") << ")\n";
    }

    if (behavior & LayerTreeAsTextDebug) {
        writeIndent(ts, indent + 1);
        ts << "(";
        if (m_client)
            ts << "client " << static_cast<void*>(m_client);
        else
            ts << "no client";
        ts << ")\n";
    }

    if (m_backgroundColor.isValid()) {
        writeIndent(ts, indent + 1);
        ts << "(backgroundColor " << m_backgroundColor.nameForRenderTreeAsText() << ")\n";
    }

    if (!m_transform.isIdentity()) {
        writeIndent(ts, indent + 1);
        ts << "(transform ";
        ts << "[" << m_transform.m11() << " " << m_transform.m12() << " " << m_transform.m13() << " " << m_transform.m14() << "] ";
        ts << "[" << m_transform.m21() << " " << m_transform.m22() << " " << m_transform.m23() << " " << m_transform.m24() << "] ";
        ts << "[" << m_transform.m31() << " " << m_transform.m32() << " " << m_transform.m33() << " " << m_transform.m34() << "] ";
        ts << "[" << m_transform.m41() << " " << m_transform.m42() << " " << m_transform.m43() << " " << m_transform.m44() << "])\n";
    }

    // Avoid dumping the sublayer transform on the root layer, because it's used for geometry flipping, whose behavior
    // differs between platforms.
    if (parent() && !m_childrenTransform.isIdentity()) {
        writeIndent(ts, indent + 1);
        ts << "(childrenTransform ";
        ts << "[" << m_childrenTransform.m11() << " " << m_childrenTransform.m12() << " " << m_childrenTransform.m13() << " " << m_childrenTransform.m14() << "] ";
        ts << "[" << m_childrenTransform.m21() << " " << m_childrenTransform.m22() << " " << m_childrenTransform.m23() << " " << m_childrenTransform.m24() << "] ";
        ts << "[" << m_childrenTransform.m31() << " " << m_childrenTransform.m32() << " " << m_childrenTransform.m33() << " " << m_childrenTransform.m34() << "] ";
        ts << "[" << m_childrenTransform.m41() << " " << m_childrenTransform.m42() << " " << m_childrenTransform.m43() << " " << m_childrenTransform.m44() << "])\n";
    }

    if (m_replicaLayer) {
        writeIndent(ts, indent + 1);
        ts << "(replica layer";
        if (behavior & LayerTreeAsTextDebug)
            ts << " " << m_replicaLayer;
        ts << ")\n";
        m_replicaLayer->dumpLayer(ts, indent + 2, behavior);
    }

    if (m_replicatedLayer) {
        writeIndent(ts, indent + 1);
        ts << "(replicated layer";
        if (behavior & LayerTreeAsTextDebug)
            ts << " " << m_replicatedLayer;
        ts << ")\n";
    }

    if (behavior & LayerTreeAsTextIncludeRepaintRects && repaintRectMap().contains(this) && !repaintRectMap().get(this).isEmpty()) {
        writeIndent(ts, indent + 1);
        ts << "(repaint rects\n";
        for (size_t i = 0; i < repaintRectMap().get(this).size(); ++i) {
            if (repaintRectMap().get(this)[i].isEmpty())
                continue;
            writeIndent(ts, indent + 2);
            ts << "(rect ";
            ts << repaintRectMap().get(this)[i].x() << " ";
            ts << repaintRectMap().get(this)[i].y() << " ";
            ts << repaintRectMap().get(this)[i].width() << " ";
            ts << repaintRectMap().get(this)[i].height();
            ts << ")\n";
        }
        writeIndent(ts, indent + 1);
        ts << ")\n";
    }

    if (behavior & LayerTreeAsTextIncludePaintingPhases && paintingPhase()) {
        writeIndent(ts, indent + 1);
        ts << "(paintingPhases\n";
        if (paintingPhase() & GraphicsLayerPaintBackground) {
            writeIndent(ts, indent + 2);
            ts << "GraphicsLayerPaintBackground\n";
        }
        if (paintingPhase() & GraphicsLayerPaintForeground) {
            writeIndent(ts, indent + 2);
            ts << "GraphicsLayerPaintForeground\n";
        }
        if (paintingPhase() & GraphicsLayerPaintMask) {
            writeIndent(ts, indent + 2);
            ts << "GraphicsLayerPaintMask\n";
        }
        if (paintingPhase() & GraphicsLayerPaintOverflowContents) {
            writeIndent(ts, indent + 2);
            ts << "GraphicsLayerPaintOverflowContents\n";
        }
        if (paintingPhase() & GraphicsLayerPaintCompositedScroll) {
            writeIndent(ts, indent + 2);
            ts << "GraphicsLayerPaintCompositedScroll\n";
        }
        writeIndent(ts, indent + 1);
        ts << ")\n";
    }

    dumpAdditionalProperties(ts, indent, behavior);
    
    if (m_children.size()) {
        writeIndent(ts, indent + 1);
        ts << "(children " << m_children.size() << "\n";
        
        unsigned i;
        for (i = 0; i < m_children.size(); i++)
            m_children[i]->dumpLayer(ts, indent + 2, behavior);
        writeIndent(ts, indent + 1);
        ts << ")\n";
    }
}

String GraphicsLayer::layerTreeAsText(LayerTreeAsTextBehavior behavior) const
{
    TextStream ts;

    dumpLayer(ts, 0, behavior);
    return ts.release();
}

} // namespace WebCore

#ifndef NDEBUG
void showGraphicsLayerTree(const WebCore::GraphicsLayer* layer)
{
    if (!layer)
        return;

    String output = layer->layerTreeAsText(LayerTreeAsTextDebug | LayerTreeAsTextIncludeVisibleRects | LayerTreeAsTextIncludeTileCaches);
    fprintf(stderr, "%s\n", output.utf8().data());
}
#endif

#endif // USE(ACCELERATED_COMPOSITING)
