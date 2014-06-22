/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "QtWebPageSGNode.h"

#include <QtGui/QPolygonF>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleRectNode>
#include <WebCore/CoordinatedGraphicsScene.h>
#include <WebCore/TransformationMatrix.h>
#include <private/qsgrendernode_p.h>

using namespace WebCore;

namespace WebKit {

class ContentsSGNode : public QSGRenderNode {
public:
    ContentsSGNode(PassRefPtr<CoordinatedGraphicsScene> scene)
        : m_scene(scene)
    {
        coordinatedGraphicsScene()->setActive(true);
    }

    virtual StateFlags changedStates()
    {
        return StateFlags(StencilState) | ColorState | BlendState;
    }

    virtual void render(const RenderState& state)
    {
        TransformationMatrix renderMatrix;
        if (pageNode()->devicePixelRatio() != 1.0) {
            renderMatrix.scale(pageNode()->devicePixelRatio());
            if (matrix())
                renderMatrix.multiply(*matrix());
        } else if (matrix())
            renderMatrix = *matrix();

        // When rendering to an intermediate surface, Qt will
        // mirror the projection matrix to fit on the destination coordinate system.
        const QMatrix4x4* projection = state.projectionMatrix;
        bool mirrored = projection && (*projection)(0, 0) * (*projection)(1, 1) - (*projection)(0, 1) * (*projection)(1, 0) > 0;

        // FIXME: Support non-rectangular clippings.
        coordinatedGraphicsScene()->paintToCurrentGLContext(renderMatrix, inheritedOpacity(), clipRect(), mirrored ? TextureMapper::PaintingMirrored : 0);
    }

    ~ContentsSGNode()
    {
        coordinatedGraphicsScene()->purgeGLResources();
    }

    const QtWebPageSGNode* pageNode() const
    {
        const QtWebPageSGNode* parent = static_cast<QtWebPageSGNode*>(this->parent());
        ASSERT(parent);
        return parent;
    }

    WebCore::CoordinatedGraphicsScene* coordinatedGraphicsScene() const { return m_scene.get(); }

private:
    QRectF clipRect() const
    {
        // Start with an invalid rect.
        QRectF resultRect(0, 0, -1, -1);

        for (const QSGClipNode* clip = clipList(); clip; clip = clip->clipList()) {
            QMatrix4x4 clipMatrix;
            if (pageNode()->devicePixelRatio() != 1.0) {
                clipMatrix.scale(pageNode()->devicePixelRatio());
                if (clip->matrix())
                    clipMatrix *= (*clip->matrix());
            } else if (clip->matrix())
                clipMatrix = *clip->matrix();

            QRectF currentClip;

            if (clip->isRectangular())
                currentClip = clipMatrix.mapRect(clip->clipRect());
            else {
                const QSGGeometry* geometry = clip->geometry();
                // Assume here that clipNode has only coordinate data.
                const QSGGeometry::Point2D* geometryPoints = geometry->vertexDataAsPoint2D();

                // Clip region should be at least triangle to make valid clip.
                if (geometry->vertexCount() < 3)
                    continue;

                QPolygonF polygon;

                for (int i = 0; i < geometry->vertexCount(); i++)
                    polygon.append(clipMatrix.map(QPointF(geometryPoints[i].x, geometryPoints[i].y)));
                currentClip = polygon.boundingRect();
            }

            if (currentClip.isEmpty())
                continue;

            if (resultRect.isValid())
                resultRect &= currentClip;
            else
                resultRect = currentClip;
        }

        return resultRect;
    }

    RefPtr<WebCore::CoordinatedGraphicsScene> m_scene;
};

QtWebPageSGNode::QtWebPageSGNode()
    : m_contentsNode(0)
    , m_backgroundNode(new QSGSimpleRectNode)
    , m_devicePixelRatio(1)
{
    appendChildNode(m_backgroundNode);
}

void QtWebPageSGNode::setBackground(const QRectF& rect, const QColor& color)
{
    m_backgroundNode->setRect(rect);
    m_backgroundNode->setColor(color);
}

void QtWebPageSGNode::setScale(float scale)
{
    QMatrix4x4 matrix;
    matrix.scale(scale);
    setMatrix(matrix);
}

void QtWebPageSGNode::setCoordinatedGraphicsScene(PassRefPtr<WebCore::CoordinatedGraphicsScene> scene)
{
    if (m_contentsNode && m_contentsNode->coordinatedGraphicsScene() == scene)
        return;

    delete m_contentsNode;
    m_contentsNode = new ContentsSGNode(scene);
    // This sets the parent node of the content to QtWebPageSGNode.
    appendChildNode(m_contentsNode);
}

} // namespace WebKit
