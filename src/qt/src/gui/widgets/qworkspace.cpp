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

#include "qworkspace.h"
#ifndef QT_NO_WORKSPACE
#include "qapplication.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qhash.h"
#include "qicon.h"
#include "qimage.h"
#include "qlabel.h"
#include "qlayout.h"
#include "qmenubar.h"
#include "qmenu.h"
#include "qpainter.h"
#include "qpointer.h"
#include "qscrollbar.h"
#include "qstyle.h"
#include "qstyleoption.h"
#include "qelapsedtimer.h"
#include "qtooltip.h"
#include "qdebug.h"
#include <private/qwidget_p.h>
#include <private/qwidgetresizehandler_p.h>
#include <private/qlayoutengine_p.h>

QT_BEGIN_NAMESPACE

class QWorkspaceTitleBarPrivate;


/**************************************************************
* QMDIControl
*
* Used for displaying MDI controls in a maximized MDI window
*
*/
class QMDIControl : public QWidget
{
    Q_OBJECT
signals:
    void _q_minimize();
    void _q_restore();
    void _q_close();

public:
    QMDIControl(QWidget *widget);

private:
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void leaveEvent(QEvent *event);
    bool event(QEvent *event);
    void initStyleOption(QStyleOptionComplex *option) const;
    QStyle::SubControl activeControl; //control locked by pressing and holding the mouse
    QStyle::SubControl hoverControl; //previously active hover control, used for tracking repaints
};

bool QMDIControl::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QStyleOptionComplex opt;
        initStyleOption(&opt);
#ifndef QT_NO_TOOLTIP
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>(event);
        QStyle::SubControl ctrl = style()->hitTestComplexControl(QStyle::CC_MdiControls, &opt,
                                                                 helpEvent->pos(), this);
        if (ctrl == QStyle::SC_MdiCloseButton)
            QToolTip::showText(helpEvent->globalPos(), QWorkspace::tr("Close"), this);
        else if (ctrl == QStyle::SC_MdiMinButton)
            QToolTip::showText(helpEvent->globalPos(), QWorkspace::tr("Minimize"), this);
        else if (ctrl == QStyle::SC_MdiNormalButton)
            QToolTip::showText(helpEvent->globalPos(), QWorkspace::tr("Restore Down"), this);
        else
            QToolTip::hideText();
#endif // QT_NO_TOOLTIP
    }
    return QWidget::event(event);
}

void QMDIControl::initStyleOption(QStyleOptionComplex *option) const
{
    option->initFrom(this);
    option->subControls = QStyle::SC_All;
    option->activeSubControls = QStyle::SC_None;
}

QMDIControl::QMDIControl(QWidget *widget)
    : QWidget(widget), activeControl(QStyle::SC_None),
      hoverControl(QStyle::SC_None)
{
    setObjectName(QLatin1String("qt_maxcontrols"));
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMouseTracking(true);
}

QSize QMDIControl::sizeHint() const
{
    ensurePolished();
    QStyleOptionComplex opt;
    initStyleOption(&opt);
    QSize size(48, 16);
    return style()->sizeFromContents(QStyle::CT_MdiControls, &opt, size, this);
}

void QMDIControl::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    QStyleOptionComplex opt;
    initStyleOption(&opt);
    QStyle::SubControl ctrl = style()->hitTestComplexControl(QStyle::CC_MdiControls, &opt,
                                                             event->pos(), this);
    activeControl = ctrl;
    update();
}

void QMDIControl::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    QStyleOptionTitleBar opt;
    initStyleOption(&opt);
    QStyle::SubControl under_mouse = style()->hitTestComplexControl(QStyle::CC_MdiControls, &opt,
                                                                    event->pos(), this);
    if (under_mouse == activeControl) {
        switch (activeControl) {
        case QStyle::SC_MdiCloseButton:
            emit _q_close();
            break;
        case QStyle::SC_MdiNormalButton:
            emit _q_restore();
            break;
        case QStyle::SC_MdiMinButton:
            emit _q_minimize();
            break;
        default:
            break;
        }
    }
    activeControl = QStyle::SC_None;
    update();
}

void QMDIControl::leaveEvent(QEvent * /*event*/)
{
    hoverControl = QStyle::SC_None;
    update();
}

void QMDIControl::mouseMoveEvent(QMouseEvent *event)
{
    QStyleOptionTitleBar opt;
    initStyleOption(&opt);
    QStyle::SubControl under_mouse = style()->hitTestComplexControl(QStyle::CC_MdiControls, &opt,
                                                                    event->pos(), this);
    //test if hover state changes
    if (hoverControl != under_mouse) {
        hoverControl = under_mouse;
        update();
    }
}

void QMDIControl::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOptionComplex opt;
    initStyleOption(&opt);
    if (activeControl == hoverControl) {
        opt.activeSubControls = activeControl;
        opt.state |= QStyle::State_Sunken;
    } else if (hoverControl != QStyle::SC_None && (activeControl == QStyle::SC_None)) {
        opt.activeSubControls = hoverControl;
        opt.state |= QStyle::State_MouseOver;
    }
    style()->drawComplexControl(QStyle::CC_MdiControls, &opt, &p, this);
}

class QWorkspaceTitleBar : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWorkspaceTitleBar)
    Q_PROPERTY(bool autoRaise READ autoRaise WRITE setAutoRaise)
    Q_PROPERTY(bool movable READ isMovable WRITE setMovable)

public:
    QWorkspaceTitleBar (QWidget *w, QWidget *parent, Qt::WindowFlags f = 0);
    ~QWorkspaceTitleBar();

    bool isActive() const;
    bool usesActiveColor() const;

    bool isMovable() const;
    void setMovable(bool);

    bool autoRaise() const;
    void setAutoRaise(bool);

    QWidget *window() const;
    bool isTool() const;

    QSize sizeHint() const;
    void initStyleOption(QStyleOptionTitleBar *option) const;

public slots:
    void setActive(bool);

signals:
    void doActivate();
    void doNormal();
    void doClose();
    void doMaximize();
    void doMinimize();
    void doShade();
    void showOperationMenu();
    void popupOperationMenu(const QPoint&);
    void doubleClicked();

protected:
    bool event(QEvent *);
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *);
#endif
    void mousePressEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void enterEvent(QEvent *e);
    void leaveEvent(QEvent *e);
    void paintEvent(QPaintEvent *p);

private:
    Q_DISABLE_COPY(QWorkspaceTitleBar)
};


class QWorkspaceTitleBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QWorkspaceTitleBar)
public:
    QWorkspaceTitleBarPrivate()
        :
        lastControl(QStyle::SC_None),
#ifndef QT_NO_TOOLTIP
        toolTip(0),
#endif
        act(0), window(0), movable(1), pressed(0), autoraise(0), moving(0)
    {
    }

    Qt::WindowFlags flags;
    QStyle::SubControl buttonDown;
    QStyle::SubControl lastControl;
    QPoint moveOffset;
#ifndef QT_NO_TOOLTIP
    QToolTip *toolTip;
#endif
    bool act                    :1;
    QPointer<QWidget> window;
    bool movable            :1;
    bool pressed            :1;
    bool autoraise          :1;
    bool moving : 1;

    int titleBarState() const;
    void readColors();
};

inline int QWorkspaceTitleBarPrivate::titleBarState() const
{
    Q_Q(const QWorkspaceTitleBar);
    uint state = window ? window->windowState() : static_cast<Qt::WindowStates>(Qt::WindowNoState);
    state |= uint((act && q->isActiveWindow()) ? QStyle::State_Active : QStyle::State_None);
    return (int)state;
}

void QWorkspaceTitleBar::initStyleOption(QStyleOptionTitleBar *option) const
{
    Q_D(const QWorkspaceTitleBar);
    option->initFrom(this);
    //################
    if (d->window && (d->flags & Qt::WindowTitleHint)) {
        option->text = d->window->windowTitle();
        QIcon icon = d->window->windowIcon();
        QSize s = icon.actualSize(QSize(64, 64));
        option->icon = icon.pixmap(s);
    }
    option->subControls = QStyle::SC_All;
    option->activeSubControls = QStyle::SC_None;
    option->titleBarState = d->titleBarState();
    option->titleBarFlags = d->flags;
    option->state &= ~QStyle::State_MouseOver;
}

QWorkspaceTitleBar::QWorkspaceTitleBar(QWidget *w, QWidget *parent, Qt::WindowFlags f)
    : QWidget(*new QWorkspaceTitleBarPrivate, parent, Qt::FramelessWindowHint)
{
    Q_D(QWorkspaceTitleBar);
    if (f == 0 && w)
        f = w->windowFlags();
    d->flags = f;
    d->window = w;
    d->buttonDown = QStyle::SC_None;
    d->act = 0;
    if (w) {
        if (w->maximumSize() != QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX))
            d->flags &= ~Qt::WindowMaximizeButtonHint;
        setWindowTitle(w->windowTitle());
    }

    d->readColors();
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    setMouseTracking(true);
    setAutoRaise(style()->styleHint(QStyle::SH_TitleBar_AutoRaise, 0, this));
}

QWorkspaceTitleBar::~QWorkspaceTitleBar()
{
}


#ifdef Q_WS_WIN
static inline QRgb colorref2qrgb(COLORREF col)
{
    return qRgb(GetRValue(col),GetGValue(col),GetBValue(col));
}
#endif

void QWorkspaceTitleBarPrivate::readColors()
{
    Q_Q(QWorkspaceTitleBar);
    QPalette pal = q->palette();

    bool colorsInitialized = false;

#ifdef Q_WS_WIN // ask system properties on windows
#ifndef SPI_GETGRADIENTCAPTIONS
#define SPI_GETGRADIENTCAPTIONS 0x1008
#endif
#ifndef COLOR_GRADIENTACTIVECAPTION
#define COLOR_GRADIENTACTIVECAPTION 27
#endif
#ifndef COLOR_GRADIENTINACTIVECAPTION
#define COLOR_GRADIENTINACTIVECAPTION 28
#endif
    if (QApplication::desktopSettingsAware()) {
        pal.setColor(QPalette::Active, QPalette::Highlight, colorref2qrgb(GetSysColor(COLOR_ACTIVECAPTION)));
        pal.setColor(QPalette::Inactive, QPalette::Highlight, colorref2qrgb(GetSysColor(COLOR_INACTIVECAPTION)));
        pal.setColor(QPalette::Active, QPalette::HighlightedText, colorref2qrgb(GetSysColor(COLOR_CAPTIONTEXT)));
        pal.setColor(QPalette::Inactive, QPalette::HighlightedText, colorref2qrgb(GetSysColor(COLOR_INACTIVECAPTIONTEXT)));

        colorsInitialized = true;
        BOOL gradient = false;
        SystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &gradient, 0);

        if (gradient) {
            pal.setColor(QPalette::Active, QPalette::Base, colorref2qrgb(GetSysColor(COLOR_GRADIENTACTIVECAPTION)));
            pal.setColor(QPalette::Inactive, QPalette::Base, colorref2qrgb(GetSysColor(COLOR_GRADIENTINACTIVECAPTION)));
        } else {
            pal.setColor(QPalette::Active, QPalette::Base, pal.color(QPalette::Active, QPalette::Highlight));
            pal.setColor(QPalette::Inactive, QPalette::Base, pal.color(QPalette::Inactive, QPalette::Highlight));
        }
    }
#endif // Q_WS_WIN
    if (!colorsInitialized) {
        pal.setColor(QPalette::Active, QPalette::Highlight,
                      pal.color(QPalette::Active, QPalette::Highlight));
        pal.setColor(QPalette::Active, QPalette::Base,
                      pal.color(QPalette::Active, QPalette::Highlight));
        pal.setColor(QPalette::Inactive, QPalette::Highlight,
                      pal.color(QPalette::Inactive, QPalette::Dark));
        pal.setColor(QPalette::Inactive, QPalette::Base,
                      pal.color(QPalette::Inactive, QPalette::Dark));
        pal.setColor(QPalette::Inactive, QPalette::HighlightedText,
                      pal.color(QPalette::Inactive, QPalette::Window));
    }

    q->setPalette(pal);
    q->setActive(act);
}

void QWorkspaceTitleBar::mousePressEvent(QMouseEvent *e)
{
    Q_D(QWorkspaceTitleBar);
    if (!d->act)
        emit doActivate();
    if (e->button() == Qt::LeftButton) {
        if (style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, 0)
            && !rect().adjusted(5, 5, -5, 0).contains(e->pos())) {
            // propagate border events to the QWidgetResizeHandler
            e->ignore();
            return;
        }

        d->pressed = true;
        QStyleOptionTitleBar opt;
        initStyleOption(&opt);
        QStyle::SubControl ctrl = style()->hitTestComplexControl(QStyle::CC_TitleBar, &opt,
                                                                 e->pos(), this);
        switch (ctrl) {
        case QStyle::SC_TitleBarSysMenu:
            if (d->flags & Qt::WindowSystemMenuHint) {
                d->buttonDown = QStyle::SC_None;
                static QElapsedTimer *t = 0;
                static QWorkspaceTitleBar *tc = 0;
                if (!t)
                    t = new QElapsedTimer;
                if (tc != this || t->elapsed() > QApplication::doubleClickInterval()) {
                    emit showOperationMenu();
                    t->start();
                    tc = this;
                } else {
                    tc = 0;
                    emit doClose();
                    return;
                }
            }
            break;

        case QStyle::SC_TitleBarShadeButton:
        case QStyle::SC_TitleBarUnshadeButton:
            if (d->flags & Qt::WindowShadeButtonHint)
                d->buttonDown = ctrl;
            break;

        case QStyle::SC_TitleBarNormalButton:
                d->buttonDown = ctrl;
            break;

        case QStyle::SC_TitleBarMinButton:
            if (d->flags & Qt::WindowMinimizeButtonHint)
                d->buttonDown = ctrl;
            break;

        case QStyle::SC_TitleBarMaxButton:
            if (d->flags & Qt::WindowMaximizeButtonHint)
                d->buttonDown = ctrl;
            break;

        case QStyle::SC_TitleBarCloseButton:
            if (d->flags & Qt::WindowSystemMenuHint)
                d->buttonDown = ctrl;
            break;

        case QStyle::SC_TitleBarLabel:
            d->buttonDown = ctrl;
            d->moveOffset = mapToParent(e->pos());
            break;

        default:
            break;
        }
        update();
    } else {
        d->pressed = false;
    }
}

#ifndef QT_NO_CONTEXTMENU
void QWorkspaceTitleBar::contextMenuEvent(QContextMenuEvent *e)
{
    QStyleOptionTitleBar opt;
    initStyleOption(&opt);
    QStyle::SubControl ctrl = style()->hitTestComplexControl(QStyle::CC_TitleBar, &opt, e->pos(),
                                                             this);
    if(ctrl == QStyle::SC_TitleBarLabel || ctrl == QStyle::SC_TitleBarSysMenu) {
        e->accept();
        emit popupOperationMenu(e->globalPos());
    } else {
        e->ignore();
    }
}
#endif // QT_NO_CONTEXTMENU

void QWorkspaceTitleBar::mouseReleaseEvent(QMouseEvent *e)
{
    Q_D(QWorkspaceTitleBar);
    if (!d->window) {
        // could have been deleted as part of a double click event on the sysmenu
        return;
    }
    if (e->button() == Qt::LeftButton && d->pressed) {
        if (style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, 0)
            && !rect().adjusted(5, 5, -5, 0).contains(e->pos())) {
            // propagate border events to the QWidgetResizeHandler
            e->ignore();
            d->buttonDown = QStyle::SC_None;
            d->pressed = false;
            return;
        }
        e->accept();
        QStyleOptionTitleBar opt;
        initStyleOption(&opt);
        QStyle::SubControl ctrl = style()->hitTestComplexControl(QStyle::CC_TitleBar, &opt,
                                                                 e->pos(), this);

        if (d->pressed) {
            update();
            d->pressed = false;
            d->moving = false;
        }
        if (ctrl == d->buttonDown) {
            d->buttonDown = QStyle::SC_None;
            switch(ctrl) {
            case QStyle::SC_TitleBarShadeButton:
            case QStyle::SC_TitleBarUnshadeButton:
                if(d->flags & Qt::WindowShadeButtonHint)
                    emit doShade();
                break;

            case QStyle::SC_TitleBarNormalButton:
                if(d->flags & Qt::WindowMinMaxButtonsHint)
                    emit doNormal();
                break;

            case QStyle::SC_TitleBarMinButton:
                if(d->flags & Qt::WindowMinimizeButtonHint) {
                    if (d->window && d->window->isMinimized())
                        emit doNormal();
                    else
                        emit doMinimize();
                }
                break;

            case QStyle::SC_TitleBarMaxButton:
                if(d->flags & Qt::WindowMaximizeButtonHint) {
                    if(d->window && d->window->isMaximized())
                        emit doNormal();
                    else
                        emit doMaximize();
                }
                break;

            case QStyle::SC_TitleBarCloseButton:
                if(d->flags & Qt::WindowSystemMenuHint) {
                    d->buttonDown = QStyle::SC_None;
                    emit doClose();
                    return;
                }
                break;

            default:
                break;
            }
        }
    } else {
        e->ignore();
    }
}

void QWorkspaceTitleBar::mouseMoveEvent(QMouseEvent *e)
{
    Q_D(QWorkspaceTitleBar);
    e->ignore();
    if ((e->buttons() & Qt::LeftButton) && style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, 0)
        && !rect().adjusted(5, 5, -5, 0).contains(e->pos()) && !d->pressed) {
        // propagate border events to the QWidgetResizeHandler
        return;
    }

    QStyleOptionTitleBar opt;
    initStyleOption(&opt);
    QStyle::SubControl under_mouse = style()->hitTestComplexControl(QStyle::CC_TitleBar, &opt,
                                                                    e->pos(), this);
    if(under_mouse != d->lastControl) {
        d->lastControl = under_mouse;
        update();
    }

    switch (d->buttonDown) {
    case QStyle::SC_None:
        break;
    case QStyle::SC_TitleBarSysMenu:
        break;
    case QStyle::SC_TitleBarLabel:
        if (d->buttonDown == QStyle::SC_TitleBarLabel && d->movable && d->pressed) {
            if (d->moving || (d->moveOffset - mapToParent(e->pos())).manhattanLength() >= 4) {
                d->moving = true;
                QPoint p = mapFromGlobal(e->globalPos());

                QWidget *parent = d->window ? d->window->parentWidget() : 0;
                if(parent && parent->inherits("QWorkspaceChild")) {
                    QWidget *workspace = parent->parentWidget();
                    p = workspace->mapFromGlobal(e->globalPos());
                    if (!workspace->rect().contains(p)) {
                        if (p.x() < 0)
                            p.rx() = 0;
                        if (p.y() < 0)
                            p.ry() = 0;
                        if (p.x() > workspace->width())
                            p.rx() = workspace->width();
                        if (p.y() > workspace->height())
                            p.ry() = workspace->height();
                    }
                }

                QPoint pp = p - d->moveOffset;
                if (!parentWidget()->isMaximized())
                    parentWidget()->move(pp);
            }
        }
        e->accept();
        break;
    default:
        break;
    }
}

bool QWorkspaceTitleBar::isTool() const
{
    Q_D(const QWorkspaceTitleBar);
    return (d->flags & Qt::WindowType_Mask) == Qt::Tool;
}

// from qwidget.cpp
extern QString qt_setWindowTitle_helperHelper(const QString &, const QWidget*);

void QWorkspaceTitleBar::paintEvent(QPaintEvent *)
{
    Q_D(QWorkspaceTitleBar);
    QStyleOptionTitleBar opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_TitleBarLabel;
    opt.activeSubControls = d->buttonDown;

    if (d->window && (d->flags & Qt::WindowTitleHint)) {
        QString title = qt_setWindowTitle_helperHelper(opt.text, d->window);
        int maxw = style()->subControlRect(QStyle::CC_TitleBar, &opt, QStyle::SC_TitleBarLabel,
                                       this).width();
        opt.text = fontMetrics().elidedText(title, Qt::ElideRight, maxw);
    }

    if (d->flags & Qt::WindowSystemMenuHint) {
        opt.subControls |= QStyle::SC_TitleBarSysMenu | QStyle::SC_TitleBarCloseButton;
        if (d->window && (d->flags & Qt::WindowShadeButtonHint)) {
            if (d->window->isMinimized())
                opt.subControls |= QStyle::SC_TitleBarUnshadeButton;
            else
                opt.subControls |= QStyle::SC_TitleBarShadeButton;
        }
        if (d->window && (d->flags & Qt::WindowMinMaxButtonsHint)) {
            if(d->window && d->window->isMinimized())
                opt.subControls |= QStyle::SC_TitleBarNormalButton;
            else
                opt.subControls |= QStyle::SC_TitleBarMinButton;
        }
        if (d->window && (d->flags & Qt::WindowMaximizeButtonHint) && !d->window->isMaximized())
            opt.subControls |= QStyle::SC_TitleBarMaxButton;
    }

    QStyle::SubControl under_mouse = QStyle::SC_None;
    under_mouse = style()->hitTestComplexControl(QStyle::CC_TitleBar, &opt,
                                                     mapFromGlobal(QCursor::pos()), this);
    if ((d->buttonDown == under_mouse) && d->pressed) {
        opt.state |= QStyle::State_Sunken;
    } else if( autoRaise() && under_mouse != QStyle::SC_None && !d->pressed) {
        opt.activeSubControls = under_mouse;
        opt.state |= QStyle::State_MouseOver;
    }
    opt.palette.setCurrentColorGroup(usesActiveColor() ? QPalette::Active : QPalette::Inactive);

    QPainter p(this);
    style()->drawComplexControl(QStyle::CC_TitleBar, &opt, &p, this);
}

void QWorkspaceTitleBar::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_D(QWorkspaceTitleBar);
    if (e->button() != Qt::LeftButton) {
        e->ignore();
        return;
    }
    e->accept();
    QStyleOptionTitleBar opt;
    initStyleOption(&opt);
    switch (style()->hitTestComplexControl(QStyle::CC_TitleBar, &opt, e->pos(), this)) {
    case QStyle::SC_TitleBarLabel:
        emit doubleClicked();
        break;

    case QStyle::SC_TitleBarSysMenu:
        if (d->flags & Qt::WindowSystemMenuHint)
            emit doClose();
        break;

    default:
        break;
    }
}

void QWorkspaceTitleBar::leaveEvent(QEvent *)
{
    Q_D(QWorkspaceTitleBar);
    d->lastControl = QStyle::SC_None;
    if(autoRaise() && !d->pressed)
        update();
}

void QWorkspaceTitleBar::enterEvent(QEvent *)
{
    Q_D(QWorkspaceTitleBar);
    if(autoRaise() && !d->pressed)
        update();
    QEvent e(QEvent::Leave);
    QApplication::sendEvent(parentWidget(), &e);
}

void QWorkspaceTitleBar::setActive(bool active)
{
    Q_D(QWorkspaceTitleBar);
    if (d->act == active)
        return ;

    d->act = active;
    update();
}

bool QWorkspaceTitleBar::isActive() const
{
    Q_D(const QWorkspaceTitleBar);
    return d->act;
}

bool QWorkspaceTitleBar::usesActiveColor() const
{
    return (isActive() && isActiveWindow()) ||
        (!window() && QWidget::window()->isActiveWindow());
}

QWidget *QWorkspaceTitleBar::window() const
{
    Q_D(const QWorkspaceTitleBar);
    return d->window;
}

bool QWorkspaceTitleBar::event(QEvent *e)
{
    Q_D(QWorkspaceTitleBar);
    if (e->type() == QEvent::ApplicationPaletteChange) {
        d->readColors();
    } else if (e->type() == QEvent::WindowActivate
               || e->type() == QEvent::WindowDeactivate) {
        if (d->act)
            update();
    }
    return QWidget::event(e);
}

void QWorkspaceTitleBar::setMovable(bool b)
{
    Q_D(QWorkspaceTitleBar);
    d->movable = b;
}

bool QWorkspaceTitleBar::isMovable() const
{
    Q_D(const QWorkspaceTitleBar);
    return d->movable;
}

void QWorkspaceTitleBar::setAutoRaise(bool b)
{
    Q_D(QWorkspaceTitleBar);
    d->autoraise = b;
}

bool QWorkspaceTitleBar::autoRaise() const
{
    Q_D(const QWorkspaceTitleBar);
    return d->autoraise;
}

QSize QWorkspaceTitleBar::sizeHint() const
{
    ensurePolished();
    QStyleOptionTitleBar opt;
    initStyleOption(&opt);
    QRect menur = style()->subControlRect(QStyle::CC_TitleBar, &opt,
                                          QStyle::SC_TitleBarSysMenu, this);
    return QSize(menur.width(), style()->pixelMetric(QStyle::PM_TitleBarHeight, &opt, this));
}

/*!
    \class QWorkspace
    \obsolete
    \brief The QWorkspace widget provides a workspace window that can be
    used in an MDI application.

    This class is deprecated. Use QMdiArea instead.

    Multiple Document Interface (MDI) applications are typically
    composed of a main window containing a menu bar, a toolbar, and
    a central QWorkspace widget. The workspace itself is used to display
    a number of child windows, each of which is a widget.

    The workspace itself is an ordinary Qt widget. It has a standard
    constructor that takes a parent widget.
    Workspaces can be placed in any layout, but are typically given
    as the central widget in a QMainWindow:

    \snippet doc/src/snippets/code/src_gui_widgets_qworkspace.cpp 0

    Child windows (MDI windows) are standard Qt widgets that are
    inserted into the workspace with addWindow(). As with top-level
    widgets, you can call functions such as show(), hide(),
    showMaximized(), and setWindowTitle() on a child window to change
    its appearance within the workspace. You can also provide widget
    flags to determine the layout of the decoration or the behavior of
    the widget itself.

    To change or retrieve the geometry of a child window, you must
    operate on its parentWidget(). The parentWidget() provides
    access to the decorated frame that contains the child window
    widget. When a child window is maximised, its decorated frame
    is hidden. If the top-level widget contains a menu bar, it will display
    the maximised window's operations menu to the left of the menu
    entries, and the window's controls to the right.

    A child window becomes active when it gets the keyboard focus,
    or when setFocus() is called. The user can activate a window by moving
    focus in the usual ways, for example by clicking a window or by pressing
    Tab. The workspace emits a signal windowActivated() when the active
    window changes, and the function activeWindow() returns a pointer to the
    active child window, or 0 if no window is active.

    The convenience function windowList() returns a list of all child
    windows. This information could be used in a popup menu
    containing a list of windows, for example. This feature is also
    available as part of the \l{Window Menu} Solution.

    QWorkspace provides two built-in layout strategies for child
    windows: cascade() and tile(). Both are slots so you can easily
    connect menu entries to them.

    \table
    \row \o \inlineimage mdi-cascade.png
         \o \inlineimage mdi-tile.png
    \endtable

    If you want your users to be able to work with child windows
    larger than the visible workspace area, set the scrollBarsEnabled
    property to true.

    \sa QDockWidget, {MDI Example}
*/


class QWorkspaceChild : public QWidget
{
    Q_OBJECT

    friend class QWorkspacePrivate;
    friend class QWorkspace;
    friend class QWorkspaceTitleBar;

public:
    QWorkspaceChild(QWidget* window, QWorkspace* parent=0, Qt::WindowFlags flags = 0);
    ~QWorkspaceChild();

    void setActive(bool);
    bool isActive() const;

    void adjustToFullscreen();

    QWidget* windowWidget() const;
    QWidget* iconWidget() const;

    void doResize();
    void doMove();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    QSize baseSize() const;

    int frameWidth() const;

    void show();

    bool isWindowOrIconVisible() const;

signals:
    void showOperationMenu();
    void popupOperationMenu(const QPoint&);

public slots:
    void activate();
    void showMinimized();
    void showMaximized();
    void showNormal();
    void showShaded();
    void internalRaise();
    void titleBarDoubleClicked();

protected:
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void childEvent(QChildEvent*);
    void resizeEvent(QResizeEvent *);
    void moveEvent(QMoveEvent *);
    bool eventFilter(QObject *, QEvent *);

    void paintEvent(QPaintEvent *);
    void changeEvent(QEvent *);

private:
    void updateMask();

    Q_DISABLE_COPY(QWorkspaceChild)

    QWidget *childWidget;
    QWidgetResizeHandler *widgetResizeHandler;
    QWorkspaceTitleBar *titlebar;
    QPointer<QWorkspaceTitleBar> iconw;
    QSize windowSize;
    QSize shadeRestore;
    QSize shadeRestoreMin;
    bool act                  :1;
    bool shademode            :1;
};

int QWorkspaceChild::frameWidth() const
{
    return contentsRect().left();
}



class QWorkspacePrivate : public QWidgetPrivate {
    Q_DECLARE_PUBLIC(QWorkspace)
public:
    QWorkspaceChild* active;
    QList<QWorkspaceChild *> windows;
    QList<QWorkspaceChild *> focus;
    QList<QWidget *> icons;
    QWorkspaceChild* maxWindow;
    QRect maxRestore;
    QPointer<QMDIControl> maxcontrols;
    QPointer<QMenuBar> maxmenubar;
    QHash<int, const char*> shortcutMap;

    int px;
    int py;
    QWidget *becomeActive;
    QPointer<QLabel> maxtools;
    QString topTitle;

    QMenu *popup, *toolPopup;
    enum WSActs { RestoreAct, MoveAct, ResizeAct, MinimizeAct, MaximizeAct, CloseAct, StaysOnTopAct, ShadeAct, NCountAct };
    QAction *actions[NCountAct];

    QScrollBar *vbar, *hbar;
    QWidget *corner;
    int yoffset, xoffset;
    QBrush background;

    void init();
    void insertIcon(QWidget* w);
    void removeIcon(QWidget* w);
    void place(QWidget*);

    QWorkspaceChild* findChild(QWidget* w);
    void showMaximizeControls();
    void hideMaximizeControls();
    void activateWindow(QWidget* w, bool change_focus = true);
    void hideChild(QWorkspaceChild *c);
    void showWindow(QWidget* w);
    void maximizeWindow(QWidget* w);
    void minimizeWindow(QWidget* w);
    void normalizeWindow(QWidget* w);

    QRect updateWorkspace();

private:
    void _q_normalizeActiveWindow();
    void _q_minimizeActiveWindow();
    void _q_showOperationMenu();
    void _q_popupOperationMenu(const QPoint&);
    void _q_operationMenuActivated(QAction *);
    void _q_scrollBarChanged();
    void _q_updateActions();
    bool inTitleChange;
};

static bool isChildOf(QWidget * child, QWidget * parent)
{
    if (!parent || !child)
        return false;
    QWidget * w = child;
    while(w && w != parent)
        w = w->parentWidget();
    return w != 0;
}

/*!
    Constructs a workspace with the given \a parent.
*/
QWorkspace::QWorkspace(QWidget *parent)
    : QWidget(*new QWorkspacePrivate, parent, 0)
{
    Q_D(QWorkspace);
    d->init();
}

#ifdef QT3_SUPPORT
/*!
    Use one of the constructors that doesn't take the \a name
    argument and then use setObjectName() instead.
*/
QWorkspace::QWorkspace(QWidget *parent, const char *name)
    : QWidget(*new QWorkspacePrivate, parent, 0)
{
    Q_D(QWorkspace);
    setObjectName(QString::fromAscii(name));
    d->init();
}
#endif // QT3_SUPPORT

/*!
    \internal
*/
void
QWorkspacePrivate::init()
{
    Q_Q(QWorkspace);

    maxcontrols = 0;
    active = 0;
    maxWindow = 0;
    maxtools = 0;
    px = 0;
    py = 0;
    becomeActive = 0;
    popup = new QMenu(q);
    toolPopup = new QMenu(q);
    popup->setObjectName(QLatin1String("qt_internal_mdi_popup"));
    toolPopup->setObjectName(QLatin1String("qt_internal_mdi_tool_popup"));

    actions[QWorkspacePrivate::RestoreAct] = new QAction(QIcon(q->style()->standardPixmap(QStyle::SP_TitleBarNormalButton, 0, q)),
                                                         QWorkspace::tr("&Restore"), q);
    actions[QWorkspacePrivate::MoveAct] = new QAction(QWorkspace::tr("&Move"), q);
    actions[QWorkspacePrivate::ResizeAct] = new QAction(QWorkspace::tr("&Size"), q);
    actions[QWorkspacePrivate::MinimizeAct] = new QAction(QIcon(q->style()->standardPixmap(QStyle::SP_TitleBarMinButton, 0, q)),
                                                          QWorkspace::tr("Mi&nimize"), q);
    actions[QWorkspacePrivate::MaximizeAct] = new QAction(QIcon(q->style()->standardPixmap(QStyle::SP_TitleBarMaxButton, 0, q)),
                                                          QWorkspace::tr("Ma&ximize"), q);
    actions[QWorkspacePrivate::CloseAct] = new QAction(QIcon(q->style()->standardPixmap(QStyle::SP_TitleBarCloseButton, 0, q)),
                                                          QWorkspace::tr("&Close")
#ifndef QT_NO_SHORTCUT
                                                          +QLatin1Char('\t')+(QString)QKeySequence(Qt::CTRL+Qt::Key_F4)
#endif
                                                          ,q);
    QObject::connect(actions[QWorkspacePrivate::CloseAct], SIGNAL(triggered()), q, SLOT(closeActiveWindow()));
    actions[QWorkspacePrivate::StaysOnTopAct] = new QAction(QWorkspace::tr("Stay on &Top"), q);
    actions[QWorkspacePrivate::StaysOnTopAct]->setChecked(true);
    actions[QWorkspacePrivate::ShadeAct] = new QAction(QIcon(q->style()->standardPixmap(QStyle::SP_TitleBarShadeButton, 0, q)),
                                                          QWorkspace::tr("Sh&ade"), q);

    QObject::connect(popup, SIGNAL(aboutToShow()), q, SLOT(_q_updateActions()));
    QObject::connect(popup, SIGNAL(triggered(QAction*)), q, SLOT(_q_operationMenuActivated(QAction*)));
    popup->addAction(actions[QWorkspacePrivate::RestoreAct]);
    popup->addAction(actions[QWorkspacePrivate::MoveAct]);
    popup->addAction(actions[QWorkspacePrivate::ResizeAct]);
    popup->addAction(actions[QWorkspacePrivate::MinimizeAct]);
    popup->addAction(actions[QWorkspacePrivate::MaximizeAct]);
    popup->addSeparator();
    popup->addAction(actions[QWorkspacePrivate::CloseAct]);

    QObject::connect(toolPopup, SIGNAL(aboutToShow()), q, SLOT(_q_updateActions()));
    QObject::connect(toolPopup, SIGNAL(triggered(QAction*)), q, SLOT(_q_operationMenuActivated(QAction*)));
    toolPopup->addAction(actions[QWorkspacePrivate::MoveAct]);
    toolPopup->addAction(actions[QWorkspacePrivate::ResizeAct]);
    toolPopup->addAction(actions[QWorkspacePrivate::StaysOnTopAct]);
    toolPopup->addSeparator();
    toolPopup->addAction(actions[QWorkspacePrivate::ShadeAct]);
    toolPopup->addAction(actions[QWorkspacePrivate::CloseAct]);

#ifndef QT_NO_SHORTCUT
    // Set up shortcut bindings (id -> slot), most used first
    QList <QKeySequence> shortcuts = QKeySequence::keyBindings(QKeySequence::NextChild);
    foreach (const QKeySequence &seq, shortcuts)
        shortcutMap.insert(q->grabShortcut(seq), "activateNextWindow");

    shortcuts = QKeySequence::keyBindings(QKeySequence::PreviousChild);
    foreach (const QKeySequence &seq, shortcuts)
        shortcutMap.insert(q->grabShortcut(seq), "activatePreviousWindow");

    shortcuts = QKeySequence::keyBindings(QKeySequence::Close);
    foreach (const QKeySequence &seq, shortcuts)
        shortcutMap.insert(q->grabShortcut(seq), "closeActiveWindow");

    shortcutMap.insert(q->grabShortcut(QKeySequence(QLatin1String("ALT+-"))), "_q_showOperationMenu");
#endif // QT_NO_SHORTCUT

    q->setBackgroundRole(QPalette::Dark);
    q->setAutoFillBackground(true);
    q->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));

    hbar = vbar = 0;
    corner = 0;
    xoffset = yoffset = 0;

    q->window()->installEventFilter(q);

    inTitleChange = false;
    updateWorkspace();
}

/*!
    Destroys the workspace and frees any allocated resources.
*/

QWorkspace::~QWorkspace()
{
}

/*! \reimp */
QSize QWorkspace::sizeHint() const
{
    QSize s(QApplication::desktop()->size());
    return QSize(s.width()*2/3, s.height()*2/3);
}


#ifdef QT3_SUPPORT
/*!
    Sets the background color to \a c.
    Use setBackground() instead.
*/
void QWorkspace::setPaletteBackgroundColor(const QColor & c)
{
    setBackground(c);
}

/*!
    Sets the background pixmap to \a pm.
    Use setBackground() instead.
*/
void QWorkspace::setPaletteBackgroundPixmap(const QPixmap & pm)
{
    setBackground(pm);
}
#endif // QT3_SUPPORT

/*!
    \property QWorkspace::background
    \brief the workspace's background
*/
QBrush QWorkspace::background() const
{
    Q_D(const QWorkspace);
    if (d->background.style() == Qt::NoBrush)
        return palette().dark();
    return d->background;
}

void QWorkspace::setBackground(const QBrush &background)
{
    Q_D(QWorkspace);
    d->background = background;
    setAttribute(Qt::WA_OpaquePaintEvent, background.style() == Qt::NoBrush);
    update();
}

/*!
    Adds widget \a w as new sub window to the workspace.  If \a flags
    are non-zero, they will override the flags set on the widget.

    Returns the widget used for the window frame.

    To remove the widget \a w from the workspace, simply call
    setParent() with the new parent (or 0 to make it a stand-alone
    window).
*/
QWidget * QWorkspace::addWindow(QWidget *w, Qt::WindowFlags flags)
{
    Q_D(QWorkspace);
    if (!w)
        return 0;

    w->setAutoFillBackground(true);

    QWidgetPrivate::adjustFlags(flags);

#if 0
    bool wasMaximized = w->isMaximized();
    bool wasMinimized = w->isMinimized();
#endif
    bool hasSize = w->testAttribute(Qt::WA_Resized);
    int x = w->x();
    int y = w->y();
    bool hasPos = w->testAttribute(Qt::WA_Moved);
    if (!hasSize && w->sizeHint().isValid())
        w->adjustSize();

    QWorkspaceChild* child = new QWorkspaceChild(w, this, flags);
    child->setObjectName(QLatin1String("qt_workspacechild"));
    child->installEventFilter(this);

    connect(child, SIGNAL(popupOperationMenu(QPoint)),
            this, SLOT(_q_popupOperationMenu(QPoint)));
    connect(child, SIGNAL(showOperationMenu()),
            this, SLOT(_q_showOperationMenu()));
    d->windows.append(child);
    if (child->isVisibleTo(this))
        d->focus.append(child);
    child->internalRaise();

    if (!hasPos)
        d->place(child);
    if (!hasSize)
        child->adjustSize();
    if (hasPos)
        child->move(x, y);

    return child;

#if 0
    if (wasMaximized)
        w->showMaximized();
    else if (wasMinimized)
        w->showMinimized();
    else if (!hasBeenHidden)
        d->activateWindow(w);

    d->updateWorkspace();
    return child;
#endif
}

/*! \reimp */
void QWorkspace::childEvent(QChildEvent * e)
{
    Q_D(QWorkspace);
    if (e->removed()) {
        if (d->windows.removeAll(static_cast<QWorkspaceChild*>(e->child()))) {
            d->focus.removeAll(static_cast<QWorkspaceChild*>(e->child()));
            if (d->maxWindow == e->child())
                d->maxWindow = 0;
            d->updateWorkspace();
        }
    }
}

/*! \reimp */
#ifndef QT_NO_WHEELEVENT
void QWorkspace::wheelEvent(QWheelEvent *e)
{
    Q_D(QWorkspace);
    if (!scrollBarsEnabled())
        return;
    // the scroll bars are children of the workspace, so if we receive
    // a wheel event we redirect to the scroll bars using a direct event
    // call, /not/ using sendEvent() because if the scroll bar ignores the
    // event QApplication::sendEvent() will propagate the event to the parent widget,
    // which is us, who /just/ sent it.
    if (d->vbar && d->vbar->isVisible() && !(e->modifiers() & Qt::AltModifier))
        d->vbar->event(e);
    else if (d->hbar && d->hbar->isVisible())
        d->hbar->event(e);
}
#endif

void QWorkspacePrivate::activateWindow(QWidget* w, bool change_focus)
{
    Q_Q(QWorkspace);
    if (!w) {
        active = 0;
        emit q->windowActivated(0);
        return;
    }
    if (!q->isVisible()) {
        becomeActive = w;
        return;
    }

    if (active && active->windowWidget() == w) {
        if (!isChildOf(q->focusWidget(), w)) // child window does not have focus
            active->setActive(true);
        return;
    }

    active = 0;
    // First deactivate all other workspace clients
    QList<QWorkspaceChild *>::Iterator it(windows.begin());
    while (it != windows.end()) {
        QWorkspaceChild* c = *it;
        ++it;
        if (c->windowWidget() == w)
            active = c;
        else
            c->setActive(false);
    }

    if (!active)
        return;

    // Then activate the new one, so the focus is stored correctly
    active->setActive(true);

    if (!active)
        return;

    if (maxWindow && maxWindow != active && active->windowWidget() &&
        (active->windowWidget()->windowFlags() & Qt::WindowMaximizeButtonHint))
        active->showMaximized();

    active->internalRaise();

    if (change_focus) {
        int from = focus.indexOf(active);
        if (from >= 0)
            focus.move(from, focus.size() - 1);
    }

    updateWorkspace();
    emit q->windowActivated(w);
}


/*!
    Returns a pointer to the widget corresponding to the active child
    window, or 0 if no window is active.

    \sa setActiveWindow()
*/
QWidget* QWorkspace::activeWindow() const
{
    Q_D(const QWorkspace);
    return d->active? d->active->windowWidget() : 0;
}

/*!
    Makes the child window that contains \a w the active child window.

    \sa activeWindow()
*/
void QWorkspace::setActiveWindow(QWidget *w)
{
    Q_D(QWorkspace);
    d->activateWindow(w, true);
    if (w && w->isMinimized())
        w->setWindowState(w->windowState() & ~Qt::WindowMinimized);
}

void QWorkspacePrivate::place(QWidget *w)
{
    Q_Q(QWorkspace);

    QList<QWidget *> widgets;
    for (QList<QWorkspaceChild *>::Iterator it(windows.begin()); it != windows.end(); ++it)
        if (*it != w)
            widgets.append(*it);

    int overlap, minOverlap = 0;
    int possible;

    QRect r1(0, 0, 0, 0);
    QRect r2(0, 0, 0, 0);
    QRect maxRect = q->rect();
    int x = maxRect.left(), y = maxRect.top();
    QPoint wpos(maxRect.left(), maxRect.top());

    bool firstPass = true;

    do {
        if (y + w->height() > maxRect.bottom()) {
            overlap = -1;
        } else if(x + w->width() > maxRect.right()) {
            overlap = -2;
        } else {
            overlap = 0;

            r1.setRect(x, y, w->width(), w->height());

            QWidget *l;
            QList<QWidget *>::Iterator it(widgets.begin());
            while (it != widgets.end()) {
                l = *it;
                ++it;

                if (maxWindow == l)
                    r2 = QStyle::visualRect(q->layoutDirection(), maxRect, maxRestore);
                else
                    r2 = QStyle::visualRect(q->layoutDirection(), maxRect,
                                            QRect(l->x(), l->y(), l->width(), l->height()));

                if (r2.intersects(r1)) {
                    r2.setCoords(qMax(r1.left(), r2.left()),
                                 qMax(r1.top(), r2.top()),
                                 qMin(r1.right(), r2.right()),
                                 qMin(r1.bottom(), r2.bottom())
                                );

                    overlap += (r2.right() - r2.left()) *
                               (r2.bottom() - r2.top());
                }
            }
        }

        if (overlap == 0) {
            wpos = QPoint(x, y);
            break;
        }

        if (firstPass) {
            firstPass = false;
            minOverlap = overlap;
        } else if (overlap >= 0 && overlap < minOverlap) {
            minOverlap = overlap;
            wpos = QPoint(x, y);
        }

        if (overlap > 0) {
            possible = maxRect.right();
            if (possible - w->width() > x) possible -= w->width();

            QWidget *l;
            QList<QWidget *>::Iterator it(widgets.begin());
            while (it != widgets.end()) {
                l = *it;
                ++it;
                if (maxWindow == l)
                    r2 = QStyle::visualRect(q->layoutDirection(), maxRect, maxRestore);
                else
                    r2 = QStyle::visualRect(q->layoutDirection(), maxRect,
                                            QRect(l->x(), l->y(), l->width(), l->height()));

                if((y < r2.bottom()) && (r2.top() < w->height() + y)) {
                    if(r2.right() > x)
                        possible = possible < r2.right() ?
                                   possible : r2.right();

                    if(r2.left() - w->width() > x)
                        possible = possible < r2.left() - w->width() ?
                                   possible : r2.left() - w->width();
                }
            }

            x = possible;
        } else if (overlap == -2) {
            x = maxRect.left();
            possible = maxRect.bottom();

            if (possible - w->height() > y) possible -= w->height();

            QWidget *l;
            QList<QWidget *>::Iterator it(widgets.begin());
            while (it != widgets.end()) {
                l = *it;
                ++it;
                if (maxWindow == l)
                    r2 = QStyle::visualRect(q->layoutDirection(), maxRect, maxRestore);
                else
                    r2 = QStyle::visualRect(q->layoutDirection(), maxRect,
                                            QRect(l->x(), l->y(), l->width(), l->height()));

                if(r2.bottom() > y)
                    possible = possible < r2.bottom() ?
                               possible : r2.bottom();

                if(r2.top() - w->height() > y)
                    possible = possible < r2.top() - w->height() ?
                               possible : r2.top() - w->height();
            }

            y = possible;
        }
    }
    while(overlap != 0 && overlap != -1);

    QRect resultRect = w->geometry();
    resultRect.moveTo(wpos);
    w->setGeometry(QStyle::visualRect(q->layoutDirection(), maxRect, resultRect));
    updateWorkspace();
}


void QWorkspacePrivate::insertIcon(QWidget* w)
{
    Q_Q(QWorkspace);
    if (!w || icons.contains(w))
        return;
    icons.append(w);
    if (w->parentWidget() != q) {
        w->setParent(q, 0);
        w->move(0,0);
    }
    QRect cr = updateWorkspace();
    int x = 0;
    int y = cr.height() - w->height();

    QList<QWidget *>::Iterator it(icons.begin());
    while (it != icons.end()) {
        QWidget* i = *it;
        ++it;
        if (x > 0 && x + i->width() > cr.width()){
            x = 0;
            y -= i->height();
        }

        if (i != w &&
            i->geometry().intersects(QRect(x, y, w->width(), w->height())))
            x += i->width();
    }
    w->move(x, y);

    if (q->isVisibleTo(q->parentWidget())) {
        w->show();
        w->lower();
    }
    updateWorkspace();
}


void QWorkspacePrivate::removeIcon(QWidget* w)
{
    if (icons.removeAll(w))
        w->hide();
}


/*! \reimp  */
void QWorkspace::resizeEvent(QResizeEvent *)
{
    Q_D(QWorkspace);
    if (d->maxWindow) {
        d->maxWindow->adjustToFullscreen();
        if (d->maxWindow->windowWidget())
            d->maxWindow->windowWidget()->overrideWindowState(Qt::WindowMaximized);
    }
    d->updateWorkspace();
}

/*! \reimp */
void QWorkspace::showEvent(QShowEvent *e)
{
    Q_D(QWorkspace);
    if (d->maxWindow)
        d->showMaximizeControls();
    QWidget::showEvent(e);
    if (d->becomeActive) {
        d->activateWindow(d->becomeActive);
        d->becomeActive = 0;
    } else if (d->windows.count() > 0 && !d->active) {
        d->activateWindow(d->windows.first()->windowWidget());
    }

//     // force a frame repaint - this is a workaround for what seems to be a bug
//     // introduced when changing the QWidget::show() implementation. Might be
//     // a windows bug as well though.
//     for (int i = 0; i < d->windows.count(); ++i) {
//      QWorkspaceChild* c = d->windows.at(i);
//         c->update(c->rect());
//     }

    d->updateWorkspace();
}

/*! \reimp */
void QWorkspace::hideEvent(QHideEvent *)
{
    Q_D(QWorkspace);
    if (!isVisible())
        d->hideMaximizeControls();
}

/*! \reimp */
void QWorkspace::paintEvent(QPaintEvent *)
{
    Q_D(QWorkspace);

    if (d->background.style() != Qt::NoBrush) {
        QPainter p(this);
        p.fillRect(0, 0, width(), height(), d->background);
    }
}

void QWorkspacePrivate::minimizeWindow(QWidget* w)
{
    QWorkspaceChild* c = findChild(w);

    if (!w || !(w->windowFlags() & Qt::WindowMinimizeButtonHint))
        return;

    if (c) {
        bool wasMax = false;
        if (c == maxWindow) {
            wasMax = true;
            maxWindow = 0;
            hideMaximizeControls();
            for (QList<QWorkspaceChild *>::Iterator it(windows.begin()); it != windows.end(); ++it) {
                QWorkspaceChild* c = *it;
                if (c->titlebar)
                    c->titlebar->setMovable(true);
                c->widgetResizeHandler->setActive(true);
            }
        }
        c->hide();
        if (wasMax)
            c->setGeometry(maxRestore);
        if (!focus.contains(c))
            focus.append(c);
        insertIcon(c->iconWidget());

        if (!maxWindow)
            activateWindow(w);

        updateWorkspace();

        w->overrideWindowState(Qt::WindowMinimized);
        c->overrideWindowState(Qt::WindowMinimized);
    }
}

void QWorkspacePrivate::normalizeWindow(QWidget* w)
{
    Q_Q(QWorkspace);
    QWorkspaceChild* c = findChild(w);
    if (!w)
        return;
    if (c) {
        w->overrideWindowState(Qt::WindowNoState);
        hideMaximizeControls();
        if (!maxmenubar || q->style()->styleHint(QStyle::SH_Workspace_FillSpaceOnMaximize, 0, q) || !maxWindow) {
            if (w->minimumSize() != w->maximumSize())
                c->widgetResizeHandler->setActive(true);
            if (c->titlebar)
                c->titlebar->setMovable(true);
        }
        w->overrideWindowState(Qt::WindowNoState);
        c->overrideWindowState(Qt::WindowNoState);

        if (c == maxWindow) {
            c->setGeometry(maxRestore);
            maxWindow = 0;
        } else {
            if (c->iconw)
                removeIcon(c->iconw->parentWidget());
            c->show();
        }

        hideMaximizeControls();
        for (QList<QWorkspaceChild *>::Iterator it(windows.begin()); it != windows.end(); ++it) {
            QWorkspaceChild* c = *it;
            if (c->titlebar)
                c->titlebar->setMovable(true);
            if (c->childWidget && c->childWidget->minimumSize() != c->childWidget->maximumSize())
                c->widgetResizeHandler->setActive(true);
        }
        activateWindow(w, true);
        updateWorkspace();
    }
}

void QWorkspacePrivate::maximizeWindow(QWidget* w)
{
    Q_Q(QWorkspace);
    QWorkspaceChild* c = findChild(w);

    if (!w || !(w->windowFlags() & Qt::WindowMaximizeButtonHint))
        return;

    if (!c || c == maxWindow)
        return;

    bool updatesEnabled = q->updatesEnabled();
    q->setUpdatesEnabled(false);

    if (c->iconw && icons.contains(c->iconw->parentWidget()))
        normalizeWindow(w);
    QRect r(c->geometry());
    QWorkspaceChild *oldMaxWindow = maxWindow;
    maxWindow = c;

    showMaximizeControls();

    c->adjustToFullscreen();
    c->show();
    c->internalRaise();
    if (oldMaxWindow != c) {
        if (oldMaxWindow) {
            oldMaxWindow->setGeometry(maxRestore);
            oldMaxWindow->overrideWindowState(Qt::WindowNoState);
            if(oldMaxWindow->windowWidget())
                oldMaxWindow->windowWidget()->overrideWindowState(Qt::WindowNoState);
        }
        maxRestore = r;
    }

    activateWindow(w);

    if(!maxmenubar || q->style()->styleHint(QStyle::SH_Workspace_FillSpaceOnMaximize, 0, q)) {
        if (!active && becomeActive) {
            active = (QWorkspaceChild*)becomeActive->parentWidget();
            active->setActive(true);
            becomeActive = 0;
            emit q->windowActivated(active->windowWidget());
        }
        c->widgetResizeHandler->setActive(false);
        if (c->titlebar)
            c->titlebar->setMovable(false);
    }
    updateWorkspace();

    w->overrideWindowState(Qt::WindowMaximized);
    c->overrideWindowState(Qt::WindowMaximized);
    q->setUpdatesEnabled(updatesEnabled);
}

void QWorkspacePrivate::showWindow(QWidget* w)
{
    if (w->isMinimized() && (w->windowFlags() & Qt::WindowMinimizeButtonHint))
        minimizeWindow(w);
    else if ((maxWindow || w->isMaximized()) && w->windowFlags() & Qt::WindowMaximizeButtonHint)
        maximizeWindow(w);
    else if (w->windowFlags() & Qt::WindowMaximizeButtonHint)
        normalizeWindow(w);
    else
        w->parentWidget()->show();
    if (maxWindow)
        maxWindow->internalRaise();
    updateWorkspace();
}


QWorkspaceChild* QWorkspacePrivate::findChild(QWidget* w)
{
    QList<QWorkspaceChild *>::Iterator it(windows.begin());
    while (it != windows.end()) {
        QWorkspaceChild* c = *it;
        ++it;
        if (c->windowWidget() == w)
            return c;
    }
    return 0;
}

/*!
    Returns a list of all visible or minimized child windows. If \a
    order is CreationOrder (the default), the windows are listed in
    the order in which they were inserted into the workspace. If \a
    order is StackingOrder, the windows are listed in their stacking
    order, with the topmost window as the last item in the list.
*/
QWidgetList QWorkspace::windowList(WindowOrder order) const
{
    Q_D(const QWorkspace);
    QWidgetList windows;
    if (order == StackingOrder) {
        QObjectList cl = children();
        for (int i = 0; i < cl.size(); ++i) {
            QWorkspaceChild *c = qobject_cast<QWorkspaceChild*>(cl.at(i));
            if (c && c->isWindowOrIconVisible())
                windows.append(c->windowWidget());
        }
    } else {
        QList<QWorkspaceChild *>::ConstIterator it(d->windows.begin());
        while (it != d->windows.end()) {
            QWorkspaceChild* c = *it;
            ++it;
            if (c && c->isWindowOrIconVisible())
                windows.append(c->windowWidget());
        }
    }
    return windows;
}


/*! \reimp */
bool QWorkspace::event(QEvent *e)
{
#ifndef QT_NO_SHORTCUT
    Q_D(QWorkspace);
    if (e->type() == QEvent::Shortcut) {
        QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
        const char *theSlot = d->shortcutMap.value(se->shortcutId(), 0);
        if (theSlot)
            QMetaObject::invokeMethod(this, theSlot);
    } else
#endif
    if (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut){
        return true;
    }
    return QWidget::event(e);
}

/*! \reimp */
bool QWorkspace::eventFilter(QObject *o, QEvent * e)
{
    Q_D(QWorkspace);
    static QElapsedTimer* t = 0;
    static QWorkspace* tc = 0;
    if (o == d->maxtools) {
        switch (e->type()) {
        case QEvent::MouseButtonPress:
            {
                QMenuBar* b = (QMenuBar*)o->parent();
                if (!t)
                    t = new QElapsedTimer;
                if (tc != this || t->elapsed() > QApplication::doubleClickInterval()) {
                    if (isRightToLeft()) {
                        QPoint p = b->mapToGlobal(QPoint(b->x() + b->width(), b->y() + b->height()));
                        p.rx() -= d->popup->sizeHint().width();
                        d->_q_popupOperationMenu(p);
                    } else {
                        d->_q_popupOperationMenu(b->mapToGlobal(QPoint(b->x(), b->y() + b->height())));
                    }
                    t->start();
                    tc = this;
                } else {
                    tc = 0;
                    closeActiveWindow();
                }
                return true;
            }
        default:
            break;
        }
        return QWidget::eventFilter(o, e);
    }
    switch (e->type()) {
    case QEvent::HideToParent:
        break;
    case QEvent::ShowToParent:
        if (QWorkspaceChild *c = qobject_cast<QWorkspaceChild*>(o))
            if (!d->focus.contains(c))
                d->focus.append(c);
        d->updateWorkspace();
        break;
    case QEvent::WindowTitleChange:
        if (!d->inTitleChange) {
            if (o == window())
                d->topTitle = window()->windowTitle();
            if (d->maxWindow && d->maxWindow->windowWidget() && d->topTitle.size()) {
                d->inTitleChange = true;
                window()->setWindowTitle(tr("%1 - [%2]")
                                         .arg(d->topTitle).arg(d->maxWindow->windowWidget()->windowTitle()));
                d->inTitleChange = false;
            }
        }
        break;

    case QEvent::ModifiedChange:
        if (o == d->maxWindow)
            window()->setWindowModified(d->maxWindow->isWindowModified());
        break;

    case QEvent::Close:
        if (o == window())
        {
            QList<QWorkspaceChild *>::Iterator it(d->windows.begin());
            while (it != d->windows.end()) {
                QWorkspaceChild* c = *it;
                ++it;
                if (c->shademode)
                    c->showShaded();
            }
        } else if (qobject_cast<QWorkspaceChild*>(o)) {
            d->popup->hide();
        }
        d->updateWorkspace();
        break;
    default:
        break;
    }
    return QWidget::eventFilter(o, e);
}

static QMenuBar *findMenuBar(QWidget *w)
{
    // don't search recursively to avoid finding a menu bar of a
    // mainwindow that happens to be a workspace window (like
    // a mainwindow in designer)
    QList<QObject *> children = w->children();
    for (int i = 0; i < children.count(); ++i) {
        QMenuBar *bar = qobject_cast<QMenuBar *>(children.at(i));
        if (bar)
            return bar;
    }
    return 0;
}

void QWorkspacePrivate::showMaximizeControls()
{
    Q_Q(QWorkspace);
    Q_ASSERT(maxWindow);

    // merge windowtitle and modified state
    if (!topTitle.size())
        topTitle = q->window()->windowTitle();

    if (maxWindow->windowWidget()) {
        QString docTitle = maxWindow->windowWidget()->windowTitle();
        if (topTitle.size() && docTitle.size()) {
            inTitleChange = true;
            q->window()->setWindowTitle(QWorkspace::tr("%1 - [%2]").arg(topTitle).arg(docTitle));
            inTitleChange = false;
        }
        q->window()->setWindowModified(maxWindow->windowWidget()->isWindowModified());
    }

    if (!q->style()->styleHint(QStyle::SH_Workspace_FillSpaceOnMaximize, 0, q)) {
        QMenuBar* b = 0;

        // Do a breadth-first search first on every parent,
        QWidget* w = q->parentWidget();
        while (w) {
            b = findMenuBar(w);
            if (b)
                break;
            w = w->parentWidget();
        }

        // last attempt.
        if (!b)
            b = findMenuBar(q->window());

        if (!b)
            return;

        if (!maxcontrols) {
            maxmenubar = b;
            maxcontrols = new QMDIControl(b);
            QObject::connect(maxcontrols, SIGNAL(_q_minimize()),
                             q, SLOT(_q_minimizeActiveWindow()));
            QObject::connect(maxcontrols, SIGNAL(_q_restore()),
                             q, SLOT(_q_normalizeActiveWindow()));
            QObject::connect(maxcontrols, SIGNAL(_q_close()),
                             q, SLOT(closeActiveWindow()));
        }

        b->setCornerWidget(maxcontrols);
        if (b->isVisible())
            maxcontrols->show();
        if (!active && becomeActive) {
            active = (QWorkspaceChild*)becomeActive->parentWidget();
            active->setActive(true);
            becomeActive = 0;
            emit q->windowActivated(active->windowWidget());
        }
        if (active) {
            if (!maxtools) {
                maxtools = new QLabel(q->window());
                maxtools->setObjectName(QLatin1String("qt_maxtools"));
                maxtools->installEventFilter(q);
            }
            if (active->windowWidget() && !active->windowWidget()->windowIcon().isNull()) {
                QIcon icon = active->windowWidget()->windowIcon();
                int iconSize = maxcontrols->size().height();
                maxtools->setPixmap(icon.pixmap(QSize(iconSize, iconSize)));
            } else {
                QPixmap pm = q->style()->standardPixmap(QStyle::SP_TitleBarMenuButton, 0, q);
                if (pm.isNull()) {
                    pm = QPixmap(14,14);
                    pm.fill(Qt::black);
                }
                maxtools->setPixmap(pm);
            }
            b->setCornerWidget(maxtools, Qt::TopLeftCorner);
            if (b->isVisible())
                maxtools->show();
        }
    }
}


void QWorkspacePrivate::hideMaximizeControls()
{
    Q_Q(QWorkspace);
    if (maxmenubar && !q->style()->styleHint(QStyle::SH_Workspace_FillSpaceOnMaximize, 0, q)) {
        if (maxmenubar) {
            maxmenubar->setCornerWidget(0, Qt::TopLeftCorner);
            maxmenubar->setCornerWidget(0, Qt::TopRightCorner);
        }
        if (maxcontrols) {
            maxcontrols->deleteLater();
            maxcontrols = 0;
        }
        if (maxtools) {
            maxtools->deleteLater();
            maxtools = 0;
        }
    }

    //unmerge the title bar/modification state
    if (topTitle.size()) {
        inTitleChange = true;
        q->window()->setWindowTitle(topTitle);
        inTitleChange = false;
    }
    q->window()->setWindowModified(false);
}

/*!
    Closes the child window that is currently active.

    \sa closeAllWindows()
*/
void QWorkspace::closeActiveWindow()
{
    Q_D(QWorkspace);
    if (d->maxWindow && d->maxWindow->windowWidget())
        d->maxWindow->windowWidget()->close();
    else if (d->active && d->active->windowWidget())
        d->active->windowWidget()->close();
    d->updateWorkspace();
}

/*!
    Closes all child windows.

    If any child window fails to accept the close event, the remaining windows
    will remain open.

    \sa closeActiveWindow()
*/
void QWorkspace::closeAllWindows()
{
    Q_D(QWorkspace);
    bool did_close = true;
    QList<QWorkspaceChild *>::const_iterator it = d->windows.constBegin();
    while (it != d->windows.constEnd() && did_close) {
        QWorkspaceChild *c = *it;
        ++it;
        if (c->windowWidget() && !c->windowWidget()->isHidden())
            did_close = c->windowWidget()->close();
    }
}

void QWorkspacePrivate::_q_normalizeActiveWindow()
{
    if (maxWindow)
        maxWindow->showNormal();
    else if (active)
        active->showNormal();
}

void QWorkspacePrivate::_q_minimizeActiveWindow()
{
    if (maxWindow)
        maxWindow->showMinimized();
    else if (active)
        active->showMinimized();
}

void QWorkspacePrivate::_q_showOperationMenu()
{
    Q_Q(QWorkspace);
    if  (!active || !active->windowWidget())
        return;
    Q_ASSERT((active->windowWidget()->windowFlags() & Qt::WindowSystemMenuHint));
    QPoint p;
    QMenu *popup = (active->titlebar && active->titlebar->isTool()) ? toolPopup : this->popup;
    if (q->isRightToLeft()) {
        p = QPoint(active->windowWidget()->mapToGlobal(QPoint(active->windowWidget()->width(),0)));
        p.rx() -= popup->sizeHint().width();
    } else {
        p = QPoint(active->windowWidget()->mapToGlobal(QPoint(0,0)));
    }
    if (!active->isVisible()) {
        p = active->iconWidget()->mapToGlobal(QPoint(0,0));
        p.ry() -= popup->sizeHint().height();
    }
    _q_popupOperationMenu(p);
}

void QWorkspacePrivate::_q_popupOperationMenu(const QPoint&  p)
{
    if (!active || !active->windowWidget() || !(active->windowWidget()->windowFlags() & Qt::WindowSystemMenuHint))
        return;
    if (active->titlebar && active->titlebar->isTool())
        toolPopup->popup(p);
    else
        popup->popup(p);
}

void QWorkspacePrivate::_q_updateActions()
{
    Q_Q(QWorkspace);
    for (int i = 1; i < NCountAct-1; i++) {
        bool enable = active != 0;
        actions[i]->setEnabled(enable);
    }

    if (!active || !active->windowWidget())
        return;

    QWidget *windowWidget = active->windowWidget();
    bool canResize = windowWidget->maximumSize() != windowWidget->minimumSize();
    actions[QWorkspacePrivate::ResizeAct]->setEnabled(canResize);
    actions[QWorkspacePrivate::MinimizeAct]->setEnabled((windowWidget->windowFlags() & Qt::WindowMinimizeButtonHint));
    actions[QWorkspacePrivate::MaximizeAct]->setEnabled((windowWidget->windowFlags() & Qt::WindowMaximizeButtonHint) && canResize);

    if (active == maxWindow) {
        actions[QWorkspacePrivate::MoveAct]->setEnabled(false);
        actions[QWorkspacePrivate::ResizeAct]->setEnabled(false);
        actions[QWorkspacePrivate::MaximizeAct]->setEnabled(false);
        actions[QWorkspacePrivate::RestoreAct]->setEnabled(true);
    } else if (active->isVisible()){
        actions[QWorkspacePrivate::RestoreAct]->setEnabled(false);
    } else {
        actions[QWorkspacePrivate::MoveAct]->setEnabled(false);
        actions[QWorkspacePrivate::ResizeAct]->setEnabled(false);
        actions[QWorkspacePrivate::MinimizeAct]->setEnabled(false);
        actions[QWorkspacePrivate::RestoreAct]->setEnabled(true);
    }
    if (active->shademode) {
        actions[QWorkspacePrivate::ShadeAct]->setIcon(
            QIcon(q->style()->standardPixmap(QStyle::SP_TitleBarUnshadeButton, 0, q)));
        actions[QWorkspacePrivate::ShadeAct]->setText(QWorkspace::tr("&Unshade"));
    } else {
        actions[QWorkspacePrivate::ShadeAct]->setIcon(
            QIcon(q->style()->standardPixmap(QStyle::SP_TitleBarShadeButton, 0, q)));
        actions[QWorkspacePrivate::ShadeAct]->setText(QWorkspace::tr("Sh&ade"));
    }
    actions[QWorkspacePrivate::StaysOnTopAct]->setEnabled(!active->shademode && canResize);
    actions[QWorkspacePrivate::StaysOnTopAct]->setChecked(
        (active->windowWidget()->windowFlags() & Qt::WindowStaysOnTopHint));
}

void QWorkspacePrivate::_q_operationMenuActivated(QAction *action)
{
    if (!active)
        return;
    if(action == actions[QWorkspacePrivate::RestoreAct]) {
        active->showNormal();
    } else if(action == actions[QWorkspacePrivate::MoveAct]) {
        active->doMove();
    } else if(action == actions[QWorkspacePrivate::ResizeAct]) {
        if (active->shademode)
            active->showShaded();
        active->doResize();
    } else if(action == actions[QWorkspacePrivate::MinimizeAct]) {
        active->showMinimized();
    } else if(action == actions[QWorkspacePrivate::MaximizeAct]) {
        active->showMaximized();
    } else if(action == actions[QWorkspacePrivate::ShadeAct]) {
        active->showShaded();
    } else if(action == actions[QWorkspacePrivate::StaysOnTopAct]) {
        if(QWidget* w = active->windowWidget()) {
            if ((w->windowFlags() & Qt::WindowStaysOnTopHint)) {
                w->overrideWindowFlags(w->windowFlags() & ~Qt::WindowStaysOnTopHint);
            } else {
                w->overrideWindowFlags(w->windowFlags() | Qt::WindowStaysOnTopHint);
                w->parentWidget()->raise();
            }
        }
    }
}


void QWorkspacePrivate::hideChild(QWorkspaceChild *c)
{
    Q_Q(QWorkspace);

//     bool updatesEnabled = q->updatesEnabled();
//     q->setUpdatesEnabled(false);
    focus.removeAll(c);
    QRect restore;
    if (maxWindow == c)
        restore = maxRestore;
    if (active == c) {
        q->setFocus();
        q->activatePreviousWindow();
    }
    if (active == c)
        activateWindow(0);
    if (maxWindow == c) {
        hideMaximizeControls();
        maxWindow = 0;
    }
    c->hide();
    if (!restore.isEmpty())
        c->setGeometry(restore);
//     q->setUpdatesEnabled(updatesEnabled);
}

/*!
    Gives the input focus to the next window in the list of child
    windows.

    \sa activatePreviousWindow()
*/
void QWorkspace::activateNextWindow()
{
    Q_D(QWorkspace);

    if (d->focus.isEmpty())
        return;
    if (!d->active) {
        if (d->focus.first())
            d->activateWindow(d->focus.first()->windowWidget(), false);
        return;
    }

    int a = d->focus.indexOf(d->active) + 1;

    a = a % d->focus.count();

    if (d->focus.at(a))
        d->activateWindow(d->focus.at(a)->windowWidget(), false);
    else
        d->activateWindow(0);
}

/*!
    Gives the input focus to the previous window in the list of child
    windows.

    \sa activateNextWindow()
*/
void QWorkspace::activatePreviousWindow()
{
    Q_D(QWorkspace);

    if (d->focus.isEmpty())
        return;
    if (!d->active) {
        if (d->focus.last())
            d->activateWindow(d->focus.first()->windowWidget(), false);
        else
            d->activateWindow(0);
        return;
    }

    int a = d->focus.indexOf(d->active) - 1;
    if (a < 0)
        a = d->focus.count()-1;

    if (d->focus.at(a))
        d->activateWindow(d->focus.at(a)->windowWidget(), false);
    else
        d->activateWindow(0);
}


/*!
    \fn void QWorkspace::windowActivated(QWidget* w)

    This signal is emitted when the child window \a w becomes active.
    Note that \a w can be 0, and that more than one signal may be
    emitted for a single activation event.

    \sa activeWindow(), windowList()
*/

/*!
    Arranges all the child windows in a cascade pattern.

    \sa tile(), arrangeIcons()
*/
void QWorkspace::cascade()
{
    Q_D(QWorkspace);
    blockSignals(true);
    if  (d->maxWindow)
        d->maxWindow->showNormal();

    if (d->vbar) {
        d->vbar->blockSignals(true);
        d->vbar->setValue(0);
        d->vbar->blockSignals(false);
        d->hbar->blockSignals(true);
        d->hbar->setValue(0);
        d->hbar->blockSignals(false);
        d->_q_scrollBarChanged();
    }

    const int xoffset = 13;
    const int yoffset = 20;

    // make a list of all relevant mdi clients
    QList<QWorkspaceChild *> widgets;
    QList<QWorkspaceChild *>::Iterator it(d->windows.begin());
    QWorkspaceChild* wc = 0;

    for (it = d->focus.begin(); it != d->focus.end(); ++it) {
        wc = *it;
        if (wc->windowWidget()->isVisibleTo(this) && !(wc->titlebar && wc->titlebar->isTool()))
            widgets.append(wc);
    }

    int x = 0;
    int y = 0;

    it = widgets.begin();
    while (it != widgets.end()) {
        QWorkspaceChild *child = *it;
        ++it;

        QSize prefSize = child->windowWidget()->sizeHint().expandedTo(qSmartMinSize(child->windowWidget()));
        if (!prefSize.isValid())
            prefSize = child->windowWidget()->size();
        prefSize = prefSize.expandedTo(qSmartMinSize(child->windowWidget()));
        if (prefSize.isValid())
            prefSize += QSize(child->baseSize().width(), child->baseSize().height());

        int w = prefSize.width();
        int h = prefSize.height();

        child->showNormal();
        if (y + h > height())
            y = 0;
        if (x + w > width())
            x = 0;
        child->setGeometry(x, y, w, h);
        x += xoffset;
        y += yoffset;
        child->internalRaise();
    }
    d->updateWorkspace();
    blockSignals(false);
}

/*!
    Arranges all child windows in a tile pattern.

    \sa cascade(), arrangeIcons()
*/
void QWorkspace::tile()
{
    Q_D(QWorkspace);
    blockSignals(true);
    QWidget *oldActive = d->active ? d->active->windowWidget() : 0;
    if  (d->maxWindow)
        d->maxWindow->showNormal();

    if (d->vbar) {
        d->vbar->blockSignals(true);
        d->vbar->setValue(0);
        d->vbar->blockSignals(false);
        d->hbar->blockSignals(true);
        d->hbar->setValue(0);
        d->hbar->blockSignals(false);
        d->_q_scrollBarChanged();
    }

    int rows = 1;
    int cols = 1;
    int n = 0;
    QWorkspaceChild* c;

    QList<QWorkspaceChild *>::Iterator it(d->windows.begin());
    while (it != d->windows.end()) {
        c = *it;
        ++it;
        if (!c->windowWidget()->isHidden()
            && !(c->windowWidget()->windowFlags() & Qt::WindowStaysOnTopHint)
            && !c->iconw)
            n++;
    }

    while (rows * cols < n) {
        if (cols <= rows)
            cols++;
        else
            rows++;
    }
    int add = cols * rows - n;
    bool* used = new bool[cols*rows];
    for (int i = 0; i < rows*cols; i++)
        used[i] = false;

    int row = 0;
    int col = 0;
    int w = width() / cols;
    int h = height() / rows;

    it = d->windows.begin();
    while (it != d->windows.end()) {
        c = *it;
        ++it;
        if (c->iconw || c->windowWidget()->isHidden() || (c->titlebar && c->titlebar->isTool()))
            continue;
        if (!row && !col) {
            w -= c->baseSize().width();
            h -= c->baseSize().height();
        }
        if ((c->windowWidget()->windowFlags() & Qt::WindowStaysOnTopHint)) {
            QPoint p = c->pos();
            if (p.x()+c->width() < 0)
                p.setX(0);
            if (p.x() > width())
                p.setX(width() - c->width());
            if (p.y() + 10 < 0)
                p.setY(0);
            if (p.y() > height())
                p.setY(height() - c->height());

            if (p != c->pos())
                c->QWidget::move(p);
        } else {
            c->showNormal();
            used[row*cols+col] = true;
            QSize sz(w, h);
            QSize bsize(c->baseSize());
            sz = sz.expandedTo(c->windowWidget()->minimumSize()).boundedTo(c->windowWidget()->maximumSize());
            sz += bsize;

            if ( add ) {
                if (sz.height() == h + bsize.height()) // no relevant constrains
                    sz.rheight() *= 2;
                used[(row+1)*cols+col] = true;
                add--;
            }

            c->setGeometry(col*w + col*bsize.width(), row*h + row*bsize.height(), sz.width(), sz.height());

            while(row < rows && col < cols && used[row*cols+col]) {
                col++;
                if (col == cols) {
                    col = 0;
                    row++;
                }
            }
        }
    }
    delete [] used;

    d->activateWindow(oldActive);
    d->updateWorkspace();
    blockSignals(false);
}

/*!
    Arranges all iconified windows at the bottom of the workspace.

    \sa cascade(), tile()
*/
void QWorkspace::arrangeIcons()
{
    Q_D(QWorkspace);

    QRect cr = d->updateWorkspace();
    int x = 0;
    int y = -1;

    QList<QWidget *>::Iterator it(d->icons.begin());
    while (it != d->icons.end()) {
        QWidget* i = *it;
        if (y == -1)
            y = cr.height() - i->height();
        if (x > 0 && x + i->width() > cr.width()) {
            x = 0;
            y -= i->height();
        }
        i->move(x, y);
        x += i->width();
        ++it;
    }
    d->updateWorkspace();
}


QWorkspaceChild::QWorkspaceChild(QWidget* window, QWorkspace *parent, Qt::WindowFlags flags)
    : QWidget(parent,
             Qt::FramelessWindowHint | Qt::SubWindow)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_NoMousePropagation);
    setMouseTracking(true);
    act = false;
    iconw = 0;
    shademode = false;
    titlebar = 0;
    setAutoFillBackground(true);

    setBackgroundRole(QPalette::Window);
    if (window) {
        flags |= (window->windowFlags() & Qt::MSWindowsOwnDC);
        if (flags)
            window->setParent(this, flags & ~Qt::WindowType_Mask);
        else
            window->setParent(this);
    }

    if (window && (flags & (Qt::WindowTitleHint
                            | Qt::WindowSystemMenuHint
                            | Qt::WindowMinimizeButtonHint
                            | Qt::WindowMaximizeButtonHint
                            | Qt::WindowContextHelpButtonHint))) {
        titlebar = new QWorkspaceTitleBar(window, this, flags);
        connect(titlebar, SIGNAL(doActivate()),
                 this, SLOT(activate()));
        connect(titlebar, SIGNAL(doClose()),
                 window, SLOT(close()));
        connect(titlebar, SIGNAL(doMinimize()),
                 this, SLOT(showMinimized()));
        connect(titlebar, SIGNAL(doNormal()),
                 this, SLOT(showNormal()));
        connect(titlebar, SIGNAL(doMaximize()),
                 this, SLOT(showMaximized()));
        connect(titlebar, SIGNAL(popupOperationMenu(QPoint)),
                 this, SIGNAL(popupOperationMenu(QPoint)));
        connect(titlebar, SIGNAL(showOperationMenu()),
                 this, SIGNAL(showOperationMenu()));
        connect(titlebar, SIGNAL(doShade()),
                 this, SLOT(showShaded()));
        connect(titlebar, SIGNAL(doubleClicked()),
                 this, SLOT(titleBarDoubleClicked()));
    }

    setMinimumSize(128, 0);
    int fw =  style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, 0, this);
    setContentsMargins(fw, fw, fw, fw);

    childWidget = window;
    if (!childWidget)
        return;

    setWindowTitle(childWidget->windowTitle());

    QPoint p;
    QSize s;
    QSize cs;

    bool hasBeenResized = childWidget->testAttribute(Qt::WA_Resized);

    if (!hasBeenResized)
        cs = childWidget->sizeHint().expandedTo(childWidget->minimumSizeHint()).expandedTo(childWidget->minimumSize()).boundedTo(childWidget->maximumSize());
    else
        cs = childWidget->size();

    windowSize = cs;

    int th = titlebar ? titlebar->sizeHint().height() : 0;
    if (titlebar) {
        if (!childWidget->windowIcon().isNull())
            titlebar->setWindowIcon(childWidget->windowIcon());

        if (style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar))
            th -= contentsRect().y();

        p = QPoint(contentsRect().x(),
                    th + contentsRect().y());
        s = QSize(cs.width() + 2*frameWidth(),
                   cs.height() + 2*frameWidth() + th);
    } else {
        p = QPoint(contentsRect().x(), contentsRect().y());
        s = QSize(cs.width() + 2*frameWidth(),
                   cs.height() + 2*frameWidth());
    }

    childWidget->move(p);
    resize(s);

    childWidget->installEventFilter(this);

    widgetResizeHandler = new QWidgetResizeHandler(this, window);
    widgetResizeHandler->setSizeProtection(!parent->scrollBarsEnabled());
    widgetResizeHandler->setFrameWidth(frameWidth());
    connect(widgetResizeHandler, SIGNAL(activate()),
             this, SLOT(activate()));
    if (!style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar))
        widgetResizeHandler->setExtraHeight(th + contentsRect().y() - 2*frameWidth());
    else
        widgetResizeHandler->setExtraHeight(th + contentsRect().y() - frameWidth());
    if (childWidget->minimumSize() == childWidget->maximumSize())
        widgetResizeHandler->setActive(QWidgetResizeHandler::Resize, false);
    setBaseSize(baseSize());
}

QWorkspaceChild::~QWorkspaceChild()
{
    QWorkspace *workspace = qobject_cast<QWorkspace*>(parentWidget());
    if (iconw) {
        if (workspace)
            workspace->d_func()->removeIcon(iconw->parentWidget());
        delete iconw->parentWidget();
    }

    if (workspace) {
        workspace->d_func()->focus.removeAll(this);
        if (workspace->d_func()->active == this)
            workspace->activatePreviousWindow();
        if (workspace->d_func()->active == this)
            workspace->d_func()->activateWindow(0);
        if (workspace->d_func()->maxWindow == this) {
            workspace->d_func()->hideMaximizeControls();
            workspace->d_func()->maxWindow = 0;
        }
    }
}

void QWorkspaceChild::moveEvent(QMoveEvent *)
{
    ((QWorkspace*)parentWidget())->d_func()->updateWorkspace();
}

void QWorkspaceChild::resizeEvent(QResizeEvent *)
{
    bool wasMax = isMaximized();
    QRect r = contentsRect();
    QRect cr;

    updateMask();

    if (titlebar) {
        int th = titlebar->sizeHint().height();
        QRect tbrect(0, 0, width(), th);
        if (!style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar))
            tbrect = QRect(r.x(), r.y(), r.width(), th);
        titlebar->setGeometry(tbrect);

        if (style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar))
            th -= frameWidth();
        cr = QRect(r.x(), r.y() + th + (shademode ? (frameWidth() * 3) : 0),
                    r.width(), r.height() - th);
    } else {
        cr = r;
    }

    if (!childWidget)
        return;

    bool doContentsResize = (windowSize == childWidget->size()
                             || !(childWidget->testAttribute(Qt::WA_Resized) && childWidget->testAttribute(Qt::WA_PendingResizeEvent))
                             ||childWidget->isMaximized());

    windowSize = cr.size();
    childWidget->move(cr.topLeft());
    if (doContentsResize)
        childWidget->resize(cr.size());
    ((QWorkspace*)parentWidget())->d_func()->updateWorkspace();

    if (wasMax) {
        overrideWindowState(Qt::WindowMaximized);
        childWidget->overrideWindowState(Qt::WindowMaximized);
    }
}

QSize QWorkspaceChild::baseSize() const
{
    int th = titlebar ? titlebar->sizeHint().height() : 0;
    if (style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar))
        th -= frameWidth();
    return QSize(2*frameWidth(), 2*frameWidth() + th);
}

QSize QWorkspaceChild::sizeHint() const
{
    if (!childWidget)
        return QWidget::sizeHint() + baseSize();

    QSize prefSize = windowWidget()->sizeHint().expandedTo(windowWidget()->minimumSizeHint());
    prefSize = prefSize.expandedTo(windowWidget()->minimumSize()).boundedTo(windowWidget()->maximumSize());
    prefSize += baseSize();

    return prefSize;
}

QSize QWorkspaceChild::minimumSizeHint() const
{
    if (!childWidget)
        return QWidget::minimumSizeHint() + baseSize();
    QSize s = childWidget->minimumSize();
    if (s.isEmpty())
        s = childWidget->minimumSizeHint();
    return s + baseSize();
}

void QWorkspaceChild::activate()
{
    ((QWorkspace*)parentWidget())->d_func()->activateWindow(windowWidget());
}

bool QWorkspaceChild::eventFilter(QObject * o, QEvent * e)
{
    if (!isActive()
        && (e->type() == QEvent::MouseButtonPress || e->type() == QEvent::FocusIn)) {
        if (iconw) {
            ((QWorkspace*)parentWidget())->d_func()->normalizeWindow(windowWidget());
            if (iconw) {
                ((QWorkspace*)parentWidget())->d_func()->removeIcon(iconw->parentWidget());
                delete iconw->parentWidget();
                iconw = 0;
            }
        }
        activate();
    }

    // for all widgets except the window, that's the only thing we
    // process, and if we have no childWidget we skip totally
    if (o != childWidget || childWidget == 0)
        return false;

    switch (e->type()) {
    case QEvent::ShowToParent:
        if (((QWorkspace*)parentWidget())->d_func()->focus.indexOf(this) < 0)
            ((QWorkspace*)parentWidget())->d_func()->focus.append(this);

        if (windowWidget() && (windowWidget()->windowFlags() & Qt::WindowStaysOnTopHint)) {
            internalRaise();
            show();
        }
        ((QWorkspace*)parentWidget())->d_func()->showWindow(windowWidget());
        break;
    case QEvent::WindowStateChange: {
        if (static_cast<QWindowStateChangeEvent*>(e)->isOverride())
            break;
        Qt::WindowStates state = windowWidget()->windowState();

        if (state & Qt::WindowMinimized) {
            ((QWorkspace*)parentWidget())->d_func()->minimizeWindow(windowWidget());
        } else if (state & Qt::WindowMaximized) {
            if (windowWidget()->maximumSize().isValid() &&
                (windowWidget()->maximumWidth() < parentWidget()->width() ||
                 windowWidget()->maximumHeight() < parentWidget()->height())) {
                windowWidget()->resize(windowWidget()->maximumSize());
                windowWidget()->overrideWindowState(Qt::WindowNoState);
                if (titlebar)
                    titlebar->update();
                break;
            }
            if ((windowWidget()->windowFlags() & Qt::WindowMaximizeButtonHint))
                ((QWorkspace*)parentWidget())->d_func()->maximizeWindow(windowWidget());
            else
                ((QWorkspace*)parentWidget())->d_func()->normalizeWindow(windowWidget());
        } else {
            ((QWorkspace*)parentWidget())->d_func()->normalizeWindow(windowWidget());
            if (iconw) {
                ((QWorkspace*)parentWidget())->d_func()->removeIcon(iconw->parentWidget());
                delete iconw->parentWidget();
            }
        }
    } break;
    case QEvent::HideToParent:
    {
        QWidget * w = iconw;
        if (w && (w = w->parentWidget())) {
            ((QWorkspace*)parentWidget())->d_func()->removeIcon(w);
            delete w;
        }
        ((QWorkspace*)parentWidget())->d_func()->hideChild(this);
    } break;
    case QEvent::WindowIconChange:
        {
            QWorkspace* ws = (QWorkspace*)parentWidget();
            if (ws->d_func()->maxtools && ws->d_func()->maxWindow == this) {
                int iconSize = ws->d_func()->maxtools->size().height();
                ws->d_func()->maxtools->setPixmap(childWidget->windowIcon().pixmap(QSize(iconSize, iconSize)));
            }
        }
        // fall through
    case QEvent::WindowTitleChange:
        setWindowTitle(windowWidget()->windowTitle());
        if (titlebar)
            titlebar->update();
        if (iconw)
            iconw->update();
        break;
    case QEvent::ModifiedChange:
        setWindowModified(windowWidget()->isWindowModified());
        if (titlebar)
            titlebar->update();
        if (iconw)
            iconw->update();
        break;
    case QEvent::Resize:
        {
            QResizeEvent* re = (QResizeEvent*)e;
            if (re->size() != windowSize && !shademode) {
                resize(re->size() + baseSize());
                childWidget->update(); //workaround
            }
        }
        break;

    case QEvent::WindowDeactivate:
        if (titlebar && titlebar->isActive()) {
            update();
        }
        break;

    case QEvent::WindowActivate:
        if (titlebar && titlebar->isActive()) {
            update();
        }
        break;

    default:
        break;
    }

    return QWidget::eventFilter(o, e);
}

void QWorkspaceChild::childEvent(QChildEvent* e)
{
    if (e->type() == QEvent::ChildRemoved && e->child() == childWidget) {
        childWidget = 0;
        if (iconw) {
            ((QWorkspace*)parentWidget())->d_func()->removeIcon(iconw->parentWidget());
            delete iconw->parentWidget();
        }
        close();
    }
}


void QWorkspaceChild::doResize()
{
    widgetResizeHandler->doResize();
}

void QWorkspaceChild::doMove()
{
    widgetResizeHandler->doMove();
}

void QWorkspaceChild::enterEvent(QEvent *)
{
}

void QWorkspaceChild::leaveEvent(QEvent *)
{
#ifndef QT_NO_CURSOR
    if (!widgetResizeHandler->isButtonDown())
        setCursor(Qt::ArrowCursor);
#endif
}

void QWorkspaceChild::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOptionFrame opt;
    opt.rect = rect();
    opt.palette = palette();
    opt.state = QStyle::State_None;
    opt.lineWidth = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, 0, this);
    opt.midLineWidth = 1;

    if (titlebar && titlebar->isActive() && isActiveWindow())
        opt.state |= QStyle::State_Active;

    style()->drawPrimitive(QStyle::PE_FrameWindow, &opt, &p, this);
}

void QWorkspaceChild::changeEvent(QEvent *ev)
{
    if(ev->type() == QEvent::StyleChange) {
        resizeEvent(0);
        if (iconw) {
            QFrame *frame = qobject_cast<QFrame*>(iconw->parentWidget());
            Q_ASSERT(frame);
            if (!style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar)) {
                frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
                frame->resize(196+2*frame->frameWidth(), 20 + 2*frame->frameWidth());
            } else {
                frame->resize(196, 20);
            }
        }
        updateMask();
    }
    QWidget::changeEvent(ev);
}

void QWorkspaceChild::setActive(bool b)
{
    if (!childWidget)
        return;

    bool hasFocus = isChildOf(window()->focusWidget(), this);
    if (act == b && (act == hasFocus))
        return;

    act = b;

    if (titlebar)
        titlebar->setActive(act);
    if (iconw)
        iconw->setActive(act);
    update();

    QList<QWidget*> wl = childWidget->findChildren<QWidget*>();
    if (act) {
        for (int i = 0; i < wl.size(); ++i) {
            QWidget *w = wl.at(i);
            w->removeEventFilter(this);
        }
        if (!hasFocus) {
            QWidget *lastfocusw = childWidget->focusWidget();
            if (lastfocusw && lastfocusw->focusPolicy() != Qt::NoFocus) {
                lastfocusw->setFocus();
            } else if (childWidget->focusPolicy() != Qt::NoFocus) {
                childWidget->setFocus();
            } else {
                // find something, anything, that accepts focus, and use that.
                for (int i = 0; i < wl.size(); ++i) {
                    QWidget *w = wl.at(i);
                    if(w->focusPolicy() != Qt::NoFocus) {
                        w->setFocus();
                        hasFocus = true;
                        break;
                    }
                }
                if (!hasFocus)
                    setFocus();
            }
        }
    } else {
        for (int i = 0; i < wl.size(); ++i) {
            QWidget *w = wl.at(i);
            w->removeEventFilter(this);
            w->installEventFilter(this);
        }
    }
}

bool QWorkspaceChild::isActive() const
{
    return act;
}

QWidget* QWorkspaceChild::windowWidget() const
{
    return childWidget;
}

bool QWorkspaceChild::isWindowOrIconVisible() const
{
    return childWidget && (!isHidden()  || (iconw && !iconw->isHidden()));
}

void QWorkspaceChild::updateMask()
{
    QStyleOptionTitleBar titleBarOptions;
    titleBarOptions.rect = rect();
    titleBarOptions.titleBarFlags = windowFlags();
    titleBarOptions.titleBarState = windowState();

    QStyleHintReturnMask frameMask;
    if (style()->styleHint(QStyle::SH_WindowFrame_Mask, &titleBarOptions, this, &frameMask)) {
        setMask(frameMask.region);
    } else if (!mask().isEmpty()) {
        clearMask();
    }

    if (iconw) {
        QFrame *frame = qobject_cast<QFrame *>(iconw->parentWidget());
        Q_ASSERT(frame);

        titleBarOptions.rect = frame->rect();
        titleBarOptions.titleBarFlags = frame->windowFlags();
        titleBarOptions.titleBarState = frame->windowState() | Qt::WindowMinimized;
        if (style()->styleHint(QStyle::SH_WindowFrame_Mask, &titleBarOptions, frame, &frameMask)) {
            frame->setMask(frameMask.region);
        } else if (!frame->mask().isEmpty()) {
            frame->clearMask();
        }
    }
}

QWidget* QWorkspaceChild::iconWidget() const
{
    if (!iconw) {
        QWorkspaceChild* that = (QWorkspaceChild*) this;

        QFrame* frame = new QFrame(that, Qt::Window);
        QVBoxLayout *vbox = new QVBoxLayout(frame);
        vbox->setMargin(0);
        QWorkspaceTitleBar *tb = new QWorkspaceTitleBar(windowWidget(), frame);
        vbox->addWidget(tb);
        tb->setObjectName(QLatin1String("_workspacechild_icon_"));
        QStyleOptionTitleBar opt;
        tb->initStyleOption(&opt);
        int th = style()->pixelMetric(QStyle::PM_TitleBarHeight, &opt, tb);
        int iconSize = style()->pixelMetric(QStyle::PM_MdiSubWindowMinimizedWidth, 0, this);
        if (!style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar)) {
            frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
            frame->resize(iconSize+2*frame->frameWidth(), th+2*frame->frameWidth());
        } else {
            frame->resize(iconSize, th);
        }

        that->iconw = tb;
        that->updateMask();
        iconw->setActive(isActive());

        connect(iconw, SIGNAL(doActivate()),
                 this, SLOT(activate()));
        connect(iconw, SIGNAL(doClose()),
                 windowWidget(), SLOT(close()));
        connect(iconw, SIGNAL(doNormal()),
                 this, SLOT(showNormal()));
        connect(iconw, SIGNAL(doMaximize()),
                 this, SLOT(showMaximized()));
        connect(iconw, SIGNAL(popupOperationMenu(QPoint)),
                 this, SIGNAL(popupOperationMenu(QPoint)));
        connect(iconw, SIGNAL(showOperationMenu()),
                 this, SIGNAL(showOperationMenu()));
        connect(iconw, SIGNAL(doubleClicked()),
                 this, SLOT(titleBarDoubleClicked()));
    }
    if (windowWidget()) {
        iconw->setWindowTitle(windowWidget()->windowTitle());
    }
    return iconw->parentWidget();
}

void QWorkspaceChild::showMinimized()
{
    windowWidget()->setWindowState(Qt::WindowMinimized | (windowWidget()->windowState() & ~Qt::WindowMaximized));
}

void QWorkspaceChild::showMaximized()
{
    windowWidget()->setWindowState(Qt::WindowMaximized | (windowWidget()->windowState() & ~Qt::WindowMinimized));
}

void QWorkspaceChild::showNormal()
{
    windowWidget()->setWindowState(windowWidget()->windowState() & ~(Qt::WindowMinimized|Qt::WindowMaximized));
}

void QWorkspaceChild::showShaded()
{
    if (!titlebar)
        return;
    ((QWorkspace*)parentWidget())->d_func()->activateWindow(windowWidget());
    QWidget* w = windowWidget();
    if (shademode) {
        w->overrideWindowState(Qt::WindowNoState);
        overrideWindowState(Qt::WindowNoState);

        shademode = false;
        resize(shadeRestore.expandedTo(minimumSizeHint()));
        setMinimumSize(shadeRestoreMin);
        style()->polish(this);
    } else {
        shadeRestore = size();
        shadeRestoreMin = minimumSize();
        setMinimumHeight(0);
        shademode = true;
        w->overrideWindowState(Qt::WindowMinimized);
        overrideWindowState(Qt::WindowMinimized);

        if (style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar))
            resize(width(), titlebar->height());
        else
            resize(width(), titlebar->height() + 2*frameWidth() + 1);
        style()->polish(this);
    }
    titlebar->update();
}

void QWorkspaceChild::titleBarDoubleClicked()
{
    if (!windowWidget())
        return;
    if (iconw)
        showNormal();
    else if (windowWidget()->windowFlags() & Qt::WindowShadeButtonHint)
            showShaded();
    else if (windowWidget()->windowFlags() & Qt::WindowMaximizeButtonHint)
        showMaximized();
}

void QWorkspaceChild::adjustToFullscreen()
{
    if (!childWidget)
        return;

    if(!((QWorkspace*)parentWidget())->d_func()->maxmenubar || style()->styleHint(QStyle::SH_Workspace_FillSpaceOnMaximize, 0, this)) {
        setGeometry(parentWidget()->rect());
    } else {
        int fw =  style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth, 0, this);
        bool noBorder = style()->styleHint(QStyle::SH_TitleBar_NoBorder, 0, titlebar);
        int th = titlebar ? titlebar->sizeHint().height() : 0;
        int w = parentWidget()->width() + 2*fw;
        int h = parentWidget()->height() + (noBorder ? fw : 2*fw) + th;
        w = qMax(w, childWidget->minimumWidth());
        h = qMax(h, childWidget->minimumHeight());
        setGeometry(-fw, (noBorder ? 0 : -fw) - th, w, h);
    }
    childWidget->overrideWindowState(Qt::WindowMaximized);
    overrideWindowState(Qt::WindowMaximized);
}

void QWorkspaceChild::internalRaise()
{

    QWidget *stackUnderWidget = 0;
    if (!windowWidget() || (windowWidget()->windowFlags() & Qt::WindowStaysOnTopHint) == 0) {

        QList<QWorkspaceChild *>::Iterator it(((QWorkspace*)parent())->d_func()->windows.begin());
        while (it != ((QWorkspace*)parent())->d_func()->windows.end()) {
            QWorkspaceChild* c = *it;
            ++it;
            if (c->windowWidget() &&
                !c->windowWidget()->isHidden() &&
                (c->windowWidget()->windowFlags() & Qt::WindowStaysOnTopHint)) {
                if (stackUnderWidget)
                    c->stackUnder(stackUnderWidget);
                else
                    c->raise();
                stackUnderWidget = c;
            }
        }
    }

    if (stackUnderWidget) {
        if (iconw)
            iconw->parentWidget()->stackUnder(stackUnderWidget);
        stackUnder(stackUnderWidget);
    } else {
        if (iconw)
            iconw->parentWidget()->raise();
        raise();
    }

}

void QWorkspaceChild::show()
{
    if (childWidget && childWidget->isHidden())
        childWidget->show();
    QWidget::show();
}

bool QWorkspace::scrollBarsEnabled() const
{
    Q_D(const QWorkspace);
    return d->vbar != 0;
}

/*!
    \property QWorkspace::scrollBarsEnabled
    \brief whether the workspace provides scroll bars

    If this property is true, the workspace will provide scroll bars if any
    of the child windows extend beyond the edges of the visible
    workspace. The workspace area will automatically increase to
    contain child windows if they are resized beyond the right or
    bottom edges of the visible area.

    If this property is false (the default), resizing child windows
    out of the visible area of the workspace is not permitted, although
    it is still possible to position them partially outside the visible area.
*/
void QWorkspace::setScrollBarsEnabled(bool enable)
{
    Q_D(QWorkspace);
    if ((d->vbar != 0) == enable)
        return;

    d->xoffset = d->yoffset = 0;
    if (enable) {
        d->vbar = new QScrollBar(Qt::Vertical, this);
        d->vbar->setObjectName(QLatin1String("vertical scrollbar"));
        connect(d->vbar, SIGNAL(valueChanged(int)), this, SLOT(_q_scrollBarChanged()));
        d->hbar = new QScrollBar(Qt::Horizontal, this);
        d->hbar->setObjectName(QLatin1String("horizontal scrollbar"));
        connect(d->hbar, SIGNAL(valueChanged(int)), this, SLOT(_q_scrollBarChanged()));
        d->corner = new QWidget(this);
        d->corner->setBackgroundRole(QPalette::Window);
        d->corner->setObjectName(QLatin1String("qt_corner"));
        d->updateWorkspace();
    } else {
        delete d->vbar;
        delete d->hbar;
        delete d->corner;
        d->vbar = d->hbar = 0;
        d->corner = 0;
    }

    QList<QWorkspaceChild *>::Iterator it(d->windows.begin());
    while (it != d->windows.end()) {
        QWorkspaceChild *child = *it;
        ++it;
        child->widgetResizeHandler->setSizeProtection(!enable);
    }
}

QRect QWorkspacePrivate::updateWorkspace()
{
    Q_Q(QWorkspace);
    QRect cr(q->rect());

    if (q->scrollBarsEnabled() && !maxWindow) {
        corner->raise();
        vbar->raise();
        hbar->raise();
        if (maxWindow)
            maxWindow->internalRaise();

        QRect r(0, 0, 0, 0);
        QList<QWorkspaceChild *>::Iterator it(windows.begin());
        while (it != windows.end()) {
            QWorkspaceChild *child = *it;
            ++it;
            if (!child->isHidden())
                r = r.unite(child->geometry());
        }
        vbar->blockSignals(true);
        hbar->blockSignals(true);

        int hsbExt = hbar->sizeHint().height();
        int vsbExt = vbar->sizeHint().width();


        bool showv = yoffset || yoffset + r.bottom() - q->height() + 1 > 0 || yoffset + r.top() < 0;
        bool showh = xoffset || xoffset + r.right() - q->width() + 1 > 0 || xoffset + r.left() < 0;

        if (showh && !showv)
            showv = yoffset + r.bottom() - q->height() + hsbExt + 1 > 0;
        if (showv && !showh)
            showh = xoffset + r.right() - q->width() + vsbExt  + 1 > 0;

        if (!showh)
            hsbExt = 0;
        if (!showv)
            vsbExt = 0;

        if (showv) {
            vbar->setSingleStep(qMax(q->height() / 12, 30));
            vbar->setPageStep(q->height() - hsbExt);
            vbar->setMinimum(qMin(0, yoffset + qMin(0, r.top())));
            vbar->setMaximum(qMax(0, yoffset + qMax(0, r.bottom() - q->height() + hsbExt + 1)));
            vbar->setGeometry(q->width() - vsbExt, 0, vsbExt, q->height() - hsbExt);
            vbar->setValue(yoffset);
            vbar->show();
        } else {
            vbar->hide();
        }

        if (showh) {
            hbar->setSingleStep(qMax(q->width() / 12, 30));
            hbar->setPageStep(q->width() - vsbExt);
            hbar->setMinimum(qMin(0, xoffset + qMin(0, r.left())));
            hbar->setMaximum(qMax(0, xoffset + qMax(0, r.right() - q->width() + vsbExt  + 1)));
            hbar->setGeometry(0, q->height() - hsbExt, q->width() - vsbExt, hsbExt);
            hbar->setValue(xoffset);
            hbar->show();
        } else {
            hbar->hide();
        }

        if (showh && showv) {
            corner->setGeometry(q->width() - vsbExt, q->height() - hsbExt, vsbExt, hsbExt);
            corner->show();
        } else {
            corner->hide();
        }

        vbar->blockSignals(false);
        hbar->blockSignals(false);

        cr.setRect(0, 0, q->width() - vsbExt, q->height() - hsbExt);
    }

    QList<QWidget *>::Iterator ii(icons.begin());
    while (ii != icons.end()) {
        QWidget* w = *ii;
        ++ii;
        int x = w->x();
        int y = w->y();
        bool m = false;
        if (x+w->width() > cr.width()) {
            m = true;
            x =  cr.width() - w->width();
        }
        if (y+w->height() >  cr.height()) {
            y =  cr.height() - w->height();
            m = true;
        }
        if (m) {
            if (QWorkspaceChild *child = qobject_cast<QWorkspaceChild*>(w))
                child->move(x, y);
            else
                w->move(x, y);
        }
    }

    return cr;

}

void QWorkspacePrivate::_q_scrollBarChanged()
{
    int ver = yoffset - vbar->value();
    int hor = xoffset - hbar->value();
    yoffset = vbar->value();
    xoffset = hbar->value();

    QList<QWorkspaceChild *>::Iterator it(windows.begin());
    while (it != windows.end()) {
        QWorkspaceChild *child = *it;
        ++it;
        // we do not use move() due to the reimplementation in QWorkspaceChild
        child->setGeometry(child->x() + hor, child->y() + ver, child->width(), child->height());
    }
    updateWorkspace();
}

/*!
    \enum QWorkspace::WindowOrder

    Specifies the order in which child windows are returned from windowList().

    \value CreationOrder The windows are returned in the order of their creation
    \value StackingOrder The windows are returned in the order of their stacking
*/

/*!\reimp */
void QWorkspace::changeEvent(QEvent *ev)
{
    Q_D(QWorkspace);
    if(ev->type() == QEvent::StyleChange) {
        if (isVisible() && d->maxWindow && d->maxmenubar) {
            if(style()->styleHint(QStyle::SH_Workspace_FillSpaceOnMaximize, 0, this)) {
                d->hideMaximizeControls(); //hide any visible maximized controls
                d->showMaximizeControls(); //updates the modification state as well
            }
        }
    }
    QWidget::changeEvent(ev);
}

QT_END_NAMESPACE

#include "moc_qworkspace.cpp"

#include "qworkspace.moc"

#endif // QT_NO_WORKSPACE
