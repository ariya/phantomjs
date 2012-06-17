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

#ifndef QGRAPHICSSCENE_P_H
#define QGRAPHICSSCENE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgraphicsscene.h"

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

#include "qgraphicssceneevent.h"
#include "qgraphicsview.h"
#include "qgraphicsview_p.h"
#include "qgraphicsitem_p.h"

#include <private/qobject_p.h>
#include <QtCore/qbitarray.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtGui/qfont.h>
#include <QtGui/qpalette.h>
#include <QtGui/qstyle.h>
#include <QtGui/qstyleoption.h>

QT_BEGIN_NAMESPACE

class QGraphicsSceneIndex;
class QGraphicsView;
class QGraphicsWidget;

class Q_AUTOTEST_EXPORT QGraphicsScenePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsScene)
public:
    QGraphicsScenePrivate();
    void init();

    static QGraphicsScenePrivate *get(QGraphicsScene *q);

    int changedSignalIndex;
    int processDirtyItemsIndex;
    int polishItemsIndex;

    QGraphicsScene::ItemIndexMethod indexMethod;
    QGraphicsSceneIndex *index;

    int lastItemCount;

    QRectF sceneRect;

    quint32 hasSceneRect : 1;
    quint32 dirtyGrowingItemsBoundingRect : 1;
    quint32 updateAll : 1;
    quint32 calledEmitUpdated : 1;
    quint32 processDirtyItemsEmitted : 1;
    quint32 needSortTopLevelItems : 1;
    quint32 holesInTopLevelSiblingIndex : 1;
    quint32 topLevelSequentialOrdering : 1;
    quint32 scenePosDescendantsUpdatePending : 1;
    quint32 stickyFocus : 1;
    quint32 hasFocus : 1;
    quint32 lastMouseGrabberItemHasImplicitMouseGrab : 1;
    quint32 allItemsIgnoreHoverEvents : 1;
    quint32 allItemsUseDefaultCursor : 1;
    quint32 painterStateProtection : 1;
    quint32 sortCacheEnabled : 1; // for compatibility
    quint32 allItemsIgnoreTouchEvents : 1;
    quint32 padding : 15;

    QRectF growingItemsBoundingRect;

    void _q_emitUpdated();
    QList<QRectF> updatedRects;

    QPainterPath selectionArea;
    int selectionChanging;
    QSet<QGraphicsItem *> selectedItems;
    QVector<QGraphicsItem *> unpolishedItems;
    QList<QGraphicsItem *> topLevelItems;

    QMap<QGraphicsItem *, QPointF> movingItemsInitialPositions;
    void registerTopLevelItem(QGraphicsItem *item);
    void unregisterTopLevelItem(QGraphicsItem *item);
    void _q_updateLater();
    void _q_polishItems();

    void _q_processDirtyItems();

    QSet<QGraphicsItem *> scenePosItems;
    void setScenePosItemEnabled(QGraphicsItem *item, bool enabled);
    void registerScenePosItem(QGraphicsItem *item);
    void unregisterScenePosItem(QGraphicsItem *item);
    void _q_updateScenePosDescendants();

    void removeItemHelper(QGraphicsItem *item);

    QBrush backgroundBrush;
    QBrush foregroundBrush;

    quint32 rectAdjust;
    QGraphicsItem *focusItem;
    QGraphicsItem *lastFocusItem;
    QGraphicsItem *passiveFocusItem;
    QGraphicsWidget *tabFocusFirst;
    QGraphicsItem *activePanel;
    QGraphicsItem *lastActivePanel;
    int activationRefCount;
    int childExplicitActivation;
    void setActivePanelHelper(QGraphicsItem *item, bool duringActivationEvent);
    void setFocusItemHelper(QGraphicsItem *item, Qt::FocusReason focusReason);

    QList<QGraphicsWidget *> popupWidgets;
    void addPopup(QGraphicsWidget *widget);
    void removePopup(QGraphicsWidget *widget, bool itemIsDying = false);

    QGraphicsItem *lastMouseGrabberItem;
    QList<QGraphicsItem *> mouseGrabberItems;
    void grabMouse(QGraphicsItem *item, bool implicit = false);
    void ungrabMouse(QGraphicsItem *item, bool itemIsDying = false);
    void clearMouseGrabber();

    QList<QGraphicsItem *> keyboardGrabberItems;
    void grabKeyboard(QGraphicsItem *item);
    void ungrabKeyboard(QGraphicsItem *item, bool itemIsDying = false);
    void clearKeyboardGrabber();

    QGraphicsItem *dragDropItem;
    QGraphicsWidget *enterWidget;
    Qt::DropAction lastDropAction;
    QList<QGraphicsItem *> cachedItemsUnderMouse;
    QList<QGraphicsItem *> hoverItems;
    QPointF lastSceneMousePos;
    void enableMouseTrackingOnViews();
    QMap<Qt::MouseButton, QPointF> mouseGrabberButtonDownPos;
    QMap<Qt::MouseButton, QPointF> mouseGrabberButtonDownScenePos;
    QMap<Qt::MouseButton, QPoint> mouseGrabberButtonDownScreenPos;
    QList<QGraphicsItem *> itemsAtPosition(const QPoint &screenPos,
                                           const QPointF &scenePos,
                                           QWidget *widget) const;
    void storeMouseButtonsForMouseGrabber(QGraphicsSceneMouseEvent *event);

    QList<QGraphicsView *> views;
    void addView(QGraphicsView *view);
    void removeView(QGraphicsView *view);

    QMultiMap<QGraphicsItem *, QGraphicsItem *> sceneEventFilters;
    void installSceneEventFilter(QGraphicsItem *watched, QGraphicsItem *filter);
    void removeSceneEventFilter(QGraphicsItem *watched, QGraphicsItem *filter);
    bool filterDescendantEvent(QGraphicsItem *item, QEvent *event);
    bool filterEvent(QGraphicsItem *item, QEvent *event);
    bool sendEvent(QGraphicsItem *item, QEvent *event);

    bool dispatchHoverEvent(QGraphicsSceneHoverEvent *hoverEvent);
    bool itemAcceptsHoverEvents_helper(const QGraphicsItem *item) const;
    void leaveScene(QWidget *viewport);

    void cloneDragDropEvent(QGraphicsSceneDragDropEvent *dest,
                           QGraphicsSceneDragDropEvent *source);
    void sendDragDropEvent(QGraphicsItem *item,
                           QGraphicsSceneDragDropEvent *dragDropEvent);
    void sendHoverEvent(QEvent::Type type, QGraphicsItem *item,
                        QGraphicsSceneHoverEvent *hoverEvent);
    void sendMouseEvent(QGraphicsSceneMouseEvent *mouseEvent);
    void mousePressEventHandler(QGraphicsSceneMouseEvent *mouseEvent);
    QGraphicsWidget *windowForItem(const QGraphicsItem *item) const;

    void drawItemHelper(QGraphicsItem *item, QPainter *painter,
                        const QStyleOptionGraphicsItem *option, QWidget *widget,
                        bool painterStateProtection);

    void drawItems(QPainter *painter, const QTransform *const viewTransform,
                   QRegion *exposedRegion, QWidget *widget);

    void drawSubtreeRecursive(QGraphicsItem *item, QPainter *painter, const QTransform *const,
                              QRegion *exposedRegion, QWidget *widget, qreal parentOpacity = qreal(1.0),
                              const QTransform *const effectTransform = 0);
    void draw(QGraphicsItem *, QPainter *, const QTransform *const, const QTransform *const,
              QRegion *, QWidget *, qreal, const QTransform *const, bool, bool);

    void markDirty(QGraphicsItem *item, const QRectF &rect = QRectF(), bool invalidateChildren = false,
                   bool force = false, bool ignoreOpacity = false, bool removingItemFromScene = false,
                   bool updateBoundingRect = false);
    void processDirtyItemsRecursive(QGraphicsItem *item, bool dirtyAncestorContainsChildren = false,
                                    qreal parentOpacity = qreal(1.0));

    inline void resetDirtyItem(QGraphicsItem *item, bool recursive = false)
    {
        Q_ASSERT(item);
        item->d_ptr->dirty = 0;
        item->d_ptr->paintedViewBoundingRectsNeedRepaint = 0;
        item->d_ptr->geometryChanged = 0;
        if (!item->d_ptr->dirtyChildren)
            recursive = false;
        item->d_ptr->dirtyChildren = 0;
        item->d_ptr->needsRepaint = QRectF();
        item->d_ptr->allChildrenDirty = 0;
        item->d_ptr->fullUpdatePending = 0;
        item->d_ptr->ignoreVisible = 0;
        item->d_ptr->ignoreOpacity = 0;
#ifndef QT_NO_GRAPHICSEFFECT
        QGraphicsEffect::ChangeFlags flags;
        if (item->d_ptr->notifyBoundingRectChanged) {
            flags |= QGraphicsEffect::SourceBoundingRectChanged;
            item->d_ptr->notifyBoundingRectChanged = 0;
        }
        if (item->d_ptr->notifyInvalidated) {
            flags |= QGraphicsEffect::SourceInvalidated;
            item->d_ptr->notifyInvalidated = 0;
        }
#endif //QT_NO_GRAPHICSEFFECT
        if (recursive) {
            for (int i = 0; i < item->d_ptr->children.size(); ++i)
                resetDirtyItem(item->d_ptr->children.at(i), recursive);
        }
#ifndef QT_NO_GRAPHICSEFFECT
        if (flags && item->d_ptr->graphicsEffect)
            item->d_ptr->graphicsEffect->sourceChanged(flags);
#endif //QT_NO_GRAPHICSEFFECT
    }

    inline void ensureSortedTopLevelItems()
    {
        if (needSortTopLevelItems) {
            qSort(topLevelItems.begin(), topLevelItems.end(), qt_notclosestLeaf);
            topLevelSequentialOrdering = false;
            needSortTopLevelItems = false;
        }
    }

    void ensureSequentialTopLevelSiblingIndexes();

    QStyle *style;
    QFont font;
    void setFont_helper(const QFont &font);
    void resolveFont();
    void updateFont(const QFont &font);
    QPalette palette;
    void setPalette_helper(const QPalette &palette);
    void resolvePalette();
    void updatePalette(const QPalette &palette);

    QStyleOptionGraphicsItem styleOptionTmp;

    QMap<int, QTouchEvent::TouchPoint> sceneCurrentTouchPoints;
    QMap<int, QGraphicsItem *> itemForTouchPointId;
    static void updateTouchPointsForItem(QGraphicsItem *item, QTouchEvent *touchEvent);
    int findClosestTouchPointId(const QPointF &scenePos);
    void touchEventHandler(QTouchEvent *touchEvent);
    bool sendTouchBeginEvent(QGraphicsItem *item, QTouchEvent *touchEvent);
    void enableTouchEventsOnViews();

    QList<QGraphicsObject *> cachedTargetItems;
#ifndef QT_NO_GESTURES
    QHash<QGraphicsObject *, QSet<QGesture *> > cachedItemGestures;
    QHash<QGraphicsObject *, QSet<QGesture *> > cachedAlreadyDeliveredGestures;
    QHash<QGesture *, QGraphicsObject *> gestureTargets;
    QHash<Qt::GestureType, int>  grabbedGestures;
    void gestureEventHandler(QGestureEvent *event);
    void gestureTargetsAtHotSpots(const QSet<QGesture *> &gestures,
                           Qt::GestureFlag flag,
                           QHash<QGraphicsObject *, QSet<QGesture *> > *targets,
                           QSet<QGraphicsObject *> *itemsSet = 0,
                           QSet<QGesture *> *normal = 0,
                           QSet<QGesture *> *conflicts = 0);
    void cancelGesturesForChildren(QGesture *original);
    void grabGesture(QGraphicsItem *, Qt::GestureType gesture);
    void ungrabGesture(QGraphicsItem *, Qt::GestureType gesture);
#endif // QT_NO_GESTURES

    void updateInputMethodSensitivityInViews();

    QList<QGraphicsItem *> modalPanels;
    void enterModal(QGraphicsItem *item,
                    QGraphicsItem::PanelModality panelModality = QGraphicsItem::NonModal);
    void leaveModal(QGraphicsItem *item);
};

// QRectF::intersects() returns false always if either the source or target
// rectangle's width or height are 0. This works around that problem.
static inline void _q_adjustRect(QRectF *rect)
{
    Q_ASSERT(rect);
    if (!rect->width())
        rect->adjust(qreal(-0.00001), 0, qreal(0.00001), 0);
    if (!rect->height())
        rect->adjust(0, qreal(-0.00001), 0, qreal(0.00001));
}

static inline QRectF adjustedItemBoundingRect(const QGraphicsItem *item)
{
    Q_ASSERT(item);
    QRectF boundingRect(item->boundingRect());
    _q_adjustRect(&boundingRect);
    return boundingRect;
}

static inline QRectF adjustedItemEffectiveBoundingRect(const QGraphicsItem *item)
{
    Q_ASSERT(item);
    QRectF boundingRect(QGraphicsItemPrivate::get(item)->effectiveBoundingRect());
    _q_adjustRect(&boundingRect);
    return boundingRect;
}

QT_END_NAMESPACE

#endif // QT_NO_GRAPHICSVIEW

#endif
