/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QGRAPHICSVIEW_H
#define QGRAPHICSVIEW_H

#include <QtCore/qmetatype.h>
#include <QtGui/qpainter.h>
#include <QtGui/qscrollarea.h>
#include <QtGui/qgraphicsscene.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

class QGraphicsItem;
class QPainterPath;
class QPolygonF;
class QStyleOptionGraphicsItem;

class QGraphicsViewPrivate;
class Q_GUI_EXPORT QGraphicsView : public QAbstractScrollArea
{
    Q_OBJECT
    Q_FLAGS(QPainter::RenderHints CacheMode OptimizationFlags)
    Q_ENUMS(ViewportAnchor DragMode ViewportUpdateMode)
    Q_PROPERTY(QBrush backgroundBrush READ backgroundBrush WRITE setBackgroundBrush)
    Q_PROPERTY(QBrush foregroundBrush READ foregroundBrush WRITE setForegroundBrush)
    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive)
    Q_PROPERTY(QRectF sceneRect READ sceneRect WRITE setSceneRect)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(QPainter::RenderHints renderHints READ renderHints WRITE setRenderHints)
    Q_PROPERTY(DragMode dragMode READ dragMode WRITE setDragMode)
    Q_PROPERTY(CacheMode cacheMode READ cacheMode WRITE setCacheMode)
    Q_PROPERTY(ViewportAnchor transformationAnchor READ transformationAnchor WRITE setTransformationAnchor)
    Q_PROPERTY(ViewportAnchor resizeAnchor READ resizeAnchor WRITE setResizeAnchor)
    Q_PROPERTY(ViewportUpdateMode viewportUpdateMode READ viewportUpdateMode WRITE setViewportUpdateMode)
#ifndef QT_NO_RUBBERBAND
    Q_PROPERTY(Qt::ItemSelectionMode rubberBandSelectionMode READ rubberBandSelectionMode WRITE setRubberBandSelectionMode)
#endif
    Q_PROPERTY(OptimizationFlags optimizationFlags READ optimizationFlags WRITE setOptimizationFlags)

public:
    enum ViewportAnchor {
        NoAnchor,
        AnchorViewCenter,
        AnchorUnderMouse
    };

    enum CacheModeFlag {
        CacheNone = 0x0,
        CacheBackground = 0x1
    };
    Q_DECLARE_FLAGS(CacheMode, CacheModeFlag)

    enum DragMode {
        NoDrag,
        ScrollHandDrag,
        RubberBandDrag
    };

    enum ViewportUpdateMode {
        FullViewportUpdate,
        MinimalViewportUpdate,
        SmartViewportUpdate,
        NoViewportUpdate,
        BoundingRectViewportUpdate
    };

    enum OptimizationFlag {
        DontClipPainter = 0x1, // obsolete
        DontSavePainterState = 0x2,
        DontAdjustForAntialiasing = 0x4,
        IndirectPainting = 0x8
    };
    Q_DECLARE_FLAGS(OptimizationFlags, OptimizationFlag)

    QGraphicsView(QWidget *parent = 0);
    QGraphicsView(QGraphicsScene *scene, QWidget *parent = 0);
    ~QGraphicsView();

    QSize sizeHint() const;

    QPainter::RenderHints renderHints() const;
    void setRenderHint(QPainter::RenderHint hint, bool enabled = true);
    void setRenderHints(QPainter::RenderHints hints);

    Qt::Alignment alignment() const;
    void setAlignment(Qt::Alignment alignment);

    ViewportAnchor transformationAnchor() const;
    void setTransformationAnchor(ViewportAnchor anchor);

    ViewportAnchor resizeAnchor() const;
    void setResizeAnchor(ViewportAnchor anchor);

    ViewportUpdateMode viewportUpdateMode() const;
    void setViewportUpdateMode(ViewportUpdateMode mode);

    OptimizationFlags optimizationFlags() const;
    void setOptimizationFlag(OptimizationFlag flag, bool enabled = true);
    void setOptimizationFlags(OptimizationFlags flags);

    DragMode dragMode() const;
    void setDragMode(DragMode mode);

#ifndef QT_NO_RUBBERBAND
    Qt::ItemSelectionMode rubberBandSelectionMode() const;
    void setRubberBandSelectionMode(Qt::ItemSelectionMode mode);
#endif

    CacheMode cacheMode() const;
    void setCacheMode(CacheMode mode);
    void resetCachedContent();

    bool isInteractive() const;
    void setInteractive(bool allowed);

    QGraphicsScene *scene() const;
    void setScene(QGraphicsScene *scene);

    QRectF sceneRect() const;
    void setSceneRect(const QRectF &rect);
    inline void setSceneRect(qreal x, qreal y, qreal w, qreal h);

    QMatrix matrix() const;
    void setMatrix(const QMatrix &matrix, bool combine = false);
    void resetMatrix();
    QTransform transform() const;
    QTransform viewportTransform() const;
    bool isTransformed() const;
    void setTransform(const QTransform &matrix, bool combine = false);
    void resetTransform();
    void rotate(qreal angle);
    void scale(qreal sx, qreal sy);
    void shear(qreal sh, qreal sv);
    void translate(qreal dx, qreal dy);

    void centerOn(const QPointF &pos);
    inline void centerOn(qreal x, qreal y);
    void centerOn(const QGraphicsItem *item);
    void ensureVisible(const QRectF &rect, int xmargin = 50, int ymargin = 50);
    inline void ensureVisible(qreal x, qreal y, qreal w, qreal h, int xmargin = 50, int ymargin = 50);
    void ensureVisible(const QGraphicsItem *item, int xmargin = 50, int ymargin = 50);
    void fitInView(const QRectF &rect, Qt::AspectRatioMode aspectRadioMode = Qt::IgnoreAspectRatio);
    inline void fitInView(qreal x, qreal y, qreal w, qreal h,
                          Qt::AspectRatioMode aspectRadioMode = Qt::IgnoreAspectRatio);
    void fitInView(const QGraphicsItem *item,
                   Qt::AspectRatioMode aspectRadioMode = Qt::IgnoreAspectRatio);

    void render(QPainter *painter, const QRectF &target = QRectF(), const QRect &source = QRect(),
                Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);

    QList<QGraphicsItem *> items() const;
    QList<QGraphicsItem *> items(const QPoint &pos) const;
    inline QList<QGraphicsItem *> items(int x, int y) const;
    QList<QGraphicsItem *> items(const QRect &rect, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    inline QList<QGraphicsItem *> items(int x, int y, int w, int h, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    QList<QGraphicsItem *> items(const QPolygon &polygon, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    QList<QGraphicsItem *> items(const QPainterPath &path, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;
    QGraphicsItem *itemAt(const QPoint &pos) const;
    inline QGraphicsItem *itemAt(int x, int y) const;

    QPointF mapToScene(const QPoint &point) const;
    QPolygonF mapToScene(const QRect &rect) const;
    QPolygonF mapToScene(const QPolygon &polygon) const;
    QPainterPath mapToScene(const QPainterPath &path) const;
    QPoint mapFromScene(const QPointF &point) const;
    QPolygon mapFromScene(const QRectF &rect) const;
    QPolygon mapFromScene(const QPolygonF &polygon) const;
    QPainterPath mapFromScene(const QPainterPath &path) const;
    inline QPointF mapToScene(int x, int y) const;
    inline QPolygonF mapToScene(int x, int y, int w, int h) const;
    inline QPoint mapFromScene(qreal x, qreal y) const;
    inline QPolygon mapFromScene(qreal x, qreal y, qreal w, qreal h) const;

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

    QBrush backgroundBrush() const;
    void setBackgroundBrush(const QBrush &brush);

    QBrush foregroundBrush() const;
    void setForegroundBrush(const QBrush &brush);

public Q_SLOTS:
    void updateScene(const QList<QRectF> &rects);
    void invalidateScene(const QRectF &rect = QRectF(), QGraphicsScene::SceneLayers layers = QGraphicsScene::AllLayers);
    void updateSceneRect(const QRectF &rect);

protected Q_SLOTS:
    void setupViewport(QWidget *widget);

protected:
    QGraphicsView(QGraphicsViewPrivate &, QWidget *parent = 0);
    bool event(QEvent *event);
    bool viewportEvent(QEvent *event);

#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event);
#endif
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void focusInEvent(QFocusEvent *event);
    bool focusNextPrevChild(bool next);
    void focusOutEvent(QFocusEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent *event);
#endif
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void scrollContentsBy(int dx, int dy);
    void showEvent(QShowEvent *event);
    void inputMethodEvent(QInputMethodEvent *event);

    virtual void drawBackground(QPainter *painter, const QRectF &rect);
    virtual void drawForeground(QPainter *painter, const QRectF &rect);
    virtual void drawItems(QPainter *painter, int numItems,
                           QGraphicsItem *items[],
                           const QStyleOptionGraphicsItem options[]);

private:
    Q_DECLARE_PRIVATE(QGraphicsView)
    Q_DISABLE_COPY(QGraphicsView)
#ifndef QT_NO_CURSOR
    Q_PRIVATE_SLOT(d_func(), void _q_setViewportCursor(const QCursor &))
    Q_PRIVATE_SLOT(d_func(), void _q_unsetViewportCursor())
#endif
    friend class QGraphicsSceneWidget;
    friend class QGraphicsScene;
    friend class QGraphicsScenePrivate;
    friend class QGraphicsItemPrivate;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGraphicsView::CacheMode)
Q_DECLARE_OPERATORS_FOR_FLAGS(QGraphicsView::OptimizationFlags)

inline void QGraphicsView::setSceneRect(qreal ax, qreal ay, qreal aw, qreal ah)
{ setSceneRect(QRectF(ax, ay, aw, ah)); }
inline void QGraphicsView::centerOn(qreal ax, qreal ay)
{ centerOn(QPointF(ax, ay)); }
inline void QGraphicsView::ensureVisible(qreal ax, qreal ay, qreal aw, qreal ah, int xmargin, int ymargin)
{ ensureVisible(QRectF(ax, ay, aw, ah), xmargin, ymargin); }
inline void QGraphicsView::fitInView(qreal ax, qreal ay, qreal w, qreal h, Qt::AspectRatioMode mode)
{ fitInView(QRectF(ax, ay, w, h), mode); }
inline QList<QGraphicsItem *> QGraphicsView::items(int ax, int ay) const
{ return items(QPoint(ax, ay)); }
inline QList<QGraphicsItem *> QGraphicsView::items(int ax, int ay, int w, int h, Qt::ItemSelectionMode mode) const
{ return items(QRect(ax, ay, w, h), mode); }
inline QGraphicsItem *QGraphicsView::itemAt(int ax, int ay) const
{ return itemAt(QPoint(ax, ay)); }
inline QPointF QGraphicsView::mapToScene(int ax, int ay) const
{ return mapToScene(QPoint(ax, ay)); }
inline QPolygonF QGraphicsView::mapToScene(int ax, int ay, int w, int h) const
{ return mapToScene(QRect(ax, ay, w, h)); }
inline QPoint QGraphicsView::mapFromScene(qreal ax, qreal ay) const
{ return mapFromScene(QPointF(ax, ay)); }
inline QPolygon QGraphicsView::mapFromScene(qreal ax, qreal ay, qreal w, qreal h) const
{ return mapFromScene(QRectF(ax, ay, w, h)); }

#endif // QT_NO_GRAPHICSVIEW

QT_END_NAMESPACE

QT_END_HEADER

#endif // QGRAPHICSVIEW_H
