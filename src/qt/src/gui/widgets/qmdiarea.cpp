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

/*!
    \class QMdiArea
    \brief The QMdiArea widget provides an area in which MDI windows are displayed.
    \since 4.3
    \ingroup mainwindow-classes


    QMdiArea functions, essentially, like a window manager for MDI
    windows. For instance, it draws the windows it manages on itself
    and arranges them in a cascading or tile pattern. QMdiArea is
    commonly used as the center widget in a QMainWindow to create MDI
    applications, but can also be placed in any layout. The following
    code adds an area to a main window:

    \snippet doc/src/snippets/mdiareasnippets.cpp 0

    Unlike the window managers for top-level windows, all window flags
    (Qt::WindowFlags) are supported by QMdiArea as long as the flags
    are supported by the current widget style. If a specific flag is
    not supported by the style (e.g., the
    \l{Qt::}{WindowShadeButtonHint}), you can still shade the window
    with showShaded().

    Subwindows in QMdiArea are instances of QMdiSubWindow. They
    are added to an MDI area with addSubWindow(). It is common to pass
    a QWidget, which is set as the internal widget, to this function,
    but it is also possible to pass a QMdiSubWindow directly.The class
    inherits QWidget, and you can use the same API as with a normal
    top-level window when programming. QMdiSubWindow also has behavior
    that is specific to MDI windows. See the QMdiSubWindow class
    description for more details.

    A subwindow becomes active when it gets the keyboard focus, or
    when setFocus() is called. The user activates a window by moving
    focus in the usual ways. The MDI area emits the
    subWindowActivated() signal when the active window changes, and
    the activeSubWindow() function returns the active subwindow.

    The convenience function subWindowList() returns a list of all
    subwindows. This information could be used in a popup menu
    containing a list of windows, for example.

    The subwindows are sorted by the current
    \l{QMdiArea::}{WindowOrder}. This is used for the subWindowList()
    and for activateNextSubWindow() and acivatePreviousSubWindow().
    Also, it is used when cascading or tiling the windows with
    cascadeSubWindows() and tileSubWindows().

    QMdiArea provides two built-in layout strategies for
    subwindows: cascadeSubWindows() and tileSubWindows(). Both are
    slots and are easily connected to menu entries.

    \table
    \row \o \inlineimage mdi-cascade.png
         \o \inlineimage mdi-tile.png
    \endtable

    \note The default scroll bar property for QMdiArea is Qt::ScrollBarAlwaysOff.

    \sa QMdiSubWindow
*/

/*!
    \fn QMdiArea::subWindowActivated(QMdiSubWindow *window)

    QMdiArea emits this signal after \a window has been activated. When \a
    window is 0, QMdiArea has just deactivated its last active window, and
    there are no active windows on the workspace.

    \sa QMdiArea::activeSubWindow()
*/

/*!
    \enum QMdiArea::AreaOption

    This enum describes options that customize the behavior of the
    QMdiArea.

    \value DontMaximizeSubWindowOnActivation When the active subwindow
    is maximized, the default behavior is to maximize the next
    subwindow that is activated. Set this option if you do not want
    this behavior.
*/

/*!
    \enum QMdiArea::WindowOrder

    Specifies the criteria to use for ordering the list of child windows
    returned by subWindowList(). The functions cascadeSubWindows() and
    tileSubWindows() follow this order when arranging the windows.

    \value CreationOrder The windows are returned in the order of
    their creation.

    \value StackingOrder The windows are returned in the order in
    which they are stacked, with the top-most window being last in
    the list.

    \value ActivationHistoryOrder The windows are returned in the order in
    which they were activated.

    \sa subWindowList()
*/

/*!
    \enum QMdiArea::ViewMode
    \since 4.4

    This enum describes the view mode of the area; i.e. how sub-windows
    will be displayed.

    \value SubWindowView Display sub-windows with window frames (default).
    \value TabbedView Display sub-windows with tabs in a tab bar.

    \sa setViewMode()
*/

#include "qmdiarea_p.h"

#ifndef QT_NO_MDIAREA

#include <QApplication>
#include <QStyle>
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
#include <QMacStyle>
#endif
#include <QChildEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QtAlgorithms>
#include <QMutableListIterator>
#include <QPainter>
#include <QFontMetrics>
#include <QStyleOption>
#include <QDesktopWidget>
#include <QDebug>
#include <qmath.h>
#include <private/qlayoutengine_p.h>

QT_BEGIN_NAMESPACE

using namespace QMdi;

// Asserts in debug mode, gives warning otherwise.
static bool sanityCheck(const QMdiSubWindow * const child, const char *where)
{
    if (!child) {
        const char error[] = "null pointer";
        Q_ASSERT_X(false, where, error);
        qWarning("%s:%s", where, error);
        return false;
    }
    return true;
}

static bool sanityCheck(const QList<QWidget *> &widgets, const int index, const char *where)
{
    if (index < 0 || index >= widgets.size()) {
        const char error[] = "index out of range";
        Q_ASSERT_X(false, where, error);
        qWarning("%s:%s", where, error);
        return false;
    }
    if (!widgets.at(index)) {
        const char error[] = "null pointer";
        Q_ASSERT_X(false, where, error);
        qWarning("%s:%s", where, error);
        return false;
    }
    return true;
}

static void setIndex(int *index, int candidate, int min, int max, bool isIncreasing)
{
    if (!index)
        return;

    if (isIncreasing) {
        if (candidate > max)
            *index = min;
        else
            *index = qMax(candidate, min);
    } else {
        if (candidate < min)
            *index = max;
        else
            *index = qMin(candidate, max);
    }
    Q_ASSERT(*index >= min && *index <= max);
}

static inline bool useScrollBar(const QRect &childrenRect, const QSize &maxViewportSize,
                                Qt::Orientation orientation)
{
    if (orientation == Qt::Horizontal)
        return  childrenRect.width() > maxViewportSize.width()
                || childrenRect.left() < 0
                || childrenRect.right() >= maxViewportSize.width();
    else
        return childrenRect.height() > maxViewportSize.height()
               || childrenRect.top() < 0
               || childrenRect.bottom() >= maxViewportSize.height();
}

// Returns the closest mdi area containing the widget (if any).
static inline QMdiArea *mdiAreaParent(QWidget *widget)
{
    if (!widget)
        return 0;

    QWidget *parent = widget->parentWidget();
    while (parent) {
        if (QMdiArea *area = qobject_cast<QMdiArea *>(parent))
            return area;
        parent = parent->parentWidget();
    }
    return 0;
}

#ifndef QT_NO_TABWIDGET
static inline QTabBar::Shape tabBarShapeFrom(QTabWidget::TabShape shape, QTabWidget::TabPosition position)
{
    const bool rounded = (shape == QTabWidget::Rounded);
    if (position == QTabWidget::North)
        return rounded ? QTabBar::RoundedNorth : QTabBar::TriangularNorth;
    if (position == QTabWidget::South)
        return rounded ? QTabBar::RoundedSouth : QTabBar::TriangularSouth;
    if (position == QTabWidget::East)
        return rounded ? QTabBar::RoundedEast : QTabBar::TriangularEast;
    if (position == QTabWidget::West)
        return rounded ? QTabBar::RoundedWest : QTabBar::TriangularWest;
    return QTabBar::RoundedNorth;
}
#endif // QT_NO_TABWIDGET

static inline QString tabTextFor(QMdiSubWindow *subWindow)
{
    if (!subWindow)
        return QString();

    QString title = subWindow->windowTitle();
    if (subWindow->isWindowModified()) {
        title.replace(QLatin1String("[*]"), QLatin1String("*"));
    } else {
        extern QString qt_setWindowTitle_helperHelper(const QString&, const QWidget*);
        title = qt_setWindowTitle_helperHelper(title, subWindow);
    }

    return title.isEmpty() ? QMdiArea::tr("(Untitled)") : title;
}

/*!
    \internal
*/
void RegularTiler::rearrange(QList<QWidget *> &widgets, const QRect &domain) const
{
    if (widgets.isEmpty())
        return;

    const int n = widgets.size();
    const int ncols = qMax(qCeil(qSqrt(qreal(n))), 1);
    const int nrows = qMax((n % ncols) ? (n / ncols + 1) : (n / ncols), 1);
    const int nspecial = (n % ncols) ? (ncols - n % ncols) : 0;
    const int dx = domain.width()  / ncols;
    const int dy = domain.height() / nrows;

    int i = 0;
    for (int row = 0; row < nrows; ++row) {
        const int y1 = int(row * (dy + 1));
        for (int col = 0; col < ncols; ++col) {
            if (row == 1 && col < nspecial)
                continue;
            const int x1 = int(col * (dx + 1));
            int x2 = int(x1 + dx);
            int y2 = int(y1 + dy);
            if (row == 0 && col < nspecial) {
                y2 *= 2;
                if (nrows != 2)
                    y2 += 1;
                else
                    y2 = domain.bottom();
            }
            if (col == ncols - 1 && x2 != domain.right())
                x2 = domain.right();
            if (row == nrows - 1 && y2 != domain.bottom())
                y2 = domain.bottom();
            if (!sanityCheck(widgets, i, "RegularTiler"))
                continue;
            QWidget *widget = widgets.at(i++);
            QRect newGeometry = QRect(QPoint(x1, y1), QPoint(x2, y2));
            widget->setGeometry(QStyle::visualRect(widget->layoutDirection(), domain, newGeometry));
        }
    }
}

/*!
    \internal
*/
void SimpleCascader::rearrange(QList<QWidget *> &widgets, const QRect &domain) const
{
    if (widgets.isEmpty())
        return;

    // Tunables:
    const int topOffset = 0;
    const int bottomOffset = 50;
    const int leftOffset = 0;
    const int rightOffset = 100;
    const int dx = 10;

    QStyleOptionTitleBar options;
    options.initFrom(widgets.at(0));
    int titleBarHeight = widgets.at(0)->style()->pixelMetric(QStyle::PM_TitleBarHeight, &options, widgets.at(0));
#if defined(Q_WS_MAC) && !defined(QT_NO_STYLE_MAC)
    // ### Remove this after the mac style has been fixed
    if (qobject_cast<QMacStyle *>(widgets.at(0)->style()))
        titleBarHeight -= 4;
#endif
    const QFontMetrics fontMetrics = QFontMetrics(QApplication::font("QWorkspaceTitleBar"));
    const int dy = qMax(titleBarHeight - (titleBarHeight - fontMetrics.height()) / 2, 1);

    const int n = widgets.size();
    const int nrows = qMax((domain.height() - (topOffset + bottomOffset)) / dy, 1);
    const int ncols = qMax(n / nrows + ((n % nrows) ? 1 : 0), 1);
    const int dcol = (domain.width() - (leftOffset + rightOffset)) / ncols;

    int i = 0;
    for (int row = 0; row < nrows; ++row) {
        for (int col = 0; col < ncols; ++col) {
            const int x = leftOffset + row * dx + col * dcol;
            const int y = topOffset + row * dy;
            if (!sanityCheck(widgets, i, "SimpleCascader"))
                continue;
            QWidget *widget = widgets.at(i++);
            QRect newGeometry = QRect(QPoint(x, y), widget->sizeHint());
            widget->setGeometry(QStyle::visualRect(widget->layoutDirection(), domain, newGeometry));
            if (i == n)
                return;
        }
    }
}

/*!
    \internal
*/
void IconTiler::rearrange(QList<QWidget *> &widgets, const QRect &domain) const
{
    if (widgets.isEmpty() || !sanityCheck(widgets, 0, "IconTiler"))
        return;

    const int n = widgets.size();
    const int width = widgets.at(0)->width();
    const int height = widgets.at(0)->height();
    const int ncols = qMax(domain.width() / width, 1);
    const int nrows = n / ncols + ((n % ncols) ? 1 : 0);

    int i = 0;
    for (int row = 0; row < nrows; ++row) {
        for (int col = 0; col < ncols; ++col) {
            const int x = col * width;
            const int y = domain.height() - height - row * height;
            if (!sanityCheck(widgets, i, "IconTiler"))
                continue;
            QWidget *widget = widgets.at(i++);
            QPoint newPos(x, y);
            QRect newGeometry = QRect(newPos.x(), newPos.y(), widget->width(), widget->height());
            widget->setGeometry(QStyle::visualRect(widget->layoutDirection(), domain, newGeometry));
            if (i == n)
                return;
        }
    }
}

/*!
    \internal
    Calculates the accumulated overlap (intersection area) between 'source' and 'rects'.
*/
int MinOverlapPlacer::accumulatedOverlap(const QRect &source, const QList<QRect> &rects)
{
    int accOverlap = 0;
    foreach (const QRect &rect, rects) {
        QRect intersection = source.intersected(rect);
        accOverlap += intersection.width() * intersection.height();
    }
    return accOverlap;
}


/*!
    \internal
    Finds among 'source' the rectangle with the minimum accumulated overlap with the
    rectangles in 'rects'.
*/
QRect MinOverlapPlacer::findMinOverlapRect(const QList<QRect> &source, const QList<QRect> &rects)
{
    int minAccOverlap = -1;
    QRect minAccOverlapRect;
    foreach (const QRect &srcRect, source) {
        const int accOverlap = accumulatedOverlap(srcRect, rects);
        if (accOverlap < minAccOverlap || minAccOverlap == -1) {
            minAccOverlap = accOverlap;
            minAccOverlapRect = srcRect;
        }
    }
    return minAccOverlapRect;
}

/*!
    \internal
    Gets candidates for the final placement.
*/
void MinOverlapPlacer::getCandidatePlacements(const QSize &size, const QList<QRect> &rects,
                                              const QRect &domain,QList<QRect> &candidates)
{
    QSet<int> xset;
    QSet<int> yset;
    xset << domain.left() << domain.right() - size.width() + 1;
    yset << domain.top();
    if (domain.bottom() - size.height() + 1 >= 0)
        yset << domain.bottom() - size.height() + 1;
    foreach (const QRect &rect, rects) {
        xset << rect.right() + 1;
        yset << rect.bottom() + 1;
    }

    QList<int> xlist = xset.values();
    qSort(xlist.begin(), xlist.end());
    QList<int> ylist = yset.values();
    qSort(ylist.begin(), ylist.end());

    foreach (int y, ylist)
        foreach (int x, xlist)
            candidates << QRect(QPoint(x, y), size);
}

/*!
    \internal
    Finds all rectangles in 'source' not completely inside 'domain'. The result is stored
    in 'result' and also removed from 'source'.
*/
void MinOverlapPlacer::findNonInsiders(const QRect &domain, QList<QRect> &source,
                                       QList<QRect> &result)
{
    QMutableListIterator<QRect> it(source);
    while (it.hasNext()) {
        const QRect srcRect = it.next();
        if (!domain.contains(srcRect)) {
            result << srcRect;
            it.remove();
        }
    }
}

/*!
   \internal
    Finds all rectangles in 'source' that overlaps 'domain' by the maximum overlap area
    between 'domain' and any rectangle in 'source'. The result is stored in 'result'.
*/
void MinOverlapPlacer::findMaxOverlappers(const QRect &domain, const QList<QRect> &source,
                                          QList<QRect> &result)
{
    int maxOverlap = -1;
    foreach (const QRect &srcRect, source) {
        QRect intersection = domain.intersected(srcRect);
        const int overlap = intersection.width() * intersection.height();
        if (overlap >= maxOverlap || maxOverlap == -1) {
            if (overlap > maxOverlap) {
                maxOverlap = overlap;
                result.clear();
            }
            result << srcRect;
        }
    }
}

/*!
   \internal
    Finds among the rectangles in 'source' the best placement. Here, 'best' means the
    placement that overlaps the rectangles in 'rects' as little as possible while at the
    same time being as much as possible inside 'domain'.
*/
QPoint MinOverlapPlacer::findBestPlacement(const QRect &domain, const QList<QRect> &rects,
                                           QList<QRect> &source)
{
    QList<QRect> nonInsiders;
    findNonInsiders(domain, source, nonInsiders);

    if (!source.empty())
        return findMinOverlapRect(source, rects).topLeft();

    QList<QRect> maxOverlappers;
    findMaxOverlappers(domain, nonInsiders, maxOverlappers);
    return findMinOverlapRect(maxOverlappers, rects).topLeft();
}


/*!
    \internal
    Places the rectangle defined by 'size' relative to 'rects' and 'domain' so that it
    overlaps 'rects' as little as possible and 'domain' as much as possible.
    Returns the position of the resulting rectangle.
*/
QPoint MinOverlapPlacer::place(const QSize &size, const QList<QRect> &rects,
                               const QRect &domain) const
{
    if (size.isEmpty() || !domain.isValid())
        return QPoint();
    foreach (const QRect &rect, rects) {
        if (!rect.isValid())
            return QPoint();
    }

    QList<QRect> candidates;
    getCandidatePlacements(size, rects, domain, candidates);
    return findBestPlacement(domain, rects, candidates);
}

#ifndef QT_NO_TABBAR
class QMdiAreaTabBar : public QTabBar
{
public:
    QMdiAreaTabBar(QWidget *parent) : QTabBar(parent) {}

protected:
    void mousePressEvent(QMouseEvent *event);
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event);
#endif

private:
    QMdiSubWindow *subWindowFromIndex(int index) const;
};

/*!
    \internal
*/
void QMdiAreaTabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::MidButton) {
        QTabBar::mousePressEvent(event);
        return;
    }

    QMdiSubWindow *subWindow = subWindowFromIndex(tabAt(event->pos()));
    if (!subWindow) {
        event->ignore();
        return;
    }

    subWindow->close();
}

#ifndef QT_NO_CONTEXTMENU
/*!
    \internal
*/
void QMdiAreaTabBar::contextMenuEvent(QContextMenuEvent *event)
{
    QPointer<QMdiSubWindow> subWindow = subWindowFromIndex(tabAt(event->pos()));
    if (!subWindow || subWindow->isHidden()) {
        event->ignore();
        return;
    }

#ifndef QT_NO_MENU
    QMdiSubWindowPrivate *subWindowPrivate = subWindow->d_func();
    if (!subWindowPrivate->systemMenu) {
        event->ignore();
        return;
    }

    QMdiSubWindow *currentSubWindow = subWindowFromIndex(currentIndex());
    Q_ASSERT(currentSubWindow);

    // We don't want these actions to show up in the system menu when the
    // current sub-window is maximized, i.e. covers the entire viewport.
    if (currentSubWindow->isMaximized()) {
        subWindowPrivate->setVisible(QMdiSubWindowPrivate::MoveAction, false);
        subWindowPrivate->setVisible(QMdiSubWindowPrivate::ResizeAction, false);
        subWindowPrivate->setVisible(QMdiSubWindowPrivate::MinimizeAction, false);
        subWindowPrivate->setVisible(QMdiSubWindowPrivate::MaximizeAction, false);
        subWindowPrivate->setVisible(QMdiSubWindowPrivate::MaximizeAction, false);
        subWindowPrivate->setVisible(QMdiSubWindowPrivate::StayOnTopAction, false);
    }

    // Show system menu.
    subWindowPrivate->systemMenu->exec(event->globalPos());
    if (!subWindow)
        return;

    // Restore action visibility.
    subWindowPrivate->updateActions();
#endif // QT_NO_MENU
}
#endif // QT_NO_CONTEXTMENU

/*!
    \internal
*/
QMdiSubWindow *QMdiAreaTabBar::subWindowFromIndex(int index) const
{
    if (index < 0 || index >= count())
        return 0;

    QMdiArea *mdiArea = qobject_cast<QMdiArea *>(parentWidget());
    Q_ASSERT(mdiArea);

    const QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();
    Q_ASSERT(index < subWindows.size());

    QMdiSubWindow *subWindow = mdiArea->subWindowList().at(index);
    Q_ASSERT(subWindow);

    return subWindow;
}
#endif // QT_NO_TABBAR

/*!
    \internal
*/
QMdiAreaPrivate::QMdiAreaPrivate()
    : cascader(0),
      regularTiler(0),
      iconTiler(0),
      placer(0),
#ifndef QT_NO_RUBBERBAND
      rubberBand(0),
#endif
#ifndef QT_NO_TABBAR
      tabBar(0),
#endif
      activationOrder(QMdiArea::CreationOrder),
      viewMode(QMdiArea::SubWindowView),
#ifndef QT_NO_TABBAR
      documentMode(false),
      tabsClosable(false),
      tabsMovable(false),
#endif
#ifndef QT_NO_TABWIDGET
      tabShape(QTabWidget::Rounded),
      tabPosition(QTabWidget::North),
#endif
      ignoreGeometryChange(false),
      ignoreWindowStateChange(false),
      isActivated(false),
      isSubWindowsTiled(false),
      showActiveWindowMaximized(false),
      tileCalledFromResizeEvent(false),
      updatesDisabledByUs(false),
      inViewModeChange(false),
      indexToNextWindow(-1),
      indexToPreviousWindow(-1),
      indexToHighlighted(-1),
      indexToLastActiveTab(-1),
      resizeTimerId(-1),
      tabToPreviousTimerId(-1)
{
}

/*!
    \internal
*/
void QMdiAreaPrivate::_q_deactivateAllWindows(QMdiSubWindow *aboutToActivate)
{
    if (ignoreWindowStateChange)
        return;

    Q_Q(QMdiArea);
    if (!aboutToActivate)
        aboutToBecomeActive = qobject_cast<QMdiSubWindow *>(q->sender());
    else
        aboutToBecomeActive = aboutToActivate;
    Q_ASSERT(aboutToBecomeActive);

    foreach (QMdiSubWindow *child, childWindows) {
        if (!sanityCheck(child, "QMdiArea::deactivateAllWindows") || aboutToBecomeActive == child)
            continue;
        // We don't want to handle signals caused by child->showNormal().
        ignoreWindowStateChange = true;
        if(!(options & QMdiArea::DontMaximizeSubWindowOnActivation) && !showActiveWindowMaximized)
            showActiveWindowMaximized = child->isMaximized() && child->isVisible();
        if (showActiveWindowMaximized && child->isMaximized()) {
            if (q->updatesEnabled()) {
                updatesDisabledByUs = true;
                q->setUpdatesEnabled(false);
            }
            child->showNormal();
        }
        if (child->isMinimized() && !child->isShaded() && !windowStaysOnTop(child))
            child->lower();
        ignoreWindowStateChange = false;
        child->d_func()->setActive(false);
    }
}

/*!
    \internal
*/
void QMdiAreaPrivate::_q_processWindowStateChanged(Qt::WindowStates oldState,
                                                   Qt::WindowStates newState)
{
    if (ignoreWindowStateChange)
        return;

    Q_Q(QMdiArea);
    QMdiSubWindow *child = qobject_cast<QMdiSubWindow *>(q->sender());
    if (!child)
        return;

    // windowActivated
    if (!(oldState & Qt::WindowActive) && (newState & Qt::WindowActive))
        emitWindowActivated(child);
    // windowDeactivated
    else if ((oldState & Qt::WindowActive) && !(newState & Qt::WindowActive))
        resetActiveWindow(child);

    // windowMinimized
    if (!(oldState & Qt::WindowMinimized) && (newState & Qt::WindowMinimized)) {
        isSubWindowsTiled = false;
        arrangeMinimizedSubWindows();
    // windowMaximized
    } else if (!(oldState & Qt::WindowMaximized) && (newState & Qt::WindowMaximized)) {
        internalRaise(child);
    // windowRestored
    } else if (!(newState & (Qt::WindowMaximized | Qt::WindowMinimized))) {
        internalRaise(child);
        if (oldState & Qt::WindowMinimized)
            arrangeMinimizedSubWindows();
    }
}

void QMdiAreaPrivate::_q_currentTabChanged(int index)
{
#ifdef QT_NO_TABBAR
    Q_UNUSED(index);
#else
    if (!tabBar || index < 0)
        return;

    // If the previous active sub-window was hidden, disable the tab.
    if (indexToLastActiveTab >= 0 && indexToLastActiveTab < tabBar->count()
        && indexToLastActiveTab < childWindows.count()) {
        QMdiSubWindow *lastActive = childWindows.at(indexToLastActiveTab);
        if (lastActive && lastActive->isHidden())
            tabBar->setTabEnabled(indexToLastActiveTab, false);
    }

    indexToLastActiveTab = index;
    Q_ASSERT(childWindows.size() > index);
    QMdiSubWindow *subWindow = childWindows.at(index);
    Q_ASSERT(subWindow);
    activateWindow(subWindow);
#endif // QT_NO_TABBAR
}

void QMdiAreaPrivate::_q_closeTab(int index)
{
#ifdef QT_NO_TABBAR
    Q_UNUSED(index);
#else
    QMdiSubWindow *subWindow = childWindows.at(index);
    Q_ASSERT(subWindow);
    subWindow->close();
#endif // QT_NO_TABBAR
}

void QMdiAreaPrivate::_q_moveTab(int from, int to)
{
#ifdef QT_NO_TABBAR
    Q_UNUSED(from);
    Q_UNUSED(to);
#else
    childWindows.move(from, to);
#endif // QT_NO_TABBAR
}

/*!
    \internal
*/
void QMdiAreaPrivate::appendChild(QMdiSubWindow *child)
{
    Q_Q(QMdiArea);
    Q_ASSERT(child && childWindows.indexOf(child) == -1);

    if (child->parent() != viewport)
        child->setParent(viewport, child->windowFlags());
    childWindows.append(QPointer<QMdiSubWindow>(child));

    if (!child->testAttribute(Qt::WA_Resized) && q->isVisible()) {
        QSize newSize(child->sizeHint().boundedTo(viewport->size()));
        child->resize(newSize.expandedTo(qSmartMinSize(child)));
    }

    if (!placer)
        placer = new MinOverlapPlacer;
    place(placer, child);

    if (hbarpolicy != Qt::ScrollBarAlwaysOff)
        child->setOption(QMdiSubWindow::AllowOutsideAreaHorizontally, true);
    else
        child->setOption(QMdiSubWindow::AllowOutsideAreaHorizontally, false);

    if (vbarpolicy != Qt::ScrollBarAlwaysOff)
        child->setOption(QMdiSubWindow::AllowOutsideAreaVertically, true);
    else
        child->setOption(QMdiSubWindow::AllowOutsideAreaVertically, false);

    internalRaise(child);
    indicesToActivatedChildren.prepend(childWindows.size() - 1);
    Q_ASSERT(indicesToActivatedChildren.size() == childWindows.size());

#ifndef QT_NO_TABBAR
    if (tabBar) {
        tabBar->addTab(child->windowIcon(), tabTextFor(child));
        updateTabBarGeometry();
        if (childWindows.count() == 1 && !(options & QMdiArea::DontMaximizeSubWindowOnActivation))
            showActiveWindowMaximized = true;
    }
#endif

    if (!(child->windowFlags() & Qt::SubWindow))
        child->setWindowFlags(Qt::SubWindow);
    child->installEventFilter(q);

    QObject::connect(child, SIGNAL(aboutToActivate()), q, SLOT(_q_deactivateAllWindows()));
    QObject::connect(child, SIGNAL(windowStateChanged(Qt::WindowStates,Qt::WindowStates)),
                     q, SLOT(_q_processWindowStateChanged(Qt::WindowStates,Qt::WindowStates)));
}

/*!
    \internal
*/
void QMdiAreaPrivate::place(Placer *placer, QMdiSubWindow *child)
{
    if (!placer || !child)
        return;

    Q_Q(QMdiArea);
    if (!q->isVisible()) {
        // The window is only laid out when it's added to QMdiArea,
        // so there's no need to check that we don't have it in the
        // list already. appendChild() ensures that.
        pendingPlacements.append(child);
        return;
    }

    QList<QRect> rects;
    QRect parentRect = q->rect();
    foreach (QMdiSubWindow *window, childWindows) {
        if (!sanityCheck(window, "QMdiArea::place") || window == child || !window->isVisibleTo(q)
                || !window->testAttribute(Qt::WA_Moved)) {
            continue;
        }
        QRect occupiedGeometry;
        if (window->isMaximized()) {
            occupiedGeometry = QRect(window->d_func()->oldGeometry.topLeft(),
                                     window->d_func()->restoreSize);
        } else {
            occupiedGeometry = window->geometry();
        }
        rects.append(QStyle::visualRect(child->layoutDirection(), parentRect, occupiedGeometry));
    }
    QPoint newPos = placer->place(child->size(), rects, parentRect);
    QRect newGeometry = QRect(newPos.x(), newPos.y(), child->width(), child->height());
    child->setGeometry(QStyle::visualRect(child->layoutDirection(), parentRect, newGeometry));
}

/*!
    \internal
*/
void QMdiAreaPrivate::rearrange(Rearranger *rearranger)
{
    if (!rearranger)
        return;

    Q_Q(QMdiArea);
    if (!q->isVisible()) {
        // Compress if we already have the rearranger in the list.
        int index = pendingRearrangements.indexOf(rearranger);
        if (index != -1)
            pendingRearrangements.move(index, pendingRearrangements.size() - 1);
        else
            pendingRearrangements.append(rearranger);
        return;
    }

    QList<QWidget *> widgets;
    const bool reverseList = rearranger->type() == Rearranger::RegularTiler;
    const QList<QMdiSubWindow *> subWindows = subWindowList(activationOrder, reverseList);
    QSize minSubWindowSize;
    foreach (QMdiSubWindow *child, subWindows) {
        if (!sanityCheck(child, "QMdiArea::rearrange") || !child->isVisible())
            continue;
        if (rearranger->type() == Rearranger::IconTiler) {
            if (child->isMinimized() && !child->isShaded() && !(child->windowFlags() & Qt::FramelessWindowHint))
                widgets.append(child);
        } else {
            if (child->isMinimized() && !child->isShaded())
                continue;
            if (child->isMaximized() || child->isShaded())
                child->showNormal();
            minSubWindowSize = minSubWindowSize.expandedTo(child->minimumSize())
                               .expandedTo(child->d_func()->internalMinimumSize);
            widgets.append(child);
        }
    }

    if (active && rearranger->type() == Rearranger::RegularTiler) {
        // Move active window in front if necessary. That's the case if we
        // have any windows with staysOnTopHint set.
        int indexToActive = widgets.indexOf((QWidget *)active);
        if (indexToActive > 0)
            widgets.move(indexToActive, 0);
    }

    QRect domain = viewport->rect();
    if (rearranger->type() == Rearranger::RegularTiler && !widgets.isEmpty())
        domain = resizeToMinimumTileSize(minSubWindowSize, widgets.count());

    rearranger->rearrange(widgets, domain);

    if (rearranger->type() == Rearranger::RegularTiler && !widgets.isEmpty()) {
        isSubWindowsTiled = true;
        updateScrollBars();
    } else if (rearranger->type() == Rearranger::SimpleCascader) {
        isSubWindowsTiled = false;
    }
}

/*!
    \internal

    Arranges all minimized windows at the bottom of the workspace.
*/
void QMdiAreaPrivate::arrangeMinimizedSubWindows()
{
    if (!iconTiler)
        iconTiler = new IconTiler;
    rearrange(iconTiler);
}

/*!
    \internal
*/
void QMdiAreaPrivate::activateWindow(QMdiSubWindow *child)
{
    if (childWindows.isEmpty()) {
        Q_ASSERT(!child);
        Q_ASSERT(!active);
        return;
    }

    if (!child) {
        if (active) {
            Q_ASSERT(active->d_func()->isActive);
            active->d_func()->setActive(false);
            resetActiveWindow();
        }
        return;
    }

    if (child->isHidden() || child == active)
        return;
    child->d_func()->setActive(true);
}

/*!
    \internal
*/
void QMdiAreaPrivate::activateCurrentWindow()
{
    QMdiSubWindow *current = q_func()->currentSubWindow();
    if (current && !isExplicitlyDeactivated(current)) {
        current->d_func()->activationEnabled = true;
        current->d_func()->setActive(true, /*changeFocus=*/false);
    }
}

void QMdiAreaPrivate::activateHighlightedWindow()
{
    if (indexToHighlighted < 0)
        return;

    Q_ASSERT(indexToHighlighted < childWindows.size());
    if (tabToPreviousTimerId != -1)
        activateWindow(nextVisibleSubWindow(-1, QMdiArea::ActivationHistoryOrder));
    else
        activateWindow(childWindows.at(indexToHighlighted));
#ifndef QT_NO_RUBBERBAND
    hideRubberBand();
#endif
}

/*!
    \internal
*/
void QMdiAreaPrivate::emitWindowActivated(QMdiSubWindow *activeWindow)
{
    Q_Q(QMdiArea);
    Q_ASSERT(activeWindow);
    if (activeWindow == active)
        return;
    Q_ASSERT(activeWindow->d_func()->isActive);

    if (!aboutToBecomeActive)
        _q_deactivateAllWindows(activeWindow);
    Q_ASSERT(aboutToBecomeActive);

    // This is true only if 'DontMaximizeSubWindowOnActivation' is disabled
    // and the previous active window was maximized.
    if (showActiveWindowMaximized) {
        if (!activeWindow->isMaximized())
            activeWindow->showMaximized();
        showActiveWindowMaximized = false;
    }

    // Put in front to update activation order.
    const int indexToActiveWindow = childWindows.indexOf(activeWindow);
    Q_ASSERT(indexToActiveWindow != -1);
    const int index = indicesToActivatedChildren.indexOf(indexToActiveWindow);
    Q_ASSERT(index != -1);
    indicesToActivatedChildren.move(index, 0);
    internalRaise(activeWindow);

    if (updatesDisabledByUs) {
        q->setUpdatesEnabled(true);
        updatesDisabledByUs = false;
    }

    Q_ASSERT(aboutToBecomeActive == activeWindow);
    active = activeWindow;
    aboutToBecomeActive = 0;
    Q_ASSERT(active->d_func()->isActive);

#ifndef QT_NO_TABBAR
    if (tabBar && tabBar->currentIndex() != indexToActiveWindow)
        tabBar->setCurrentIndex(indexToActiveWindow);
#endif

    if (active->isMaximized() && scrollBarsEnabled())
        updateScrollBars();

    emit q->subWindowActivated(active);
}

/*!
    \internal
*/
void QMdiAreaPrivate::resetActiveWindow(QMdiSubWindow *deactivatedWindow)
{
    Q_Q(QMdiArea);
    if (deactivatedWindow) {
        if (deactivatedWindow != active)
            return;
        active = 0;
        if ((aboutToBecomeActive || isActivated || lastWindowAboutToBeDestroyed())
            && !isExplicitlyDeactivated(deactivatedWindow) && !q->window()->isMinimized()) {
            return;
        }
        emit q->subWindowActivated(0);
        return;
    }

    if (aboutToBecomeActive)
        return;

    active = 0;
    emit q->subWindowActivated(0);
}

/*!
    \internal
*/
void QMdiAreaPrivate::updateActiveWindow(int removedIndex, bool activeRemoved)
{
    Q_ASSERT(indicesToActivatedChildren.size() == childWindows.size());

#ifndef QT_NO_TABBAR
    if (tabBar && removedIndex >= 0) {
        tabBar->blockSignals(true);
        tabBar->removeTab(removedIndex);
        updateTabBarGeometry();
        tabBar->blockSignals(false);
    }
#endif

    if (childWindows.isEmpty()) {
        showActiveWindowMaximized = false;
        resetActiveWindow();
        return;
    }

    if (indexToHighlighted >= 0) {
#ifndef QT_NO_RUBBERBAND
        // Hide rubber band if highlighted window is removed.
        if (indexToHighlighted == removedIndex)
            hideRubberBand();
        else
#endif
        // or update index if necessary.
        if (indexToHighlighted > removedIndex)
            --indexToHighlighted;
    }

    // Update indices list
    for (int i = 0; i < indicesToActivatedChildren.size(); ++i) {
        int *index = &indicesToActivatedChildren[i];
        if (*index > removedIndex)
            --*index;
    }

    if (!activeRemoved)
        return;

    // Activate next window.
    QMdiSubWindow *next = nextVisibleSubWindow(0, activationOrder, removedIndex);
    if (next)
        activateWindow(next);
}

/*!
    \internal
*/
void QMdiAreaPrivate::updateScrollBars()
{
    if (ignoreGeometryChange || !scrollBarsEnabled())
        return;

    Q_Q(QMdiArea);
    QSize maxSize = q->maximumViewportSize();
    QSize hbarExtent = hbar->sizeHint();
    QSize vbarExtent = vbar->sizeHint();

    if (q->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0, q)) {
        const int doubleFrameWidth = frameWidth * 2;
        if (hbarpolicy == Qt::ScrollBarAlwaysOn)
            maxSize.rheight() -= doubleFrameWidth;
        if (vbarpolicy == Qt::ScrollBarAlwaysOn)
            maxSize.rwidth() -= doubleFrameWidth;
        hbarExtent.rheight() += doubleFrameWidth;
        vbarExtent.rwidth() += doubleFrameWidth;
    }

    const QRect childrenRect = active && active->isMaximized()
                               ? active->geometry() : viewport->childrenRect();
    bool useHorizontalScrollBar = useScrollBar(childrenRect, maxSize, Qt::Horizontal);
    bool useVerticalScrollBar = useScrollBar(childrenRect, maxSize, Qt::Vertical);

    if (useHorizontalScrollBar && !useVerticalScrollBar) {
        const QSize max = maxSize - QSize(0, hbarExtent.height());
        useVerticalScrollBar = useScrollBar(childrenRect, max, Qt::Vertical);
    }

    if (useVerticalScrollBar && !useHorizontalScrollBar) {
        const QSize max = maxSize - QSize(vbarExtent.width(), 0);
        useHorizontalScrollBar = useScrollBar(childrenRect, max, Qt::Horizontal);
    }

    if (useHorizontalScrollBar && hbarpolicy != Qt::ScrollBarAlwaysOn)
        maxSize.rheight() -= hbarExtent.height();
    if (useVerticalScrollBar && vbarpolicy != Qt::ScrollBarAlwaysOn)
        maxSize.rwidth() -= vbarExtent.width();

    QRect viewportRect(QPoint(0, 0), maxSize);
    const int startX = q->isLeftToRight() ? childrenRect.left() : viewportRect.right()
                                                                  - childrenRect.right();

    // Horizontal scroll bar.
    if (isSubWindowsTiled && hbar->value() != 0)
        hbar->setValue(0);
    const int xOffset = startX + hbar->value();
    hbar->setRange(qMin(0, xOffset),
                   qMax(0, xOffset + childrenRect.width() - viewportRect.width()));
    hbar->setPageStep(childrenRect.width());
    hbar->setSingleStep(childrenRect.width() / 20);

    // Vertical scroll bar.
    if (isSubWindowsTiled && vbar->value() != 0)
        vbar->setValue(0);
    const int yOffset = childrenRect.top() + vbar->value();
    vbar->setRange(qMin(0, yOffset),
                   qMax(0, yOffset + childrenRect.height() - viewportRect.height()));
    vbar->setPageStep(childrenRect.height());
    vbar->setSingleStep(childrenRect.height() / 20);
}

/*!
    \internal
*/
void QMdiAreaPrivate::internalRaise(QMdiSubWindow *mdiChild) const
{
    if (!sanityCheck(mdiChild, "QMdiArea::internalRaise") || childWindows.size() < 2)
        return;

    QMdiSubWindow *stackUnderChild = 0;
    if (!windowStaysOnTop(mdiChild)) {
        foreach (QObject *object, viewport->children()) {
            QMdiSubWindow *child = qobject_cast<QMdiSubWindow *>(object);
            if (!child || !childWindows.contains(child))
                continue;
            if (!child->isHidden() && windowStaysOnTop(child)) {
                if (stackUnderChild)
                    child->stackUnder(stackUnderChild);
                else
                    child->raise();
                stackUnderChild = child;
            }
        }
    }

    if (stackUnderChild)
        mdiChild->stackUnder(stackUnderChild);
    else
        mdiChild->raise();
}

QRect QMdiAreaPrivate::resizeToMinimumTileSize(const QSize &minSubWindowSize, int subWindowCount)
{
    Q_Q(QMdiArea);
    if (!minSubWindowSize.isValid() || subWindowCount <= 0)
        return viewport->rect();

    // Calculate minimum size.
    const int columns = qMax(qCeil(qSqrt(qreal(subWindowCount))), 1);
    const int rows = qMax((subWindowCount % columns) ? (subWindowCount / columns + 1)
                                                     : (subWindowCount / columns), 1);
    const int minWidth = minSubWindowSize.width() * columns;
    const int minHeight = minSubWindowSize.height() * rows;

    // Increase area size if necessary. Scroll bars are provided if we're not able
    // to resize to the minimum size.
    if (!tileCalledFromResizeEvent) {
        QWidget *topLevel = q;
        // Find the topLevel for this area, either a real top-level or a sub-window.
        while (topLevel && !topLevel->isWindow() && topLevel->windowType() != Qt::SubWindow)
            topLevel = topLevel->parentWidget();
        // We don't want sub-subwindows to be placed at the edge, thus add 2 pixels.
        int minAreaWidth = minWidth + left + right + 2;
        int minAreaHeight = minHeight + top + bottom + 2;
        if (hbar->isVisible())
            minAreaHeight += hbar->height();
        if (vbar->isVisible())
            minAreaWidth += vbar->width();
        if (q->style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, 0, q)) {
            const int frame = q->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, q);
            minAreaWidth += 2 * frame;
            minAreaHeight += 2 * frame;
        }
        const QSize diff = QSize(minAreaWidth, minAreaHeight).expandedTo(q->size()) - q->size();
        topLevel->resize(topLevel->size() + diff);
    }

    QRect domain = viewport->rect();

    // Adjust domain width and provide horizontal scroll bar.
    if (domain.width() < minWidth) {
        domain.setWidth(minWidth);
        if (hbarpolicy == Qt::ScrollBarAlwaysOff)
            q->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        else
            hbar->setValue(0);
    }
    // Adjust domain height and provide vertical scroll bar.
    if (domain.height() < minHeight) {
        domain.setHeight(minHeight);
        if (vbarpolicy  == Qt::ScrollBarAlwaysOff)
            q->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        else
            vbar->setValue(0);
    }
    return domain;
}

/*!
    \internal
*/
bool QMdiAreaPrivate::scrollBarsEnabled() const
{
    return hbarpolicy != Qt::ScrollBarAlwaysOff || vbarpolicy != Qt::ScrollBarAlwaysOff;
}

/*!
    \internal
*/
bool QMdiAreaPrivate::lastWindowAboutToBeDestroyed() const
{
    if (childWindows.count() != 1)
        return false;

    QMdiSubWindow *last = childWindows.at(0);
    if (!last)
        return true;

    if (!last->testAttribute(Qt::WA_DeleteOnClose))
        return false;

    return last->d_func()->data.is_closing;
}

/*!
    \internal
*/
void QMdiAreaPrivate::setChildActivationEnabled(bool enable, bool onlyNextActivationEvent) const
{
    foreach (QMdiSubWindow *subWindow, childWindows) {
        if (!subWindow || !subWindow->isVisible())
            continue;
        if (onlyNextActivationEvent)
            subWindow->d_func()->ignoreNextActivationEvent = !enable;
        else
            subWindow->d_func()->activationEnabled = enable;
    }
}

/*!
    \internal
    \reimp
*/
void QMdiAreaPrivate::scrollBarPolicyChanged(Qt::Orientation orientation, Qt::ScrollBarPolicy policy)
{
    if (childWindows.isEmpty())
        return;

    const QMdiSubWindow::SubWindowOption option = orientation == Qt::Horizontal ?
        QMdiSubWindow::AllowOutsideAreaHorizontally : QMdiSubWindow::AllowOutsideAreaVertically;
    const bool enable = policy != Qt::ScrollBarAlwaysOff;
    foreach (QMdiSubWindow *child, childWindows) {
        if (!sanityCheck(child, "QMdiArea::scrollBarPolicyChanged"))
            continue;
        child->setOption(option, enable);
    }
    updateScrollBars();
}

QList<QMdiSubWindow*>
QMdiAreaPrivate::subWindowList(QMdiArea::WindowOrder order, bool reversed) const
{
    QList<QMdiSubWindow *> list;
    if (childWindows.isEmpty())
        return list;

    if (order == QMdiArea::CreationOrder) {
        foreach (QMdiSubWindow *child, childWindows) {
            if (!child)
                continue;
            if (!reversed)
                list.append(child);
            else
                list.prepend(child);
        }
    } else if (order == QMdiArea::StackingOrder) {
        foreach (QObject *object, viewport->children()) {
            QMdiSubWindow *child = qobject_cast<QMdiSubWindow *>(object);
            if (!child || !childWindows.contains(child))
                continue;
            if (!reversed)
                list.append(child);
            else
                list.prepend(child);
        }
    } else { // ActivationHistoryOrder
        Q_ASSERT(indicesToActivatedChildren.size() == childWindows.size());
        for (int i = indicesToActivatedChildren.count() - 1; i >= 0; --i) {
            QMdiSubWindow *child = childWindows.at(indicesToActivatedChildren.at(i));
            if (!child)
                continue;
            if (!reversed)
                list.append(child);
            else
                list.prepend(child);
        }
    }
    return list;
}

/*!
    \internal
*/
void QMdiAreaPrivate::disconnectSubWindow(QObject *subWindow)
{
    if (!subWindow)
        return;

    Q_Q(QMdiArea);
    QObject::disconnect(subWindow, 0, q, 0);
    subWindow->removeEventFilter(q);
}

/*!
    \internal
*/
QMdiSubWindow *QMdiAreaPrivate::nextVisibleSubWindow(int increaseFactor, QMdiArea::WindowOrder order,
                                                     int removedIndex, int fromIndex) const
{
    if (childWindows.isEmpty())
        return 0;

    Q_Q(const QMdiArea);
    const QList<QMdiSubWindow *> subWindows = q->subWindowList(order);
    QMdiSubWindow *current = 0;

    if (removedIndex < 0) {
        if (fromIndex >= 0 && fromIndex < subWindows.size())
            current = childWindows.at(fromIndex);
        else
            current = q->currentSubWindow();
    }

    // There's no current sub-window (removed or deactivated),
    // so we have to pick the last active or the next in creation order.
    if (!current) {
        if (removedIndex >= 0 && order == QMdiArea::CreationOrder) {
            int candidateIndex = -1;
            setIndex(&candidateIndex, removedIndex, 0, subWindows.size() - 1, true);
            current = childWindows.at(candidateIndex);
        } else {
            current = subWindows.back();
        }
    }
    Q_ASSERT(current);

    // Find the index for the current sub-window in the given activation order
    const int indexToCurrent = subWindows.indexOf(current);
    const bool increasing = increaseFactor > 0 ? true : false;

    // and use that index + increseFactor as a candidate.
    int index = -1;
    setIndex(&index, indexToCurrent + increaseFactor, 0, subWindows.size() - 1, increasing);
    Q_ASSERT(index != -1);

    // Try to find another window if the candidate is hidden.
    while (subWindows.at(index)->isHidden()) {
        setIndex(&index, index + increaseFactor, 0, subWindows.size() - 1, increasing);
        if (index == indexToCurrent)
            break;
    }

    if (!subWindows.at(index)->isHidden())
        return subWindows.at(index);
    return 0;
}

/*!
    \internal
*/
void QMdiAreaPrivate::highlightNextSubWindow(int increaseFactor)
{
    if (childWindows.size() == 1)
        return;

    Q_Q(QMdiArea);
    // There's no highlighted sub-window atm, use current.
    if (indexToHighlighted < 0) {
        QMdiSubWindow *current = q->currentSubWindow();
        if (!current)
            return;
        indexToHighlighted = childWindows.indexOf(current);
    }

    Q_ASSERT(indexToHighlighted >= 0);
    Q_ASSERT(indexToHighlighted < childWindows.size());

    QMdiSubWindow *highlight = nextVisibleSubWindow(increaseFactor, activationOrder, -1, indexToHighlighted);
    if (!highlight)
        return;

#ifndef QT_NO_RUBBERBAND
    if (!rubberBand) {
        rubberBand = new QRubberBand(QRubberBand::Rectangle, viewport);
        // For accessibility to identify this special widget.
        rubberBand->setObjectName(QLatin1String("qt_rubberband"));
        rubberBand->setWindowFlags(rubberBand->windowFlags() | Qt::WindowStaysOnTopHint);
    }
#endif

    // Only highlight if we're not switching back to the previously active window (Ctrl-Tab once).
#ifndef QT_NO_RUBBERBAND
    if (tabToPreviousTimerId == -1)
        showRubberBandFor(highlight);
#endif

    indexToHighlighted = childWindows.indexOf(highlight);
    Q_ASSERT(indexToHighlighted >= 0);
}

/*!
    \internal
    \since 4.4
*/
void QMdiAreaPrivate::setViewMode(QMdiArea::ViewMode mode)
{
    Q_Q(QMdiArea);
    if (viewMode == mode || inViewModeChange)
        return;

    // Just a guard since we cannot set viewMode = mode here.
    inViewModeChange = true;

#ifndef QT_NO_TABBAR
    if (mode == QMdiArea::TabbedView) {
        Q_ASSERT(!tabBar);
        tabBar = new QMdiAreaTabBar(q);
        tabBar->setDocumentMode(documentMode);
        tabBar->setTabsClosable(tabsClosable);
        tabBar->setMovable(tabsMovable);
#ifndef QT_NO_TABWIDGET
        tabBar->setShape(tabBarShapeFrom(tabShape, tabPosition));
#endif

        isSubWindowsTiled = false;

        foreach (QMdiSubWindow *subWindow, childWindows)
            tabBar->addTab(subWindow->windowIcon(), tabTextFor(subWindow));

        QMdiSubWindow *current = q->currentSubWindow();
        if (current) {
            tabBar->setCurrentIndex(childWindows.indexOf(current));
            // Restore sub-window (i.e. cleanup buttons in menu bar and window title).
            if (current->isMaximized())
                current->showNormal();

            viewMode = mode;

            // Now, maximize it.
            if (!q->testOption(QMdiArea::DontMaximizeSubWindowOnActivation)) {
                current->showMaximized();
            }
        } else {
            viewMode = mode;
        }

        if (q->isVisible())
            tabBar->show();
        updateTabBarGeometry();

        QObject::connect(tabBar, SIGNAL(currentChanged(int)), q, SLOT(_q_currentTabChanged(int)));
        QObject::connect(tabBar, SIGNAL(tabCloseRequested(int)), q, SLOT(_q_closeTab(int)));
        QObject::connect(tabBar, SIGNAL(tabMoved(int,int)), q, SLOT(_q_moveTab(int,int)));
    } else
#endif // QT_NO_TABBAR
    { // SubWindowView
#ifndef QT_NO_TABBAR
        delete tabBar;
        tabBar = 0;
#endif // QT_NO_TABBAR

        viewMode = mode;
        q->setViewportMargins(0, 0, 0, 0);
        indexToLastActiveTab = -1;

        QMdiSubWindow *current = q->currentSubWindow();
        if (current && current->isMaximized())
            current->showNormal();
    }

    Q_ASSERT(viewMode == mode);
    inViewModeChange = false;
}

#ifndef QT_NO_TABBAR
/*!
    \internal
*/
void QMdiAreaPrivate::updateTabBarGeometry()
{
    if (!tabBar)
        return;

    Q_Q(QMdiArea);
#ifndef QT_NO_TABWIDGET
    Q_ASSERT(tabBarShapeFrom(tabShape, tabPosition) == tabBar->shape());
#endif
    const QSize tabBarSizeHint = tabBar->sizeHint();

    int areaHeight = q->height();
    if (hbar && hbar->isVisible())
        areaHeight -= hbar->height();

    int areaWidth = q->width();
    if (vbar && vbar->isVisible())
        areaWidth -= vbar->width();

    QRect tabBarRect;
#ifndef QT_NO_TABWIDGET
    switch (tabPosition) {
    case QTabWidget::North:
        q->setViewportMargins(0, tabBarSizeHint.height(), 0, 0);
        tabBarRect = QRect(0, 0, areaWidth, tabBarSizeHint.height());
        break;
    case QTabWidget::South:
        q->setViewportMargins(0, 0, 0, tabBarSizeHint.height());
        tabBarRect = QRect(0, areaHeight - tabBarSizeHint.height(), areaWidth, tabBarSizeHint.height());
        break;
    case QTabWidget::East:
        if (q->layoutDirection() == Qt::LeftToRight)
            q->setViewportMargins(0, 0, tabBarSizeHint.width(), 0);
        else
            q->setViewportMargins(tabBarSizeHint.width(), 0, 0, 0);
        tabBarRect = QRect(areaWidth - tabBarSizeHint.width(), 0, tabBarSizeHint.width(), areaHeight);
        break;
    case QTabWidget::West:
        if (q->layoutDirection() == Qt::LeftToRight)
            q->setViewportMargins(tabBarSizeHint.width(), 0, 0, 0);
        else
            q->setViewportMargins(0, 0, tabBarSizeHint.width(), 0);
        tabBarRect = QRect(0, 0, tabBarSizeHint.width(), areaHeight);
        break;
    default:
        break;
    }
#endif // QT_NO_TABWIDGET

    tabBar->setGeometry(QStyle::visualRect(q->layoutDirection(), q->contentsRect(), tabBarRect));
}

/*!
    \internal
*/
void QMdiAreaPrivate::refreshTabBar()
{
    if (!tabBar)
        return;

    tabBar->setDocumentMode(documentMode);
    tabBar->setTabsClosable(tabsClosable);
    tabBar->setMovable(tabsMovable);
#ifndef QT_NO_TABWIDGET
    tabBar->setShape(tabBarShapeFrom(tabShape, tabPosition));
#endif
    updateTabBarGeometry();
}
#endif // QT_NO_TABBAR

/*!
    Constructs an empty mdi area. \a parent is passed to QWidget's
    constructor.
*/
QMdiArea::QMdiArea(QWidget *parent)
    : QAbstractScrollArea(*new QMdiAreaPrivate, parent)
{
    setBackground(palette().brush(QPalette::Dark));
    setFrameStyle(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewport(0);
    setFocusPolicy(Qt::NoFocus);
    QApplication::instance()->installEventFilter(this);
}

/*!
    Destroys the MDI area.
*/
QMdiArea::~QMdiArea()
{
    Q_D(QMdiArea);
    delete d->cascader;
    d->cascader = 0;

    delete d->regularTiler;
    d->regularTiler = 0;

    delete d->iconTiler;
    d->iconTiler = 0;

    delete d->placer;
    d->placer = 0;
}

/*!
    \reimp
*/
QSize QMdiArea::sizeHint() const
{
    // Calculate a proper scale factor for QDesktopWidget::size().
    // This also takes into account that we can have nested workspaces.
    int nestedCount = 0;
    QWidget *widget = this->parentWidget();
    while (widget) {
        if (qobject_cast<QMdiArea *>(widget))
            ++nestedCount;
        widget = widget->parentWidget();
    }
    const int scaleFactor = 3 * (nestedCount + 1);

    QSize desktopSize = QApplication::desktop()->size();
    QSize size(desktopSize.width() * 2 / scaleFactor, desktopSize.height() * 2 / scaleFactor);
    foreach (QMdiSubWindow *child, d_func()->childWindows) {
        if (!sanityCheck(child, "QMdiArea::sizeHint"))
            continue;
        size = size.expandedTo(child->sizeHint());
    }
    return size.expandedTo(QApplication::globalStrut());
}

/*!
    \reimp
*/
QSize QMdiArea::minimumSizeHint() const
{
    Q_D(const QMdiArea);
    QSize size(style()->pixelMetric(QStyle::PM_MdiSubWindowMinimizedWidth, 0, this),
               style()->pixelMetric(QStyle::PM_TitleBarHeight, 0, this));
    size = size.expandedTo(QAbstractScrollArea::minimumSizeHint());
    if (!d->scrollBarsEnabled()) {
        foreach (QMdiSubWindow *child, d->childWindows) {
            if (!sanityCheck(child, "QMdiArea::sizeHint"))
                continue;
            size = size.expandedTo(child->minimumSizeHint());
        }
    }
    return size.expandedTo(QApplication::globalStrut());
}

/*!
    Returns a pointer to the current subwindow, or 0 if there is
    no current subwindow.

    This function will return the same as activeSubWindow() if
    the QApplication containing QMdiArea is active.

    \sa activeSubWindow(), QApplication::activeWindow()
*/
QMdiSubWindow *QMdiArea::currentSubWindow() const
{
    Q_D(const QMdiArea);
    if (d->childWindows.isEmpty())
        return 0;

    if (d->active)
        return d->active;

    if (d->isActivated && !window()->isMinimized())
        return 0;

    Q_ASSERT(d->indicesToActivatedChildren.count() > 0);
    int index = d->indicesToActivatedChildren.at(0);
    Q_ASSERT(index >= 0 && index < d->childWindows.size());
    QMdiSubWindow *current = d->childWindows.at(index);
    Q_ASSERT(current);
    return current;
}

/*!
    Returns a pointer to the current active subwindow. If no
    window is currently active, 0 is returned.

    Subwindows are treated as top-level windows with respect to
    window state, i.e., if a widget outside the MDI area is the active
    window, no subwindow will be active. Note that if a widget in the
    window in which the MDI area lives gains focus, the window will be
    activated.

    \sa setActiveSubWindow(), Qt::WindowState
*/
QMdiSubWindow *QMdiArea::activeSubWindow() const
{
    Q_D(const QMdiArea);
    return d->active;
}

/*!
    Activates the subwindow \a window. If \a window is 0, any
    current active window is deactivated.

    \sa activeSubWindow()
*/
void QMdiArea::setActiveSubWindow(QMdiSubWindow *window)
{
    Q_D(QMdiArea);
    if (!window) {
        d->activateWindow(0);
        return;
    }

    if (d->childWindows.isEmpty()) {
        qWarning("QMdiArea::setActiveSubWindow: workspace is empty");
        return;
    }

    if (d->childWindows.indexOf(window) == -1) {
        qWarning("QMdiArea::setActiveSubWindow: window is not inside workspace");
        return;
    }

    d->activateWindow(window);
}

/*!
    Closes the active subwindow.

    \sa closeAllSubWindows()
*/
void QMdiArea::closeActiveSubWindow()
{
    Q_D(QMdiArea);
    if (d->active)
        d->active->close();
}

/*!
    Returns a list of all subwindows in the MDI area. If \a order is
    CreationOrder (the default), the windows are sorted in the order
    in which they were inserted into the workspace. If \a order is
    StackingOrder, the windows are listed in their stacking order,
    with the topmost window as the last item in the list. If \a order
    is ActivationHistoryOrder, the windows are listed according to
    their recent activation history.

    \sa WindowOrder
*/
QList<QMdiSubWindow *> QMdiArea::subWindowList(WindowOrder order) const
{
    Q_D(const QMdiArea);
    return d->subWindowList(order, false);
}

/*!
    Closes all subwindows by sending a QCloseEvent to each window.
    You may receive subWindowActivated() signals from subwindows
    before they are closed (if the MDI area activates the subwindow
    when another is closing).

    Subwindows that ignore the close event will remain open.

    \sa closeActiveSubWindow()
*/
void QMdiArea::closeAllSubWindows()
{
    Q_D(QMdiArea);
    if (d->childWindows.isEmpty())
        return;

    d->isSubWindowsTiled = false;
    foreach (QMdiSubWindow *child, d->childWindows) {
        if (!sanityCheck(child, "QMdiArea::closeAllSubWindows"))
            continue;
        child->close();
    }

    d->updateScrollBars();
}

/*!
    Gives the keyboard focus to another window in the list of child
    windows.  The window activated will be the next one determined
    by the current \l{QMdiArea::WindowOrder} {activation order}.

    \sa activatePreviousSubWindow(), QMdiArea::WindowOrder
*/
void QMdiArea::activateNextSubWindow()
{
    Q_D(QMdiArea);
    if (d->childWindows.isEmpty())
        return;

    QMdiSubWindow *next = d->nextVisibleSubWindow(1, d->activationOrder);
    if (next)
        d->activateWindow(next);
}

/*!
    Gives the keyboard focus to another window in the list of child
    windows.  The window activated will be the previous one determined
    by the current \l{QMdiArea::WindowOrder} {activation order}.

    \sa activateNextSubWindow(), QMdiArea::WindowOrder
*/
void QMdiArea::activatePreviousSubWindow()
{
    Q_D(QMdiArea);
    if (d->childWindows.isEmpty())
        return;

    QMdiSubWindow *previous = d->nextVisibleSubWindow(-1, d->activationOrder);
    if (previous)
        d->activateWindow(previous);
}

/*!
    Adds \a widget as a new subwindow to the MDI area.  If \a
    windowFlags are non-zero, they will override the flags set on the
    widget.

    The \a widget can be either a QMdiSubWindow or another QWidget
    (in which case the MDI area will create a subwindow and set the \a
    widget as the internal widget).

    \note Once the subwindow has been added, its parent will be the
    \e{viewport widget} of the QMdiArea.

    \snippet doc/src/snippets/mdiareasnippets.cpp 1

    When you create your own subwindow, you must set the
    Qt::WA_DeleteOnClose widget attribute if you want the window to be
    deleted when closed in the MDI area. If not, the window will be
    hidden and the MDI area will not activate the next subwindow.

    Returns the QMdiSubWindow that is added to the MDI area.

    \sa removeSubWindow()
*/
QMdiSubWindow *QMdiArea::addSubWindow(QWidget *widget, Qt::WindowFlags windowFlags)
{
    if (!widget) {
        qWarning("QMdiArea::addSubWindow: null pointer to widget");
        return 0;
    }

    Q_D(QMdiArea);
    // QWidget::setParent clears focusWidget so store it
    QWidget *childFocus = widget->focusWidget();
    QMdiSubWindow *child = qobject_cast<QMdiSubWindow *>(widget);

    // Widget is already a QMdiSubWindow
    if (child) {
        if (d->childWindows.indexOf(child) != -1) {
            qWarning("QMdiArea::addSubWindow: window is already added");
            return child;
        }
        child->setParent(viewport(), windowFlags ? windowFlags : child->windowFlags());
    // Create a QMdiSubWindow
    } else {
        child = new QMdiSubWindow(viewport(), windowFlags);
        child->setAttribute(Qt::WA_DeleteOnClose);
        child->setWidget(widget);
        Q_ASSERT(child->testAttribute(Qt::WA_DeleteOnClose));
    }

    if (childFocus)
        childFocus->setFocus();
    d->appendChild(child);
    return child;
}

/*!
    Removes \a widget from the MDI area. The \a widget must be
    either a QMdiSubWindow or a widget that is the internal widget of
    a subwindow. Note \a widget is never actually deleted by QMdiArea.
    If a QMdiSubWindow is passed in its parent is set to 0 and it is
    removed, but if an internal widget is passed in the child widget
    is set to 0 but the QMdiSubWindow is not removed.

    \sa addSubWindow()
*/
void QMdiArea::removeSubWindow(QWidget *widget)
{
    if (!widget) {
        qWarning("QMdiArea::removeSubWindow: null pointer to widget");
        return;
    }

    Q_D(QMdiArea);
    if (d->childWindows.isEmpty())
        return;

    if (QMdiSubWindow *child = qobject_cast<QMdiSubWindow *>(widget)) {
        int index = d->childWindows.indexOf(child);
        if (index == -1) {
            qWarning("QMdiArea::removeSubWindow: window is not inside workspace");
            return;
        }
        d->disconnectSubWindow(child);
        d->childWindows.removeAll(child);
        d->indicesToActivatedChildren.removeAll(index);
        d->updateActiveWindow(index, d->active == child);
        child->setParent(0);
        return;
    }

    bool found = false;
    foreach (QMdiSubWindow *child, d->childWindows) {
        if (!sanityCheck(child, "QMdiArea::removeSubWindow"))
            continue;
        if (child->widget() == widget) {
            child->setWidget(0);
            Q_ASSERT(!child->widget());
            found = true;
            break;
        }
    }

    if (!found)
        qWarning("QMdiArea::removeSubWindow: widget is not child of any window inside QMdiArea");
}

/*!
    \property QMdiArea::background
    \brief the background brush for the workspace

    This property sets the background brush for the workspace area
    itself. By default, it is a gray color, but can be any brush
    (e.g., colors, gradients or pixmaps).
*/
QBrush QMdiArea::background() const
{
    return d_func()->background;
}

void QMdiArea::setBackground(const QBrush &brush)
{
    Q_D(QMdiArea);
    if (d->background != brush) {
        d->background = brush;
        d->viewport->setAttribute(Qt::WA_OpaquePaintEvent, brush.isOpaque());
        update();
    }
}


/*!
    \property QMdiArea::activationOrder
    \brief the ordering criteria for subwindow lists
    \since 4.4

    This property specifies the ordering criteria for the list of
    subwindows returned by subWindowList(). By default, it is the window
    creation order.

    \sa subWindowList()
*/
QMdiArea::WindowOrder QMdiArea::activationOrder() const
{
    Q_D(const QMdiArea);
    return d->activationOrder;
}

void QMdiArea::setActivationOrder(WindowOrder order)
{
    Q_D(QMdiArea);
    if (order != d->activationOrder)
        d->activationOrder = order;
}

/*!
    If \a on is true, \a option is enabled on the MDI area; otherwise
    it is disabled. See AreaOption for the effect of each option.

    \sa AreaOption, testOption()
*/
void QMdiArea::setOption(AreaOption option, bool on)
{
    Q_D(QMdiArea);
    if (on && !(d->options & option))
        d->options |= option;
    else if (!on && (d->options & option))
        d->options &= ~option;
}

/*!
    Returns true if \a option is enabled; otherwise returns false.

    \sa AreaOption, setOption()
*/
bool QMdiArea::testOption(AreaOption option) const
{
    return d_func()->options & option;
}

/*!
    \property QMdiArea::viewMode
    \brief the way sub-windows are displayed in the QMdiArea.
    \since 4.4

    By default, the SubWindowView is used to display sub-windows.

    \sa ViewMode, setTabShape(), setTabPosition()
*/
QMdiArea::ViewMode QMdiArea::viewMode() const
{
    Q_D(const QMdiArea);
    return d->viewMode;
}

void QMdiArea::setViewMode(ViewMode mode)
{
    Q_D(QMdiArea);
    d->setViewMode(mode);
}

#ifndef QT_NO_TABBAR
/*!
    \property QMdiArea::documentMode
    \brief whether the tab bar is set to document mode in tabbed view mode.
    \since 4.5

    Document mode is disabled by default.

    \sa QTabBar::documentMode, setViewMode()
*/
bool QMdiArea::documentMode() const
{
    Q_D(const QMdiArea);
    return d->documentMode;
}

void QMdiArea::setDocumentMode(bool enabled)
{
    Q_D(QMdiArea);
    if (d->documentMode == enabled)
        return;

    d->documentMode = enabled;
    d->refreshTabBar();
}

/*!
    \property QMdiArea::tabsClosable
    \brief whether the tab bar should place close buttons on each tab in tabbed view mode.
    \since 4.8

    Tabs are not closable by default.

    \sa QTabBar::tabsClosable, setViewMode()
*/
bool QMdiArea::tabsClosable() const
{
    Q_D(const QMdiArea);
    return d->tabsClosable;
}

void QMdiArea::setTabsClosable(bool closable)
{
    Q_D(QMdiArea);
    if (d->tabsClosable == closable)
        return;

    d->tabsClosable = closable;
    d->refreshTabBar();
}

/*!
    \property QMdiArea::tabsMovable
    \brief whether the user can move the tabs within the tabbar area in tabbed view mode.
    \since 4.8

    Tabs are not movable by default.

    \sa QTabBar::movable, setViewMode()
*/
bool QMdiArea::tabsMovable() const
{
    Q_D(const QMdiArea);
    return d->tabsMovable;
}

void QMdiArea::setTabsMovable(bool movable)
{
    Q_D(QMdiArea);
    if (d->tabsMovable == movable)
        return;

    d->tabsMovable = movable;
    d->refreshTabBar();
}
#endif // QT_NO_TABBAR

#ifndef QT_NO_TABWIDGET
/*!
    \property QMdiArea::tabShape
    \brief the shape of the tabs in tabbed view mode.
    \since 4.4

    Possible values for this property are QTabWidget::Rounded
    (default) or QTabWidget::Triangular.

    \sa QTabWidget::TabShape, setViewMode()
*/
QTabWidget::TabShape QMdiArea::tabShape() const
{
    Q_D(const QMdiArea);
    return d->tabShape;
}

void QMdiArea::setTabShape(QTabWidget::TabShape shape)
{
    Q_D(QMdiArea);
    if (d->tabShape == shape)
        return;

    d->tabShape = shape;
    d->refreshTabBar();
}

/*!
    \property QMdiArea::tabPosition
    \brief the position of the tabs in tabbed view mode.
    \since 4.4

    Possible values for this property are described by the
    QTabWidget::TabPosition enum.

    \sa QTabWidget::TabPosition, setViewMode()
*/
QTabWidget::TabPosition QMdiArea::tabPosition() const
{
    Q_D(const QMdiArea);
    return d->tabPosition;
}

void QMdiArea::setTabPosition(QTabWidget::TabPosition position)
{
    Q_D(QMdiArea);
    if (d->tabPosition == position)
        return;

    d->tabPosition = position;
    d->refreshTabBar();
}
#endif // QT_NO_TABWIDGET

/*!
    \reimp
*/
void QMdiArea::childEvent(QChildEvent *childEvent)
{
    Q_D(QMdiArea);
    if (childEvent->type() == QEvent::ChildPolished) {
        if (QMdiSubWindow *mdiChild = qobject_cast<QMdiSubWindow *>(childEvent->child())) {
            if (d->childWindows.indexOf(mdiChild) == -1)
                d->appendChild(mdiChild);
        }
    }
}

/*!
    \reimp
*/
void QMdiArea::resizeEvent(QResizeEvent *resizeEvent)
{
    Q_D(QMdiArea);
    if (d->childWindows.isEmpty()) {
        resizeEvent->ignore();
        return;
    }

#ifndef QT_NO_TABBAR
    d->updateTabBarGeometry();
#endif

    // Re-tile the views if we're in tiled mode. Re-tile means we will change
    // the geometry of the children, which in turn means 'isSubWindowsTiled'
    // is set to false, so we have to update the state at the end.
    if (d->isSubWindowsTiled) {
        d->tileCalledFromResizeEvent = true;
        tileSubWindows();
        d->tileCalledFromResizeEvent = false;
        d->isSubWindowsTiled = true;
        d->startResizeTimer();
        // We don't have scroll bars or any maximized views.
        return;
    }

    // Resize maximized views.
    bool hasMaximizedSubWindow = false;
    foreach (QMdiSubWindow *child, d->childWindows) {
        if (sanityCheck(child, "QMdiArea::resizeEvent") && child->isMaximized()
                && child->size() != resizeEvent->size()) {
            child->resize(resizeEvent->size());
            if (!hasMaximizedSubWindow)
                hasMaximizedSubWindow = true;
        }
    }

    d->updateScrollBars();

    // Minimized views are stacked under maximized views so there's
    // no need to re-arrange minimized views on-demand. Start a timer
    // just to make things faster with subsequent resize events.
    if (hasMaximizedSubWindow)
        d->startResizeTimer();
    else
        d->arrangeMinimizedSubWindows();
}

/*!
    \reimp
*/
void QMdiArea::timerEvent(QTimerEvent *timerEvent)
{
    Q_D(QMdiArea);
    if (timerEvent->timerId() == d->resizeTimerId) {
        killTimer(d->resizeTimerId);
        d->resizeTimerId = -1;
        d->arrangeMinimizedSubWindows();
    } else if (timerEvent->timerId() == d->tabToPreviousTimerId) {
        killTimer(d->tabToPreviousTimerId);
        d->tabToPreviousTimerId = -1;
        if (d->indexToHighlighted < 0)
            return;
#ifndef QT_NO_RUBBERBAND
        // We're not doing a "quick switch" ... show rubber band.
        Q_ASSERT(d->indexToHighlighted < d->childWindows.size());
        Q_ASSERT(d->rubberBand);
        d->showRubberBandFor(d->childWindows.at(d->indexToHighlighted));
#endif
    }
}

/*!
    \reimp
*/
void QMdiArea::showEvent(QShowEvent *showEvent)
{
    Q_D(QMdiArea);
    if (!d->pendingRearrangements.isEmpty()) {
        bool skipPlacement = false;
        foreach (Rearranger *rearranger, d->pendingRearrangements) {
            // If this is the case, we don't have to lay out pending child windows
            // since the rearranger will find a placement for them.
            if (rearranger->type() != Rearranger::IconTiler && !skipPlacement)
                skipPlacement = true;
            d->rearrange(rearranger);
        }
        d->pendingRearrangements.clear();

        if (skipPlacement && !d->pendingPlacements.isEmpty())
            d->pendingPlacements.clear();
    }

    if (!d->pendingPlacements.isEmpty()) {
        foreach (QMdiSubWindow *window, d->pendingPlacements) {
            if (!window)
                continue;
            if (!window->testAttribute(Qt::WA_Resized)) {
                QSize newSize(window->sizeHint().boundedTo(viewport()->size()));
                window->resize(newSize.expandedTo(qSmartMinSize(window)));
            }
            if (!window->testAttribute(Qt::WA_Moved) && !window->isMinimized()
                    && !window->isMaximized()) {
                d->place(d->placer, window);
            }
        }
        d->pendingPlacements.clear();
    }

    d->setChildActivationEnabled(true);
    d->activateCurrentWindow();

    QAbstractScrollArea::showEvent(showEvent);
}

/*!
    \reimp
*/
bool QMdiArea::viewportEvent(QEvent *event)
{
    Q_D(QMdiArea);
    switch (event->type()) {
    case QEvent::ChildRemoved: {
        d->isSubWindowsTiled = false;
        QObject *removedChild = static_cast<QChildEvent *>(event)->child();
        for (int i = 0; i < d->childWindows.size(); ++i) {
            QObject *child = d->childWindows.at(i);
            if (!child || child == removedChild || !child->parent()
                    || child->parent() != viewport()) {
                if (!testOption(DontMaximizeSubWindowOnActivation)) {
                    // In this case we can only rely on the child being a QObject
                    // (or 0), but let's try and see if we can get more information.
                    QWidget *mdiChild = qobject_cast<QWidget *>(removedChild);
                    if (mdiChild && mdiChild->isMaximized())
                        d->showActiveWindowMaximized = true;
                }
                d->disconnectSubWindow(child);
                const bool activeRemoved = i == d->indicesToActivatedChildren.at(0);
                d->childWindows.removeAt(i);
                d->indicesToActivatedChildren.removeAll(i);
                d->updateActiveWindow(i, activeRemoved);
                d->arrangeMinimizedSubWindows();
                break;
            }
        }
        d->updateScrollBars();
        break;
    }
    case QEvent::Destroy:
        d->isSubWindowsTiled = false;
        d->resetActiveWindow();
        d->childWindows.clear();
        qWarning("QMdiArea: Deleting the view port is undefined, use setViewport instead.");
        break;
    default:
        break;
    }
    return QAbstractScrollArea::viewportEvent(event);
}

/*!
    \reimp
*/
void QMdiArea::scrollContentsBy(int dx, int dy)
{
    Q_D(QMdiArea);
    const bool wasSubWindowsTiled = d->isSubWindowsTiled;
    d->ignoreGeometryChange = true;
    viewport()->scroll(isLeftToRight() ? dx : -dx, dy);
    d->arrangeMinimizedSubWindows();
    d->ignoreGeometryChange = false;
    if (wasSubWindowsTiled)
        d->isSubWindowsTiled = true;
}

/*!
    Arranges all child windows in a tile pattern.

    \sa cascadeSubWindows()
*/
void QMdiArea::tileSubWindows()
{
    Q_D(QMdiArea);
    if (!d->regularTiler)
        d->regularTiler = new RegularTiler;
    d->rearrange(d->regularTiler);
}

/*!
    Arranges all the child windows in a cascade pattern.

    \sa tileSubWindows()
*/
void QMdiArea::cascadeSubWindows()
{
    Q_D(QMdiArea);
    if (!d->cascader)
        d->cascader = new SimpleCascader;
    d->rearrange(d->cascader);
}

/*!
    \reimp
*/
bool QMdiArea::event(QEvent *event)
{
    Q_D(QMdiArea);
    switch (event->type()) {
#ifdef Q_WS_WIN
    // QWidgetPrivate::hide_helper activates another sub-window when closing a
    // modal dialog on Windows (see activateWindow() inside the the ifdef).
    case QEvent::WindowUnblocked:
        d->activateCurrentWindow();
        break;
#endif
    case QEvent::WindowActivate: {
        d->isActivated = true;
        if (d->childWindows.isEmpty())
            break;
        if (!d->active)
            d->activateCurrentWindow();
        d->setChildActivationEnabled(false, true);
        break;
    }
    case QEvent::WindowDeactivate:
        d->isActivated = false;
        d->setChildActivationEnabled(false, true);
        break;
    case QEvent::StyleChange:
        // Re-tile the views if we're in tiled mode. Re-tile means we will change
        // the geometry of the children, which in turn means 'isSubWindowsTiled'
        // is set to false, so we have to update the state at the end.
        if (d->isSubWindowsTiled) {
            tileSubWindows();
            d->isSubWindowsTiled = true;
        }
        break;
    case QEvent::WindowIconChange:
        foreach (QMdiSubWindow *window, d->childWindows) {
            if (sanityCheck(window, "QMdiArea::WindowIconChange"))
                QApplication::sendEvent(window, event);
        }
        break;
    case QEvent::Hide:
        d->setActive(d->active, false, false);
        d->setChildActivationEnabled(false);
        break;
#ifndef QT_NO_TABBAR
    case QEvent::LayoutDirectionChange:
        d->updateTabBarGeometry();
        break;
#endif
    default:
        break;
    }
    return QAbstractScrollArea::event(event);
}

/*!
    \reimp
*/
bool QMdiArea::eventFilter(QObject *object, QEvent *event)
{
    if (!object)
        return QAbstractScrollArea::eventFilter(object, event);

    Q_D(QMdiArea);
    // Global key events with Ctrl modifier.
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {

        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        // Ingore key events without a Ctrl modifier (except for press/release on the modifier itself).
#ifdef Q_WS_MAC
        if (!(keyEvent->modifiers() & Qt::MetaModifier) && keyEvent->key() != Qt::Key_Meta)
#else
        if (!(keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() != Qt::Key_Control)
#endif
            return QAbstractScrollArea::eventFilter(object, event);

        // Find closest mdi area (in case we have a nested workspace).
        QMdiArea *area = mdiAreaParent(static_cast<QWidget *>(object));
        if (!area)
            return QAbstractScrollArea::eventFilter(object, event);

        const bool keyPress = (event->type() == QEvent::KeyPress) ? true : false;

        // 1) Ctrl-Tab once -> activate the previously active window.
        // 2) Ctrl-Tab (Tab, Tab, ...) -> iterate through all windows (activateNextSubWindow()).
        // 3) Ctrl-Shift-Tab (Tab, Tab, ...) -> iterate through all windows in the opposite
        //    direction (activatePreviousSubWindow())
        switch (keyEvent->key()) {
#ifdef Q_WS_MAC
        case Qt::Key_Meta:
#else
        case Qt::Key_Control:
#endif
            if (keyPress)
                area->d_func()->startTabToPreviousTimer();
            else
                area->d_func()->activateHighlightedWindow();
            break;
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            if (keyPress)
                area->d_func()->highlightNextSubWindow(keyEvent->key() == Qt::Key_Tab ? 1 : -1);
            return true;
#ifndef QT_NO_RUBBERBAND
        case Qt::Key_Escape:
            area->d_func()->hideRubberBand();
            break;
#endif
        default:
            break;
        }
        return QAbstractScrollArea::eventFilter(object, event);
    }

    QMdiSubWindow *subWindow = qobject_cast<QMdiSubWindow *>(object);

    if (!subWindow) {
        // QApplication events:
        if (event->type() == QEvent::ApplicationActivate && !d->active
            && isVisible() && !window()->isMinimized()) {
            d->activateCurrentWindow();
        } else if (event->type() == QEvent::ApplicationDeactivate && d->active) {
            d->setActive(d->active, false, false);
        }
        return QAbstractScrollArea::eventFilter(object, event);
    }

    // QMdiSubWindow events:
    switch (event->type()) {
    case QEvent::Move:
    case QEvent::Resize:
        if (d->tileCalledFromResizeEvent)
            break;
        d->updateScrollBars();
        if (!subWindow->isMinimized())
            d->isSubWindowsTiled = false;
        break;
    case QEvent::Show:
#ifndef QT_NO_TABBAR
        if (d->tabBar) {
            const int tabIndex = d->childWindows.indexOf(subWindow);
            if (!d->tabBar->isTabEnabled(tabIndex))
                d->tabBar->setTabEnabled(tabIndex, true);
        }
#endif // QT_NO_TABBAR
        // fall through
    case QEvent::Hide:
        d->isSubWindowsTiled = false;
        break;
#ifndef QT_NO_RUBBERBAND
    case QEvent::Close:
        if (d->childWindows.indexOf(subWindow) == d->indexToHighlighted)
            d->hideRubberBand();
        break;
#endif
#ifndef QT_NO_TABBAR
    case QEvent::WindowTitleChange:
    case QEvent::ModifiedChange:
        if (d->tabBar)
            d->tabBar->setTabText(d->childWindows.indexOf(subWindow), tabTextFor(subWindow));
        break;
    case QEvent::WindowIconChange:
        if (d->tabBar)
            d->tabBar->setTabIcon(d->childWindows.indexOf(subWindow), subWindow->windowIcon());
        break;
#endif // QT_NO_TABBAR
    default:
        break;
    }
    return QAbstractScrollArea::eventFilter(object, event);
}

/*!
    \reimp
*/
void QMdiArea::paintEvent(QPaintEvent *paintEvent)
{
    Q_D(QMdiArea);
    QPainter painter(d->viewport);
    const QVector<QRect> &exposedRects = paintEvent->region().rects();
    for (int i = 0; i < exposedRects.size(); ++i)
        painter.fillRect(exposedRects.at(i), d->background);
}

/*!
    This slot is called by QAbstractScrollArea after setViewport() has been
    called. Reimplement this function in a subclass of QMdiArea to
    initialize the new \a viewport before it is used.

    \sa setViewport()
*/
void QMdiArea::setupViewport(QWidget *viewport)
{
    Q_D(QMdiArea);
    if (viewport)
        viewport->setAttribute(Qt::WA_OpaquePaintEvent, d->background.isOpaque());
    foreach (QMdiSubWindow *child, d->childWindows) {
        if (!sanityCheck(child, "QMdiArea::setupViewport"))
            continue;
        child->setParent(viewport, child->windowFlags());
    }
}

QT_END_NAMESPACE

#include "moc_qmdiarea.cpp"

#endif // QT_NO_MDIAREA
