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

#include "qdockwidget.h"

#ifndef QT_NO_DOCKWIDGET
#include <qaction.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qdrawutil.h>
#include <qevent.h>
#include <qfontmetrics.h>
#include <qmainwindow.h>
#include <qrubberband.h>
#include <qstylepainter.h>
#include <qtoolbutton.h>
#include <qdebug.h>

#include <private/qwidgetresizehandler_p.h>

#include "qdockwidget_p.h"
#include "qmainwindowlayout_p.h"
#ifdef Q_WS_MAC
#include <private/qapplication_p.h>
#include <private/qt_mac_p.h>
#include <qmacstyle_mac.h>
#endif

QT_BEGIN_NAMESPACE

extern QString qt_setWindowTitle_helperHelper(const QString&, const QWidget*); // qwidget.cpp

// qmainwindow.cpp
extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

static inline bool hasFeature(const QDockWidgetPrivate *priv, QDockWidget::DockWidgetFeature feature)
{ return (priv->features & feature) == feature; }

static inline bool hasFeature(const QDockWidget *dockwidget, QDockWidget::DockWidgetFeature feature)
{ return (dockwidget->features() & feature) == feature; }


/*
    A Dock Window:

    [+] is the float button
    [X] is the close button

    +-------------------------------+
    | Dock Window Title       [+][X]|
    +-------------------------------+
    |                               |
    | place to put the single       |
    | QDockWidget child (this space |
    | does not yet have a name)     |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    |                               |
    +-------------------------------+

*/

/******************************************************************************
** QDockWidgetTitleButton
*/

class QDockWidgetTitleButton : public QAbstractButton
{
    Q_OBJECT

public:
    QDockWidgetTitleButton(QDockWidget *dockWidget);

    QSize sizeHint() const;
    inline QSize minimumSizeHint() const
    { return sizeHint(); }

    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    void paintEvent(QPaintEvent *event);
};


QDockWidgetTitleButton::QDockWidgetTitleButton(QDockWidget *dockWidget)
    : QAbstractButton(dockWidget)
{
    setFocusPolicy(Qt::NoFocus);
}

QSize QDockWidgetTitleButton::sizeHint() const
{
    ensurePolished();

    int size = 2*style()->pixelMetric(QStyle::PM_DockWidgetTitleBarButtonMargin, 0, this);
    if (!icon().isNull()) {
        int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
        QSize sz = icon().actualSize(QSize(iconSize, iconSize));
        size += qMax(sz.width(), sz.height());
    }

    return QSize(size, size);
}

void QDockWidgetTitleButton::enterEvent(QEvent *event)
{
    if (isEnabled()) update();
    QAbstractButton::enterEvent(event);
}

void QDockWidgetTitleButton::leaveEvent(QEvent *event)
{
    if (isEnabled()) update();
    QAbstractButton::leaveEvent(event);
}

void QDockWidgetTitleButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    QStyleOptionToolButton opt;
    opt.init(this);
    opt.state |= QStyle::State_AutoRaise;

    if (style()->styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, 0, this))
    {
        if (isEnabled() && underMouse() && !isChecked() && !isDown())
            opt.state |= QStyle::State_Raised;
        if (isChecked())
            opt.state |= QStyle::State_On;
        if (isDown())
            opt.state |= QStyle::State_Sunken;
        style()->drawPrimitive(QStyle::PE_PanelButtonTool, &opt, &p, this);
    }

    opt.icon = icon();
    opt.subControls = 0;
    opt.activeSubControls = 0;
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    int size = style()->pixelMetric(QStyle::PM_SmallIconSize, 0, this);
    opt.iconSize = QSize(size, size);
    style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &p, this);
}

/******************************************************************************
** QDockWidgetLayout
*/

QDockWidgetLayout::QDockWidgetLayout(QWidget *parent)
    : QLayout(parent), verticalTitleBar(false), item_list(RoleCount, 0)
{
}

QDockWidgetLayout::~QDockWidgetLayout()
{
    qDeleteAll(item_list);
}

bool QDockWidgetLayout::nativeWindowDeco() const
{
    return nativeWindowDeco(parentWidget()->isWindow());
}

bool QDockWidgetLayout::nativeWindowDeco(bool floating) const
{
#if defined(Q_WS_X11) || defined(Q_WS_QWS) || defined(Q_WS_WINCE)
    Q_UNUSED(floating);
    return false;
#else
    return floating && item_list[QDockWidgetLayout::TitleBar] == 0;
#endif
}


void QDockWidgetLayout::addItem(QLayoutItem*)
{
    qWarning() << "QDockWidgetLayout::addItem(): please use QDockWidgetLayout::setWidget()";
    return;
}

QLayoutItem *QDockWidgetLayout::itemAt(int index) const
{
    int cnt = 0;
    for (int i = 0; i < item_list.count(); ++i) {
        QLayoutItem *item = item_list.at(i);
        if (item == 0)
            continue;
        if (index == cnt++)
            return item;
    }
    return 0;
}

QLayoutItem *QDockWidgetLayout::takeAt(int index)
{
    int j = 0;
    for (int i = 0; i < item_list.count(); ++i) {
        QLayoutItem *item = item_list.at(i);
        if (item == 0)
            continue;
        if (index == j) {
            item_list[i] = 0;
            invalidate();
            return item;
        }
        ++j;
    }
    return 0;
}

int QDockWidgetLayout::count() const
{
    int result = 0;
    for (int i = 0; i < item_list.count(); ++i) {
        if (item_list.at(i))
            ++result;
    }
    return result;
}

QSize QDockWidgetLayout::sizeFromContent(const QSize &content, bool floating) const
{
    QSize result = content;

    if (verticalTitleBar) {
        result.setHeight(qMax(result.height(), minimumTitleWidth()));
        result.setWidth(qMax(content.width(), 0));
    } else {
        result.setHeight(qMax(result.height(), 0));
        result.setWidth(qMax(content.width(), minimumTitleWidth()));
    }

    QDockWidget *w = qobject_cast<QDockWidget*>(parentWidget());
    const bool nativeDeco = nativeWindowDeco(floating);

    int fw = floating && !nativeDeco
            ? w->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, w)
            : 0;

    const int th = titleHeight();
    if (!nativeDeco) {
        if (verticalTitleBar)
            result += QSize(th + 2*fw, 2*fw);
        else
            result += QSize(2*fw, th + 2*fw);
    }

    result.setHeight(qMin(result.height(), (int) QWIDGETSIZE_MAX));
    result.setWidth(qMin(result.width(), (int) QWIDGETSIZE_MAX));

    if (content.width() < 0)
        result.setWidth(-1);
    if (content.height() < 0)
        result.setHeight(-1);

    int left, top, right, bottom;
    w->getContentsMargins(&left, &top, &right, &bottom);
    //we need to substract the contents margin (it will be added by the caller)
    QSize min = w->minimumSize() - QSize(left + right, top + bottom);
    QSize max = w->maximumSize() - QSize(left + right, top + bottom);

    /* A floating dockwidget will automatically get its minimumSize set to the layout's
       minimum size + deco. We're *not* interested in this, we only take minimumSize()
       into account if the user set it herself. Otherwise we end up expanding the result
       of a calculation for a non-floating dock widget to a floating dock widget's
       minimum size + window decorations. */

    uint explicitMin = 0;
    uint explicitMax = 0;
    if (w->d_func()->extra != 0) {
        explicitMin = w->d_func()->extra->explicitMinSize;
        explicitMax = w->d_func()->extra->explicitMaxSize;
    }

    if (!(explicitMin & Qt::Horizontal) || min.width() == 0)
        min.setWidth(-1);
    if (!(explicitMin & Qt::Vertical) || min.height() == 0)
        min.setHeight(-1);

    if (!(explicitMax & Qt::Horizontal))
        max.setWidth(QWIDGETSIZE_MAX);
    if (!(explicitMax & Qt::Vertical))
        max.setHeight(QWIDGETSIZE_MAX);

    return result.boundedTo(max).expandedTo(min);
}

QSize QDockWidgetLayout::sizeHint() const
{
    QDockWidget *w = qobject_cast<QDockWidget*>(parentWidget());

    QSize content(-1, -1);
    if (item_list[Content] != 0)
        content = item_list[Content]->sizeHint();

    return sizeFromContent(content, w->isFloating());
}

QSize QDockWidgetLayout::maximumSize() const
{
    if (item_list[Content] != 0) {
        const QSize content = item_list[Content]->maximumSize();
        return sizeFromContent(content, parentWidget()->isWindow());
    } else {
        return parentWidget()->maximumSize();
    }

}

QSize QDockWidgetLayout::minimumSize() const
{
    QDockWidget *w = qobject_cast<QDockWidget*>(parentWidget());

    QSize content(0, 0);
    if (item_list[Content] != 0)
        content = item_list[Content]->minimumSize();

    return sizeFromContent(content, w->isFloating());
}

QWidget *QDockWidgetLayout::widgetForRole(Role r) const
{
    QLayoutItem *item = item_list.at(r);
    return item == 0 ? 0 : item->widget();
}

QLayoutItem *QDockWidgetLayout::itemForRole(Role r) const
{
    return item_list.at(r);
}

void QDockWidgetLayout::setWidgetForRole(Role r, QWidget *w)
{
    QWidget *old = widgetForRole(r);
    if (old != 0) {
        old->hide();
        removeWidget(old);
    }

    if (w != 0) {
        addChildWidget(w);
        item_list[r] = new QWidgetItemV2(w);
        w->show();
    } else {
        item_list[r] = 0;
    }

    invalidate();
}

static inline int pick(bool vertical, const QSize &size)
{
    return vertical ? size.height() : size.width();
}

static inline int perp(bool vertical, const QSize &size)
{
    return vertical ? size.width() : size.height();
}

int QDockWidgetLayout::minimumTitleWidth() const
{
    QDockWidget *q = qobject_cast<QDockWidget*>(parentWidget());

    if (QWidget *title = widgetForRole(TitleBar))
        return pick(verticalTitleBar, title->minimumSizeHint());

    QSize closeSize(0, 0);
    QSize floatSize(0, 0);
    if (hasFeature(q, QDockWidget::DockWidgetClosable)) {
        if (QLayoutItem *item = item_list[CloseButton])
            closeSize = item->widget()->sizeHint();
    }
    if (hasFeature(q, QDockWidget::DockWidgetFloatable)) {
        if (QLayoutItem *item = item_list[FloatButton])
            floatSize = item->widget()->sizeHint();
    }

    int titleHeight = this->titleHeight();

    int mw = q->style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, 0, q);
    int fw = q->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, q);

    return pick(verticalTitleBar, closeSize)
            + pick(verticalTitleBar, floatSize)
            + titleHeight + 2*fw + 3*mw;
}

int QDockWidgetLayout::titleHeight() const
{
    QDockWidget *q = qobject_cast<QDockWidget*>(parentWidget());

    if (QWidget *title = widgetForRole(TitleBar))
        return perp(verticalTitleBar, title->sizeHint());

    QSize closeSize(0, 0);
    QSize floatSize(0, 0);
    if (QLayoutItem *item = item_list[CloseButton])
        closeSize = item->widget()->sizeHint();
    if (QLayoutItem *item = item_list[FloatButton])
        floatSize = item->widget()->sizeHint();

    int buttonHeight = qMax(perp(verticalTitleBar, closeSize),
                            perp(verticalTitleBar, floatSize));

    QFontMetrics titleFontMetrics = q->fontMetrics();
#ifdef Q_WS_MAC
    if (qobject_cast<QMacStyle *>(q->style())) {
        //### this breaks on proxy styles.  (But is this code still called?)
        QFont font = qt_app_fonts_hash()->value("QToolButton", q->font());
        titleFontMetrics = QFontMetrics(font);
    }
#endif

    int mw = q->style()->pixelMetric(QStyle::PM_DockWidgetTitleMargin, 0, q);

    return qMax(buttonHeight + 2, titleFontMetrics.height() + 2*mw);
}

void QDockWidgetLayout::setGeometry(const QRect &geometry)
{
    QDockWidget *q = qobject_cast<QDockWidget*>(parentWidget());

    bool nativeDeco = nativeWindowDeco();

    int fw = q->isFloating() && !nativeDeco
            ? q->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, q)
            : 0;

    if (nativeDeco) {
        if (QLayoutItem *item = item_list[Content])
            item->setGeometry(geometry);
    } else {
        int titleHeight = this->titleHeight();

        if (verticalTitleBar) {
            _titleArea = QRect(QPoint(fw, fw),
                                QSize(titleHeight, geometry.height() - (fw * 2)));
        } else {
            _titleArea = QRect(QPoint(fw, fw),
                                QSize(geometry.width() - (fw * 2), titleHeight));
        }

        if (QLayoutItem *item = item_list[TitleBar]) {
            item->setGeometry(_titleArea);
        } else {
            QStyleOptionDockWidgetV2 opt;
            q->initStyleOption(&opt);

            if (QLayoutItem *item = item_list[CloseButton]) {
                if (!item->isEmpty()) {
                    QRect r = q->style()
                        ->subElementRect(QStyle::SE_DockWidgetCloseButton,
                                            &opt, q);
                    if (!r.isNull())
                        item->setGeometry(r);
                }
            }

            if (QLayoutItem *item = item_list[FloatButton]) {
                if (!item->isEmpty()) {
                    QRect r = q->style()
                        ->subElementRect(QStyle::SE_DockWidgetFloatButton,
                                            &opt, q);
                    if (!r.isNull())
                        item->setGeometry(r);
                }
            }
        }

        if (QLayoutItem *item = item_list[Content]) {
            QRect r = geometry;
            if (verticalTitleBar) {
                r.setLeft(_titleArea.right() + 1);
                r.adjust(0, fw, -fw, -fw);
            } else {
                r.setTop(_titleArea.bottom() + 1);
                r.adjust(fw, 0, -fw, -fw);
            }
            item->setGeometry(r);
        }
    }
}

void QDockWidgetLayout::setVerticalTitleBar(bool b)
{
    if (b == verticalTitleBar)
        return;
    verticalTitleBar = b;
    invalidate();
    parentWidget()->update();
}

/******************************************************************************
** QDockWidgetItem
*/

QDockWidgetItem::QDockWidgetItem(QDockWidget *dockWidget)
    : QWidgetItem(dockWidget)
{
}

QSize QDockWidgetItem::minimumSize() const
{
    QSize widgetMin(0, 0);
    if (QLayoutItem *item = dockWidgetChildItem())
        widgetMin = item->minimumSize();
    return dockWidgetLayout()->sizeFromContent(widgetMin, false);
}

QSize QDockWidgetItem::maximumSize() const
{
    if (QLayoutItem *item = dockWidgetChildItem()) {
        return dockWidgetLayout()->sizeFromContent(item->maximumSize(), false);
    } else {
        return QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
}


QSize QDockWidgetItem::sizeHint() const
{
    if (QLayoutItem *item = dockWidgetChildItem()) {
         return dockWidgetLayout()->sizeFromContent(item->sizeHint(), false);
    } else {
        return QWidgetItem::sizeHint();
    }
}

/******************************************************************************
** QDockWidgetPrivate
*/

void QDockWidgetPrivate::init()
{
    Q_Q(QDockWidget);

    QDockWidgetLayout *layout = new QDockWidgetLayout(q);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    QAbstractButton *button = new QDockWidgetTitleButton(q);
    button->setObjectName(QLatin1String("qt_dockwidget_floatbutton"));
    QObject::connect(button, SIGNAL(clicked()), q, SLOT(_q_toggleTopLevel()));
    layout->setWidgetForRole(QDockWidgetLayout::FloatButton, button);

    button = new QDockWidgetTitleButton(q);
    button->setObjectName(QLatin1String("qt_dockwidget_closebutton"));
    QObject::connect(button, SIGNAL(clicked()), q, SLOT(close()));
    layout->setWidgetForRole(QDockWidgetLayout::CloseButton, button);

    resizer = new QWidgetResizeHandler(q);
    resizer->setMovingEnabled(false);
    resizer->setActive(false);

#ifndef QT_NO_ACTION
    toggleViewAction = new QAction(q);
    toggleViewAction->setCheckable(true);
    fixedWindowTitle = qt_setWindowTitle_helperHelper(q->windowTitle(), q);
    toggleViewAction->setText(fixedWindowTitle);
    QObject::connect(toggleViewAction, SIGNAL(triggered(bool)),
                        q, SLOT(_q_toggleView(bool)));
#endif

    updateButtons();
}

/*!
    Initialize \a option with the values from this QDockWidget. This method
    is useful for subclasses when they need a QStyleOptionDockWidget, but don't want
    to fill in all the information themselves.

    \sa QStyleOption::initFrom()
*/
void QDockWidget::initStyleOption(QStyleOptionDockWidget *option) const
{
    Q_D(const QDockWidget);

    if (!option)
        return;
    QDockWidgetLayout *dwlayout = qobject_cast<QDockWidgetLayout*>(layout());

    option->initFrom(this);
    option->rect = dwlayout->titleArea();
    option->title = d->fixedWindowTitle;
    option->closable = hasFeature(this, QDockWidget::DockWidgetClosable);
    option->movable = hasFeature(this, QDockWidget::DockWidgetMovable);
    option->floatable = hasFeature(this, QDockWidget::DockWidgetFloatable);

    QDockWidgetLayout *l = qobject_cast<QDockWidgetLayout*>(layout());
    QStyleOptionDockWidgetV2 *v2
        = qstyleoption_cast<QStyleOptionDockWidgetV2*>(option);
    if (v2 != 0)
        v2->verticalTitleBar = l->verticalTitleBar;
}

void QDockWidgetPrivate::_q_toggleView(bool b)
{
    Q_Q(QDockWidget);
    if (b == q->isHidden()) {
        if (b)
            q->show();
        else
            q->close();
    }
}

void QDockWidgetPrivate::updateButtons()
{
    Q_Q(QDockWidget);
    QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);

    QStyleOptionDockWidget opt;
    q->initStyleOption(&opt);

    bool customTitleBar = dwLayout->widgetForRole(QDockWidgetLayout::TitleBar) != 0;
    bool nativeDeco = dwLayout->nativeWindowDeco();
    bool hideButtons = nativeDeco || customTitleBar;

    bool canClose = hasFeature(this, QDockWidget::DockWidgetClosable);
    bool canFloat = hasFeature(this, QDockWidget::DockWidgetFloatable);

    QAbstractButton *button
        = qobject_cast<QAbstractButton*>(dwLayout->widgetForRole(QDockWidgetLayout::FloatButton));
    button->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarNormalButton, &opt, q));
    button->setVisible(canFloat && !hideButtons);

    button
        = qobject_cast <QAbstractButton*>(dwLayout->widgetForRole(QDockWidgetLayout::CloseButton));
    button->setIcon(q->style()->standardIcon(QStyle::SP_TitleBarCloseButton, &opt, q));
    button->setVisible(canClose && !hideButtons);

    q->setAttribute(Qt::WA_ContentsPropagated,
                    (canFloat || canClose) && !hideButtons);

    layout->invalidate();
}

void QDockWidgetPrivate::_q_toggleTopLevel()
{
    Q_Q(QDockWidget);
    q->setFloating(!q->isFloating());
}

void QDockWidgetPrivate::initDrag(const QPoint &pos, bool nca)
{
    if (state != 0)
        return;

    Q_Q(QDockWidget);
    QMainWindow *win = qobject_cast<QMainWindow*>(parent);
    Q_ASSERT(win != 0);
    QMainWindowLayout *layout = qt_mainwindow_layout(win);
    Q_ASSERT(layout != 0);
    if (layout->pluggingWidget != 0) // the main window is animating a docking operation
        return;

    state = new QDockWidgetPrivate::DragState;
    state->dragging = false;
    state->widgetItem = 0;
    state->ownWidgetItem = false;
    state->nca = nca;
    state->ctrlDrag = false;

    if (!q->isFloating()) {
        // When dragging the widget out of the docking area,
        // use the middle of title area as pressPos
        QDockWidgetLayout *dwlayout = qobject_cast<QDockWidgetLayout*>(q->layout());
        Q_ASSERT(dwlayout != 0);
        int width = undockedGeometry.isNull() ? q->width() : undockedGeometry.width();
        state->pressPos.setY(dwlayout->titleArea().height() / 2);
        state->pressPos.setX(width / 2);
    } else {
        state->pressPos = pos;
    }
}

void QDockWidgetPrivate::startDrag()
{
    Q_Q(QDockWidget);

    if (state == 0 || state->dragging)
        return;

    QMainWindowLayout *layout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q->parentWidget()));
    Q_ASSERT(layout != 0);

    state->widgetItem = layout->unplug(q);
    if (state->widgetItem == 0) {
        /* I have a QMainWindow parent, but I was never inserted with
            QMainWindow::addDockWidget, so the QMainWindowLayout has no
            widget item for me. :( I have to create it myself, and then
            delete it if I don't get dropped into a dock area. */
        state->widgetItem = new QDockWidgetItem(q);
        state->ownWidgetItem = true;
    }

    if (state->ctrlDrag)
        layout->restore();

    state->dragging = true;
}

void QDockWidgetPrivate::endDrag(bool abort)
{
    Q_Q(QDockWidget);
    Q_ASSERT(state != 0);

    q->releaseMouse();

    if (state->dragging) {
        QMainWindowLayout *mwLayout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q->parentWidget()));
        Q_ASSERT(mwLayout != 0);

        if (abort || !mwLayout->plug(state->widgetItem)) {
            if (hasFeature(this, QDockWidget::DockWidgetFloatable)) {
                if (state->ownWidgetItem)
                    delete state->widgetItem;
                mwLayout->restore();
#ifdef Q_WS_X11
                // get rid of the X11BypassWindowManager window flag and activate the resizer
                Qt::WindowFlags flags = q->windowFlags();
                flags &= ~Qt::X11BypassWindowManagerHint;
                q->setWindowFlags(flags);
                resizer->setActive(QWidgetResizeHandler::Resize, true);
                q->show();
#else
                QDockWidgetLayout *myLayout
                    = qobject_cast<QDockWidgetLayout*>(layout);
                resizer->setActive(QWidgetResizeHandler::Resize,
                                    myLayout->widgetForRole(QDockWidgetLayout::TitleBar) != 0);
#endif
                undockedGeometry = q->geometry();
                q->activateWindow();
            } else {
                mwLayout->revert(state->widgetItem);
            }
        }
    }
    delete state;
    state = 0;
}

bool QDockWidgetPrivate::isAnimating() const
{
    Q_Q(const QDockWidget);

    QMainWindow *mainWin = qobject_cast<QMainWindow*>(parent);
    if (mainWin == 0)
        return false;

    QMainWindowLayout *mainWinLayout = qt_mainwindow_layout(mainWin);
    if (mainWinLayout == 0)
        return false;

    return (void*)mainWinLayout->pluggingWidget == (void*)q;
}

bool QDockWidgetPrivate::mousePressEvent(QMouseEvent *event)
{
#if !defined(QT_NO_MAINWINDOW)
    Q_Q(QDockWidget);

    QDockWidgetLayout *dwLayout
        = qobject_cast<QDockWidgetLayout*>(layout);

    if (!dwLayout->nativeWindowDeco()) {
        QRect titleArea = dwLayout->titleArea();

        if (event->button() != Qt::LeftButton ||
            !titleArea.contains(event->pos()) ||
            // check if the tool window is movable... do nothing if it
            // is not (but allow moving if the window is floating)
            (!hasFeature(this, QDockWidget::DockWidgetMovable) && !q->isFloating()) ||
            qobject_cast<QMainWindow*>(parent) == 0 ||
            isAnimating() || state != 0) {
            return false;
        }

        initDrag(event->pos(), false);

        if (state)
            state->ctrlDrag = hasFeature(this, QDockWidget::DockWidgetFloatable) && event->modifiers() & Qt::ControlModifier;

        return true;
    }

#endif // !defined(QT_NO_MAINWINDOW)
    return false;
}

bool QDockWidgetPrivate::mouseDoubleClickEvent(QMouseEvent *event)
{
    QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);

    if (!dwLayout->nativeWindowDeco()) {
        QRect titleArea = dwLayout->titleArea();

        if (event->button() == Qt::LeftButton && titleArea.contains(event->pos()) &&
            hasFeature(this, QDockWidget::DockWidgetFloatable)) {
            _q_toggleTopLevel();
            return true;
        }
    }
    return false;
}

bool QDockWidgetPrivate::mouseMoveEvent(QMouseEvent *event)
{
    bool ret = false;
#if !defined(QT_NO_MAINWINDOW)
    Q_Q(QDockWidget);

    if (!state)
        return ret;

    QDockWidgetLayout *dwlayout
        = qobject_cast<QDockWidgetLayout *>(layout);
    QMainWindowLayout *mwlayout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q->parentWidget()));
    if (!dwlayout->nativeWindowDeco()) {
        if (!state->dragging
            && mwlayout->pluggingWidget == 0
            && (event->pos() - state->pressPos).manhattanLength()
                > QApplication::startDragDistance()) {
            startDrag();
#ifdef Q_OS_WIN
            grabMouseWhileInWindow();
#else
            q->grabMouse();
#endif
            ret = true;
        }
    }

    if (state->dragging && !state->nca) {
        QPoint pos = event->globalPos() - state->pressPos;
        q->move(pos);

        if (!state->ctrlDrag)
            mwlayout->hover(state->widgetItem, event->globalPos());

        ret = true;
    }

#endif // !defined(QT_NO_MAINWINDOW)
    return ret;
}

bool QDockWidgetPrivate::mouseReleaseEvent(QMouseEvent *event)
{
#if !defined(QT_NO_MAINWINDOW)

    if (event->button() == Qt::LeftButton && state && !state->nca) {
        endDrag();
        return true; //filter out the event
    }

#endif // !defined(QT_NO_MAINWINDOW)
    return false;
}

void QDockWidgetPrivate::nonClientAreaMouseEvent(QMouseEvent *event)
{
    Q_Q(QDockWidget);

    int fw = q->style()->pixelMetric(QStyle::PM_DockWidgetFrameWidth, 0, q);

    QRect geo = q->geometry();
    QRect titleRect = q->frameGeometry();
#ifdef Q_WS_MAC
    if ((features & QDockWidget::DockWidgetVerticalTitleBar)) {
        titleRect.setTop(geo.top());
        titleRect.setBottom(geo.bottom());
        titleRect.setRight(geo.left() - 1);
    } else
#endif
    {
        titleRect.setLeft(geo.left());
        titleRect.setRight(geo.right());
        titleRect.setBottom(geo.top() - 1);
        titleRect.adjust(0, fw, 0, 0);
    }

    switch (event->type()) {
        case QEvent::NonClientAreaMouseButtonPress:
            if (!titleRect.contains(event->globalPos()))
                break;
            if (state != 0)
                break;
            if (qobject_cast<QMainWindow*>(parent) == 0)
                break;
            if (isAnimating())
                break;
            initDrag(event->pos(), true);
            if (state == 0)
                break;
#ifdef Q_OS_WIN
            // On Windows, NCA mouse events don't contain modifier info
            state->ctrlDrag = GetKeyState(VK_CONTROL) & 0x8000;
#else
            state->ctrlDrag = event->modifiers() & Qt::ControlModifier;
#endif
            startDrag();
            break;
        case QEvent::NonClientAreaMouseMove:
            if (state == 0 || !state->dragging)
                break;
            if (state->nca) {
                endDrag();
            }
#ifdef Q_OS_MAC
            else { // workaround for lack of mouse-grab on Mac
                QMainWindowLayout *layout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q->parentWidget()));
                Q_ASSERT(layout != 0);

                q->move(event->globalPos() - state->pressPos);
                if (!state->ctrlDrag)
                    layout->hover(state->widgetItem, event->globalPos());
            }
#endif
            break;
        case QEvent::NonClientAreaMouseButtonRelease:
#ifdef Q_OS_MAC
                        if (state)
                                endDrag();
#endif
                        break;
        case QEvent::NonClientAreaMouseButtonDblClick:
            _q_toggleTopLevel();
            break;
        default:
            break;
    }
}

void QDockWidgetPrivate::moveEvent(QMoveEvent *event)
{
    Q_Q(QDockWidget);

    if (state == 0 || !state->dragging || !state->nca || !q->isWindow())
        return;

    // When the native window frame is being dragged, all we get is these mouse
    // move events.

    if (state->ctrlDrag)
        return;

    QMainWindowLayout *layout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q->parentWidget()));
    Q_ASSERT(layout != 0);

    QPoint globalMousePos = event->pos() + state->pressPos;
    layout->hover(state->widgetItem, globalMousePos);
}

void QDockWidgetPrivate::unplug(const QRect &rect)
{
    Q_Q(QDockWidget);
    QRect r;
    if (!undockedGeometry.isNull()) {
        r = undockedGeometry;
    } else {
        r = rect;
        r.moveTopLeft(q->mapToGlobal(QPoint(0, 0)));
        QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);
        if (dwLayout->nativeWindowDeco(true))
            r.adjust(0, dwLayout->titleHeight(), 0, 0);
    }

    setWindowState(true, true, r);
}

void QDockWidgetPrivate::plug(const QRect &rect)
{
    setWindowState(false, false, rect);
}

void QDockWidgetPrivate::setWindowState(bool floating, bool unplug, const QRect &rect)
{
    Q_Q(QDockWidget);

    if (!floating && parent) {
        QMainWindowLayout *mwlayout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q->parentWidget()));
        if (mwlayout && mwlayout->dockWidgetArea(q) == Qt::NoDockWidgetArea)
            return; // this dockwidget can't be redocked
    }

    bool wasFloating = q->isFloating();
    bool hidden = q->isHidden();

    if (q->isVisible())
        q->hide();

    Qt::WindowFlags flags = floating ? Qt::Tool : Qt::Widget;

    QDockWidgetLayout *dwLayout = qobject_cast<QDockWidgetLayout*>(layout);
    const bool nativeDeco = dwLayout->nativeWindowDeco(floating);

    if (nativeDeco) {
        flags |= Qt::CustomizeWindowHint | Qt::WindowTitleHint;
        if (hasFeature(this, QDockWidget::DockWidgetClosable))
            flags |= Qt::WindowCloseButtonHint;
    } else {
        flags |= Qt::FramelessWindowHint;
    }

    if (unplug)
        flags |= Qt::X11BypassWindowManagerHint;

    q->setWindowFlags(flags);

#if defined(Q_WS_MAC) && !defined(QT_MAC_USE_COCOA)
    if (floating && nativeDeco && (q->features() & QDockWidget::DockWidgetVerticalTitleBar)) {
        ChangeWindowAttributes(HIViewGetWindow(HIViewRef(q->winId())), kWindowSideTitlebarAttribute, 0);
    }
#endif

    if (!rect.isNull())
        q->setGeometry(rect);

    updateButtons();

    if (!hidden)
        q->show();

    if (floating != wasFloating) {
        emit q->topLevelChanged(floating);
        if (!floating && parent) {
            QMainWindowLayout *mwlayout = qt_mainwindow_layout(qobject_cast<QMainWindow *>(q->parentWidget()));
            if (mwlayout)
                emit q->dockLocationChanged(mwlayout->dockWidgetArea(q));
        }
    }

    resizer->setActive(QWidgetResizeHandler::Resize, !unplug && floating && !nativeDeco);
}

/*!
    \class QDockWidget

    \brief The QDockWidget class provides a widget that can be docked
    inside a QMainWindow or floated as a top-level window on the
    desktop.

    \ingroup mainwindow-classes

    QDockWidget provides the concept of dock widgets, also know as
    tool palettes or utility windows.  Dock windows are secondary
    windows placed in the \e {dock widget area} around the
    \l{QMainWindow::centralWidget()}{central widget} in a
    QMainWindow.

    \image mainwindow-docks.png

    Dock windows can be moved inside their current area, moved into
    new areas and floated (e.g., undocked) by the end-user.  The
    QDockWidget API allows the programmer to restrict the dock widgets
    ability to move, float and close, as well as the areas in which
    they can be placed.

    \section1 Appearance

    A QDockWidget consists of a title bar and the content area.  The
    title bar displays the dock widgets \link QWidget::windowTitle()
    window title\endlink, a \e float button and a \e close button.
    Depending on the state of the QDockWidget, the \e float and \e
    close buttons may be either disabled or not shown at all.

    The visual appearance of the title bar and buttons is dependent
    on the \l{QStyle}{style} in use.

    A QDockWidget acts as a wrapper for its child widget, set with setWidget().
    Custom size hints, minimum and maximum sizes and size policies should be
    implemented in the child widget. QDockWidget will respect them, adjusting
    its own constraints to include the frame and title. Size constraints
    should not be set on the QDockWidget itself, because they change depending
    on whether it is docked; a docked QDockWidget has no frame and a smaller title
    bar.

    \sa QMainWindow, {Dock Widgets Example}
*/

/*!
    \enum QDockWidget::DockWidgetFeature

    \value DockWidgetClosable   The dock widget can be closed. On some systems the dock
	                            widget always has a close button when it's floating
								(for example on MacOS 10.5).
    \value DockWidgetMovable    The dock widget can be moved between docks
                                by the user.
    \value DockWidgetFloatable  The dock widget can be detached from the
                                main window, and floated as an independent
                                window.
    \value DockWidgetVerticalTitleBar The dock widget displays a vertical title
                                  bar on its left side. This can be used to
                                  increase the amount of vertical space in
                                  a QMainWindow.
    \value AllDockWidgetFeatures  (Deprecated) The dock widget can be closed, moved,
                                  and floated. Since new features might be added in future
                                  releases, the look and behavior of dock widgets might
                                  change if you use this flag. Please specify individual
                                  flags instead.
    \value NoDockWidgetFeatures   The dock widget cannot be closed, moved,
                                  or floated.

    \omitvalue DockWidgetFeatureMask
    \omitvalue Reserved
*/

/*!
    \property QDockWidget::windowTitle
    \brief the dock widget title (caption)

    By default, this property contains an empty string.
*/

/*!
    Constructs a QDockWidget with parent \a parent and window flags \a
    flags. The dock widget will be placed in the left dock widget
    area.
*/
QDockWidget::QDockWidget(QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*new QDockWidgetPrivate, parent, flags)
{
    Q_D(QDockWidget);
    d->init();
}

/*!
    Constructs a QDockWidget with parent \a parent and window flags \a
    flags. The dock widget will be placed in the left dock widget
    area.

    The window title is set to \a title. This title is used when the
    QDockWidget is docked and undocked. It is also used in the context
    menu provided by QMainWindow.

    \sa setWindowTitle()
*/
QDockWidget::QDockWidget(const QString &title, QWidget *parent, Qt::WindowFlags flags)
    : QWidget(*new QDockWidgetPrivate, parent, flags)
{
    Q_D(QDockWidget);
    d->init();
    setWindowTitle(title);
}

/*!
    Destroys the dock widget.
*/
QDockWidget::~QDockWidget()
{ }

/*!
    Returns the widget for the dock widget. This function returns zero
    if the widget has not been set.

    \sa setWidget()
*/
QWidget *QDockWidget::widget() const
{
    QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(this->layout());
    return layout->widgetForRole(QDockWidgetLayout::Content);
}

/*!
    Sets the widget for the dock widget to \a widget.

    If the dock widget is visible when \a widget is added, you must
    \l{QWidget::}{show()} it explicitly.

    Note that you must add the layout of the \a widget before you call
    this function; if not, the \a widget will not be visible.

    \sa widget()
*/
void QDockWidget::setWidget(QWidget *widget)
{
    QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(this->layout());
    layout->setWidgetForRole(QDockWidgetLayout::Content, widget);
}

/*!
    \property QDockWidget::features
    \brief whether the dock widget is movable, closable, and floatable

    By default, this property is set to a combination of DockWidgetClosable,
    DockWidgetMovable and DockWidgetFloatable.

    \sa DockWidgetFeature
*/

void QDockWidget::setFeatures(QDockWidget::DockWidgetFeatures features)
{
    Q_D(QDockWidget);
    features &= DockWidgetFeatureMask;
    if (d->features == features)
        return;
    const bool closableChanged = (d->features ^ features) & DockWidgetClosable;
    d->features = features;
    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(this->layout());
    layout->setVerticalTitleBar(features & DockWidgetVerticalTitleBar);
    d->updateButtons();
    d->toggleViewAction->setEnabled((d->features & DockWidgetClosable) == DockWidgetClosable);
    emit featuresChanged(d->features);
    update();
    if (closableChanged && layout->nativeWindowDeco()) {
        //this ensures the native decoration is drawn
        d->setWindowState(true /*floating*/, true /*unplug*/);
    }
}

QDockWidget::DockWidgetFeatures QDockWidget::features() const
{
    Q_D(const QDockWidget);
    return d->features;
}

/*!
    \property QDockWidget::floating
    \brief whether the dock widget is floating

    A floating dock widget is presented to the user as an independent
    window "on top" of its parent QMainWindow, instead of being
    docked in the QMainWindow.

    By default, this property is true.

    \sa isWindow()
*/
void QDockWidget::setFloating(bool floating)
{
    Q_D(QDockWidget);

    // the initial click of a double-click may have started a drag...
    if (d->state != 0)
        d->endDrag(true);

    QRect r = d->undockedGeometry;

    d->setWindowState(floating, false, floating ? r : QRect());

    if (floating && r.isNull()) {
        if (x() < 0 || y() < 0) //may happen if we have been hidden
            move(QPoint());
        setAttribute(Qt::WA_Moved, false); //we want it at the default position
    }
}

/*!
    \property QDockWidget::allowedAreas
    \brief areas where the dock widget may be placed

    The default is Qt::AllDockWidgetAreas.

    \sa Qt::DockWidgetArea
*/

void QDockWidget::setAllowedAreas(Qt::DockWidgetAreas areas)
{
    Q_D(QDockWidget);
    areas &= Qt::DockWidgetArea_Mask;
    if (areas == d->allowedAreas)
        return;
    d->allowedAreas = areas;
    emit allowedAreasChanged(d->allowedAreas);
}

Qt::DockWidgetAreas QDockWidget::allowedAreas() const
{
    Q_D(const QDockWidget);
    return d->allowedAreas;
}

/*!
    \fn bool QDockWidget::isAreaAllowed(Qt::DockWidgetArea area) const

    Returns true if this dock widget can be placed in the given \a area;
    otherwise returns false.
*/

/*! \reimp */
void QDockWidget::changeEvent(QEvent *event)
{
    Q_D(QDockWidget);
    QDockWidgetLayout *layout = qobject_cast<QDockWidgetLayout*>(this->layout());

    switch (event->type()) {
    case QEvent::ModifiedChange:
    case QEvent::WindowTitleChange:
        update(layout->titleArea());
#ifndef QT_NO_ACTION
        d->fixedWindowTitle = qt_setWindowTitle_helperHelper(windowTitle(), this);
        d->toggleViewAction->setText(d->fixedWindowTitle);
#endif
#ifndef QT_NO_TABBAR
        {
            QMainWindow *win = qobject_cast<QMainWindow*>(parentWidget());
            if (QMainWindowLayout *winLayout = qt_mainwindow_layout(win)) {
                if (QDockAreaLayoutInfo *info = winLayout->layoutState.dockAreaLayout.info(this))
                    info->updateTabBar();
            }
        }
#endif // QT_NO_TABBAR
        break;
    default:
        break;
    }
    QWidget::changeEvent(event);
}

/*! \reimp */
void QDockWidget::closeEvent(QCloseEvent *event)
{
    Q_D(QDockWidget);
    if (d->state)
        d->endDrag(true);
    QWidget::closeEvent(event);
}

/*! \reimp */
void QDockWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(this->layout());
    bool customTitleBar = layout->widgetForRole(QDockWidgetLayout::TitleBar) != 0;
    bool nativeDeco = layout->nativeWindowDeco();

    if (!nativeDeco && !customTitleBar) {
        QStylePainter p(this);
        // ### Add PixelMetric to change spacers, so style may show border
        // when not floating.
        if (isFloating()) {
            QStyleOptionFrame framOpt;
            framOpt.init(this);
            p.drawPrimitive(QStyle::PE_FrameDockWidget, framOpt);
        }

        // Title must be painted after the frame, since the areas overlap, and
        // the title may wish to extend out to all sides (eg. XP style)
        QStyleOptionDockWidgetV2 titleOpt;
        initStyleOption(&titleOpt);
        p.drawControl(QStyle::CE_DockWidgetTitle, titleOpt);
    }
}

/*! \reimp */
bool QDockWidget::event(QEvent *event)
{
    Q_D(QDockWidget);

    QMainWindow *win = qobject_cast<QMainWindow*>(parentWidget());
    QMainWindowLayout *layout = qt_mainwindow_layout(win);

    switch (event->type()) {
#ifndef QT_NO_ACTION
    case QEvent::Hide:
        if (layout != 0)
            layout->keepSize(this);
        d->toggleViewAction->setChecked(false);
        emit visibilityChanged(false);
        break;
    case QEvent::Show:
        d->toggleViewAction->setChecked(true);
        emit visibilityChanged(geometry().right() >= 0 && geometry().bottom() >= 0);
        break;
#endif
    case QEvent::ApplicationLayoutDirectionChange:
    case QEvent::LayoutDirectionChange:
    case QEvent::StyleChange:
    case QEvent::ParentChange:
        d->updateButtons();
        break;
    case QEvent::ZOrderChange: {
        bool onTop = false;
        if (win != 0) {
            const QObjectList &siblings = win->children();
            onTop = siblings.count() > 0 && siblings.last() == (QObject*)this;
        }
        if (!isFloating() && layout != 0 && onTop)
            layout->raise(this);
        break;
    }
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        update(qobject_cast<QDockWidgetLayout *>(this->layout())->titleArea());
        break;
    case QEvent::ContextMenu:
        if (d->state) {
            event->accept();
            return true;
        }
        break;
        // return true after calling the handler since we don't want
        // them to be passed onto the default handlers
    case QEvent::MouseButtonPress:
        if (d->mousePressEvent(static_cast<QMouseEvent *>(event)))
            return true;
        break;
    case QEvent::MouseButtonDblClick:
        if (d->mouseDoubleClickEvent(static_cast<QMouseEvent *>(event)))
            return true;
        break;
    case QEvent::MouseMove:
        if (d->mouseMoveEvent(static_cast<QMouseEvent *>(event)))
            return true;
        break;
#ifdef Q_OS_WIN
    case QEvent::Leave:
        if (d->state != 0 && d->state->dragging && !d->state->nca) {
            // This is a workaround for loosing the mouse on Vista.
            QPoint pos = QCursor::pos();
            QMouseEvent fake(QEvent::MouseMove, mapFromGlobal(pos), pos, Qt::NoButton,
                             QApplication::mouseButtons(), QApplication::keyboardModifiers());
            d->mouseMoveEvent(&fake);
        }
        break;
#endif
    case QEvent::MouseButtonRelease:
        if (d->mouseReleaseEvent(static_cast<QMouseEvent *>(event)))
            return true;
        break;
    case QEvent::NonClientAreaMouseMove:
    case QEvent::NonClientAreaMouseButtonPress:
    case QEvent::NonClientAreaMouseButtonRelease:
    case QEvent::NonClientAreaMouseButtonDblClick:
        d->nonClientAreaMouseEvent(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::Move:
        d->moveEvent(static_cast<QMoveEvent*>(event));
        break;
    case QEvent::Resize:
        // if the mainwindow is plugging us, we don't want to update undocked geometry
        if (isFloating() && layout != 0 && layout->pluggingWidget != this)
            d->undockedGeometry = geometry();
        break;
    default:
        break;
    }
    return QWidget::event(event);
}

#ifndef QT_NO_ACTION
/*!
  Returns a checkable action that can be used to show or close this
  dock widget.

  The action's text is set to the dock widget's window title.

  \sa QAction::text QWidget::windowTitle
 */
QAction * QDockWidget::toggleViewAction() const
{
    Q_D(const QDockWidget);
    return d->toggleViewAction;
}
#endif // QT_NO_ACTION

/*!
    \fn void QDockWidget::featuresChanged(QDockWidget::DockWidgetFeatures features)

    This signal is emitted when the \l features property changes. The
    \a features parameter gives the new value of the property.
*/

/*!
    \fn void QDockWidget::topLevelChanged(bool topLevel)

    This signal is emitted when the \l floating property changes.
    The \a topLevel parameter is true if the dock widget is now floating;
    otherwise it is false.

    \sa isWindow()
*/

/*!
    \fn void QDockWidget::allowedAreasChanged(Qt::DockWidgetAreas allowedAreas)

    This signal is emitted when the \l allowedAreas property changes. The
    \a allowedAreas parameter gives the new value of the property.
*/

/*!
    \fn void QDockWidget::visibilityChanged(bool visible)
    \since 4.3

    This signal is emitted when the dock widget becomes \a visible (or
    invisible). This happens when the widget is hidden or shown, as
    well as when it is docked in a tabbed dock area and its tab
    becomes selected or unselected.
*/

/*!
    \fn void QDockWidget::dockLocationChanged(Qt::DockWidgetArea area)
    \since 4.3

    This signal is emitted when the dock widget is moved to another
    dock \a area, or is moved to a different location in its current
    dock area. This happens when the dock widget is moved
    programmatically or is dragged to a new location by the user.
*/

/*!
    \since 4.3

    Sets an arbitrary \a widget as the dock widget's title bar. If \a widget
    is 0, any custom title bar widget previously set on the dock widget is
    removed, but not deleted, and the default title bar will be used
    instead.

    If a title bar widget is set, QDockWidget will not use native window
    decorations when it is floated.

    Here are some tips for implementing custom title bars:

    \list
    \o Mouse events that are not explicitly handled by the title bar widget
       must be ignored by calling QMouseEvent::ignore(). These events then
       propagate to the QDockWidget parent, which handles them in the usual
       manner, moving when the title bar is dragged, docking and undocking
       when it is double-clicked, etc.

    \o When DockWidgetVerticalTitleBar is set on QDockWidget, the title
       bar widget is repositioned accordingly. In resizeEvent(), the title
       bar should check what orientation it should assume:
       \snippet doc/src/snippets/code/src_gui_widgets_qdockwidget.cpp 0

    \o The title bar widget must have a valid QWidget::sizeHint() and
       QWidget::minimumSizeHint(). These functions should take into account
       the current orientation of the title bar.

    \o It is not possible to remove a title bar from a dock widget. However,
       a similar effect can be achieved by setting a default constructed
       QWidget as the title bar widget.
    \endlist

    Using qobject_cast() as shown above, the title bar widget has full access
    to its parent QDockWidget. Hence it can perform such operations as docking
    and hiding in response to user actions.

    \sa titleBarWidget() DockWidgetVerticalTitleBar
*/

void QDockWidget::setTitleBarWidget(QWidget *widget)
{
    Q_D(QDockWidget);
    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(this->layout());
    layout->setWidgetForRole(QDockWidgetLayout::TitleBar, widget);
    d->updateButtons();
    if (isWindow()) {
        //this ensures the native decoration is drawn
        d->setWindowState(true /*floating*/, true /*unplug*/);
    }
}

/*!
    \since 4.3
    Returns the custom title bar widget set on the QDockWidget, or 0 if no
    custom title bar has been set.

    \sa setTitleBarWidget()
*/

QWidget *QDockWidget::titleBarWidget() const
{
    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(this->layout());
    return layout->widgetForRole(QDockWidgetLayout::TitleBar);
}

QT_END_NAMESPACE

#include "qdockwidget.moc"
#include "moc_qdockwidget.cpp"

#endif // QT_NO_DOCKWIDGET
