/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "GraphicsLayerTextureMapper.h"

#include "TextureMapperNode.h"

namespace WebCore {

GraphicsLayerTextureMapper::GraphicsLayerTextureMapper(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_node(adoptPtr(new TextureMapperNode()))
    , m_changeMask(0)
{
}

void GraphicsLayerTextureMapper::notifyChange(TextureMapperNode::ChangeMask changeMask)
{
    m_changeMask |= changeMask;
    if (!client())
        return;
    client()->notifySyncRequired(this);
}

void GraphicsLayerTextureMapper::didSynchronize()
{
    m_syncQueued = false;
    m_changeMask = 0;
    m_pendingContent.needsDisplay = false;
    m_pendingContent.needsDisplayRect = IntRect();
}

void GraphicsLayerTextureMapper::setName(const String& name)
{
    GraphicsLayer::setName(name);
}

GraphicsLayerTextureMapper::~GraphicsLayerTextureMapper()
{
}

/* \reimp (GraphicsLayer.h): The current size might change, thus we need to update the whole display.
*/
void GraphicsLayerTextureMapper::setNeedsDisplay()
{
    m_pendingContent.needsDisplay = true;
    notifyChange(TextureMapperNode::DisplayChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setNeedsDisplayInRect(const FloatRect& rect)
{
    if (m_pendingContent.needsDisplay)
        return;
    m_pendingContent.needsDisplayRect.unite(IntRect(rect));
    notifyChange(TextureMapperNode::DisplayChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setParent(GraphicsLayer* layer)
{
    notifyChange(TextureMapperNode::ParentChange);
    GraphicsLayer::setParent(layer);
}

/* \reimp (GraphicsLayer.h)
*/
bool GraphicsLayerTextureMapper::setChildren(const Vector<GraphicsLayer*>& children)
{
    notifyChange(TextureMapperNode::ChildrenChange);
    return GraphicsLayer::setChildren(children);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChild(GraphicsLayer* layer)
{
    notifyChange(TextureMapperNode::ChildrenChange);
    GraphicsLayer::addChild(layer);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildAtIndex(GraphicsLayer* layer, int index)
{
    GraphicsLayer::addChildAtIndex(layer, index);
    notifyChange(TextureMapperNode::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling)
{
     GraphicsLayer::addChildAbove(layer, sibling);
     notifyChange(TextureMapperNode::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling)
{

    GraphicsLayer::addChildBelow(layer, sibling);
    notifyChange(TextureMapperNode::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
bool GraphicsLayerTextureMapper::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (GraphicsLayer::replaceChild(oldChild, newChild)) {
        notifyChange(TextureMapperNode::ChildrenChange);
        return true;
    }
    return false;
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::removeFromParent()
{
    if (!parent())
        return;
    notifyChange(TextureMapperNode::ParentChange);
    GraphicsLayer::removeFromParent();
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setMaskLayer(GraphicsLayer* value)
{
    if (value == maskLayer())
        return;
    GraphicsLayer::setMaskLayer(value);
    notifyChange(TextureMapperNode::MaskLayerChange);
}


/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setReplicatedByLayer(GraphicsLayer* value)
{
    if (value == replicaLayer())
        return;
    GraphicsLayer::setReplicatedByLayer(value);
    notifyChange(TextureMapperNode::ReplicaLayerChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setPosition(const FloatPoint& value)
{
    if (value == position())
        return;
    GraphicsLayer::setPosition(value);
    notifyChange(TextureMapperNode::PositionChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setAnchorPoint(const FloatPoint3D& value)
{
    if (value == anchorPoint())
        return;
    GraphicsLayer::setAnchorPoint(value);
    notifyChange(TextureMapperNode::AnchorPointChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setSize(const FloatSize& value)
{
    if (value == size())
        return;

    GraphicsLayer::setSize(value);
    notifyChange(TextureMapperNode::SizeChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setTransform(const TransformationMatrix& value)
{
    if (value == transform())
        return;

    GraphicsLayer::setTransform(value);
    notifyChange(TextureMapperNode::TransformChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setChildrenTransform(const TransformationMatrix& value)
{
    if (value == childrenTransform())
        return;
    GraphicsLayer::setChildrenTransform(value);
    notifyChange(TextureMapperNode::ChildrenTransformChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setPreserves3D(bool value)
{
    if (value == preserves3D())
        return;
    GraphicsLayer::setPreserves3D(value);
    notifyChange(TextureMapperNode::Preserves3DChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setMasksToBounds(bool value)
{
    if (value == masksToBounds())
        return;
    GraphicsLayer::setMasksToBounds(value);
    notifyChange(TextureMapperNode::MasksToBoundsChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setDrawsContent(bool value)
{
    if (value == drawsContent())
        return;
    notifyChange(TextureMapperNode::DrawsContentChange);
    GraphicsLayer::setDrawsContent(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setBackgroundColor(const Color& value)
{
    if (value == m_pendingContent.backgroundColor)
        return;
    m_pendingContent.backgroundColor = value;
    GraphicsLayer::setBackgroundColor(value);
    notifyChange(TextureMapperNode::BackgroundColorChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::clearBackgroundColor()
{
    if (!m_pendingContent.backgroundColor.isValid())
        return;
    m_pendingContent.backgroundColor = Color();
    GraphicsLayer::clearBackgroundColor();
    notifyChange(TextureMapperNode::BackgroundColorChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsOpaque(bool value)
{
    if (value == contentsOpaque())
        return;
    notifyChange(TextureMapperNode::ContentsOpaqueChange);
    GraphicsLayer::setContentsOpaque(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setBackfaceVisibility(bool value)
{
    if (value == backfaceVisibility())
        return;
    GraphicsLayer::setBackfaceVisibility(value);
    notifyChange(TextureMapperNode::BackfaceVisibilityChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setOpacity(float value)
{
    if (value == opacity())
        return;
    GraphicsLayer::setOpacity(value);
    notifyChange(TextureMapperNode::OpacityChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsRect(const IntRect& value)
{
    if (value == contentsRect())
        return;
    GraphicsLayer::setContentsRect(value);
    notifyChange(TextureMapperNode::ContentsRectChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsToImage(Image* image)
{
    notifyChange(TextureMapperNode::ContentChange);
    m_pendingContent.contentType = image ? TextureMapperNode::DirectImageContentType : TextureMapperNode::HTMLContentType;
    m_pendingContent.image = image;
    GraphicsLayer::setContentsToImage(image);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsBackgroundColor(const Color& color)
{
    notifyChange(TextureMapperNode::ContentChange);
    m_pendingContent.contentType = TextureMapperNode::ColorContentType;
    m_pendingContent.backgroundColor = color;
    GraphicsLayer::setContentsBackgroundColor(color);
}


void GraphicsLayerTextureMapper::setContentsToMedia(PlatformLayer* media)
{
    GraphicsLayer::setContentsToMedia(media);
    notifyChange(TextureMapperNode::ContentChange);
    m_pendingContent.contentType = media ? TextureMapperNode::MediaContentType : TextureMapperNode::HTMLContentType;
    if (media)
        m_pendingContent.media = static_cast<TextureMapperMediaLayer*>(media);
    else
        m_pendingContent.media = 0;
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsOrientation(CompositingCoordinatesOrientation orientation)
{
    if (contentsOrientation() == orientation)
        return;
    notifyChange(TextureMapperNode::ContentsOrientationChange);
    GraphicsLayer::setContentsOrientation(orientation);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::syncCompositingStateForThisLayerOnly()
{
    m_node->syncCompositingState(this, false);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::syncCompositingState()
{
    m_node->syncCompositingState(this, true);
}

/* \reimp (GraphicsLayer.h)
*/
PlatformLayer* GraphicsLayerTextureMapper::platformLayer() const
{
    return m_node.get();
}

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerClient* client)
{
    return new GraphicsLayerTextureMapper(client);
}

}
