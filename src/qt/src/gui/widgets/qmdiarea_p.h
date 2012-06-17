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

#ifndef QMDIAREA_P_H
#define QMDIAREA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmdiarea.h"
#include "qmdisubwindow.h"

#ifndef QT_NO_MDIAREA

#include <QList>
#include <QRect>
#include <QPoint>
#include <QtGui/qapplication.h>
#include <private/qmdisubwindow_p.h>
#include <private/qabstractscrollarea_p.h>

QT_BEGIN_NAMESPACE

namespace QMdi {
class Rearranger
{
public:
    enum Type {
        RegularTiler,
        SimpleCascader,
        IconTiler
    };

    // Rearranges widgets relative to domain.
    virtual void rearrange(QList<QWidget *> &widgets, const QRect &domain) const = 0;
    virtual Type type() const = 0;
    virtual ~Rearranger() {}
};

class RegularTiler : public Rearranger
{
    // Rearranges widgets according to a regular tiling pattern
    // covering the entire domain.
    // Both positions and sizes may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const;
    inline Type type() const { return Rearranger::RegularTiler; }
};

class SimpleCascader : public Rearranger
{
    // Rearranges widgets according to a simple, regular cascading pattern.
    // Widgets are resized to minimumSize.
    // Both positions and sizes may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const;
    inline Type type() const { return Rearranger::SimpleCascader; }
};

class IconTiler : public Rearranger
{
    // Rearranges icons (assumed to be the same size) according to a regular
    // tiling pattern filling up the domain from the bottom.
    // Only positions may change.
    void rearrange(QList<QWidget *> &widgets, const QRect &domain) const;
    inline Type type() const { return Rearranger::IconTiler; }
};

class Placer
{
public:
    // Places the rectangle defined by 'size' relative to 'rects' and 'domain'.
    // Returns the position of the resulting rectangle.
    virtual QPoint place(
        const QSize &size, const QList<QRect> &rects, const QRect &domain) const = 0;
    virtual ~Placer() {}
};

class MinOverlapPlacer : public Placer
{
    QPoint place(const QSize &size, const QList<QRect> &rects, const QRect &domain) const;
    static int accumulatedOverlap(const QRect &source, const QList<QRect> &rects);
    static QRect findMinOverlapRect(const QList<QRect> &source, const QList<QRect> &rects);
    static void getCandidatePlacements(
        const QSize &size, const QList<QRect> &rects, const QRect &domain,
        QList<QRect> &candidates);
    static QPoint findBestPlacement(
        const QRect &domain, const QList<QRect> &rects, QList<QRect> &source);
    static void findNonInsiders(
        const QRect &domain, QList<QRect> &source, QList<QRect> &result);
    static void findMaxOverlappers(
        const QRect &domain, const QList<QRect> &source, QList<QRect> &result);
};
} // namespace QMdi

class QMdiAreaTabBar;
class QMdiAreaPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QMdiArea)
public:
    QMdiAreaPrivate();

    // Variables.
    QMdi::Rearranger *cascader;
    QMdi::Rearranger *regularTiler;
    QMdi::Rearranger *iconTiler;
    QMdi::Placer *placer;
#ifndef QT_NO_RUBBERBAND
    QRubberBand *rubberBand;
#endif
    QMdiAreaTabBar *tabBar;
    QList<QMdi::Rearranger *> pendingRearrangements;
    QList< QPointer<QMdiSubWindow> > pendingPlacements;
    QList< QPointer<QMdiSubWindow> > childWindows;
    QList<int> indicesToActivatedChildren;
    QPointer<QMdiSubWindow> active;
    QPointer<QMdiSubWindow> aboutToBecomeActive;
    QBrush background;
    QMdiArea::WindowOrder activationOrder;
    QMdiArea::AreaOptions options;
    QMdiArea::ViewMode viewMode;
#ifndef QT_NO_TABBAR
    bool documentMode;
    bool tabsClosable;
    bool tabsMovable;
#endif
#ifndef QT_NO_TABWIDGET
    QTabWidget::TabShape tabShape;
    QTabWidget::TabPosition tabPosition;
#endif
    bool ignoreGeometryChange;
    bool ignoreWindowStateChange;
    bool isActivated;
    bool isSubWindowsTiled;
    bool showActiveWindowMaximized;
    bool tileCalledFromResizeEvent;
    bool updatesDisabledByUs;
    bool inViewModeChange;
    int indexToNextWindow;
    int indexToPreviousWindow;
    int indexToHighlighted;
    int indexToLastActiveTab;
    int resizeTimerId;
    int tabToPreviousTimerId;

    // Slots.
    void _q_deactivateAllWindows(QMdiSubWindow *aboutToActivate = 0);
    void _q_processWindowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState);
    void _q_currentTabChanged(int index);
    void _q_closeTab(int index);
    void _q_moveTab(int from, int to);

    // Functions.
    void appendChild(QMdiSubWindow *child);
    void place(QMdi::Placer *placer, QMdiSubWindow *child);
    void rearrange(QMdi::Rearranger *rearranger);
    void arrangeMinimizedSubWindows();
    void activateWindow(QMdiSubWindow *child);
    void activateCurrentWindow();
    void activateHighlightedWindow();
    void emitWindowActivated(QMdiSubWindow *child);
    void resetActiveWindow(QMdiSubWindow *child = 0);
    void updateActiveWindow(int removedIndex, bool activeRemoved);
    void updateScrollBars();
    void internalRaise(QMdiSubWindow *child) const;
    bool scrollBarsEnabled() const;
    bool lastWindowAboutToBeDestroyed() const;
    void setChildActivationEnabled(bool enable = true, bool onlyNextActivationEvent = false) const;
    QRect resizeToMinimumTileSize(const QSize &minSubWindowSize, int subWindowCount);
    void scrollBarPolicyChanged(Qt::Orientation, Qt::ScrollBarPolicy); // reimp
    QMdiSubWindow *nextVisibleSubWindow(int increaseFactor, QMdiArea::WindowOrder,
                                        int removed = -1, int fromIndex = -1) const;
    void highlightNextSubWindow(int increaseFactor);
    QList<QMdiSubWindow *> subWindowList(QMdiArea::WindowOrder, bool reversed = false) const;
    void disconnectSubWindow(QObject *subWindow);
    void setViewMode(QMdiArea::ViewMode mode);
#ifndef QT_NO_TABBAR
    void updateTabBarGeometry();
    void refreshTabBar();
#endif

    inline void startResizeTimer()
    {
        Q_Q(QMdiArea);
        if (resizeTimerId > 0)
            q->killTimer(resizeTimerId);
        resizeTimerId = q->startTimer(200);
    }

    inline void startTabToPreviousTimer()
    {
        Q_Q(QMdiArea);
        if (tabToPreviousTimerId > 0)
            q->killTimer(tabToPreviousTimerId);
        tabToPreviousTimerId = q->startTimer(QApplication::keyboardInputInterval());
    }

    inline bool windowStaysOnTop(QMdiSubWindow *subWindow) const
    {
        if (!subWindow)
            return false;
        return subWindow->windowFlags() & Qt::WindowStaysOnTopHint;
    }

    inline bool isExplicitlyDeactivated(QMdiSubWindow *subWindow) const
    {
        if (!subWindow)
            return true;
        return subWindow->d_func()->isExplicitlyDeactivated;
    }

    inline void setActive(QMdiSubWindow *subWindow, bool active = true, bool changeFocus = true) const
    {
        if (subWindow)
            subWindow->d_func()->setActive(active, changeFocus);
    }

#ifndef QT_NO_RUBBERBAND
    inline void showRubberBandFor(QMdiSubWindow *subWindow)
    {
        if (!subWindow || !rubberBand)
            return;
        rubberBand->setGeometry(subWindow->geometry());
        rubberBand->raise();
        rubberBand->show();
    }

    inline void hideRubberBand()
    {
        if (rubberBand && rubberBand->isVisible())
            rubberBand->hide();
        indexToHighlighted = -1;
    }
#endif // QT_NO_RUBBERBAND
};

#endif // QT_NO_MDIAREA

QT_END_NAMESPACE

#endif // QMDIAREA_P_H
