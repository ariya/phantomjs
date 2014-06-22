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

#ifndef QtWebPageSGNode_h
#define QtWebPageSGNode_h

#include <QtQuick/QSGTransformNode>
#include <wtf/PassRefPtr.h>

QT_BEGIN_NAMESPACE
class QQuickItem;
class QSGSimpleRectNode;
QT_END_NAMESPACE

namespace WebCore {
class CoordinatedGraphicsScene;
}

namespace WebKit {

class ContentsSGNode;

class QtWebPageSGNode : public QSGTransformNode {
    public:
        QtWebPageSGNode();
        void setBackground(const QRectF&, const QColor&);
        void setScale(float);
        void setCoordinatedGraphicsScene(PassRefPtr<WebCore::CoordinatedGraphicsScene>);
        qreal devicePixelRatio() const { return m_devicePixelRatio; }
        void setDevicePixelRatio(qreal devicePixelRatio) { m_devicePixelRatio = devicePixelRatio; }

    private:
        ContentsSGNode* m_contentsNode;
        QSGSimpleRectNode* m_backgroundNode;
        qreal m_devicePixelRatio;
};

} // namespace WebKit

#endif /* QtWebPageSGNode_h */
