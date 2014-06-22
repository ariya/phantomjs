/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "PageClientQt.h"

#include <QGraphicsScene>
#include <QGraphicsView>

#if defined(Q_WS_X11)
#include <QX11Info>
#endif

#ifdef QT_OPENGL_LIB
#include <QGLWidget>
#endif

#if USE(ACCELERATED_COMPOSITING)
#include "TextureMapper.h"
#include "texmap/TextureMapperLayer.h"
#endif

QWindow* QWebPageClient::ownerWindow() const
{
    QWidget* widget = qobject_cast<QWidget*>(ownerWidget());
    if (!widget)
        return 0;
    if (QWindow *window = widget->windowHandle())
        return window;
    if (const QWidget *nativeParent = widget->nativeParentWidget())
        return nativeParent->windowHandle();
    return 0;
}

namespace WebCore {

void PageClientQWidget::scroll(int dx, int dy, const QRect& rectToScroll)
{
    view->scroll(qreal(dx), qreal(dy), rectToScroll);
}

void PageClientQWidget::update(const QRect & dirtyRect)
{
    view->update(dirtyRect);
}

void PageClientQWidget::repaintViewport()
{
    update(view->rect());
    QMetaObject::invokeMethod(page, "repaintRequested", Qt::QueuedConnection, Q_ARG(QRect, view->rect()));
}

void PageClientQWidget::setInputMethodEnabled(bool enable)
{
    view->setAttribute(Qt::WA_InputMethodEnabled, enable);
}

bool PageClientQWidget::inputMethodEnabled() const
{
    return view->testAttribute(Qt::WA_InputMethodEnabled);
}

void PageClientQWidget::setInputMethodHints(Qt::InputMethodHints hints)
{
    view->setInputMethodHints(hints);
}

PageClientQWidget::~PageClientQWidget()
{
}

#ifndef QT_NO_CURSOR
QCursor PageClientQWidget::cursor() const
{
    return view->cursor();
}

void PageClientQWidget::updateCursor(const QCursor& cursor)
{
    view->setCursor(cursor);
}
#endif

QPalette PageClientQWidget::palette() const
{
    return view->palette();
}

int PageClientQWidget::screenNumber() const
{
#if defined(Q_WS_X11)
    return view->x11Info().screen();
#endif
    return 0;
}

QObject* PageClientQWidget::ownerWidget() const
{
    return view;
}

QRect PageClientQWidget::geometryRelativeToOwnerWidget() const
{
    return view->geometry();
}

QPoint PageClientQWidget::mapToOwnerWindow(const QPoint& point) const
{
    QWidget* widget = qobject_cast<QWidget*>(ownerWidget());
    // Can be false both if ownerWidget() is native or if it doesn't have any native parent.
    if (const QWidget *nativeParent = widget->nativeParentWidget())
        return widget->mapTo(nativeParent, point);

    return point;
}

QObject* PageClientQWidget::pluginParent() const
{
    return view;
}

QStyle* PageClientQWidget::style() const
{
    return view->style();
}

QRectF PageClientQWidget::windowRect() const
{
    return QRectF(view->window()->geometry());
}

void PageClientQWidget::setWidgetVisible(Widget* widget, bool visible)
{
    QWidget* qtWidget = qobject_cast<QWidget*>(widget->platformWidget());
    if (!qtWidget)
        return;
    qtWidget->setVisible(visible);
}

#if !defined(QT_NO_GRAPHICSVIEW)
PageClientQGraphicsWidget::~PageClientQGraphicsWidget()
{
    delete overlay;
}

void PageClientQGraphicsWidget::scroll(int dx, int dy, const QRect& rectToScroll)
{
    view->scroll(qreal(dx), qreal(dy), rectToScroll);
}

void PageClientQGraphicsWidget::update(const QRect& dirtyRect)
{
    view->update(dirtyRect);

    if (overlay)
        overlay->update(QRectF(dirtyRect));
}

void PageClientQGraphicsWidget::repaintViewport()
{
    update(view->boundingRect().toAlignedRect());
    QMetaObject::invokeMethod(page, "repaintRequested", Qt::QueuedConnection, Q_ARG(QRect, view->boundingRect().toAlignedRect()));
}

bool PageClientQGraphicsWidget::makeOpenGLContextCurrentIfAvailable()
{
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL) && defined(QT_OPENGL_LIB)
    QGraphicsView* graphicsView = firstGraphicsView();
    if (graphicsView && graphicsView->viewport()) {
        QGLWidget* glWidget = qobject_cast<QGLWidget*>(graphicsView->viewport());
        if (glWidget) {
            // The GL context belonging to the QGLWidget viewport must be current when TextureMapper is being created.
            glWidget->makeCurrent();
            return true;
        }
    }
#endif
    return false;
}

void PageClientQGraphicsWidget::setInputMethodEnabled(bool enable)
{
    view->setFlag(QGraphicsItem::ItemAcceptsInputMethod, enable);
}

bool PageClientQGraphicsWidget::inputMethodEnabled() const
{
    return view->flags() & QGraphicsItem::ItemAcceptsInputMethod;
}

void PageClientQGraphicsWidget::setInputMethodHints(Qt::InputMethodHints hints)
{
    view->setInputMethodHints(hints);
}

#ifndef QT_NO_CURSOR
QCursor PageClientQGraphicsWidget::cursor() const
{
    return view->cursor();
}

void PageClientQGraphicsWidget::updateCursor(const QCursor& cursor)
{
    view->setCursor(cursor);
}
#endif

QPalette PageClientQGraphicsWidget::palette() const
{
    return view->palette();
}

int PageClientQGraphicsWidget::screenNumber() const
{
#if defined(Q_WS_X11)
    if (QGraphicsView* graphicsView = firstGraphicsView())
        return graphicsView->x11Info().screen();
#endif
    return 0;
}

QObject* PageClientQGraphicsWidget::ownerWidget() const
{
    if (QGraphicsScene* scene = view->scene()) {
        const QList<QGraphicsView*> views = scene->views();
        return views.value(0);
    }
    return 0;
}

QRect PageClientQGraphicsWidget::geometryRelativeToOwnerWidget() const
{
    if (QGraphicsView* graphicsView = firstGraphicsView())
        return graphicsView->mapFromScene(view->boundingRect()).boundingRect();
    return QRect();
}

QPoint PageClientQGraphicsWidget::mapToOwnerWindow(const QPoint& point) const
{
    if (const QGraphicsView* graphicsView = firstGraphicsView()) {
        if (const QWidget *nativeParent = graphicsView->nativeParentWidget())
            return graphicsView->mapTo(nativeParent, graphicsView->mapFromScene(view->mapToScene(point)));
        else
            return graphicsView->mapFromScene(view->mapToScene(point));
    }
    return point;
}

#if USE(TILED_BACKING_STORE)
QRectF PageClientQGraphicsWidget::graphicsItemVisibleRect() const
{
    QGraphicsView* graphicsView = firstGraphicsView();
    if (!graphicsView)
        return QRectF();

    int xOffset = graphicsView->horizontalScrollBar()->value();
    int yOffset = graphicsView->verticalScrollBar()->value();
    return view->mapRectFromScene(QRectF(QPointF(xOffset, yOffset), graphicsView->viewport()->size()));
}
#endif

QObject* PageClientQGraphicsWidget::pluginParent() const
{
    return view;
}

QStyle* PageClientQGraphicsWidget::style() const
{
    return view->style();
}

void PageClientQGraphicsWidget::setWidgetVisible(Widget*, bool)
{
    // Doesn't make sense, does it?
}

QRectF PageClientQGraphicsWidget::windowRect() const
{
    if (!view->scene())
        return QRectF();

    // The sceneRect is a good approximation of the size of the application, independent of the view.
    return view->scene()->sceneRect();
}

QGraphicsView* PageClientQGraphicsWidget::firstGraphicsView() const
{
    if (view->scene() && !view->scene()->views().isEmpty())
        return view->scene()->views().first();
    return 0;
}
#endif // QT_NO_GRAPHICSVIEW

} // namespace WebCore

