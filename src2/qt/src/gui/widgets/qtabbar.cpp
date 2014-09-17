/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qlayoutengine_p.h"
#include "qabstractitemdelegate.h"
#include "qapplication.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qevent.h"
#include "qpainter.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qstylepainter.h"
#include "qtabwidget.h"
#include "qtooltip.h"
#include "qwhatsthis.h"
#include "private/qtextengine_p.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif

#include "qdebug.h"
#include "private/qtabbar_p.h"

#ifndef QT_NO_TABBAR

#ifdef Q_WS_MAC
#include <private/qt_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#endif

#ifndef QT_NO_STYLE_S60
#include "qs60style.h"
#endif

QT_BEGIN_NAMESPACE


inline static bool verticalTabs(QTabBar::Shape shape)
{
    return shape == QTabBar::RoundedWest
           || shape == QTabBar::RoundedEast
           || shape == QTabBar::TriangularWest
           || shape == QTabBar::TriangularEast;
}

void QTabBarPrivate::updateMacBorderMetrics()
{
#if (defined Q_WS_MAC) && (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
        Q_Q(QTabBar);
        ::HIContentBorderMetrics metrics;

        // TODO: get metrics to preserve the bottom value
        // TODO: test tab bar position

        OSWindowRef window = qt_mac_window_for(q);

        // push base line separator down to the client are so we can paint over it (Carbon)
        metrics.top = (documentMode && q->isVisible()) ? 1 : 0;
        metrics.bottom = 0;
        metrics.left = 0;
        metrics.right = 0;
        qt_mac_updateContentBorderMetricts(window, metrics);
#if QT_MAC_USE_COCOA
        // In Cocoa we need to keep track of the drawRect method.
        // If documentMode is enabled we need to change it, unless
        // a toolbar is present.
        // Notice that all the information is kept in the window,
        // that's why we get the private widget for it instead of
        // the private widget for this widget.
        QWidgetPrivate *privateWidget = qt_widget_private(q->window());
        if(privateWidget)
            privateWidget->changeMethods = documentMode;
        // Since in Cocoa there is no simple way to remove the baseline, so we just ask the
        // top level to do the magic for us.
        privateWidget->syncUnifiedMode();
#endif // QT_MAC_USE_COCOA
    }
#endif
}

/*!
    Initialize \a option with the values from the tab at \a tabIndex. This method
    is useful for subclasses when they need a QStyleOptionTab, QStyleOptionTabV2,
    or QStyleOptionTabV3 but don't want to fill in all the information themselves.
    This function will check the version of the QStyleOptionTab and fill in the
    additional values for a QStyleOptionTabV2 and QStyleOptionTabV3.

    \sa QStyleOption::initFrom() QTabWidget::initStyleOption()
*/
void QTabBar::initStyleOption(QStyleOptionTab *option, int tabIndex) const
{
    Q_D(const QTabBar);
    int totalTabs = d->tabList.size();

    if (!option || (tabIndex < 0 || tabIndex >= totalTabs))
        return;

    const QTabBarPrivate::Tab &tab = d->tabList.at(tabIndex);
    option->initFrom(this);
    option->state &= ~(QStyle::State_HasFocus | QStyle::State_MouseOver);
    option->rect = tabRect(tabIndex);
    bool isCurrent = tabIndex == d->currentIndex;
    option->row = 0;
    if (tabIndex == d->pressedIndex)
        option->state |= QStyle::State_Sunken;
    if (isCurrent)
        option->state |= QStyle::State_Selected;
    if (isCurrent && hasFocus())
        option->state |= QStyle::State_HasFocus;
    if (!tab.enabled)
        option->state &= ~QStyle::State_Enabled;
    if (isActiveWindow())
        option->state |= QStyle::State_Active;
    if (!d->dragInProgress && option->rect == d->hoverRect)
        option->state |= QStyle::State_MouseOver;
    option->shape = d->shape;
    option->text = tab.text;

    if (tab.textColor.isValid())
        option->palette.setColor(foregroundRole(), tab.textColor);

    option->icon = tab.icon;
    if (QStyleOptionTabV2 *optionV2 = qstyleoption_cast<QStyleOptionTabV2 *>(option))
        optionV2->iconSize = iconSize();  // Will get the default value then.

    if (QStyleOptionTabV3 *optionV3 = qstyleoption_cast<QStyleOptionTabV3 *>(option)) {
        optionV3->leftButtonSize = tab.leftWidget ? tab.leftWidget->size() : QSize();
        optionV3->rightButtonSize = tab.rightWidget ? tab.rightWidget->size() : QSize();
        optionV3->documentMode = d->documentMode;
    }

    if (tabIndex > 0 && tabIndex - 1 == d->currentIndex)
        option->selectedPosition = QStyleOptionTab::PreviousIsSelected;
    else if (tabIndex + 1 < totalTabs && tabIndex + 1 == d->currentIndex)
        option->selectedPosition = QStyleOptionTab::NextIsSelected;
    else
        option->selectedPosition = QStyleOptionTab::NotAdjacent;

    bool paintBeginning = (tabIndex == 0) || (d->dragInProgress && tabIndex == d->pressedIndex + 1);
    bool paintEnd = (tabIndex == totalTabs - 1) || (d->dragInProgress && tabIndex == d->pressedIndex - 1);
    if (paintBeginning) {
        if (paintEnd)
            option->position = QStyleOptionTab::OnlyOneTab;
        else
            option->position = QStyleOptionTab::Beginning;
    } else if (paintEnd) {
        option->position = QStyleOptionTab::End;
    } else {
        option->position = QStyleOptionTab::Middle;
    }

#ifndef QT_NO_TABWIDGET
    if (const QTabWidget *tw = qobject_cast<const QTabWidget *>(parentWidget())) {
        if (tw->cornerWidget(Qt::TopLeftCorner) || tw->cornerWidget(Qt::BottomLeftCorner))
            option->cornerWidgets |= QStyleOptionTab::LeftCornerWidget;
        if (tw->cornerWidget(Qt::TopRightCorner) || tw->cornerWidget(Qt::BottomRightCorner))
            option->cornerWidgets |= QStyleOptionTab::RightCornerWidget;
    }
#endif

    QRect textRect = style()->subElementRect(QStyle::SE_TabBarTabText, option, this);
    option->text = fontMetrics().elidedText(option->text, d->elideMode, textRect.width(),
                        Qt::TextShowMnemonic);
}

/*!
    \class QTabBar
    \brief The QTabBar class provides a tab bar, e.g. for use in tabbed dialogs.

    \ingroup basicwidgets


    QTabBar is straightforward to use; it draws the tabs using one of
    the predefined \link QTabBar::Shape shapes\endlink, and emits a
    signal when a tab is selected. It can be subclassed to tailor the
    look and feel. Qt also provides a ready-made \l{QTabWidget}.

    Each tab has a tabText(), an optional tabIcon(), an optional
    tabToolTip(), optional tabWhatsThis() and optional tabData().
    The tabs's attributes can be changed with setTabText(), setTabIcon(),
    setTabToolTip(), setTabWhatsThis and setTabData(). Each tabs can be
    enabled or disabled individually with setTabEnabled().

    Each tab can display text in a distinct color. The current text color
    for a tab can be found with the tabTextColor() function. Set the text
    color for a particular tab with setTabTextColor().

    Tabs are added using addTab(), or inserted at particular positions
    using insertTab(). The total number of tabs is given by
    count(). Tabs can be removed from the tab bar with
    removeTab(). Combining removeTab() and insertTab() allows you to
    move tabs to different positions.

    The \l shape property defines the tabs' appearance. The choice of
    shape is a matter of taste, although tab dialogs (for preferences
    and similar) invariably use \l RoundedNorth.
    Tab controls in windows other than dialogs almost
    always use either \l RoundedSouth or \l TriangularSouth. Many
    spreadsheets and other tab controls in which all the pages are
    essentially similar use \l TriangularSouth, whereas \l
    RoundedSouth is used mostly when the pages are different (e.g. a
    multi-page tool palette). The default in QTabBar is \l
    RoundedNorth.

    The most important part of QTabBar's API is the currentChanged()
    signal.  This is emitted whenever the current tab changes (even at
    startup, when the current tab changes from 'none'). There is also
    a slot, setCurrentIndex(), which can be used to select a tab
    programmatically. The function currentIndex() returns the index of
    the current tab, \l count holds the number of tabs.

    QTabBar creates automatic mnemonic keys in the manner of QAbstractButton;
    e.g. if a tab's label is "\&Graphics", Alt+G becomes a shortcut
    key for switching to that tab.

    The following virtual functions may need to be reimplemented in
    order to tailor the look and feel or store extra data with each
    tab:

    \list
    \i tabSizeHint() calcuates the size of a tab.
    \i tabInserted() notifies that a new tab was added.
    \i tabRemoved() notifies that a tab was removed.
    \i tabLayoutChange() notifies that the tabs have been re-laid out.
    \i paintEvent() paints all tabs.
    \endlist

    For subclasses, you might also need the tabRect() functions which
    returns the visual geometry of a single tab.

    \table 100%
    \row \o \inlineimage plastique-tabbar.png Screenshot of a Plastique style tab bar
         \o A tab bar shown in the Plastique widget style.
    \row \o \inlineimage plastique-tabbar-truncated.png Screenshot of a truncated Plastique tab bar
         \o A truncated tab bar shown in the Plastique widget style.
    \endtable

    \sa QTabWidget
*/

/*!
    \enum QTabBar::Shape

    This enum type lists the built-in shapes supported by QTabBar. Treat these
    as hints as some styles may not render some of the shapes. However,
    position should be honored.

    \value RoundedNorth  The normal rounded look above the pages

    \value RoundedSouth  The normal rounded look below the pages

    \value RoundedWest  The normal rounded look on the left side of the pages

    \value RoundedEast  The normal rounded look on the right side the pages

    \value TriangularNorth  Triangular tabs above the pages.

    \value TriangularSouth  Triangular tabs similar to those used in
    the Excel spreadsheet, for example

    \value TriangularWest  Triangular tabs on the left of the pages.

    \value TriangularEast  Triangular tabs on the right of the pages.
    \omitvalue RoundedAbove
    \omitvalue RoundedBelow
    \omitvalue TriangularAbove
    \omitvalue TriangularBelow
*/

/*!
    \fn void QTabBar::currentChanged(int index)

    This signal is emitted when the tab bar's current tab changes. The
    new current has the given \a index, or -1 if there isn't a new one
    (for example, if there are no tab in the QTabBar)
*/

/*!
    \fn void QTabBar::tabCloseRequested(int index)
    \since 4.5

    This signal is emitted when the close button on a tab is clicked.
    The \a index is the index that should be removed.

    \sa setTabsClosable()
*/

/*!
    \fn void QTabBar::tabMoved(int from, int to)
    \since 4.5

    This signal is emitted when the tab has moved the tab
    at index position \a from to index position \a to.

    note: QTabWidget will automatically move the page when
    this signal is emitted from its tab bar.

    \sa moveTab()
*/

int QTabBarPrivate::extraWidth() const
{
    Q_Q(const QTabBar);
    return 2 * qMax(q->style()->pixelMetric(QStyle::PM_TabBarScrollButtonWidth, 0, q),
                    QApplication::globalStrut().width());
}

void QTabBarPrivate::init()
{
    Q_Q(QTabBar);
    leftB = new QToolButton(q);
    leftB->setAutoRepeat(true);
    QObject::connect(leftB, SIGNAL(clicked()), q, SLOT(_q_scrollTabs()));
    leftB->hide();
    rightB = new QToolButton(q);
    rightB->setAutoRepeat(true);
    QObject::connect(rightB, SIGNAL(clicked()), q, SLOT(_q_scrollTabs()));
    rightB->hide();
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled()) {
        leftB->setFocusPolicy(Qt::NoFocus);
        rightB->setFocusPolicy(Qt::NoFocus);
        q->setFocusPolicy(Qt::NoFocus);
    } else
#endif
    q->setFocusPolicy(Qt::TabFocus);
    q->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    elideMode = Qt::TextElideMode(q->style()->styleHint(QStyle::SH_TabBar_ElideMode, 0, q));
    useScrollButtons = !q->style()->styleHint(QStyle::SH_TabBar_PreferNoArrows, 0, q);
}

QTabBarPrivate::Tab *QTabBarPrivate::at(int index)
{
    return validIndex(index)?&tabList[index]:0;
}

const QTabBarPrivate::Tab *QTabBarPrivate::at(int index) const
{
    return validIndex(index)?&tabList[index]:0;
}

int QTabBarPrivate::indexAtPos(const QPoint &p) const
{
    Q_Q(const QTabBar);
    if (q->tabRect(currentIndex).contains(p))
        return currentIndex;
    for (int i = 0; i < tabList.count(); ++i)
        if (tabList.at(i).enabled && q->tabRect(i).contains(p))
            return i;
    return -1;
}

void QTabBarPrivate::layoutTabs()
{
    Q_Q(QTabBar);
    scrollOffset = 0;
    layoutDirty = false;
    QSize size = q->size();
    int last, available;
    int maxExtent;
    int i;
    bool vertTabs = verticalTabs(shape);
    int tabChainIndex = 0;

    Qt::Alignment tabAlignment = Qt::Alignment(q->style()->styleHint(QStyle::SH_TabBar_Alignment, 0, q));
    QVector<QLayoutStruct> tabChain(tabList.count() + 2);

    // We put an empty item at the front and back and set its expansive attribute
    // depending on tabAlignment.
    tabChain[tabChainIndex].init();
    tabChain[tabChainIndex].expansive = (tabAlignment != Qt::AlignLeft)
                                        && (tabAlignment != Qt::AlignJustify);
    tabChain[tabChainIndex].empty = true;
    ++tabChainIndex;

    // We now go through our list of tabs and set the minimum size and the size hint
    // This will allow us to elide text if necessary. Since we don't set
    // a maximum size, tabs will EXPAND to fill up the empty space.
    // Since tab widget is rather *ahem* strict about keeping the geometry of the
    // tab bar to its absolute minimum, this won't bleed through, but will show up
    // if you use tab bar on its own (a.k.a. not a bug, but a feature).
    // Update: if expanding is false, we DO set a maximum size to prevent the tabs
    // being wider than necessary.
    if (!vertTabs) {
        int minx = 0;
        int x = 0;
        int maxHeight = 0;
        for (i = 0; i < tabList.count(); ++i, ++tabChainIndex) {
            QSize sz = q->tabSizeHint(i);
            tabList[i].maxRect = QRect(x, 0, sz.width(), sz.height());
            x += sz.width();
            maxHeight = qMax(maxHeight, sz.height());
            sz = minimumTabSizeHint(i);
            tabList[i].minRect = QRect(minx, 0, sz.width(), sz.height());
            minx += sz.width();
            tabChain[tabChainIndex].init();
            tabChain[tabChainIndex].sizeHint = tabList.at(i).maxRect.width();
            tabChain[tabChainIndex].minimumSize = sz.width();
            tabChain[tabChainIndex].empty = false;
            tabChain[tabChainIndex].expansive = true;

            if (!expanding)
                tabChain[tabChainIndex].maximumSize = tabChain[tabChainIndex].sizeHint;
        }

        last = minx;
        available = size.width();
        maxExtent = maxHeight;
    } else {
        int miny = 0;
        int y = 0;
        int maxWidth = 0;
        for (i = 0; i < tabList.count(); ++i, ++tabChainIndex) {
            QSize sz = q->tabSizeHint(i);
            tabList[i].maxRect = QRect(0, y, sz.width(), sz.height());
            y += sz.height();
            maxWidth = qMax(maxWidth, sz.width());
            sz = minimumTabSizeHint(i);
            tabList[i].minRect = QRect(0, miny, sz.width(), sz.height());
            miny += sz.height();
            tabChain[tabChainIndex].init();
            tabChain[tabChainIndex].sizeHint = tabList.at(i).maxRect.height();
            tabChain[tabChainIndex].minimumSize = sz.height();
            tabChain[tabChainIndex].empty = false;
            tabChain[tabChainIndex].expansive = true;

            if (!expanding)
                tabChain[tabChainIndex].maximumSize = tabChain[tabChainIndex].sizeHint;
        }

        last = miny;
        available = size.height();
        maxExtent = maxWidth;
    }

    Q_ASSERT(tabChainIndex == tabChain.count() - 1); // add an assert just to make sure.
    // Mirror our front item.
    tabChain[tabChainIndex].init();
    tabChain[tabChainIndex].expansive = (tabAlignment != Qt::AlignRight)
                                        && (tabAlignment != Qt::AlignJustify);
    tabChain[tabChainIndex].empty = true;

    // Do the calculation
    qGeomCalc(tabChain, 0, tabChain.count(), 0, qMax(available, last), 0);

    // Use the results
    for (i = 0; i < tabList.count(); ++i) {
        const QLayoutStruct &lstruct = tabChain.at(i + 1);
        if (!vertTabs)
            tabList[i].rect.setRect(lstruct.pos, 0, lstruct.size, maxExtent);
        else
            tabList[i].rect.setRect(0, lstruct.pos, maxExtent, lstruct.size);
    }

    if (useScrollButtons && tabList.count() && last > available) {
        int extra = extraWidth();
#ifndef QT_NO_STYLE_S60
        QS60Style *s60Style = qobject_cast<QS60Style*>(QApplication::style());
#endif
        if (!vertTabs) {
            Qt::LayoutDirection ld = q->layoutDirection();
            QRect arrows = QStyle::visualRect(ld, q->rect(),
                                              QRect(available - extra, 0, extra, size.height()));
            int buttonOverlap = q->style()->pixelMetric(QStyle::PM_TabBar_ScrollButtonOverlap, 0, q);

            if (ld == Qt::LeftToRight) {
// In S60style, tab scroll buttons are layoutted separately, on the sides of the tabbar.
#ifndef QT_NO_STYLE_S60
                if (s60Style) {
                    rightB->setGeometry(arrows.left() + extra / 2, arrows.top(), extra / 2, arrows.height());
                    leftB->setGeometry(0, arrows.top(), extra / 2, arrows.height());
                } else {
#endif
                leftB->setGeometry(arrows.left(), arrows.top(), extra/2, arrows.height());
                rightB->setGeometry(arrows.right() - extra/2 + buttonOverlap, arrows.top(),
                                    extra/2, arrows.height());
#ifndef QT_NO_STYLE_S60
                }
#endif
                leftB->setArrowType(Qt::LeftArrow);
                rightB->setArrowType(Qt::RightArrow);
            } else {
#ifndef QT_NO_STYLE_S60
                if (s60Style) {
                    rightB->setGeometry(arrows.left() + extra / 2, arrows.top(), extra / 2, arrows.height());
                    leftB->setGeometry(0, arrows.top(), extra / 2, arrows.height());
                } else {
#endif
                rightB->setGeometry(arrows.left(), arrows.top(), extra/2, arrows.height());
                leftB->setGeometry(arrows.right() - extra/2 + buttonOverlap, arrows.top(),
                                    extra/2, arrows.height());
#ifndef QT_NO_STYLE_S60
                }
#endif
                rightB->setArrowType(Qt::LeftArrow);
                leftB->setArrowType(Qt::RightArrow);
            }
        } else {
#ifndef QT_NO_STYLE_S60
            if (s60Style) {
                QRect arrows = QRect(0, 0, size.width(), available );
                leftB->setGeometry(arrows.left(), arrows.top(), arrows.width(), extra / 2);
                leftB->setArrowType(Qt::UpArrow);
                rightB->setGeometry(arrows.left(), arrows.bottom() - extra / 2 + 1,
                                    arrows.width(), extra / 2);
                rightB->setArrowType(Qt::DownArrow);
            } else {
#endif
            QRect arrows = QRect(0, available - extra, size.width(), extra );
            leftB->setGeometry(arrows.left(), arrows.top(), arrows.width(), extra/2);
            leftB->setArrowType(Qt::UpArrow);
            rightB->setGeometry(arrows.left(), arrows.bottom() - extra/2 + 1,
                                arrows.width(), extra/2);
            rightB->setArrowType(Qt::DownArrow);
#ifndef QT_NO_STYLE_S60
            }
#endif
        }
        leftB->setEnabled(scrollOffset > 0);
        rightB->setEnabled(last - scrollOffset >= available - extra);
        leftB->show();
        rightB->show();
    } else {
        rightB->hide();
        leftB->hide();
    }

    layoutWidgets();
    q->tabLayoutChange();
}

void QTabBarPrivate::makeVisible(int index)
{
    Q_Q(QTabBar);
    if (!validIndex(index) || leftB->isHidden())
        return;

    const QRect tabRect = tabList.at(index).rect;
    const int oldScrollOffset = scrollOffset;
    const bool horiz = !verticalTabs(shape);
    const int available = (horiz ? q->width() : q->height()) - extraWidth();
    const int start = horiz ? tabRect.left() : tabRect.top();
    const int end = horiz ? tabRect.right() : tabRect.bottom();
    if (start < scrollOffset) // too far left
        scrollOffset = start - (index ? 8 : 0);
    else if (end > scrollOffset + available) // too far right
        scrollOffset = end - available + 1;

    leftB->setEnabled(scrollOffset > 0);
    const int last = horiz ? tabList.last().rect.right() : tabList.last().rect.bottom();
    rightB->setEnabled(last - scrollOffset >= available);
    if (oldScrollOffset != scrollOffset) {
        q->update();
        layoutWidgets();
    }
}

void QTabBarPrivate::layoutTab(int index)
{
    Q_Q(QTabBar);
    Q_ASSERT(index >= 0);

    Tab &tab = tabList[index];
    bool vertical = verticalTabs(shape);
    if (!(tab.leftWidget || tab.rightWidget))
        return;

    QStyleOptionTabV3 opt;
    q->initStyleOption(&opt, index);
    if (tab.leftWidget) {
        QRect rect = q->style()->subElementRect(QStyle::SE_TabBarTabLeftButton, &opt, q);
        QPoint p = rect.topLeft();
        if ((index == pressedIndex) || paintWithOffsets) {
            if (vertical)
                p.setY(p.y() + tabList[index].dragOffset);
            else
                p.setX(p.x() + tabList[index].dragOffset);
        }
        tab.leftWidget->move(p);
    }
    if (tab.rightWidget) {
        QRect rect = q->style()->subElementRect(QStyle::SE_TabBarTabRightButton, &opt, q);
        QPoint p = rect.topLeft();
        if ((index == pressedIndex) || paintWithOffsets) {
            if (vertical)
                p.setY(p.y() + tab.dragOffset);
            else
                p.setX(p.x() + tab.dragOffset);
        }
        tab.rightWidget->move(p);
    }
}

void QTabBarPrivate::layoutWidgets(int start)
{
    Q_Q(QTabBar);
    for (int i = start; i < q->count(); ++i) {
        layoutTab(i);
    }
}

void QTabBarPrivate::_q_closeTab()
{
    Q_Q(QTabBar);
    QObject *object = q->sender();
    int tabToClose = -1;
    QTabBar::ButtonPosition closeSide = (QTabBar::ButtonPosition)q->style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, q);
    for (int i = 0; i < tabList.count(); ++i) {
        if (closeSide == QTabBar::LeftSide) {
            if (tabList.at(i).leftWidget == object) {
                tabToClose = i;
                break;
            }
        } else {
            if (tabList.at(i).rightWidget == object) {
                tabToClose = i;
                break;
            }
        }
    }
    if (tabToClose != -1)
        emit q->tabCloseRequested(tabToClose);
}

void QTabBarPrivate::_q_scrollTabs()
{
    Q_Q(QTabBar);
    const QObject *sender = q->sender();
    int i = -1;
    if (!verticalTabs(shape)) {
        if (sender == leftB) {
            for (i = tabList.count() - 1; i >= 0; --i) {
                if (tabList.at(i).rect.left() - scrollOffset < 0) {
                    makeVisible(i);
                    return;
                }
            }
        } else if (sender == rightB) {
            int availableWidth = q->width() - extraWidth();
            for (i = 0; i < tabList.count(); ++i) {
                if (tabList.at(i).rect.right() - scrollOffset > availableWidth) {
                    makeVisible(i);
                    return;
                }
            }
        }
    } else { // vertical
        if (sender == leftB) {
            for (i = tabList.count() - 1; i >= 0; --i) {
                if (tabList.at(i).rect.top() - scrollOffset < 0) {
                    makeVisible(i);
                    return;
                }
            }
        } else if (sender == rightB) {
            int available = q->height() - extraWidth();
            for (i = 0; i < tabList.count(); ++i) {
                if (tabList.at(i).rect.bottom() - scrollOffset > available) {
                    makeVisible(i);
                    return;
                }
            }
        }
    }
}

void QTabBarPrivate::refresh()
{
    Q_Q(QTabBar);

    // be safe in case a subclass is also handling move with the tabs
    if (pressedIndex != -1
        && movable
        && QApplication::mouseButtons() == Qt::NoButton) {
        moveTabFinished(pressedIndex);
        if (!validIndex(pressedIndex))
            pressedIndex = -1;
    }

    if (!q->isVisible()) {
        layoutDirty = true;
    } else {
        layoutTabs();
        makeVisible(currentIndex);
        q->update();
        q->updateGeometry();
    }
}

/*!
    Creates a new tab bar with the given \a parent.
*/
QTabBar::QTabBar(QWidget* parent)
    :QWidget(*new QTabBarPrivate, parent, 0)
{
    Q_D(QTabBar);
    d->init();
}


/*!
    Destroys the tab bar.
*/
QTabBar::~QTabBar()
{
}

/*!
    \property QTabBar::shape
    \brief the shape of the tabs in the tab bar

    Possible values for this property are described by the Shape enum.
*/


QTabBar::Shape QTabBar::shape() const
{
    Q_D(const QTabBar);
    return d->shape;
}

void QTabBar::setShape(Shape shape)
{
    Q_D(QTabBar);
    if (d->shape == shape)
        return;
    d->shape = shape;
    d->refresh();
}

/*!
    \property QTabBar::drawBase
    \brief defines whether or not tab bar should draw its base.

    If true then QTabBar draws a base in relation to the styles overlab.
    Otherwise only the tabs are drawn.

    \sa QStyle::pixelMetric() QStyle::PM_TabBarBaseOverlap QStyleOptionTabBarBaseV2
*/

void QTabBar::setDrawBase(bool drawBase)
{
    Q_D(QTabBar);
    if (d->drawBase == drawBase)
        return;
    d->drawBase = drawBase;
    update();
}

bool QTabBar::drawBase() const
{
    Q_D(const QTabBar);
    return d->drawBase;
}

/*!
    Adds a new tab with text \a text. Returns the new
    tab's index.
*/
int QTabBar::addTab(const QString &text)
{
    return insertTab(-1, text);
}

/*!
    \overload

    Adds a new tab with icon \a icon and text \a
    text. Returns the new tab's index.
*/
int QTabBar::addTab(const QIcon& icon, const QString &text)
{
    return insertTab(-1, icon, text);
}

/*!
    Inserts a new tab with text \a text at position \a index. If \a
    index is out of range, the new tab is appened. Returns the new
    tab's index.
*/
int QTabBar::insertTab(int index, const QString &text)
{
    return insertTab(index, QIcon(), text);
}

/*!\overload

    Inserts a new tab with icon \a icon and text \a text at position
    \a index. If \a index is out of range, the new tab is
    appended. Returns the new tab's index.

    If the QTabBar was empty before this function is called, the inserted tab
    becomes the current tab.

    Inserting a new tab at an index less than or equal to the current index
    will increment the current index, but keep the current tab.
*/
int QTabBar::insertTab(int index, const QIcon& icon, const QString &text)
{
    Q_D(QTabBar);
    if (!d->validIndex(index)) {
        index = d->tabList.count();
        d->tabList.append(QTabBarPrivate::Tab(icon, text));
    } else {
        d->tabList.insert(index, QTabBarPrivate::Tab(icon, text));
    }
#ifndef QT_NO_SHORTCUT
    d->tabList[index].shortcutId = grabShortcut(QKeySequence::mnemonic(text));
#endif
    d->refresh();
    if (d->tabList.count() == 1)
        setCurrentIndex(index);
    else if (index <= d->currentIndex)
        ++d->currentIndex;

    if (d->closeButtonOnTabs) {
        QStyleOptionTabV3 opt;
        initStyleOption(&opt, index);
        ButtonPosition closeSide = (ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, this);
        QAbstractButton *closeButton = new CloseButton(this);
        connect(closeButton, SIGNAL(clicked()), this, SLOT(_q_closeTab()));
        setTabButton(index, closeSide, closeButton);
    }

    for (int i = 0; i < d->tabList.count(); ++i) {
        if (d->tabList[i].lastTab >= index)
            ++d->tabList[i].lastTab;
    }

    tabInserted(index);
    return index;
}


/*!
    Removes the tab at position \a index.

    \sa SelectionBehavior
 */
void QTabBar::removeTab(int index)
{
    Q_D(QTabBar);
    if (d->validIndex(index)) {
#ifndef QT_NO_SHORTCUT
        releaseShortcut(d->tabList.at(index).shortcutId);
#endif
        if (d->tabList[index].leftWidget) {
            d->tabList[index].leftWidget->hide();
            d->tabList[index].leftWidget->deleteLater();
            d->tabList[index].leftWidget = 0;
        }
        if (d->tabList[index].rightWidget) {
            d->tabList[index].rightWidget->hide();
            d->tabList[index].rightWidget->deleteLater();
            d->tabList[index].rightWidget = 0;
        }

        int newIndex = d->tabList[index].lastTab;
        d->tabList.removeAt(index);
        for (int i = 0; i < d->tabList.count(); ++i) {
            if (d->tabList[i].lastTab == index)
                d->tabList[i].lastTab = -1;
            if (d->tabList[i].lastTab > index)
                --d->tabList[i].lastTab;
        }
        if (index == d->currentIndex) {
            // The current tab is going away, in order to make sure
            // we emit that "current has changed", we need to reset this
            // around.
            d->currentIndex = -1;
            if (d->tabList.size() > 0) {
                switch(d->selectionBehaviorOnRemove) {
                case SelectPreviousTab:
                    if (newIndex > index)
                        newIndex--;
                    if (d->validIndex(newIndex))
                        break;
                    // else fallthrough
                case SelectRightTab:
                    newIndex = index;
                    if (newIndex >= d->tabList.size())
                        newIndex = d->tabList.size() - 1;
                    break;
                case SelectLeftTab:
                    newIndex = index - 1;
                    if (newIndex < 0)
                        newIndex = 0;
                    break;
                default:
                    break;
                }

                if (d->validIndex(newIndex)) {
                    // don't loose newIndex's old through setCurrentIndex
                    int bump = d->tabList[newIndex].lastTab;
                    setCurrentIndex(newIndex);
                    d->tabList[newIndex].lastTab = bump;
                }
            } else {
                emit currentChanged(-1);
            }
        } else if (index < d->currentIndex) {
            setCurrentIndex(d->currentIndex - 1);
        }
        d->refresh();
        tabRemoved(index);
    }
}


/*!
    Returns true if the tab at position \a index is enabled; otherwise
    returns false.
*/
bool QTabBar::isTabEnabled(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->enabled;
    return false;
}

/*!
    If \a enabled is true then the tab at position \a index is
    enabled; otherwise the item at position \a index is disabled.
*/
void QTabBar::setTabEnabled(int index, bool enabled)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        tab->enabled = enabled;
#ifndef QT_NO_SHORTCUT
        setShortcutEnabled(tab->shortcutId, enabled);
#endif
        update();
        if (!enabled && index == d->currentIndex)
            setCurrentIndex(d->validIndex(index+1)?index+1:0);
        else if (enabled && !d->validIndex(d->currentIndex))
            setCurrentIndex(index);
    }
}


/*!
    Returns the text of the tab at position \a index, or an empty
    string if \a index is out of range.
*/
QString QTabBar::tabText(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->text;
    return QString();
}

/*!
    Sets the text of the tab at position \a index to \a text.
*/
void QTabBar::setTabText(int index, const QString &text)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        tab->text = text;
#ifndef QT_NO_SHORTCUT
        releaseShortcut(tab->shortcutId);
        tab->shortcutId = grabShortcut(QKeySequence::mnemonic(text));
        setShortcutEnabled(tab->shortcutId, tab->enabled);
#endif
        d->refresh();
    }
}

/*!
    Returns the text color of the tab with the given \a index, or a invalid
    color if \a index is out of range.

    \sa setTabTextColor()
*/
QColor QTabBar::tabTextColor(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->textColor;
    return QColor();
}

/*!
    Sets the color of the text in the tab with the given \a index to the specified \a color.

    If an invalid color is specified, the tab will use the QTabBar foreground role instead.

    \sa tabTextColor()
*/
void QTabBar::setTabTextColor(int index, const QColor &color)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        tab->textColor = color;
        update(tabRect(index));
    }
}

/*!
    Returns the icon of the tab at position \a index, or a null icon
    if \a index is out of range.
*/
QIcon QTabBar::tabIcon(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->icon;
    return QIcon();
}

/*!
    Sets the icon of the tab at position \a index to \a icon.
*/
void QTabBar::setTabIcon(int index, const QIcon & icon)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index)) {
        bool simpleIconChange = (!icon.isNull() && !tab->icon.isNull());
        tab->icon = icon;
        if (simpleIconChange)
            update(tabRect(index));
        else
            d->refresh();
    }
}

#ifndef QT_NO_TOOLTIP
/*!
    Sets the tool tip of the tab at position \a index to \a tip.
*/
void QTabBar::setTabToolTip(int index, const QString & tip)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index))
        tab->toolTip = tip;
}

/*!
    Returns the tool tip of the tab at position \a index, or an empty
    string if \a index is out of range.
*/
QString QTabBar::tabToolTip(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->toolTip;
    return QString();
}
#endif // QT_NO_TOOLTIP

#ifndef QT_NO_WHATSTHIS
/*!
    \since 4.1

    Sets the What's This help text of the tab at position \a index
    to \a text.
*/
void QTabBar::setTabWhatsThis(int index, const QString &text)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index))
        tab->whatsThis = text;
}

/*!
    \since 4.1

    Returns the What's This help text of the tab at position \a index,
    or an empty string if \a index is out of range.
*/
QString QTabBar::tabWhatsThis(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->whatsThis;
    return QString();
}

#endif // QT_NO_WHATSTHIS

/*!
    Sets the data of the tab at position \a index to \a data.
*/
void QTabBar::setTabData(int index, const QVariant & data)
{
    Q_D(QTabBar);
    if (QTabBarPrivate::Tab *tab = d->at(index))
        tab->data = data;
}

/*!
    Returns the data of the tab at position \a index, or a null
    variant if \a index is out of range.
*/
QVariant QTabBar::tabData(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index))
        return tab->data;
    return QVariant();
}

/*!
    Returns the visual rectangle of the tab at position \a
    index, or a null rectangle if \a index is out of range.
*/
QRect QTabBar::tabRect(int index) const
{
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index)) {
        if (d->layoutDirty)
            const_cast<QTabBarPrivate*>(d)->layoutTabs();
        QRect r = tab->rect;
        if (verticalTabs(d->shape))
            r.translate(0, -d->scrollOffset);
        else
            r.translate(-d->scrollOffset, 0);
        if (!verticalTabs(d->shape))
            r = QStyle::visualRect(layoutDirection(), rect(), r);
        return r;
    }
    return QRect();
}

/*!
    \since 4.3
    Returns the index of the tab that covers \a position or -1 if no
    tab covers \a position;
*/

int QTabBar::tabAt(const QPoint &position) const
{
    Q_D(const QTabBar);
    if (d->validIndex(d->currentIndex)
        && tabRect(d->currentIndex).contains(position)) {
        return d->currentIndex;
    }
    const int max = d->tabList.size();
    for (int i = 0; i < max; ++i) {
        if (tabRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

/*!
    \property QTabBar::currentIndex
    \brief the index of the tab bar's visible tab

    The current index is -1 if there is no current tab.
*/

int QTabBar::currentIndex() const
{
    Q_D(const QTabBar);
    if (d->validIndex(d->currentIndex))
        return d->currentIndex;
    return -1;
}


void QTabBar::setCurrentIndex(int index)
{
    Q_D(QTabBar);
    if (d->dragInProgress && d->pressedIndex != -1)
        return;

    int oldIndex = d->currentIndex;
    if (d->validIndex(index) && d->currentIndex != index) {
        d->currentIndex = index;
        update();
        d->makeVisible(index);
        d->tabList[index].lastTab = oldIndex;
        if (oldIndex >= 0 && oldIndex < count())
            d->layoutTab(oldIndex);
        d->layoutTab(index);
#ifndef QT_NO_ACCESSIBILITY
        if (QAccessible::isActive()) {
            QAccessible::updateAccessibility(this, oldIndex + 1, QAccessible::Selection);

            QAccessible::updateAccessibility(this, index + 1, QAccessible::Focus);
            QAccessible::updateAccessibility(this, index + 1, QAccessible::Selection);
        }
#endif
#ifdef QT3_SUPPORT
        emit selected(index);
#endif
        emit currentChanged(index);
    }
}

/*!
    \property QTabBar::iconSize
    \brief The size for icons in the tab bar
    \since 4.1

    The default value is style-dependent. \c iconSize is a maximum
    size; icons that are smaller are not scaled up.

    \sa QTabWidget::iconSize
*/
QSize QTabBar::iconSize() const
{
    Q_D(const QTabBar);
    if (d->iconSize.isValid())
        return d->iconSize;
    int iconExtent = style()->pixelMetric(QStyle::PM_TabBarIconSize, 0, this);
    return QSize(iconExtent, iconExtent);

}

void QTabBar::setIconSize(const QSize &size)
{
    Q_D(QTabBar);
    d->iconSize = size;
    d->layoutDirty = true;
    update();
    updateGeometry();
}

/*!
    \property QTabBar::count
    \brief the number of tabs in the tab bar
*/

int QTabBar::count() const
{
    Q_D(const QTabBar);
    return d->tabList.count();
}


/*!\reimp
 */
QSize QTabBar::sizeHint() const
{
    Q_D(const QTabBar);
    if (d->layoutDirty)
        const_cast<QTabBarPrivate*>(d)->layoutTabs();
    QRect r;
    for (int i = 0; i < d->tabList.count(); ++i)
        r = r.united(d->tabList.at(i).maxRect);
    QSize sz = QApplication::globalStrut();
    return r.size().expandedTo(sz);
}

/*!\reimp
 */
QSize QTabBar::minimumSizeHint() const
{
    Q_D(const QTabBar);
    if (d->layoutDirty)
        const_cast<QTabBarPrivate*>(d)->layoutTabs();
    if (!d->useScrollButtons) {
        QRect r;
        for (int i = 0; i < d->tabList.count(); ++i)
            r = r.united(d->tabList.at(i).minRect);
        return r.size().expandedTo(QApplication::globalStrut());
    }
    if (verticalTabs(d->shape))
        return QSize(sizeHint().width(), d->rightB->sizeHint().height() * 2 + 75);
    else
        return QSize(d->rightB->sizeHint().width() * 2 + 75, sizeHint().height());
}

// Compute the most-elided possible text, for minimumSizeHint
static QString computeElidedText(Qt::TextElideMode mode, const QString &text)
{
    if (text.length() <= 3)
        return text;

    static const QLatin1String Ellipses("...");
    QString ret;
    switch (mode) {
    case Qt::ElideRight:
        ret = text.left(2) + Ellipses;
        break;
    case Qt::ElideMiddle:
        ret = text.left(1) + Ellipses + text.right(1);
        break;
    case Qt::ElideLeft:
        ret = Ellipses + text.right(2);
        break;
    case Qt::ElideNone:
        ret = text;
        break;
    }
    return ret;
}

QSize QTabBarPrivate::minimumTabSizeHint(int index)
{
    Q_Q(QTabBar);
    // ### Qt 5: make this a protected virtual function in QTabBar
    Tab &tab = tabList[index];
    QString oldText = tab.text;
    tab.text = computeElidedText(elideMode, oldText);
    QSize size = q->tabSizeHint(index);
    tab.text = oldText;
    return size;
}

/*!
    Returns the size hint for the tab at position \a index.
*/
QSize QTabBar::tabSizeHint(int index) const
{
    //Note: this must match with the computations in QCommonStylePrivate::tabLayout
    Q_D(const QTabBar);
    if (const QTabBarPrivate::Tab *tab = d->at(index)) {
        QStyleOptionTabV3 opt;
        initStyleOption(&opt, index);
        opt.text = d->tabList.at(index).text;
        QSize iconSize = tab->icon.isNull() ? QSize(0, 0) : opt.iconSize;
        int hframe = style()->pixelMetric(QStyle::PM_TabBarTabHSpace, &opt, this);
        int vframe = style()->pixelMetric(QStyle::PM_TabBarTabVSpace, &opt, this);
        const QFontMetrics fm = fontMetrics();

        int maxWidgetHeight = qMax(opt.leftButtonSize.height(), opt.rightButtonSize.height());
        int maxWidgetWidth = qMax(opt.leftButtonSize.width(), opt.rightButtonSize.width());

        int widgetWidth = 0;
        int widgetHeight = 0;
        int padding = 0;
        if (!opt.leftButtonSize.isEmpty()) {
            padding += 4;
            widgetWidth += opt.leftButtonSize.width();
            widgetHeight += opt.leftButtonSize.height();
        }
        if (!opt.rightButtonSize.isEmpty()) {
            padding += 4;
            widgetWidth += opt.rightButtonSize.width();
            widgetHeight += opt.rightButtonSize.height();
        }
        if (!opt.icon.isNull())
            padding += 4;

        QSize csz;
        if (verticalTabs(d->shape)) {
            csz = QSize( qMax(maxWidgetWidth, qMax(fm.height(), iconSize.height())) + vframe,
                    fm.size(Qt::TextShowMnemonic, tab->text).width() + iconSize.width() + hframe + widgetHeight + padding);
        } else {
            csz = QSize(fm.size(Qt::TextShowMnemonic, tab->text).width() + iconSize.width() + hframe
                  + widgetWidth + padding,
                  qMax(maxWidgetHeight, qMax(fm.height(), iconSize.height())) + vframe);
        }

        QSize retSize = style()->sizeFromContents(QStyle::CT_TabBarTab, &opt, csz, this);
        return retSize;
    }
    return QSize();
}

/*!
  This virtual handler is called after a new tab was added or
  inserted at position \a index.

  \sa tabRemoved()
 */
void QTabBar::tabInserted(int index)
{
    Q_UNUSED(index)
}

/*!
  This virtual handler is called after a tab was removed from
  position \a index.

  \sa tabInserted()
 */
void QTabBar::tabRemoved(int index)
{
    Q_UNUSED(index)
}

/*!
  This virtual handler is called whenever the tab layout changes.

  \sa tabRect()
 */
void QTabBar::tabLayoutChange()
{
}


/*!\reimp
 */
void QTabBar::showEvent(QShowEvent *)
{
    Q_D(QTabBar);
    if (d->layoutDirty)
        d->refresh();
    if (!d->validIndex(d->currentIndex))
        setCurrentIndex(0);
    d->updateMacBorderMetrics();
}

/*!\reimp
 */
void QTabBar::hideEvent(QHideEvent *)
{
    Q_D(QTabBar);
    d->updateMacBorderMetrics();
}

/*!\reimp
 */
bool QTabBar::event(QEvent *event)
{
    Q_D(QTabBar);
    if (event->type() == QEvent::HoverMove
        || event->type() == QEvent::HoverEnter) {
        QHoverEvent *he = static_cast<QHoverEvent *>(event);
        if (!d->hoverRect.contains(he->pos())) {
            QRect oldHoverRect = d->hoverRect;
            for (int i = 0; i < d->tabList.count(); ++i) {
                QRect area = tabRect(i);
                if (area.contains(he->pos())) {
                    d->hoverRect = area;
                    break;
                }
            }
            if (he->oldPos() != QPoint(-1, -1))
                update(oldHoverRect);
            update(d->hoverRect);
        }
        return true;
    } else if (event->type() == QEvent::HoverLeave ) {
        QRect oldHoverRect = d->hoverRect;
        d->hoverRect = QRect();
        update(oldHoverRect);
        return true;
#ifndef QT_NO_TOOLTIP
    } else if (event->type() == QEvent::ToolTip) {
        if (const QTabBarPrivate::Tab *tab = d->at(tabAt(static_cast<QHelpEvent*>(event)->pos()))) {
            if (!tab->toolTip.isEmpty()) {
                QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), tab->toolTip, this);
                return true;
            }
        }
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_WHATSTHIS
    } else if (event->type() == QEvent::QueryWhatsThis) {
        const QTabBarPrivate::Tab *tab = d->at(d->indexAtPos(static_cast<QHelpEvent*>(event)->pos()));
        if (!tab || tab->whatsThis.isEmpty())
            event->ignore();
        return true;
    } else if (event->type() == QEvent::WhatsThis) {
        if (const QTabBarPrivate::Tab *tab = d->at(d->indexAtPos(static_cast<QHelpEvent*>(event)->pos()))) {
            if (!tab->whatsThis.isEmpty()) {
                QWhatsThis::showText(static_cast<QHelpEvent*>(event)->globalPos(),
                                     tab->whatsThis, this);
                return true;
            }
        }
#endif // QT_NO_WHATSTHIS
#ifndef QT_NO_SHORTCUT
    } else if (event->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(event);
        for (int i = 0; i < d->tabList.count(); ++i) {
            const QTabBarPrivate::Tab *tab = &d->tabList.at(i);
            if (tab->shortcutId == se->shortcutId()) {
                setCurrentIndex(i);
                return true;
            }
        }
#endif
    }
    return QWidget::event(event);
}

/*!\reimp
 */
void QTabBar::resizeEvent(QResizeEvent *)
{
    Q_D(QTabBar);
    if (d->layoutDirty)
        updateGeometry();
    d->layoutTabs();

    d->makeVisible(d->currentIndex);
}

/*!\reimp
 */
void QTabBar::paintEvent(QPaintEvent *)
{
    Q_D(QTabBar);

    QStyleOptionTabBarBaseV2 optTabBase;
    QTabBarPrivate::initStyleBaseOption(&optTabBase, this, size());

    QStylePainter p(this);
    int selected = -1;
    int cut = -1;
    bool rtl = optTabBase.direction == Qt::RightToLeft;
    bool vertical = verticalTabs(d->shape);
    QStyleOptionTab cutTab;
    selected = d->currentIndex;
    if (d->dragInProgress)
        selected = d->pressedIndex;

    for (int i = 0; i < d->tabList.count(); ++i)
         optTabBase.tabBarRect |= tabRect(i);

    optTabBase.selectedTabRect = tabRect(selected);

    if (d->drawBase)
        p.drawPrimitive(QStyle::PE_FrameTabBarBase, optTabBase);

    for (int i = 0; i < d->tabList.count(); ++i) {
        QStyleOptionTabV3 tab;
        initStyleOption(&tab, i);
        if (d->paintWithOffsets && d->tabList[i].dragOffset != 0) {
            if (vertical) {
                tab.rect.moveTop(tab.rect.y() + d->tabList[i].dragOffset);
            } else {
                tab.rect.moveLeft(tab.rect.x() + d->tabList[i].dragOffset);
            }
        }
        if (!(tab.state & QStyle::State_Enabled)) {
            tab.palette.setCurrentColorGroup(QPalette::Disabled);
        }
        // If this tab is partially obscured, make a note of it so that we can pass the information
        // along when we draw the tear.
        if (((!vertical && (!rtl && tab.rect.left() < 0)) || (rtl && tab.rect.right() > width()))
            || (vertical && tab.rect.top() < 0)) {
            cut = i;
            cutTab = tab;
        }
        // Don't bother drawing a tab if the entire tab is outside of the visible tab bar.
        if ((!vertical && (tab.rect.right() < 0 || tab.rect.left() > width()))
            || (vertical && (tab.rect.bottom() < 0 || tab.rect.top() > height())))
            continue;

        optTabBase.tabBarRect |= tab.rect;
        if (i == selected)
            continue;

        p.drawControl(QStyle::CE_TabBarTab, tab);
    }

    // Draw the selected tab last to get it "on top"
    if (selected >= 0) {
        QStyleOptionTabV3 tab;
        initStyleOption(&tab, selected);
        if (d->paintWithOffsets && d->tabList[selected].dragOffset != 0) {
            if (vertical)
                tab.rect.moveTop(tab.rect.y() + d->tabList[selected].dragOffset);
            else
                tab.rect.moveLeft(tab.rect.x() + d->tabList[selected].dragOffset);
        }
        if (!d->dragInProgress)
            p.drawControl(QStyle::CE_TabBarTab, tab);
        else {
            int taboverlap = style()->pixelMetric(QStyle::PM_TabBarTabOverlap, 0, this);
            d->movingTab->setGeometry(tab.rect.adjusted(-taboverlap, 0, taboverlap, 0));
        }
    }

    // Only draw the tear indicator if necessary. Most of the time we don't need too.
    if (d->leftB->isVisible() && cut >= 0) {
        cutTab.rect = rect();
        cutTab.rect = style()->subElementRect(QStyle::SE_TabBarTearIndicator, &cutTab, this);
        p.drawPrimitive(QStyle::PE_IndicatorTabTear, cutTab);
    }
}

/*
    Given that index at position from moved to position to where return where index goes.
 */
int QTabBarPrivate::calculateNewPosition(int from, int to, int index) const
{
    if (index == from)
        return to;

    int start = qMin(from, to);
    int end = qMax(from, to);
    if (index >= start && index <= end)
        index += (from < to) ? -1 : 1;
    return index;
}

/*!
    Moves the item at index position \a from to index position \a to.
    \since 4.5

    \sa tabMoved(), tabLayoutChange()
 */
void QTabBar::moveTab(int from, int to)
{
    Q_D(QTabBar);
    if (from == to
        || !d->validIndex(from)
        || !d->validIndex(to))
        return;

    bool vertical = verticalTabs(d->shape);
    int oldPressedPosition = 0;
    if (d->pressedIndex != -1) {
        // Record the position of the pressed tab before reordering the tabs.
        oldPressedPosition = vertical ? d->tabList[d->pressedIndex].rect.y()
                             : d->tabList[d->pressedIndex].rect.x();
    }

    // Update the locations of the tabs first
    int start = qMin(from, to);
    int end = qMax(from, to);
    int width = vertical ? d->tabList[from].rect.height() : d->tabList[from].rect.width();
    if (from < to)
        width *= -1;
    bool rtl = isRightToLeft();
    for (int i = start; i <= end; ++i) {
        if (i == from)
            continue;
        if (vertical)
            d->tabList[i].rect.moveTop(d->tabList[i].rect.y() + width);
        else
            d->tabList[i].rect.moveLeft(d->tabList[i].rect.x() + width);
        int direction = -1;
        if (rtl && !vertical)
            direction *= -1;
        if (d->tabList[i].dragOffset != 0)
            d->tabList[i].dragOffset += (direction * width);
    }

    if (vertical) {
        if (from < to)
            d->tabList[from].rect.moveTop(d->tabList[to].rect.bottom() + 1);
        else
            d->tabList[from].rect.moveTop(d->tabList[to].rect.top() - width);
    } else {
        if (from < to)
            d->tabList[from].rect.moveLeft(d->tabList[to].rect.right() + 1);
        else
            d->tabList[from].rect.moveLeft(d->tabList[to].rect.left() - width);
    }

    // Move the actual data structures
    d->tabList.move(from, to);

    // update lastTab locations
    for (int i = 0; i < d->tabList.count(); ++i)
        d->tabList[i].lastTab = d->calculateNewPosition(from, to, d->tabList[i].lastTab);

    // update external variables
    d->currentIndex = d->calculateNewPosition(from, to, d->currentIndex);

    // If we are in the middle of a drag update the dragStartPosition
    if (d->pressedIndex != -1) {
        d->pressedIndex = d->calculateNewPosition(from, to, d->pressedIndex);
        int newPressedPosition = vertical ? d->tabList[d->pressedIndex].rect.top() : d->tabList[d->pressedIndex].rect.left();
        int diff = oldPressedPosition - newPressedPosition;
        if (isRightToLeft() && !vertical)
            diff *= -1;
        if (vertical)
            d->dragStartPosition.setY(d->dragStartPosition.y() - diff);
        else
            d->dragStartPosition.setX(d->dragStartPosition.x() - diff);
    }

    d->layoutWidgets(start);
    update();
    emit tabMoved(from, to);
    emit tabLayoutChange();
}

void QTabBarPrivate::slide(int from, int to)
{
    Q_Q(QTabBar);
    if (from == to
            || !validIndex(from)
            || !validIndex(to))
        return;
    bool vertical = verticalTabs(shape);
    int preLocation = vertical ? q->tabRect(from).y() : q->tabRect(from).x();
    q->setUpdatesEnabled(false);
    q->moveTab(from, to);
    q->setUpdatesEnabled(true);
    int postLocation = vertical ? q->tabRect(to).y() : q->tabRect(to).x();
    int length = postLocation - preLocation;
    tabList[to].dragOffset -= length;
    tabList[to].startAnimation(this, ANIMATION_DURATION);
}

void QTabBarPrivate::moveTab(int index, int offset)
{
    if (!validIndex(index))
        return;
    tabList[index].dragOffset = offset;
    layoutTab(index); // Make buttons follow tab
    q_func()->update();
}

/*!\reimp
*/
void QTabBar::mousePressEvent(QMouseEvent *event)
{
    Q_D(QTabBar);
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    // Be safe!
    if (d->pressedIndex != -1 && d->movable)
        d->moveTabFinished(d->pressedIndex);

    d->pressedIndex = d->indexAtPos(event->pos());
#ifdef Q_WS_MAC
    d->previousPressedIndex = d->pressedIndex;
#endif
    if (d->validIndex(d->pressedIndex)) {
        QStyleOptionTabBarBaseV2 optTabBase;
        optTabBase.init(this);
        optTabBase.documentMode = d->documentMode;
        if (event->type() == style()->styleHint(QStyle::SH_TabBar_SelectMouseType, &optTabBase, this))
            setCurrentIndex(d->pressedIndex);
        else
            repaint(tabRect(d->pressedIndex));
        if (d->movable) {
            d->dragStartPosition = event->pos();
        }
    }
}

/*!\reimp
 */
void QTabBar::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QTabBar);
    if (d->movable) {
        // Be safe!
        if (d->pressedIndex != -1
            && event->buttons() == Qt::NoButton)
            d->moveTabFinished(d->pressedIndex);
        
        // Start drag
        if (!d->dragInProgress && d->pressedIndex != -1) {
            if ((event->pos() - d->dragStartPosition).manhattanLength() > QApplication::startDragDistance()) {
                d->dragInProgress = true;
                d->setupMovableTab();
            }
        }

        int offset = (event->pos() - d->dragStartPosition).manhattanLength();
        if (event->buttons() == Qt::LeftButton
            && offset > QApplication::startDragDistance()
            && d->validIndex(d->pressedIndex)) {
            bool vertical = verticalTabs(d->shape);
            int dragDistance;
            if (vertical) {
                dragDistance = (event->pos().y() - d->dragStartPosition.y());
            } else {
                dragDistance = (event->pos().x() - d->dragStartPosition.x());
            }
            d->tabList[d->pressedIndex].dragOffset = dragDistance;

            QRect startingRect = tabRect(d->pressedIndex);
            if (vertical)
                startingRect.moveTop(startingRect.y() + dragDistance);
            else
                startingRect.moveLeft(startingRect.x() + dragDistance);

            int overIndex;
            if (dragDistance < 0)
                overIndex = tabAt(startingRect.topLeft());
            else
                overIndex = tabAt(startingRect.topRight());

            if (overIndex != d->pressedIndex && overIndex != -1) {
                int offset = 1;
                if (isRightToLeft() && !vertical)
                    offset *= -1;
                if (dragDistance < 0) {
                    dragDistance *= -1;
                    offset *= -1;
                }
                for (int i = d->pressedIndex;
                     offset > 0 ? i < overIndex : i > overIndex;
                     i += offset) {
                    QRect overIndexRect = tabRect(overIndex);
                    int needsToBeOver = (vertical ? overIndexRect.height() : overIndexRect.width()) / 2;
                    if (dragDistance > needsToBeOver)
                        d->slide(i + offset, d->pressedIndex);
                }
            }
            // Buttons needs to follow the dragged tab
            d->layoutTab(d->pressedIndex);

            update();
        }
#ifdef Q_WS_MAC
    } else if (!d->documentMode && event->buttons() == Qt::LeftButton && d->previousPressedIndex != -1) {
        int newPressedIndex = d->indexAtPos(event->pos());
        if (d->pressedIndex == -1 && d->previousPressedIndex == newPressedIndex) {
            d->pressedIndex = d->previousPressedIndex;
            update(tabRect(d->pressedIndex));
        } else if(d->pressedIndex != newPressedIndex) {
            d->pressedIndex = -1;
            update(tabRect(d->previousPressedIndex));
        }
#endif
    }

    if (event->buttons() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    QStyleOptionTabBarBaseV2 optTabBase;
    optTabBase.init(this);
    optTabBase.documentMode = d->documentMode;
}

void QTabBarPrivate::setupMovableTab()
{
    Q_Q(QTabBar);
    if (!movingTab)
        movingTab = new QWidget(q);

    int taboverlap = q->style()->pixelMetric(QStyle::PM_TabBarTabOverlap, 0 ,q);
    QRect grabRect = q->tabRect(pressedIndex);
    grabRect.adjust(-taboverlap, 0, taboverlap, 0);

    QPixmap grabImage(grabRect.size());
    grabImage.fill(Qt::transparent);
    QStylePainter p(&grabImage, q);
    p.initFrom(q);

    QStyleOptionTabV3 tab;
    q->initStyleOption(&tab, pressedIndex);
    tab.rect.moveTopLeft(QPoint(taboverlap, 0));
    p.drawControl(QStyle::CE_TabBarTab, tab);
    p.end();

    QPalette pal;
    pal.setBrush(QPalette::All, QPalette::Window, grabImage);
    movingTab->setPalette(pal);
    movingTab->setGeometry(grabRect);
    movingTab->setAutoFillBackground(true);
    movingTab->raise();

    // Re-arrange widget order to avoid overlaps
    if (tabList[pressedIndex].leftWidget)
        tabList[pressedIndex].leftWidget->raise();
    if (tabList[pressedIndex].rightWidget)
        tabList[pressedIndex].rightWidget->raise();
    if (leftB)
        leftB->raise();
    if (rightB)
        rightB->raise();
    movingTab->setVisible(true);
}

void QTabBarPrivate::moveTabFinished(int index)
{
    Q_Q(QTabBar);
    bool cleanup = (pressedIndex == index) || (pressedIndex == -1) || !validIndex(index);
    bool allAnimationsFinished = true;
#ifndef QT_NO_ANIMATION
    for(int i = 0; allAnimationsFinished && i < tabList.count(); ++i) {
        const Tab &t = tabList.at(i);
        if (t.animation && t.animation->state() == QAbstractAnimation::Running)
            allAnimationsFinished = false;
    }
#endif //QT_NO_ANIMATION
    if (allAnimationsFinished && cleanup) {
        if(movingTab)
            movingTab->setVisible(false); // We might not get a mouse release
        for (int i = 0; i < tabList.count(); ++i) {
            tabList[i].dragOffset = 0;
        }
        if (pressedIndex != -1 && movable) {
            pressedIndex = -1;
            dragInProgress = false;
            dragStartPosition = QPoint();
        }
        layoutWidgets();
    } else {
        if (!validIndex(index))
            return;
        tabList[index].dragOffset = 0;
    }
    q->update();
}

/*!\reimp
*/
void QTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QTabBar);
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
#ifdef Q_WS_MAC
    d->previousPressedIndex = -1;
#endif
    if (d->movable && d->dragInProgress && d->validIndex(d->pressedIndex)) {
        int length = d->tabList[d->pressedIndex].dragOffset;
        int width = verticalTabs(d->shape)
            ? tabRect(d->pressedIndex).height()
            : tabRect(d->pressedIndex).width();
        int duration = qMin(ANIMATION_DURATION,
                (qAbs(length) * ANIMATION_DURATION) / width);
        d->tabList[d->pressedIndex].startAnimation(d, duration);
        d->dragInProgress = false;
        d->movingTab->setVisible(false);
        d->dragStartPosition = QPoint();
    }

    int i = d->indexAtPos(event->pos()) == d->pressedIndex ? d->pressedIndex : -1;
    d->pressedIndex = -1;
    QStyleOptionTabBarBaseV2 optTabBase;
    optTabBase.initFrom(this);
    optTabBase.documentMode = d->documentMode;
    if (style()->styleHint(QStyle::SH_TabBar_SelectMouseType, &optTabBase, this) == QEvent::MouseButtonRelease)
        setCurrentIndex(i);
}

/*!\reimp
 */
void QTabBar::keyPressEvent(QKeyEvent *event)
{
    Q_D(QTabBar);
    if (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right) {
        event->ignore();
        return;
    }
    int offset = event->key() == (isRightToLeft() ? Qt::Key_Right : Qt::Key_Left) ? -1 : 1;
    d->setCurrentNextEnabledIndex(offset);
}

/*!\reimp
 */
#ifndef QT_NO_WHEELEVENT
void QTabBar::wheelEvent(QWheelEvent *event)
{
    Q_D(QTabBar);
    int offset = event->delta() > 0 ? -1 : 1;
    d->setCurrentNextEnabledIndex(offset);
    QWidget::wheelEvent(event);
}
#endif //QT_NO_WHEELEVENT

void QTabBarPrivate::setCurrentNextEnabledIndex(int offset)
{
    Q_Q(QTabBar);
    for (int index = currentIndex + offset; validIndex(index); index += offset) {
        if (tabList.at(index).enabled) {
            q->setCurrentIndex(index);
            break;
        }
    }
}

/*!\reimp
 */
void QTabBar::changeEvent(QEvent *event)
{
    Q_D(QTabBar);
    if (event->type() == QEvent::StyleChange) {
        if (!d->elideModeSetByUser)
            d->elideMode = Qt::TextElideMode(style()->styleHint(QStyle::SH_TabBar_ElideMode, 0, this));
        if (!d->useScrollButtonsSetByUser)
            d->useScrollButtons = !style()->styleHint(QStyle::SH_TabBar_PreferNoArrows, 0, this);
        d->refresh();
    } else if (event->type() == QEvent::FontChange) {
        d->refresh();
    }
    QWidget::changeEvent(event);
}

/*!
    \property QTabBar::elideMode
    \brief how to elide text in the tab bar
    \since 4.2

    This property controls how items are elided when there is not
    enough space to show them for a given tab bar size.

    By default the value is style dependent.

    \sa QTabWidget::elideMode usesScrollButtons QStyle::SH_TabBar_ElideMode
*/

Qt::TextElideMode QTabBar::elideMode() const
{
    Q_D(const QTabBar);
    return d->elideMode;
}

void QTabBar::setElideMode(Qt::TextElideMode mode)
{
    Q_D(QTabBar);
    d->elideMode = mode;
    d->elideModeSetByUser = true;
    d->refresh();
}

/*!
    \property QTabBar::usesScrollButtons
    \brief Whether or not a tab bar should use buttons to scroll tabs when it
    has many tabs.
    \since 4.2

    When there are too many tabs in a tab bar for its size, the tab bar can either choose
    to expand its size or to add buttons that allow you to scroll through the tabs.

    By default the value is style dependant.

    \sa elideMode QTabWidget::usesScrollButtons QStyle::SH_TabBar_PreferNoArrows
*/
bool QTabBar::usesScrollButtons() const
{
    return d_func()->useScrollButtons;
}

void QTabBar::setUsesScrollButtons(bool useButtons)
{
    Q_D(QTabBar);
    d->useScrollButtonsSetByUser = true;
    if (d->useScrollButtons == useButtons)
        return;
    d->useScrollButtons = useButtons;
    d->refresh();
}

/*!
    \fn void QTabBar::setCurrentTab(int index)

    Use setCurrentIndex() instead.
*/

/*!
    \fn void QTabBar::selected(int index);

    Use currentChanged() instead.
*/


/*!
    \property QTabBar::tabsClosable
    \brief Whether or not a tab bar should place close buttons on each tab
    \since 4.5

    When tabsClosable is set to true a close button will appear on the tab on
    either the left or right hand side depending upon the style.  When the button
    is clicked the tab the signal tabCloseRequested will be emitted.

    By default the value is false.

    \sa setTabButton(), tabRemoved()
*/

bool QTabBar::tabsClosable() const
{
    Q_D(const QTabBar);
    return d->closeButtonOnTabs;
}

void QTabBar::setTabsClosable(bool closable)
{
    Q_D(QTabBar);
    if (d->closeButtonOnTabs == closable)
        return;
    d->closeButtonOnTabs = closable;
    ButtonPosition closeSide = (ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, this);
    if (!closable) {
        for (int i = 0; i < d->tabList.count(); ++i) {
            if (closeSide == LeftSide && d->tabList[i].leftWidget) {
                d->tabList[i].leftWidget->deleteLater();
                d->tabList[i].leftWidget = 0;
            }
            if (closeSide == RightSide && d->tabList[i].rightWidget) {
                d->tabList[i].rightWidget->deleteLater();
                d->tabList[i].rightWidget = 0;
            }
        }
    } else {
        bool newButtons = false;
        for (int i = 0; i < d->tabList.count(); ++i) {
            if (tabButton(i, closeSide))
                continue;
            newButtons = true;
            QAbstractButton *closeButton = new CloseButton(this);
            connect(closeButton, SIGNAL(clicked()), this, SLOT(_q_closeTab()));
            setTabButton(i, closeSide, closeButton);
        }
        if (newButtons)
            d->layoutTabs();
    }
    update();
}

/*!
    \enum QTabBar::ButtonPosition
    \since 4.5

    This enum type lists the location of the widget on a tab.

    \value LeftSide Left side of the tab.

    \value RightSide Right side of the tab.

*/

/*!
    \enum QTabBar::SelectionBehavior
    \since 4.5

    This enum type lists the behavior of QTabBar when a tab is removed
    and the tab being removed is also the current tab.

    \value SelectLeftTab  Select the tab to the left of the one being removed.

    \value SelectRightTab  Select the tab to the right of the one being removed.

    \value SelectPreviousTab  Select the previously selected tab.

*/

/*!
    \property QTabBar::selectionBehaviorOnRemove
    \brief What tab should be set as current when removeTab is called if
    the removed tab is also the current tab.
    \since 4.5

    By default the value is SelectRightTab.

    \sa removeTab()
*/


QTabBar::SelectionBehavior QTabBar::selectionBehaviorOnRemove() const
{
    Q_D(const QTabBar);
    return d->selectionBehaviorOnRemove;
}

void QTabBar::setSelectionBehaviorOnRemove(QTabBar::SelectionBehavior behavior)
{
    Q_D(QTabBar);
    d->selectionBehaviorOnRemove = behavior;
}

/*!
    \property QTabBar::expanding
    \brief When expanding is true QTabBar will expand the tabs to use the empty space.
    \since 4.5

    By default the value is true.

    \sa QTabWidget::documentMode
*/

bool QTabBar::expanding() const
{
    Q_D(const QTabBar);
    return d->expanding;
}

void QTabBar::setExpanding(bool enabled)
{
    Q_D(QTabBar);
    if (d->expanding == enabled)
        return;
    d->expanding = enabled;
    d->layoutTabs();
}

/*!
    \property QTabBar::movable
    \brief This property holds whether the user can move the tabs
    within the tabbar area.

    \since 4.5

    By default, this property is false;
*/

bool QTabBar::isMovable() const
{
    Q_D(const QTabBar);
    return d->movable;
}

void QTabBar::setMovable(bool movable)
{
    Q_D(QTabBar);
    d->movable = movable;
}


/*!
    \property QTabBar::documentMode
    \brief Whether or not the tab bar is rendered in a mode suitable for the main window.
    \since 4.5

    This property is used as a hint for styles to draw the tabs in a different
    way then they would normally look in a tab widget.  On Mac OS X this will
    look similar to the tabs in Safari or Leopard's Terminal.app.

    \sa QTabWidget::documentMode
*/
bool QTabBar::documentMode() const
{
    return d_func()->documentMode;
}

void QTabBar::setDocumentMode(bool enabled)
{
    Q_D(QTabBar);

    d->documentMode = enabled;
    d->updateMacBorderMetrics();
}

/*!
    Sets \a widget on the tab \a index.  The widget is placed
    on the left or right hand side depending upon the \a position.
    \since 4.5

    Any previously set widget in \a position is hidden.

    The tab bar will take ownership of the widget and so all widgets set here
    will be deleted by the tab bar when it is destroyed unless you separately
    reparent the widget after setting some other widget (or 0).

    \sa tabsClosable()
  */
void QTabBar::setTabButton(int index, ButtonPosition position, QWidget *widget)
{
    Q_D(QTabBar);
    if (index < 0 || index >= d->tabList.count())
        return;
    if (widget) {
        widget->setParent(this);
        // make sure our left and right widgets stay on top
        widget->lower();
        widget->show();
    }
    if (position == LeftSide) {
        if (d->tabList[index].leftWidget)
            d->tabList[index].leftWidget->hide();
        d->tabList[index].leftWidget = widget;
    } else {
        if (d->tabList[index].rightWidget)
            d->tabList[index].rightWidget->hide();
        d->tabList[index].rightWidget = widget;
    }
    d->layoutTabs();
    d->refresh();
    update();
}

/*!
    Returns the widget set a tab \a index and \a position or 0 if
    one is not set.
  */
QWidget *QTabBar::tabButton(int index, ButtonPosition position) const
{
    Q_D(const QTabBar);
    if (index < 0 || index >= d->tabList.count())
        return 0;
    if (position == LeftSide)
        return d->tabList.at(index).leftWidget;
    else
        return d->tabList.at(index).rightWidget;
}

CloseButton::CloseButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif
#ifndef QT_NO_TOOLTIP
    setToolTip(tr("Close Tab"));
#endif
    resize(sizeHint());
}

QSize CloseButton::sizeHint() const
{
    ensurePolished();
    int width = style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, 0, this);
    int height = style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, 0, this);
    return QSize(width, height);
}

void CloseButton::enterEvent(QEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::enterEvent(event);
}

void CloseButton::leaveEvent(QEvent *event)
{
    if (isEnabled())
        update();
    QAbstractButton::leaveEvent(event);
}

void CloseButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOption opt;
    opt.init(this);
    opt.state |= QStyle::State_AutoRaise;
    if (isEnabled() && underMouse() && !isChecked() && !isDown())
        opt.state |= QStyle::State_Raised;
    if (isChecked())
        opt.state |= QStyle::State_On;
    if (isDown())
        opt.state |= QStyle::State_Sunken;

    if (const QTabBar *tb = qobject_cast<const QTabBar *>(parent())) {
        int index = tb->currentIndex();
        QTabBar::ButtonPosition position = (QTabBar::ButtonPosition)style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, tb);
        if (tb->tabButton(index, position) == this)
            opt.state |= QStyle::State_Selected;
    }

    style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &opt, &p, this);
}

QT_END_NAMESPACE

#include "moc_qtabbar.cpp"

#endif // QT_NO_TABBAR
