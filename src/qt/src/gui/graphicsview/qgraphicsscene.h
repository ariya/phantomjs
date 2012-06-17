/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QGRAPHICSSCENE_H
#define QGRAPHICSSCENE_H

#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtGui/qbrush.h>
#include <QtGui/qfont.h>
#include <QtGui/qtransform.h>
#include <QtGui/qmatrix.h>
#include <QtGui/qpen.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

template<typename T> class QList;
class QFocusEvent;
class QFont;
class QFontMetrics;
class QGraphicsEllipseItem;
class QGraphicsItem;
class QGraphicsItemGroup;
class QGraphicsLineItem;
class QGraphicsPathItem;
class QGraphicsPixmapItem;
class QGraphicsPolygonItem;
class QGraphicsProxyWidget;
class QGraphicsRectItem;
class QGraphicsSceneContextMenuEvent;
class QGraphicsSceneDragDropEvent;
class QGraphicsSceneEvent;
class QGraphicsSceneHelpEvent;
class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;
class QGraphicsView;
class QGraphicsWidget;
class QGraphicsSceneIndex;
class QHelpEvent;
class QInputMethodEvent;
class QKeyEvent;
class QLineF;
class QPainterPath;
class QPixmap;
class QPointF;
class QPolygonF;
class QRectF;
class QSizeF;
class QStyle;
class QStyleOptionGraphicsItem;

class QGraphicsScenePrivate;
class Q_GUI_EXPORT QGraphicsScene : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QBrush backgroundBrush READ backgroundBrush WRITE setBackgroundBrush)
    Q_PROPERTY(QBrush foregroundBrush READ foregroundBrush WRITE setForegroundBrush)
    Q_PROPERTY(ItemIndexMethod itemIndexMethod READ itemIndexMethod WRITE setItemIndexMethod)
    Q_PROPERTY(QRectF sceneRect READ sceneRect WRITE setSceneRect)
    Q_PROPERTY(int bspTreeDepth READ bspTreeDepth WRITE setBspTreeDepth)
    Q_PROPERTY(QPalette palette READ palette WRITE setPalette)
    Q_PROPERTY(QFont font READ font WRITE setFont)
    Q_PROPERTY(bool sortCacheEnabled READ isSortCacheEnabled WRITE setSortCacheEnabled)
    Q_PROPERTY(bool stickyFocus READ stickyFocus WRITE setStickyFocus)

public:
    enum ItemIndexMethod {
        BspTreeIndex,
        NoIndex = -1
    };

    enum SceneLayer {
        ItemLayer = 0x1,
        BackgroundLayer = 0x2,
        ForegroundLayer = 0x4,
        AllLayers = 0xffff
    };
    Q_DECLARE_FLAGS(SceneLayers, SceneLayer)

    QGraphicsScene(QObject *parent = 0);
    QGraphicsScene(const QRectF &sceneRect, QObject *parent = 0);
    QGraphicsScene(qreal x, qreal y, qreal width, qreal height, QObject *parent = 0);
    virtual ~QGraphicsScene();

    QRectF sceneRect() const;
    inline qreal width() const { return sceneRect().width(); }
    inline qreal height() const { return sceneRect().height(); }
    void setSceneRect(const QRectF &rect);
    inline void setSceneRect(qreal x, qreal y, qreal w, qreal h)
    { setSceneRect(QRectF(x, y, w, h)); }

    void render(QPainter *painter,
                const QRectF &target = QRectF(), const QRectF &source = QRectF(),
                Qt::AspectRatioMode aspectRatioMode = Qt::KeepAspectRatio);

    ItemIndexMethod itemIndexMethod() const;
    void setItemIndexMethod(ItemIndexMethod method);

    bool isSortCacheEnabled() const;
    void setSortCacheEnabled(bool enabled);

    int bspTreeDepth() const;
    void setBspTreeDepth(int depth);

    QRectF itemsBoundingRect() const;

    QList<QGraphicsItem *> items() const;
    QList<QGraphicsItem *> items(Qt::SortOrder order) const; // ### Qt 5: unify

    QList<QGraphicsItem *> items(const QPointF &pos, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    QList<QGraphicsItem *> items(const QRectF &rect, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    QList<QGraphicsItem *> items(const QPolygonF &polygon, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;
    QList<QGraphicsItem *> items(const QPainterPath &path, Qt::ItemSelectionMode mode, Qt::SortOrder order, const QTransform &deviceTransform = QTransform()) const;

    QList<QGraphicsItem *> items(const QPointF &pos) const; // ### obsolete
    QList<QGraphicsItem *> items(const QRectF &rect, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const; // ### obsolete
    QList<QGraphicsItem *> items(const QPolygonF &polygon, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const; // ### obsolete
    QList<QGraphicsItem *> items(const QPainterPath &path, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const; // ### obsolete

    QList<QGraphicsItem *> collidingItems(const QGraphicsItem *item, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const;

    QGraphicsItem *itemAt(const QPointF &pos) const; // ### obsolete
    QGraphicsItem *itemAt(const QPointF &pos, const QTransform &deviceTransform) const;

    inline QList<QGraphicsItem *> items(qreal x, qreal y, qreal w, qreal h, Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const
    { return items(QRectF(x, y, w, h), mode); } // ### obsolete
    inline QList<QGraphicsItem *> items(qreal x, qreal y, qreal w, qreal h, Qt::ItemSelectionMode mode, Qt::SortOrder order,
                                        const QTransform &deviceTransform = QTransform()) const
    { return items(QRectF(x, y, w, h), mode, order, deviceTransform); }
    inline QGraphicsItem *itemAt(qreal x, qreal y) const // ### obsolete
    { return itemAt(QPointF(x, y)); }
    inline QGraphicsItem *itemAt(qreal x, qreal y, const QTransform &deviceTransform) const
    { return itemAt(QPointF(x, y), deviceTransform); }

    QList<QGraphicsItem *> selectedItems() const;
    QPainterPath selectionArea() const;
    void setSelectionArea(const QPainterPath &path); // ### obsolete
    void setSelectionArea(const QPainterPath &path, const QTransform &deviceTransform);
    void setSelectionArea(const QPainterPath &path, Qt::ItemSelectionMode mode); // ### obsolete
    void setSelectionArea(const QPainterPath &path, Qt::ItemSelectionMode mode, const QTransform &deviceTransform);

    QGraphicsItemGroup *createItemGroup(const QList<QGraphicsItem *> &items);
    void destroyItemGroup(QGraphicsItemGroup *group);

    void addItem(QGraphicsItem *item);
    QGraphicsEllipseItem *addEllipse(const QRectF &rect, const QPen &pen = QPen(), const QBrush &brush = QBrush());
    QGraphicsLineItem *addLine(const QLineF &line, const QPen &pen = QPen());
    QGraphicsPathItem *addPath(const QPainterPath &path, const QPen &pen = QPen(), const QBrush &brush = QBrush());
    QGraphicsPixmapItem *addPixmap(const QPixmap &pixmap);
    QGraphicsPolygonItem *addPolygon(const QPolygonF &polygon, const QPen &pen = QPen(), const QBrush &brush = QBrush());
    QGraphicsRectItem *addRect(const QRectF &rect, const QPen &pen = QPen(), const QBrush &brush = QBrush());
    QGraphicsTextItem *addText(const QString &text, const QFont &font = QFont());
    QGraphicsSimpleTextItem *addSimpleText(const QString &text, const QFont &font = QFont());
    QGraphicsProxyWidget *addWidget(QWidget *widget, Qt::WindowFlags wFlags = 0);
    inline QGraphicsEllipseItem *addEllipse(qreal x, qreal y, qreal w, qreal h, const QPen &pen = QPen(), const QBrush &brush = QBrush())
    { return addEllipse(QRectF(x, y, w, h), pen, brush); }
    inline QGraphicsLineItem *addLine(qreal x1, qreal y1, qreal x2, qreal y2, const QPen &pen = QPen())
    { return addLine(QLineF(x1, y1, x2, y2), pen); }
    inline QGraphicsRectItem *addRect(qreal x, qreal y, qreal w, qreal h, const QPen &pen = QPen(), const QBrush &brush = QBrush())
    { return addRect(QRectF(x, y, w, h), pen, brush); }
    void removeItem(QGraphicsItem *item);

    QGraphicsItem *focusItem() const;
    void setFocusItem(QGraphicsItem *item, Qt::FocusReason focusReason = Qt::OtherFocusReason);
    bool hasFocus() const;
    void setFocus(Qt::FocusReason focusReason = Qt::OtherFocusReason);
    void clearFocus();

    void setStickyFocus(bool enabled);
    bool stickyFocus() const;

    QGraphicsItem *mouseGrabberItem() const;

    QBrush backgroundBrush() const;
    void setBackgroundBrush(const QBrush &brush);

    QBrush foregroundBrush() const;
    void setForegroundBrush(const QBrush &brush);

    virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

    QList <QGraphicsView *> views() const;

    inline void update(qreal x, qreal y, qreal w, qreal h)
    { update(QRectF(x, y, w, h)); }
    inline void invalidate(qreal x, qreal y, qreal w, qreal h, SceneLayers layers = AllLayers)
    { invalidate(QRectF(x, y, w, h), layers); }

    QStyle *style() const;
    void setStyle(QStyle *style);

    QFont font() const;
    void setFont(const QFont &font);

    QPalette palette() const;
    void setPalette(const QPalette &palette);

    bool isActive() const;
    QGraphicsItem *activePanel() const;
    void setActivePanel(QGraphicsItem *item);
    QGraphicsWidget *activeWindow() const;
    void setActiveWindow(QGraphicsWidget *widget);

    bool sendEvent(QGraphicsItem *item, QEvent *event);

public Q_SLOTS:
    void update(const QRectF &rect = QRectF());
    void invalidate(const QRectF &rect = QRectF(), SceneLayers layers = AllLayers);
    void advance();
    void clearSelection();
    void clear();

protected:
    bool event(QEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
    virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
    virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
    virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);
    virtual void helpEvent(QGraphicsSceneHelpEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    virtual void inputMethodEvent(QInputMethodEvent *event);

    virtual void drawBackground(QPainter *painter, const QRectF &rect);
    virtual void drawForeground(QPainter *painter, const QRectF &rect);
    virtual void drawItems(QPainter *painter, int numItems,
                           QGraphicsItem *items[],
                           const QStyleOptionGraphicsItem options[],
                           QWidget *widget = 0);

protected Q_SLOTS:
    bool focusNextPrevChild(bool next);

Q_SIGNALS:
    void changed(const QList<QRectF> &region);
    void sceneRectChanged(const QRectF &rect);
    void selectionChanged();

private:
    Q_DECLARE_PRIVATE(QGraphicsScene)
    Q_DISABLE_COPY(QGraphicsScene)
    Q_PRIVATE_SLOT(d_func(), void _q_emitUpdated())
    Q_PRIVATE_SLOT(d_func(), void _q_polishItems())
    Q_PRIVATE_SLOT(d_func(), void _q_processDirtyItems())
    Q_PRIVATE_SLOT(d_func(), void _q_updateScenePosDescendants())
    friend class QGraphicsItem;
    friend class QGraphicsItemPrivate;
    friend class QGraphicsObject;
    friend class QGraphicsView;
    friend class QGraphicsViewPrivate;
    friend class QGraphicsWidget;
    friend class QGraphicsWidgetPrivate;
    friend class QGraphicsEffect;
    friend class QGraphicsSceneIndex;
    friend class QGraphicsSceneIndexPrivate;
    friend class QGraphicsSceneBspTreeIndex;
    friend class QGraphicsSceneBspTreeIndexPrivate;
    friend class QGraphicsItemEffectSourcePrivate;
#ifndef QT_NO_GESTURES
    friend class QGesture;
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QGraphicsScene::SceneLayers)

#endif // QT_NO_GRAPHICSVIEW

QT_END_NAMESPACE

QT_END_HEADER

#endif
