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

#include "qtabwidget.h"

#ifndef QT_NO_TABWIDGET
#include "private/qwidget_p.h"
#include "private/qtabbar_p.h"
#include "qapplication.h"
#include "qbitmap.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qlayout.h"
#include "qstackedwidget.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qstylepainter.h"
#include "qtabbar.h"
#include "qtoolbutton.h"

QT_BEGIN_NAMESPACE

/*!
    \class QTabWidget
    \brief The QTabWidget class provides a stack of tabbed widgets.

    \ingroup organizers
    \ingroup basicwidgets


    A tab widget provides a tab bar (see QTabBar) and a "page area"
    that is used to display pages related to each tab. By default, the
    tab bar is shown above the page area, but different configurations
    are available (see \l{TabPosition}). Each tab is associated with a
    different widget (called a page). Only the current page is shown in
    the page area; all the other pages are hidden. The user can show a
    different page by clicking on its tab or by pressing its
    Alt+\e{letter} shortcut if it has one.

    The normal way to use QTabWidget is to do the following:
    \list 1
    \i Create a QTabWidget.
    \i Create a QWidget for each of the pages in the tab dialog, but
       do not specify parent widgets for them.
    \i Insert child widgets into the page widget, using layouts to
       position them as normal.
    \i Call addTab() or insertTab() to put the page widgets into the
       tab widget, giving each tab a suitable label with an optional
       keyboard shortcut.
    \endlist

    The position of the tabs is defined by \l tabPosition, their shape
    by \l tabShape.

    The signal currentChanged() is emitted when the user selects a
    page.

    The current page index is available as currentIndex(), the current
    page widget with currentWidget().  You can retrieve a pointer to a
    page widget with a given index using widget(), and can find the
    index position of a widget with indexOf(). Use setCurrentWidget()
    or setCurrentIndex() to show a particular page.

    You can change a tab's text and icon using setTabText() or
    setTabIcon(). A tab and its associated page can be removed with
    removeTab().

    Each tab is either enabled or disabled at any given time (see
    setTabEnabled()). If a tab is enabled, the tab text is drawn
    normally and the user can select that tab. If it is disabled, the
    tab is drawn in a different way and the user cannot select that
    tab. Note that even if a tab is disabled, the page can still be
    visible, for example if all of the tabs happen to be disabled.

    Tab widgets can be a very good way to split up a complex dialog.
    An alternative is to use a QStackedWidget for which you provide some
    means of navigating between pages, for example, a QToolBar or a
    QListWidget.

    Most of the functionality in QTabWidget is provided by a QTabBar
    (at the top, providing the tabs) and a QStackedWidget (most of the
    area, organizing the individual pages).

    \table 100%
    \row \o \inlineimage windowsxp-tabwidget.png Screenshot of a Windows XP style tab widget
         \o \inlineimage macintosh-tabwidget.png Screenshot of a Macintosh style tab widget
         \o \inlineimage plastique-tabwidget.png Screenshot of a Plastique style tab widget
    \row \o A Windows XP style tab widget.
         \o A Macintosh style tab widget.
         \o A Plastique style tab widget.
    \endtable

    \sa QTabBar, QStackedWidget, QToolBox, {Tab Dialog Example}
*/

/*!
    \enum QTabWidget::TabPosition

    This enum type defines where QTabWidget draws the tab row:

    \value North  The tabs are drawn above the pages.
    \value South  The tabs are drawn below the pages.
    \value West  The tabs are drawn to the left of the pages.
    \value East  The tabs are drawn to the right of the pages.
    \omitvalue Bottom
    \omitvalue Top
*/

/*!
    \enum QTabWidget::TabShape

    This enum type defines the shape of the tabs:
    \value Rounded  The tabs are drawn with a rounded look. This is the default
                    shape.
    \value Triangular  The tabs are drawn with a triangular look.
*/

/*!
    \fn void QTabWidget::selected(const QString &tabLabel)

    This signal is emitted whenever a tab is selected (raised),
    including during the first show().

    You can normally use currentChanged() instead.
*/

/*!
    \fn void QTabWidget::currentChanged(int index)

    This signal is emitted whenever the current page index changes.
    The parameter is the new current page \a index position, or -1
    if there isn't a new one (for example, if there are no widgets
    in the QTabWidget)

    \sa currentWidget() currentIndex
*/

/*!
    \fn void QTabWidget::tabCloseRequested(int index)
    \since 4.5

    This signal is emitted when the close button on a tab is clicked.
    The \a index is the index that should be removed.

    \sa setTabsClosable()
*/

class QTabWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QTabWidget)

public:
    QTabWidgetPrivate();
    ~QTabWidgetPrivate();
    void updateTabBarPosition();
    void _q_showTab(int);
    void _q_removeTab(int);
    void _q_tabMoved(int from, int to);
    void init();
    bool hasHeightForWidth() const;

    QTabBar *tabs;
    QStackedWidget *stack;
    QRect panelRect;
    bool dirty;
    QTabWidget::TabPosition pos;
    QTabWidget::TabShape shape;
    int alignment;
    QWidget *leftCornerWidget;
    QWidget *rightCornerWidget;
};

QTabWidgetPrivate::QTabWidgetPrivate()
    : tabs(0), stack(0), dirty(true),
      pos(QTabWidget::North), shape(QTabWidget::Rounded),
      leftCornerWidget(0), rightCornerWidget(0)
{}

QTabWidgetPrivate::~QTabWidgetPrivate()
{}

void QTabWidgetPrivate::init()
{
    Q_Q(QTabWidget);

    stack = new QStackedWidget(q);
    stack->setObjectName(QLatin1String("qt_tabwidget_stackedwidget"));
    stack->setLineWidth(0);
    // hack so that QMacStyle::layoutSpacing() can detect tab widget pages
    stack->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred, QSizePolicy::TabWidget));

    QObject::connect(stack, SIGNAL(widgetRemoved(int)), q, SLOT(_q_removeTab(int)));
    QTabBar *tabBar = new QTabBar(q);
    tabBar->setObjectName(QLatin1String("qt_tabwidget_tabbar"));
    tabBar->setDrawBase(false);
    q->setTabBar(tabBar);

    q->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding,
                                 QSizePolicy::TabWidget));
#ifdef QT_KEYPAD_NAVIGATION
    if (QApplication::keypadNavigationEnabled())
        q->setFocusPolicy(Qt::NoFocus);
    else
#endif
    q->setFocusPolicy(Qt::TabFocus);
    q->setFocusProxy(tabs);
    q->setTabPosition(static_cast<QTabWidget::TabPosition> (q->style()->styleHint(
                      QStyle::SH_TabWidget_DefaultTabPosition, 0, q )));

}

bool QTabWidgetPrivate::hasHeightForWidth() const
{
    bool has = size_policy.hasHeightForWidth();
    if (!has && stack)
        has = qt_widget_private(stack)->hasHeightForWidth();
    return has;
}


/*!
    Initialize \a option with the values from this QTabWidget. This method is useful
    for subclasses when they need a QStyleOptionTabWidgetFrame, but don't want to fill
    in all the information themselves.

    \sa QStyleOption::initFrom() QTabBar::initStyleOption()
*/
void QTabWidget::initStyleOption(QStyleOptionTabWidgetFrame *option) const
{
    if (!option)
        return;

    Q_D(const QTabWidget);
    option->initFrom(this);

    if (documentMode())
        option->lineWidth = 0;
    else
        option->lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this);

    int exth = style()->pixelMetric(QStyle::PM_TabBarBaseHeight, 0, this);
    QSize t(0, d->stack->frameWidth());
    if (d->tabs->isVisibleTo(const_cast<QTabWidget *>(this))) {
        t = d->tabs->sizeHint();
        if (documentMode()) {
            if (tabPosition() == East || tabPosition() == West) {
                t.setHeight(height());
            } else {
                t.setWidth(width());
            }
        }
    }

    if (d->rightCornerWidget) {
        const QSize rightCornerSizeHint = d->rightCornerWidget->sizeHint();
        const QSize bounds(rightCornerSizeHint.width(), t.height() - exth);
        option->rightCornerWidgetSize = rightCornerSizeHint.boundedTo(bounds);
    } else {
        option->rightCornerWidgetSize = QSize(0, 0);
    }

    if (d->leftCornerWidget) {
        const QSize leftCornerSizeHint = d->leftCornerWidget->sizeHint();
        const QSize bounds(leftCornerSizeHint.width(), t.height() - exth);
        option->leftCornerWidgetSize = leftCornerSizeHint.boundedTo(bounds);
    } else {
        option->leftCornerWidgetSize = QSize(0, 0);
    }

    switch (d->pos) {
    case QTabWidget::North:
        option->shape = d->shape == QTabWidget::Rounded ? QTabBar::RoundedNorth
                                                        : QTabBar::TriangularNorth;
        break;
    case QTabWidget::South:
        option->shape = d->shape == QTabWidget::Rounded ? QTabBar::RoundedSouth
                                                        : QTabBar::TriangularSouth;
        break;
    case QTabWidget::West:
        option->shape = d->shape == QTabWidget::Rounded ? QTabBar::RoundedWest
                                                        : QTabBar::TriangularWest;
        break;
    case QTabWidget::East:
        option->shape = d->shape == QTabWidget::Rounded ? QTabBar::RoundedEast
                                                        : QTabBar::TriangularEast;
        break;
    }

    option->tabBarSize = t;

    if (QStyleOptionTabWidgetFrameV2 *tabframe = qstyleoption_cast<QStyleOptionTabWidgetFrameV2*>(option)) {
        QRect tbRect = tabBar()->geometry();
        QRect selectedTabRect = tabBar()->tabRect(tabBar()->currentIndex());
        tabframe->tabBarRect = tbRect;
        selectedTabRect.moveTopLeft(selectedTabRect.topLeft() + tbRect.topLeft());
        tabframe->selectedTabRect = selectedTabRect;
    }
}

/*!
    Constructs a tabbed widget with parent \a parent.
*/
QTabWidget::QTabWidget(QWidget *parent)
    : QWidget(*new QTabWidgetPrivate, parent, 0)
{
    Q_D(QTabWidget);
    d->init();
}

#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QTabWidget::QTabWidget(QWidget *parent, const char *name, Qt::WindowFlags f)
    : QWidget(*new QTabWidgetPrivate, parent, f)
{
    Q_D(QTabWidget);
    setObjectName(QString::fromAscii(name));
    d->init();
}
#endif

/*!
    Destroys the tabbed widget.
*/
QTabWidget::~QTabWidget()
{
}

/*!
    \fn int QTabWidget::addTab(QWidget *page, const QString &label)

    Adds a tab with the given \a page and \a label to the tab widget,
    and returns the index of the tab in the tab bar.

    If the tab's \a label contains an ampersand, the letter following
    the ampersand is used as a shortcut for the tab, e.g. if the
    label is "Bro\&wse" then Alt+W becomes a shortcut which will
    move the focus to this tab.

    \note If you call addTab() after show(), the layout system will try
    to adjust to the changes in its widgets hierarchy and may cause
    flicker. To prevent this, you can set the QWidget::updatesEnabled
    property to false prior to changes; remember to set the property
    to true when the changes are done, making the widget receive paint
    events again.

    \sa insertTab()
*/
int QTabWidget::addTab(QWidget *child, const QString &label)
{
    return insertTab(-1, child, label);
}


/*!
    \fn int QTabWidget::addTab(QWidget *page, const QIcon &icon, const QString &label)
    \overload

    Adds a tab with the given \a page, \a icon, and \a label to the tab
    widget, and returns the index of the tab in the tab bar.

    This function is the same as addTab(), but with an additional \a
    icon.
*/
int QTabWidget::addTab(QWidget *child, const QIcon& icon, const QString &label)
{
    return insertTab(-1, child, icon, label);
}


/*!
    \fn int QTabWidget::insertTab(int index, QWidget *page, const QString &label)

    Inserts a tab with the given \a label and \a page into the tab
    widget at the specified \a index, and returns the index of the
    inserted tab in the tab bar.

    The label is displayed in the tab and may vary in appearance depending
    on the configuration of the tab widget.

    If the tab's \a label contains an ampersand, the letter following
    the ampersand is used as a shortcut for the tab, e.g. if the
    label is "Bro\&wse" then Alt+W becomes a shortcut which will
    move the focus to this tab.

    If \a index is out of range, the tab is simply appended.
    Otherwise it is inserted at the specified position.

    If the QTabWidget was empty before this function is called, the
    new page becomes the current page. Inserting a new tab at an index
    less than or equal to the current index will increment the current
    index, but keep the current page.

    \note If you call insertTab() after show(), the layout system will try
    to adjust to the changes in its widgets hierarchy and may cause
    flicker. To prevent this, you can set the QWidget::updatesEnabled
    property to false prior to changes; remember to set the property
    to true when the changes are done, making the widget receive paint
    events again.

    \sa addTab()
*/
int QTabWidget::insertTab(int index, QWidget *w, const QString &label)
{
    return insertTab(index, w, QIcon(), label);
}


/*!
    \fn int QTabWidget::insertTab(int index, QWidget *page, const QIcon& icon, const QString &label)
    \overload

    Inserts a tab with the given \a label, \a page, and \a icon into
    the tab widget at the specified \a index, and returns the index of the
    inserted tab in the tab bar.

    This function is the same as insertTab(), but with an additional
    \a icon.
*/
int QTabWidget::insertTab(int index, QWidget *w, const QIcon& icon, const QString &label)
{
    Q_D(QTabWidget);
    if(!w)
        return -1;
    index = d->stack->insertWidget(index, w);
    d->tabs->insertTab(index, icon, label);
    setUpLayout();
    tabInserted(index);

    return index;
}


/*!
    Defines a new \a label for the page at position \a index's tab.

    If the provided text contains an ampersand character ('&'), a
    shortcut is automatically created for it. The character that
    follows the '&' will be used as the shortcut key. Any previous
    shortcut will be overwritten, or cleared if no shortcut is defined
    by the text. See the \l {QShortcut#mnemonic}{QShortcut}
    documentation for details (to display an actual ampersand, use
    '&&').

*/
void QTabWidget::setTabText(int index, const QString &label)
{
    Q_D(QTabWidget);
    d->tabs->setTabText(index, label);
    setUpLayout();
}

/*!
    Returns the label text for the tab on the page at position \a index.
*/

QString QTabWidget::tabText(int index) const
{
    Q_D(const QTabWidget);
    return d->tabs->tabText(index);
}

/*!
    \overload

    Sets the \a icon for the tab at position \a index.
*/
void QTabWidget::setTabIcon(int index, const QIcon &icon)
{
    Q_D(QTabWidget);
    d->tabs->setTabIcon(index, icon);
    setUpLayout();
}

/*!
    Returns the icon for the tab on the page at position \a index.
*/

QIcon QTabWidget::tabIcon(int index) const
{
    Q_D(const QTabWidget);
    return d->tabs->tabIcon(index);
}

/*!
    Returns true if the page at position \a index is enabled; otherwise returns false.

    \sa setTabEnabled(), QWidget::isEnabled()
*/

bool QTabWidget::isTabEnabled(int index) const
{
    Q_D(const QTabWidget);
    return d->tabs->isTabEnabled(index);
}

/*!
    If \a enable is true, the page at position \a index is enabled; otherwise the page at position \a index is
    disabled. The page's tab is redrawn appropriately.

    QTabWidget uses QWidget::setEnabled() internally, rather than
    keeping a separate flag.

    Note that even a disabled tab/page may be visible. If the page is
    visible already, QTabWidget will not hide it; if all the pages are
    disabled, QTabWidget will show one of them.

    \sa isTabEnabled(), QWidget::setEnabled()
*/

void QTabWidget::setTabEnabled(int index, bool enable)
{
    Q_D(QTabWidget);
    d->tabs->setTabEnabled(index, enable);
    if (QWidget *widget = d->stack->widget(index))
        widget->setEnabled(enable);
}

/*!
  \fn void QTabWidget::setCornerWidget(QWidget *widget, Qt::Corner corner)

  Sets the given \a widget to be shown in the specified \a corner of the
  tab widget. The geometry of the widget is determined based on the widget's
  sizeHint() and the style().

  Only the horizontal element of the \a corner will be used.

  Passing 0 shows no widget in the corner.

  Any previously set corner widget is hidden.

  All widgets set here will be deleted by the tab widget when it is
  destroyed unless you separately reparent the widget after setting
  some other corner widget (or 0).

  Note: Corner widgets are designed for \l North and \l South tab positions;
  other orientations are known to not work properly.

  \sa cornerWidget(), setTabPosition()
*/
void QTabWidget::setCornerWidget(QWidget * widget, Qt::Corner corner)
{
    Q_D(QTabWidget);
    if (widget && widget->parentWidget() != this)
        widget->setParent(this);

    if (corner & Qt::TopRightCorner) {
        if (d->rightCornerWidget)
            d->rightCornerWidget->hide();
        d->rightCornerWidget = widget;
    } else {
        if (d->leftCornerWidget)
            d->leftCornerWidget->hide();
        d->leftCornerWidget = widget;
    }
    setUpLayout();
}

/*!
    Returns the widget shown in the \a corner of the tab widget or 0.
*/
QWidget * QTabWidget::cornerWidget(Qt::Corner corner) const
{
    Q_D(const QTabWidget);
    if (corner & Qt::TopRightCorner)
        return d->rightCornerWidget;
    return d->leftCornerWidget;
}

/*!
   Removes the tab at position \a index from this stack of widgets.
   The page widget itself is not deleted.

   \sa addTab(), insertTab()
*/
void QTabWidget::removeTab(int index)
{
    Q_D(QTabWidget);
    if (QWidget *w = d->stack->widget(index))
        d->stack->removeWidget(w);
}

/*!
    Returns a pointer to the page currently being displayed by the tab
    dialog. The tab dialog does its best to make sure that this value
    is never 0 (but if you try hard enough, it can be).

    \sa currentIndex(), setCurrentWidget()
*/

QWidget * QTabWidget::currentWidget() const
{
    Q_D(const QTabWidget);
    return d->stack->currentWidget();
}

/*!
    Makes \a widget the current widget. The \a widget used must be a page in
    this tab widget.

    \sa addTab(), setCurrentIndex(), currentWidget()
 */
void QTabWidget::setCurrentWidget(QWidget *widget)
{
    Q_D(const QTabWidget);
    d->tabs->setCurrentIndex(indexOf(widget));
}


/*!
    \property QTabWidget::currentIndex
    \brief the index position of the current tab page

    The current index is -1 if there is no current widget.

    By default, this property contains a value of -1 because there are initially
    no tabs in the widget.
*/

int QTabWidget::currentIndex() const
{
    Q_D(const QTabWidget);
    return d->tabs->currentIndex();
}

void QTabWidget::setCurrentIndex(int index)
{
    Q_D(QTabWidget);
    d->tabs->setCurrentIndex(index);
}


/*!
    Returns the index position of the page occupied by the widget \a
    w, or -1 if the widget cannot be found.
*/
int QTabWidget::indexOf(QWidget* w) const
{
    Q_D(const QTabWidget);
    return d->stack->indexOf(w);
}


/*!
    \reimp
*/
void QTabWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    setUpLayout();
}

/*!
    Replaces the dialog's QTabBar heading with the tab bar \a tb. Note
    that this must be called \e before any tabs have been added, or
    the behavior is undefined.

    \sa tabBar()
*/
void QTabWidget::setTabBar(QTabBar* tb)
{
    Q_D(QTabWidget);
    Q_ASSERT(tb);

    if (tb->parentWidget() != this) {
        tb->setParent(this);
        tb->show();
    }
    delete d->tabs;
    d->tabs = tb;
    setFocusProxy(d->tabs);
    connect(d->tabs, SIGNAL(currentChanged(int)),
            this, SLOT(_q_showTab(int)));
    connect(d->tabs, SIGNAL(tabMoved(int,int)),
            this, SLOT(_q_tabMoved(int,int)));
    if (d->tabs->tabsClosable())
        connect(d->tabs, SIGNAL(tabCloseRequested(int)),
                this, SIGNAL(tabCloseRequested(int)));
    tb->setExpanding(!documentMode());
    setUpLayout();
}


/*!
    Returns the current QTabBar.

    \sa setTabBar()
*/
QTabBar* QTabWidget::tabBar() const
{
    Q_D(const QTabWidget);
    return d->tabs;
}

/*!
    Ensures that the selected tab's page is visible and appropriately
    sized.
*/

void QTabWidgetPrivate::_q_showTab(int index)
{
    Q_Q(QTabWidget);
    if (index < stack->count() && index >= 0)
        stack->setCurrentIndex(index);
    emit q->currentChanged(index);
#ifdef QT3_SUPPORT
    emit q->selected(q->tabText(index));
    emit q->currentChanged(stack->widget(index));
#endif
}

void QTabWidgetPrivate::_q_removeTab(int index)
{
    Q_Q(QTabWidget);
    tabs->removeTab(index);
    q->setUpLayout();
    q->tabRemoved(index);
}

void QTabWidgetPrivate::_q_tabMoved(int from, int to)
{
    stack->blockSignals(true);
    QWidget *w = stack->widget(from);
    stack->removeWidget(w);
    stack->insertWidget(to, w);
    stack->blockSignals(false);
}

/*
    Set up the layout.
    Get subrect from the current style, and set the geometry for the
    stack widget, tab bar and corner widgets.
*/
void QTabWidget::setUpLayout(bool onlyCheck)
{
    Q_D(QTabWidget);
    if (onlyCheck && !d->dirty)
        return; // nothing to do

    QStyleOptionTabWidgetFrameV2 option;
    initStyleOption(&option);

    // this must be done immediately, because QWidgetItem relies on it (even if !isVisible())
    d->setLayoutItemMargins(QStyle::SE_TabWidgetLayoutItem, &option);

    if (!isVisible()) {
        d->dirty = true;
        return; // we'll do it later
    }

    QRect tabRect = style()->subElementRect(QStyle::SE_TabWidgetTabBar, &option, this);
    d->panelRect = style()->subElementRect(QStyle::SE_TabWidgetTabPane, &option, this);
    QRect contentsRect = style()->subElementRect(QStyle::SE_TabWidgetTabContents, &option, this);
    QRect leftCornerRect = style()->subElementRect(QStyle::SE_TabWidgetLeftCorner, &option, this);
    QRect rightCornerRect = style()->subElementRect(QStyle::SE_TabWidgetRightCorner, &option, this);

    d->tabs->setGeometry(tabRect);
    d->stack->setGeometry(contentsRect);
    if (d->leftCornerWidget)
        d->leftCornerWidget->setGeometry(leftCornerRect);
    if (d->rightCornerWidget)
        d->rightCornerWidget->setGeometry(rightCornerRect);

    if (!onlyCheck)
        update();
    updateGeometry();
}

/*!
    \internal
*/
static inline QSize basicSize(
    bool horizontal, const QSize &lc, const QSize &rc, const QSize &s, const QSize &t)
{
    return horizontal
        ? QSize(qMax(s.width(), t.width() + rc.width() + lc.width()),
                s.height() + (qMax(rc.height(), qMax(lc.height(), t.height()))))
        : QSize(s.width() + (qMax(rc.width(), qMax(lc.width(), t.width()))),
                qMax(s.height(), t.height() + rc.height() + lc.height()));
}

/*!
    \reimp
*/
QSize QTabWidget::sizeHint() const
{
    Q_D(const QTabWidget);
    QSize lc(0, 0), rc(0, 0);
    QStyleOptionTabWidgetFrameV2 opt;
    initStyleOption(&opt);
    opt.state = QStyle::State_None;

    if (d->leftCornerWidget)
        lc = d->leftCornerWidget->sizeHint();
    if(d->rightCornerWidget)
        rc = d->rightCornerWidget->sizeHint();
    if (!d->dirty) {
        QTabWidget *that = (QTabWidget*)this;
        that->setUpLayout(true);
    }
    QSize s(d->stack->sizeHint());
    QSize t(d->tabs->sizeHint());
    if(usesScrollButtons())
        t = t.boundedTo(QSize(200,200));
    else
        t = t.boundedTo(QApplication::desktop()->size());

    QSize sz = basicSize(d->pos == North || d->pos == South, lc, rc, s, t);

    return style()->sizeFromContents(QStyle::CT_TabWidget, &opt, sz, this)
                    .expandedTo(QApplication::globalStrut());
}


/*!
    \reimp

    Returns a suitable minimum size for the tab widget.
*/
QSize QTabWidget::minimumSizeHint() const
{
    Q_D(const QTabWidget);
    QSize lc(0, 0), rc(0, 0);

    if(d->leftCornerWidget)
        lc = d->leftCornerWidget->minimumSizeHint();
    if(d->rightCornerWidget)
        rc = d->rightCornerWidget->minimumSizeHint();
    if (!d->dirty) {
        QTabWidget *that = (QTabWidget*)this;
        that->setUpLayout(true);
    }
    QSize s(d->stack->minimumSizeHint());
    QSize t(d->tabs->minimumSizeHint());

    QSize sz = basicSize(d->pos == North || d->pos == South, lc, rc, s, t);

    QStyleOptionTabWidgetFrameV2 opt;
    initStyleOption(&opt);
    opt.palette = palette();
    opt.state = QStyle::State_None;
    return style()->sizeFromContents(QStyle::CT_TabWidget, &opt, sz, this)
                    .expandedTo(QApplication::globalStrut());
}

/*!
    \reimp
    \since 4.8
*/
int QTabWidget::heightForWidth(int width) const
{
    Q_D(const QTabWidget);
    QStyleOptionTabWidgetFrameV2 opt;
    initStyleOption(&opt);
    opt.state = QStyle::State_None;

    QSize zero(0,0);
    const QSize padding = style()->sizeFromContents(QStyle::CT_TabWidget, &opt, zero, this)
                                  .expandedTo(QApplication::globalStrut());

    QSize lc(0, 0), rc(0, 0);
    if (d->leftCornerWidget)
        lc = d->leftCornerWidget->sizeHint();
    if(d->rightCornerWidget)
        rc = d->rightCornerWidget->sizeHint();
    if (!d->dirty) {
        QTabWidget *that = (QTabWidget*)this;
        that->setUpLayout(true);
    }
    QSize t(d->tabs->sizeHint());

    if(usesScrollButtons())
        t = t.boundedTo(QSize(200,200));
    else
        t = t.boundedTo(QApplication::desktop()->size());

    const bool tabIsHorizontal = (d->pos == North || d->pos == South);
    const int contentsWidth = width - padding.width();
    int stackWidth = contentsWidth;
    if (!tabIsHorizontal)
        stackWidth -= qMax(t.width(), qMax(lc.width(), rc.width()));

    int stackHeight = d->stack->heightForWidth(stackWidth);
    QSize s(stackWidth, stackHeight);

    QSize contentSize = basicSize(tabIsHorizontal, lc, rc, s, t);
    return (contentSize + padding).expandedTo(QApplication::globalStrut()).height();
}


/*!
    \reimp
 */
void QTabWidget::showEvent(QShowEvent *)
{
    setUpLayout();
}

void QTabWidgetPrivate::updateTabBarPosition()
{
    Q_Q(QTabWidget);
    switch (pos) {
    case QTabWidget::North:
        tabs->setShape(shape == QTabWidget::Rounded ? QTabBar::RoundedNorth
                                                    : QTabBar::TriangularNorth);
        break;
    case QTabWidget::South:
        tabs->setShape(shape == QTabWidget::Rounded ? QTabBar::RoundedSouth
                                                    : QTabBar::TriangularSouth);
        break;
    case QTabWidget::West:
        tabs->setShape(shape == QTabWidget::Rounded ? QTabBar::RoundedWest
                                                    : QTabBar::TriangularWest);
        break;
    case QTabWidget::East:
        tabs->setShape(shape == QTabWidget::Rounded ? QTabBar::RoundedEast
                                                    : QTabBar::TriangularEast);
        break;
    }
    q->setUpLayout();
}

/*!
    \property QTabWidget::tabPosition
    \brief the position of the tabs in this tab widget

    Possible values for this property are described by the TabPosition
    enum.

    By default, this property is set to \l North.

    \sa TabPosition
*/
QTabWidget::TabPosition QTabWidget::tabPosition() const
{
    Q_D(const QTabWidget);
    return d->pos;
}

void QTabWidget::setTabPosition(TabPosition pos)
{
    Q_D(QTabWidget);
    if (d->pos == pos)
        return;
    d->pos = pos;
    d->updateTabBarPosition();
}

/*!
    \property QTabWidget::tabsClosable
    \brief whether close buttons are automatically added to each tab.

    \since 4.5

    \sa QTabBar::tabsClosable()
*/
bool QTabWidget::tabsClosable() const
{
    return tabBar()->tabsClosable();
}

void QTabWidget::setTabsClosable(bool closeable)
{
    if (tabsClosable() == closeable)
        return;

    tabBar()->setTabsClosable(closeable);
    if (closeable)
        connect(tabBar(), SIGNAL(tabCloseRequested(int)),
                this, SIGNAL(tabCloseRequested(int)));
    else
        disconnect(tabBar(), SIGNAL(tabCloseRequested(int)),
                  this, SIGNAL(tabCloseRequested(int)));
    setUpLayout();
}

/*!
    \property QTabWidget::movable
    \brief This property holds whether the user can move the tabs
    within the tabbar area.

    \since 4.5

    By default, this property is false;
*/

bool QTabWidget::isMovable() const
{
    return tabBar()->isMovable();
}

void QTabWidget::setMovable(bool movable)
{
    tabBar()->setMovable(movable);
}

/*!
    \property QTabWidget::tabShape
    \brief the shape of the tabs in this tab widget

    Possible values for this property are QTabWidget::Rounded
    (default) or QTabWidget::Triangular.

    \sa TabShape
*/

QTabWidget::TabShape QTabWidget::tabShape() const
{
    Q_D(const QTabWidget);
    return d->shape;
}

void QTabWidget::setTabShape(TabShape s)
{
    Q_D(QTabWidget);
    if (d->shape == s)
        return;
    d->shape = s;
    d->updateTabBarPosition();
}

/*!
    \reimp
 */
bool QTabWidget::event(QEvent *ev)
{
    if (ev->type() == QEvent::LayoutRequest)
        setUpLayout();
    return QWidget::event(ev);
}

/*!
    \reimp
 */
void QTabWidget::changeEvent(QEvent *ev)
{
    if (ev->type() == QEvent::StyleChange
#ifdef Q_WS_MAC
            || ev->type() == QEvent::MacSizeChange
#endif
            )
        setUpLayout();
    QWidget::changeEvent(ev);
}


/*!
    \reimp
 */
void QTabWidget::keyPressEvent(QKeyEvent *e)
{
    Q_D(QTabWidget);
    if (((e->key() == Qt::Key_Tab || e->key() == Qt::Key_Backtab) &&
          count() > 1 && e->modifiers() & Qt::ControlModifier)
#ifdef QT_KEYPAD_NAVIGATION
          || QApplication::keypadNavigationEnabled() && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) && count() > 1
#endif
       ) {
        int pageCount = d->tabs->count();
        int page = currentIndex();
        int dx = (e->key() == Qt::Key_Backtab || e->modifiers() & Qt::ShiftModifier) ? -1 : 1;
#ifdef QT_KEYPAD_NAVIGATION
        if (QApplication::keypadNavigationEnabled() && (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right))
            dx = e->key() == (isRightToLeft() ? Qt::Key_Right : Qt::Key_Left) ? -1 : 1;
#endif
        for (int pass = 0; pass < pageCount; ++pass) {
            page+=dx;
            if (page < 0
#ifdef QT_KEYPAD_NAVIGATION
                && !e->isAutoRepeat()
#endif
               ) {
                page = count() - 1;
            } else if (page >= pageCount
#ifdef QT_KEYPAD_NAVIGATION
                       && !e->isAutoRepeat()
#endif
                      ) {
                page = 0;
            }
            if (d->tabs->isTabEnabled(page)) {
                setCurrentIndex(page);
                break;
            }
        }
        if (!QApplication::focusWidget())
            d->tabs->setFocus();
    } else {
        e->ignore();
    }
}

/*!
    Returns the tab page at index position \a index or 0 if the \a
    index is out of range.
*/
QWidget *QTabWidget::widget(int index) const
{
    Q_D(const QTabWidget);
    return d->stack->widget(index);
}

/*!
    \property QTabWidget::count
    \brief the number of tabs in the tab bar

    By default, this property contains a value of 0.
*/
int QTabWidget::count() const
{
    Q_D(const QTabWidget);
    return d->tabs->count();
}

#ifndef QT_NO_TOOLTIP
/*!
    Sets the tab tool tip for the page at position \a index to \a tip.

    \sa  tabToolTip()
*/
void QTabWidget::setTabToolTip(int index, const QString & tip)
{
    Q_D(QTabWidget);
    d->tabs->setTabToolTip(index, tip);
}

/*!
    Returns the tab tool tip for the page at position \a index or
    an empty string if no tool tip has been set.

    \sa setTabToolTip()
*/
QString QTabWidget::tabToolTip(int index) const
{
    Q_D(const QTabWidget);
    return d->tabs->tabToolTip(index);
}
#endif // QT_NO_TOOLTIP

#ifndef QT_NO_WHATSTHIS
/*!
    \since 4.1

    Sets the What's This help text for the page at position \a index
    to \a text.
*/
void QTabWidget::setTabWhatsThis(int index, const QString &text)
{
    Q_D(QTabWidget);
    d->tabs->setTabWhatsThis(index, text);
}

/*!
    \since 4.1

    Returns the What's This help text for the page at position \a index,
    or an empty string if no help text has been set.
*/
QString QTabWidget::tabWhatsThis(int index) const
{
    Q_D(const QTabWidget);
    return d->tabs->tabWhatsThis(index);
}
#endif // QT_NO_WHATSTHIS

/*!
  This virtual handler is called after a new tab was added or
  inserted at position \a index.

  \sa tabRemoved()
 */
void QTabWidget::tabInserted(int index)
{
    Q_UNUSED(index)
}

/*!
  This virtual handler is called after a tab was removed from
  position \a index.

  \sa tabInserted()
 */
void QTabWidget::tabRemoved(int index)
{
    Q_UNUSED(index)
}

/*!
    \fn void QTabWidget::paintEvent(QPaintEvent *event)

    Paints the tab widget's tab bar in response to the paint \a event.
*/
void QTabWidget::paintEvent(QPaintEvent *)
{
    Q_D(QTabWidget);
    if (documentMode()) {
        QStylePainter p(this, tabBar());
        if (QWidget *w = cornerWidget(Qt::TopLeftCorner)) {
            QStyleOptionTabBarBaseV2 opt;
            QTabBarPrivate::initStyleBaseOption(&opt, tabBar(), w->size());
            opt.rect.moveLeft(w->x() + opt.rect.x());
            opt.rect.moveTop(w->y() + opt.rect.y());
            p.drawPrimitive(QStyle::PE_FrameTabBarBase, opt);
        }
        if (QWidget *w = cornerWidget(Qt::TopRightCorner)) {
            QStyleOptionTabBarBaseV2 opt;
            QTabBarPrivate::initStyleBaseOption(&opt, tabBar(), w->size());
            opt.rect.moveLeft(w->x() + opt.rect.x());
            opt.rect.moveTop(w->y() + opt.rect.y());
            p.drawPrimitive(QStyle::PE_FrameTabBarBase, opt);
        }
        return;
    }
    QStylePainter p(this);

    QStyleOptionTabWidgetFrameV2 opt;
    initStyleOption(&opt);
    opt.rect = d->panelRect;
    p.drawPrimitive(QStyle::PE_FrameTabWidget, opt);
}

/*!
    \property QTabWidget::iconSize
    \brief The size for icons in the tab bar
    \since 4.2

    The default value is style-dependent. This is the maximum size
    that the icons will have. Icons are not scaled up if they are of
    smaller size.

    \sa QTabBar::iconSize
*/
QSize QTabWidget::iconSize() const
{
    return d_func()->tabs->iconSize();
}

void QTabWidget::setIconSize(const QSize &size)
{
    d_func()->tabs->setIconSize(size);
}

/*!
    \property QTabWidget::elideMode
    \brief how to elide text in the tab bar
    \since 4.2

    This property controls how items are elided when there is not
    enough space to show them for a given tab bar size.

    By default the value is style dependant.

    \sa QTabBar::elideMode usesScrollButtons QStyle::SH_TabBar_ElideMode
*/
Qt::TextElideMode QTabWidget::elideMode() const
{
    return d_func()->tabs->elideMode();
}

void QTabWidget::setElideMode(Qt::TextElideMode mode)
{
    d_func()->tabs->setElideMode(mode);
}

/*!
    \property QTabWidget::usesScrollButtons
    \brief Whether or not a tab bar should use buttons to scroll tabs when it
    has many tabs.
    \since 4.2

    When there are too many tabs in a tab bar for its size, the tab bar can either choose
    to expand its size or to add buttons that allow you to scroll through the tabs.

    By default the value is style dependant.

    \sa elideMode QTabBar::usesScrollButtons QStyle::SH_TabBar_PreferNoArrows
*/
bool QTabWidget::usesScrollButtons() const
{
    return d_func()->tabs->usesScrollButtons();
}

void QTabWidget::setUsesScrollButtons(bool useButtons)
{
    d_func()->tabs->setUsesScrollButtons(useButtons);
}

/*!
    \property QTabWidget::documentMode
    \brief Whether or not the tab widget is rendered in a mode suitable for document
     pages. This is the same as document mode on Mac OS X.
    \since 4.5

    When this property is set the tab widget frame is not rendered. This mode is useful
    for showing document-type pages where the page covers most of the tab widget
    area.

    \sa elideMode, QTabBar::documentMode, QTabBar::usesScrollButtons, QStyle::SH_TabBar_PreferNoArrows
*/
bool QTabWidget::documentMode() const
{
    Q_D(const QTabWidget);
    return d->tabs->documentMode();
}

void QTabWidget::setDocumentMode(bool enabled)
{
    Q_D(QTabWidget);
    d->tabs->setDocumentMode(enabled);
    d->tabs->setExpanding(!enabled);
    d->tabs->setDrawBase(enabled);
    setUpLayout();
}

/*!
    Removes all the pages, but does not delete them. Calling this function
    is equivalent to calling removeTab() until the tab widget is empty.
*/
void QTabWidget::clear()
{
    // ### optimize by introduce QStackedLayout::clear()
    while (count())
        removeTab(0);
}

/*!
    \fn void QTabWidget::insertTab(QWidget *widget, const QString &label, int index)

    Use insertTab(index, widget, label) instead.
*/

/*!
    \fn void QTabWidget::insertTab(QWidget *widget, const QIcon& icon, const QString &label, int index)

    Use insertTab(index, widget, icon, label) instead.
*/

/*!
    \fn void QTabWidget::changeTab(QWidget *widget, const QString
    &label)

    Use setTabText() instead.

*/

/*!
    \fn void QTabWidget::changeTab(QWidget *widget, const QIcon& icon, const QString &label)

    Use setTabText() and setTabIcon() instead.
*/

/*!
    \fn bool QTabWidget::isTabEnabled( QWidget *widget) const

    Use isTabEnabled(tabWidget->indexOf(widget)) instead.
*/

/*!
    \fn void QTabWidget::setTabEnabled(QWidget *widget, bool b)

    Use setTabEnabled(tabWidget->indexOf(widget), b) instead.
*/

/*!
    \fn QString QTabWidget::tabLabel(QWidget *widget) const

    Use tabText(tabWidget->indexOf(widget)) instead.
*/

/*!
    \fn void QTabWidget::setTabLabel(QWidget *widget, const QString
    &label)

    Use setTabText(tabWidget->indexOf(widget), label) instead.
*/

/*!
    \fn QIcon QTabWidget::tabIconSet(QWidget * widget) const

    Use tabIcon(tabWidget->indexOf(widget)) instead.
*/

/*!
    \fn void QTabWidget::setTabIconSet(QWidget * widget, const QIcon & icon)

    Use setTabIcon(tabWidget->indexOf(widget), icon) instead.
*/

/*!
    \fn void QTabWidget::removeTabToolTip(QWidget * widget)

    Use setTabToolTip(tabWidget->indexOf(widget), QString()) instead.
*/

/*!
    \fn void QTabWidget::setTabToolTip(QWidget * widget, const QString & tip)

    Use setTabToolTip(tabWidget->indexOf(widget), tip) instead.
*/

/*!
    \fn QString QTabWidget::tabToolTip(QWidget * widget) const

    Use tabToolTip(tabWidget->indexOf(widget)) instead.
*/

/*!
    \fn QWidget * QTabWidget::currentPage() const

    Use currentWidget() instead.
*/

/*!
    \fn QWidget *QTabWidget::page(int index) const

    Use widget() instead.
*/

/*!
    \fn QString QTabWidget::label(int index) const

    Use tabText() instead.
*/

/*!
    \fn int QTabWidget::currentPageIndex() const

    Use currentIndex() instead.
*/

/*!
    \fn int QTabWidget::margin() const

    This function is kept only to make old code compile.
    This functionality is no longer supported by QTabWidget.

    \sa contentsRect(), setContentsMargins()
*/

/*!
    \fn void QTabWidget::setMargin(int margin)

    This function is kept only to make old code compile.
    This functionality is no longer supported by QTabWidget.

    \sa contentsRect(), setContentsMargins()
*/

/*!
    \fn void QTabWidget::setCurrentPage(int index)

    Use setCurrentIndex() instead.
*/

/*!
    \fn void QTabWidget::showPage(QWidget *widget)

    Use setCurrentIndex(indexOf(widget)) instead.
*/

/*!
    \fn void QTabWidget::removePage(QWidget *widget)

    Use removeTab(indexOf(widget)) instead.
*/

/*!
    \fn void QTabWidget::currentChanged(QWidget *widget)

    Use currentChanged(int) instead.
*/

QT_END_NAMESPACE

#include "moc_qtabwidget.cpp"

#endif //QT_NO_TABWIDGET
