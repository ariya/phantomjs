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

#include "qwhatsthis.h"
#ifndef QT_NO_WHATSTHIS
#include "qpointer.h"
#include "qapplication.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qpixmap.h"
#include "qpainter.h"
#include "qtimer.h"
#include "qhash.h"
#include "qaction.h"
#include "qcursor.h"
#include "qbitmap.h"
#include "qtextdocument.h"
#include "../text/qtextdocumentlayout_p.h"
#include "qtoolbutton.h"
#include "qdebug.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#if defined(Q_WS_WIN)
#include "qt_windows.h"
#ifndef SPI_GETDROPSHADOW
#define SPI_GETDROPSHADOW                   0x1024
#endif
#endif
#if defined(Q_WS_X11)
#include "qx11info_x11.h"
#include <qwidget.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QWhatsThis
    \brief The QWhatsThis class provides a simple description of any
    widget, i.e. answering the question "What's This?".

    \ingroup helpsystem


    "What's This?" help is part of an application's online help
    system, and provides users with information about the
    functionality and usage of a particular widget. "What's This?"
    help texts are typically longer and more detailed than \link
    QToolTip tooltips\endlink, but generally provide less information
    than that supplied by separate help windows.

    QWhatsThis provides a single window with an explanatory text that
    pops up when the user asks "What's This?". The default way for
    users to ask the question is to move the focus to the relevant
    widget and press Shift+F1. The help text appears immediately; it
    goes away as soon as the user does something else.
    (Note that if there is a shortcut for Shift+F1, this mechanism
    will not work.) Some dialogs provide a "?" button that users can
    click to enter "What's This?" mode; they then click the relevant
    widget to pop up the "What's This?" window. It is also possible to
    provide a a menu option or toolbar button to switch into "What's
    This?" mode.

    To add "What's This?" text to a widget or an action, you simply
    call QWidget::setWhatsThis() or QAction::setWhatsThis().

    The text can be either rich text or plain text. If you specify a
    rich text formatted string, it will be rendered using the default
    stylesheet, making it possible to embed images in the displayed
    text. To be as fast as possible, the default stylesheet uses a
    simple method to determine whether the text can be rendered as
    plain text. See Qt::mightBeRichText() for details.

    \snippet doc/src/snippets/whatsthis/whatsthis.cpp 0

    An alternative way to enter "What's This?" mode is to call
    createAction(), and add the returned QAction to either a menu or
    a tool bar. By invoking this context help action (in the picture
    below, the button with the arrow and question mark icon) the user
    switches into "What's This?" mode. If they now click on a widget
    the appropriate help text is shown. The mode is left when help is
    given or when the user presses Esc.

    \img whatsthis.png

    You can enter "What's This?" mode programmatically with
    enterWhatsThisMode(), check the mode with inWhatsThisMode(), and
    return to normal mode with leaveWhatsThisMode().

    If you want to control the "What's This?" behavior of a widget
    manually see Qt::WA_CustomWhatsThis.

    It is also possible to show different help texts for different
    regions of a widget, by using a QHelpEvent of type
    QEvent::WhatsThis. Intercept the help event in your widget's
    QWidget::event() function and call QWhatsThis::showText() with the
    text you want to display for the position specified in
    QHelpEvent::pos(). If the text is rich text and the user clicks
    on a link, the widget also receives a QWhatsThisClickedEvent with
    the link's reference as QWhatsThisClickedEvent::href(). If a
    QWhatsThisClickedEvent is handled (i.e. QWidget::event() returns
    true), the help window remains visible. Call
    QWhatsThis::hideText() to hide it explicitly.

    \sa QToolTip
*/

Q_CORE_EXPORT void qDeleteInEventHandler(QObject *o);

class QWhatsThat : public QWidget
{
    Q_OBJECT

public:
    QWhatsThat(const QString& txt, QWidget* parent, QWidget *showTextFor);
    ~QWhatsThat() ;

    static QWhatsThat *instance;

protected:
    void showEvent(QShowEvent *e);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void keyPressEvent(QKeyEvent*);
    void paintEvent(QPaintEvent*);

private:
    QPointer<QWidget>widget;
    bool pressed;
    QString text;
    QTextDocument* doc;
    QString anchor;
    QPixmap background;
};

QWhatsThat *QWhatsThat::instance = 0;

// shadowWidth not const, for XP drop-shadow-fu turns it to 0
static int shadowWidth = 6;   // also used as '5' and '6' and even '8' below
static const int vMargin = 8;
static const int hMargin = 12;

QWhatsThat::QWhatsThat(const QString& txt, QWidget* parent, QWidget *showTextFor)
    : QWidget(parent, Qt::Popup),
      widget(showTextFor), pressed(false), text(txt)
{
    delete instance;
    instance = this;
    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    if (parent)
        setPalette(parent->palette());
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
#ifndef QT_NO_CURSOR
    setCursor(Qt::ArrowCursor);
#endif
    QRect r;
    doc = 0;
    ensurePolished(); // Ensures style sheet font before size calc
    if (Qt::mightBeRichText(text)) {
        doc = new QTextDocument();
        doc->setUndoRedoEnabled(false);
        doc->setDefaultFont(QApplication::font(this));
#ifdef QT_NO_TEXTHTMLPARSER
        doc->setPlainText(text);
#else
        doc->setHtml(text);
#endif
        doc->setUndoRedoEnabled(false);
        doc->adjustSize();
        r.setTop(0);
        r.setLeft(0);
        r.setSize(doc->size().toSize());
    }
    else
    {
        int sw = QApplication::desktop()->width() / 3;
        if (sw < 200)
            sw = 200;
        else if (sw > 300)
            sw = 300;

        r = fontMetrics().boundingRect(0, 0, sw, 1000,
                                        Qt::AlignLeft + Qt::AlignTop
                                        + Qt::TextWordWrap + Qt::TextExpandTabs,
                                        text);
    }
#if defined(Q_WS_WIN)
    if ((QSysInfo::WindowsVersion >= QSysInfo::WV_XP
        && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)))
    {
        BOOL shadow;
        SystemParametersInfo(SPI_GETDROPSHADOW, 0, &shadow, 0);
        shadowWidth = shadow ? 0 : 6;
    }
#endif
    resize(r.width() + 2*hMargin + shadowWidth, r.height() + 2*vMargin + shadowWidth);
}

QWhatsThat::~QWhatsThat()
{
    instance = 0;
    if (doc)
        delete doc;
}

void QWhatsThat::showEvent(QShowEvent *)
{
    background = QPixmap::grabWindow(QApplication::desktop()->internalWinId(),
                                     x(), y(), width(), height());
}

void QWhatsThat::mousePressEvent(QMouseEvent* e)
{
    pressed = true;
    if (e->button() == Qt::LeftButton && rect().contains(e->pos())) {
        if (doc)
            anchor = doc->documentLayout()->anchorAt(e->pos() -  QPoint(hMargin, vMargin));
        return;
    }
    close();
}

void QWhatsThat::mouseReleaseEvent(QMouseEvent* e)
{
    if (!pressed)
        return;
    if (widget && e->button() == Qt::LeftButton && doc && rect().contains(e->pos())) {
        QString a = doc->documentLayout()->anchorAt(e->pos() -  QPoint(hMargin, vMargin));
        QString href;
        if (anchor == a)
            href = a;
        anchor.clear();
        if (!href.isEmpty()) {
            QWhatsThisClickedEvent e(href);
            if (QApplication::sendEvent(widget, &e))
                return;
        }
    }
    close();
}

void QWhatsThat::mouseMoveEvent(QMouseEvent* e)
{
#ifdef QT_NO_CURSOR
    Q_UNUSED(e);
#else
    if (!doc)
        return;
    QString a = doc->documentLayout()->anchorAt(e->pos() -  QPoint(hMargin, vMargin));
    if (!a.isEmpty())
        setCursor(Qt::PointingHandCursor);
    else
        setCursor(Qt::ArrowCursor);
#endif
}

void QWhatsThat::keyPressEvent(QKeyEvent*)
{
    close();
}

void QWhatsThat::paintEvent(QPaintEvent*)
{
    bool drawShadow = true;
#if defined(Q_WS_WIN)
    if ((QSysInfo::WindowsVersion >= QSysInfo::WV_XP
        && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)))
    {
        BOOL shadow;
        SystemParametersInfo(SPI_GETDROPSHADOW, 0, &shadow, 0);
        drawShadow = !shadow;
    }
#elif defined(Q_WS_MAC) || defined(Q_WS_QWS)
    drawShadow = false; // never draw it on OS X or QWS, as we get it for free
#endif

    QRect r = rect();
    r.adjust(0, 0, -1, -1);
    if (drawShadow)
        r.adjust(0, 0, -shadowWidth, -shadowWidth);
    QPainter p(this);
    p.drawPixmap(0, 0, background);
    p.setPen(QPen(palette().toolTipText(), 0));
    p.setBrush(palette().toolTipBase());
    p.drawRect(r);
    int w = r.width();
    int h = r.height();
    p.setPen(palette().brush(QPalette::Dark).color());
    p.drawRect(1, 1, w-2, h-2);
    if (drawShadow) {
        p.setPen(palette().shadow().color());
        p.drawPoint(w + 5, 6);
        p.drawLine(w + 3, 6, w + 5, 8);
        p.drawLine(w + 1, 6, w + 5, 10);
        int i;
        for(i=7; i < h; i += 2)
            p.drawLine(w, i, w + 5, i + 5);
        for(i = w - i + h; i > 6; i -= 2)
            p.drawLine(i, h, i + 5, h + 5);
        for(; i > 0 ; i -= 2)
            p.drawLine(6, h + 6 - i, i + 5, h + 5);
    }
    r.adjust(0, 0, 1, 1);
    p.setPen(palette().toolTipText().color());
    r.adjust(hMargin, vMargin, -hMargin, -vMargin);

    if (doc) {
        p.translate(r.x(), r.y());
        QRect rect = r;
        rect.translate(-r.x(), -r.y());
        p.setClipRect(rect);
        QAbstractTextDocumentLayout::PaintContext context;
        context.palette.setBrush(QPalette::Text, context.palette.toolTipText());
        doc->documentLayout()->draw(&p, context);
    }
    else
    {
        p.drawText(r, Qt::AlignLeft + Qt::AlignTop + Qt::TextWordWrap + Qt::TextExpandTabs, text);
    }
}

static const char * const button_image[] = {
"16 16 3 1",
"         c None",
"o        c #000000",
"a        c #000080",
"o        aaaaa  ",
"oo      aaa aaa ",
"ooo    aaa   aaa",
"oooo   aa     aa",
"ooooo  aa     aa",
"oooooo  a    aaa",
"ooooooo     aaa ",
"oooooooo   aaa  ",
"ooooooooo aaa   ",
"ooooo     aaa   ",
"oo ooo          ",
"o  ooo    aaa   ",
"    ooo   aaa   ",
"    ooo         ",
"     ooo        ",
"     ooo        "};

class QWhatsThisPrivate : public QObject
{
 public:
    QWhatsThisPrivate();
    ~QWhatsThisPrivate();
    static QWhatsThisPrivate *instance;
    bool eventFilter(QObject *, QEvent *);
    QPointer<QAction> action;
#ifdef QT3_SUPPORT
    QPointer<QToolButton> button;
#endif
    static void say(QWidget *, const QString &, int x = 0, int y = 0);
    static void notifyToplevels(QEvent *e);
    bool leaveOnMouseRelease;
};

void QWhatsThisPrivate::notifyToplevels(QEvent *e)
{
    QWidgetList toplevels = QApplication::topLevelWidgets();
    for (int i = 0; i < toplevels.count(); ++i) {
        register QWidget *w = toplevels.at(i);
        QApplication::sendEvent(w, e);
    }
}

QWhatsThisPrivate *QWhatsThisPrivate::instance = 0;

QWhatsThisPrivate::QWhatsThisPrivate()
    : leaveOnMouseRelease(false)
{
    instance = this;
    qApp->installEventFilter(this);

    QPoint pos = QCursor::pos();
    if (QWidget *w = QApplication::widgetAt(pos)) {
        QHelpEvent e(QEvent::QueryWhatsThis, w->mapFromGlobal(pos), pos);
        bool sentEvent = QApplication::sendEvent(w, &e);
#ifdef QT_NO_CURSOR
        Q_UNUSED(sentEvent);
#else
        QApplication::setOverrideCursor((!sentEvent || !e.isAccepted())?
                                        Qt::ForbiddenCursor:Qt::WhatsThisCursor);
    } else {
        QApplication::setOverrideCursor(Qt::WhatsThisCursor);
#endif
    }
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::updateAccessibility(this, 0, QAccessible::ContextHelpStart);
#endif
}

QWhatsThisPrivate::~QWhatsThisPrivate()
{
    if (action)
        action->setChecked(false);
#ifdef QT3_SUPPORT
    if (button)
        button->setChecked(false);
#endif
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::updateAccessibility(this, 0, QAccessible::ContextHelpEnd);
#endif
    instance = 0;
}

bool QWhatsThisPrivate::eventFilter(QObject *o, QEvent *e)
{
    if (!o->isWidgetType())
        return false;
    QWidget * w = static_cast<QWidget *>(o);
    bool customWhatsThis = w->testAttribute(Qt::WA_CustomWhatsThis);
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    {
        QMouseEvent *me = static_cast<QMouseEvent*>(e);
        if (me->button() == Qt::RightButton || customWhatsThis)
            return false;
        QHelpEvent e(QEvent::WhatsThis, me->pos(), me->globalPos());
        if (!QApplication::sendEvent(w, &e) || !e.isAccepted())
            leaveOnMouseRelease = true;

    } break;

    case QEvent::MouseMove:
    {
        QMouseEvent *me = static_cast<QMouseEvent*>(e);
        QHelpEvent e(QEvent::QueryWhatsThis, me->pos(), me->globalPos());
        bool sentEvent = QApplication::sendEvent(w, &e);
#ifdef QT_NO_CURSOR
        Q_UNUSED(sentEvent);
#else
        QApplication::changeOverrideCursor((!sentEvent || !e.isAccepted())?
                                           Qt::ForbiddenCursor:Qt::WhatsThisCursor);
#endif
    }
    // fall through
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        if (leaveOnMouseRelease && e->type() == QEvent::MouseButtonRelease)
            QWhatsThis::leaveWhatsThisMode();
        if (static_cast<QMouseEvent*>(e)->button() == Qt::RightButton || customWhatsThis)
            return false; // ignore RMB release
        break;
    case QEvent::KeyPress:
    {
        QKeyEvent* kev = (QKeyEvent*)e;

        if (kev->key() == Qt::Key_Escape) {
            QWhatsThis::leaveWhatsThisMode();
            return true;
        } else if (customWhatsThis) {
            return false;
        } else if (kev->key() == Qt::Key_Menu ||
                    (kev->key() == Qt::Key_F10 &&
                      kev->modifiers() == Qt::ShiftModifier)) {
            // we don't react to these keys, they are used for context menus
            return false;
        } else if (kev->key() != Qt::Key_Shift && kev->key() != Qt::Key_Alt // not a modifier key
                   && kev->key() != Qt::Key_Control && kev->key() != Qt::Key_Meta) {
            QWhatsThis::leaveWhatsThisMode();
        }
    } break;
    default:
        return false;
    }
    return true;
}

class QWhatsThisAction: public QAction
{
    Q_OBJECT

public:
    explicit QWhatsThisAction(QObject* parent = 0);

private slots:
    void actionTriggered();
};

QWhatsThisAction::QWhatsThisAction(QObject *parent) : QAction(tr("What's This?"), parent)
{
#ifndef QT_NO_IMAGEFORMAT_XPM
    QPixmap p((const char**)button_image);
    setIcon(p);
#endif
    setCheckable(true);
    connect(this, SIGNAL(triggered()), this, SLOT(actionTriggered()));
#ifndef QT_NO_SHORTCUT
    setShortcut(Qt::ShiftModifier + Qt::Key_F1);
#endif
}

void QWhatsThisAction::actionTriggered()
{
    if (isChecked()) {
        QWhatsThis::enterWhatsThisMode();
        QWhatsThisPrivate::instance->action = this;
    }
}

QWhatsThis::QWhatsThis()
{
}

#ifdef QT3_SUPPORT
/*!
    \obsolete

    Sets the What's This text \a s for the widget \a w.

    Use QWidget::setWhatsThis() or QAction::setWhatsThis() instead.
*/
void QWhatsThis::add(QWidget *w, const QString &s)
{
    w->setWhatsThis(s);
}

/*!
    \obsolete

    Remove's the What's This text for the widget \a w.

    Use QWidget::setWhatsThis() or QAction::setWhatsThis() instead.
*/
void QWhatsThis::remove(QWidget *w)
{
    w->setWhatsThis(QString());
}

class QWhatsThisButton : public QToolButton
{
    Q_OBJECT
public:
    QWhatsThisButton(QWidget *p) : QToolButton(p) {
        setCheckable(true);
        QPixmap pix( const_cast<const char**>(button_image) );
        setIcon( pix );
        QObject::connect(this, SIGNAL(toggled(bool)), this, SLOT(whatToggled(bool)));
        setAutoRaise(true);
        setFocusPolicy(Qt::NoFocus);
    }

public slots:
    void whatToggled(bool b) {
        if (b) {
            QWhatsThis::enterWhatsThisMode();
            QWhatsThisPrivate::instance->button = this;
        }
    }
};

/*!
    Returns a new "What's This?" QToolButton with the given \a
    parent. To do this now, create your own QToolButton and a
    QWhatsThis object and call the QWhatsThis object's showText()
    function when the QToolButton is invoked.

    Use createAction() instead.
*/
QToolButton * QWhatsThis::whatsThisButton(QWidget * parent)
{
    return new QWhatsThisButton(parent);
}
#endif

/*!
    This function switches the user interface into "What's This?"
    mode. The user interface can be switched back into normal mode by
    the user (e.g. by them clicking or pressing Esc), or
    programmatically by calling leaveWhatsThisMode().

    When entering "What's This?" mode, a QEvent of type
    Qt::EnterWhatsThisMode is sent to all toplevel widgets.

    \sa inWhatsThisMode() leaveWhatsThisMode()
*/
void QWhatsThis::enterWhatsThisMode()
{
    if (QWhatsThisPrivate::instance)
        return;
    (void) new QWhatsThisPrivate;
    QEvent e(QEvent::EnterWhatsThisMode);
    QWhatsThisPrivate::notifyToplevels(&e);
 }

/*!
    Returns true if the user interface is in "What's This?" mode;
    otherwise returns false.

    \sa enterWhatsThisMode()
*/
bool QWhatsThis::inWhatsThisMode()
{
    return (QWhatsThisPrivate::instance != 0);
}

/*!
    If the user interface is in "What's This?" mode, this function
    switches back to normal mode; otherwise it does nothing.

    When leaving "What's This?" mode, a QEvent of type
    Qt::LeaveWhatsThisMode is sent to all toplevel widgets.

    \sa enterWhatsThisMode() inWhatsThisMode()
*/
void QWhatsThis::leaveWhatsThisMode()
{
    delete QWhatsThisPrivate::instance;
    QEvent e(QEvent::LeaveWhatsThisMode);
    QWhatsThisPrivate::notifyToplevels(&e);
}

void QWhatsThisPrivate::say(QWidget * widget, const QString &text, int x, int y)
{
    if (text.size() == 0)
        return;
    // make a fresh widget, and set it up
    QWhatsThat *whatsThat = new QWhatsThat(
        text,
#if defined(Q_WS_X11) && !defined(QT_NO_CURSOR)
        QApplication::desktop()->screen(widget ? widget->x11Info().screen() : QCursor::x11Screen()),
#else
        0,
#endif
        widget
       );


    // okay, now to find a suitable location

    int scr = (widget ?
                QApplication::desktop()->screenNumber(widget) :
#if defined(Q_WS_X11) && !defined(QT_NO_CURSOR)
                QCursor::x11Screen()
#else
                QApplication::desktop()->screenNumber(QPoint(x,y))
#endif // Q_WS_X11
               );
    QRect screen = QApplication::desktop()->screenGeometry(scr);

    int w = whatsThat->width();
    int h = whatsThat->height();
    int sx = screen.x();
    int sy = screen.y();

    // first try locating the widget immediately above/below,
    // with nice alignment if possible.
    QPoint pos;
    if (widget)
        pos = widget->mapToGlobal(QPoint(0,0));

    if (widget && w > widget->width() + 16)
        x = pos.x() + widget->width()/2 - w/2;
    else
        x = x - w/2;

        // squeeze it in if that would result in part of what's this
        // being only partially visible
    if (x + w  + shadowWidth > sx+screen.width())
        x = (widget? (qMin(screen.width(),
                           pos.x() + widget->width())
                     ) : screen.width())
            - w;

    if (x < sx)
        x = sx;

    if (widget && h > widget->height() + 16) {
        y = pos.y() + widget->height() + 2; // below, two pixels spacing
        // what's this is above or below, wherever there's most space
        if (y + h + 10 > sy+screen.height())
            y = pos.y() + 2 - shadowWidth - h; // above, overlap
    }
    y = y + 2;

        // squeeze it in if that would result in part of what's this
        // being only partially visible
    if (y + h + shadowWidth > sy+screen.height())
        y = (widget ? (qMin(screen.height(),
                             pos.y() + widget->height())
                       ) : screen.height())
            - h;
    if (y < sy)
        y = sy;

    whatsThat->move(x, y);
    whatsThat->show();
    whatsThat->grabKeyboard();
}

/*!
    Shows \a text as a "What's This?" window, at global position \a
    pos. The optional widget argument, \a w, is used to determine the
    appropriate screen on multi-head systems.

    \sa hideText()
*/
void QWhatsThis::showText(const QPoint &pos, const QString &text, QWidget *w)
{
    leaveWhatsThisMode();
    QWhatsThisPrivate::say(w, text, pos.x(), pos.y());
}

/*!
    If a "What's This?" window is showing, this destroys it.

    \sa showText()
*/
void QWhatsThis::hideText()
{
    qDeleteInEventHandler(QWhatsThat::instance);
}

/*!
    Returns a ready-made QAction, used to invoke "What's This?" context
    help, with the given \a parent.

    The returned QAction provides a convenient way to let users enter
    "What's This?" mode.
*/
QAction *QWhatsThis::createAction(QObject *parent)
{
    return new QWhatsThisAction(parent);
}

QT_END_NAMESPACE

#include "qwhatsthis.moc"

#endif // QT_NO_WHATSTHIS
