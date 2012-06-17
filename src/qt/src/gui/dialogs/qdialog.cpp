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

#include "qdialog.h"


#include "qevent.h"
#include "qdesktopwidget.h"
#include "qpushbutton.h"
#include "qapplication.h"
#include "qlayout.h"
#include "qsizegrip.h"
#include "qwhatsthis.h"
#include "qmenu.h"
#include "qcursor.h"
#include "private/qdialog_p.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#if defined(Q_WS_WINCE)
#include "qt_windows.h"
#include "qmenubar.h"
#include "qpointer.h"
#include "qguifunctions_wince.h"
extern bool qt_wince_is_mobile();     //defined in qguifunctions_wce.cpp
extern bool qt_wince_is_smartphone(); //is defined in qguifunctions_wce.cpp
#elif defined(Q_WS_X11)
#  include "../kernel/qt_x11_p.h"
#elif defined(Q_OS_SYMBIAN)
#   include "qfiledialog.h"
#   include "qfontdialog.h"
#   include "qwizard.h"
#   include "private/qt_s60_p.h"
#endif

#if defined(Q_WS_S60)
#include <AknUtils.h>               // AknLayoutUtils
#endif

#ifndef SPI_GETSNAPTODEFBUTTON
#   define SPI_GETSNAPTODEFBUTTON  95
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QDialog
    \brief The QDialog class is the base class of dialog windows.

    \ingroup dialog-classes
    \ingroup abstractwidgets


    A dialog window is a top-level window mostly used for short-term
    tasks and brief communications with the user. QDialogs may be
    modal or modeless. QDialogs can
    provide a \link #return return
    value\endlink, and they can have \link #default default
    buttons\endlink. QDialogs can also have a QSizeGrip in their
    lower-right corner, using setSizeGripEnabled().

    Note that QDialog (an any other widget that has type Qt::Dialog) uses
    the parent widget slightly differently from other classes in Qt. A
    dialog is always a top-level widget, but if it has a parent, its
    default location is centered on top of the parent's top-level widget
    (if it is not top-level itself). It will also share the parent's
    taskbar entry.

    Use the overload of the QWidget::setParent() function to change
    the ownership of a QDialog widget. This function allows you to
    explicitly set the window flags of the reparented widget; using
    the overloaded function will clear the window flags specifying the
    window-system properties for the widget (in particular it will
    reset the Qt::Dialog flag).

    \section1 Modal Dialogs

    A \bold{modal} dialog is a dialog that blocks input to other
    visible windows in the same application. Dialogs that are used to
    request a file name from the user or that are used to set
    application preferences are usually modal. Dialogs can be
    \l{Qt::ApplicationModal}{application modal} (the default) or
    \l{Qt::WindowModal}{window modal}.

    When an application modal dialog is opened, the user must finish
    interacting with the dialog and close it before they can access
    any other window in the application. Window modal dialogs only
    block access to the window associated with the dialog, allowing
    the user to continue to use other windows in an application.

    The most common way to display a modal dialog is to call its
    exec() function. When the user closes the dialog, exec() will
    provide a useful \link #return return value\endlink. Typically,
    to get the dialog to close and return the appropriate value, we
    connect a default button, e.g. \gui OK, to the accept() slot and a
    \gui Cancel button to the reject() slot.
    Alternatively you can call the done() slot with \c Accepted or
    \c Rejected.

    An alternative is to call setModal(true) or setWindowModality(),
    then show(). Unlike exec(), show() returns control to the caller
    immediately. Calling setModal(true) is especially useful for
    progress dialogs, where the user must have the ability to interact
    with the dialog, e.g.  to cancel a long running operation. If you
    use show() and setModal(true) together to perform a long operation,
    you must call QApplication::processEvents() periodically during
    processing to enable the user to interact with the dialog. (See
    QProgressDialog.)

    \section1 Modeless Dialogs

    A \bold{modeless} dialog is a dialog that operates
    independently of other windows in the same application. Find and
    replace dialogs in word-processors are often modeless to allow the
    user to interact with both the application's main window and with
    the dialog.

    Modeless dialogs are displayed using show(), which returns control
    to the caller immediately.

    If you invoke the \l{QWidget::show()}{show()} function after hiding
    a dialog, the dialog will be displayed in its original position. This is
    because the window manager decides the position for windows that
    have not been explicitly placed by the programmer. To preserve the
    position of a dialog that has been moved by the user, save its position
    in your \l{QWidget::closeEvent()}{closeEvent()}  handler and then
    move the dialog to that position, before showing it again.

    \target default
    \section1 Default Button

    A dialog's \e default button is the button that's pressed when the
    user presses Enter (Return). This button is used to signify that
    the user accepts the dialog's settings and wants to close the
    dialog. Use QPushButton::setDefault(), QPushButton::isDefault()
    and QPushButton::autoDefault() to set and control the dialog's
    default button.

    \target escapekey
    \section1 Escape Key

    If the user presses the Esc key in a dialog, QDialog::reject()
    will be called. This will cause the window to close: The \link
    QCloseEvent close event \endlink cannot be \link
    QCloseEvent::ignore() ignored \endlink.

    \section1 Extensibility

    Extensibility is the ability to show the dialog in two ways: a
    partial dialog that shows the most commonly used options, and a
    full dialog that shows all the options. Typically an extensible
    dialog will initially appear as a partial dialog, but with a
    \gui More toggle button. If the user presses the \gui More button down,
    the dialog is expanded. The \l{Extension Example} shows how to achieve
    extensible dialogs using Qt.

    \target return
    \section1 Return Value (Modal Dialogs)

    Modal dialogs are often used in situations where a return value is
    required, e.g. to indicate whether the user pressed \gui OK or
    \gui Cancel. A dialog can be closed by calling the accept() or the
    reject() slots, and exec() will return \c Accepted or \c Rejected
    as appropriate. The exec() call returns the result of the dialog.
    The result is also available from result() if the dialog has not
    been destroyed.

    In order to modify your dialog's close behavior, you can reimplement
    the functions accept(), reject() or done(). The
    \l{QWidget::closeEvent()}{closeEvent()} function should only be
    reimplemented to preserve the dialog's position or to override the
    standard close or reject behavior.

    \target examples
    \section1 Code Examples

    A modal dialog:

    \snippet doc/src/snippets/dialogs/dialogs.cpp 1

    A modeless dialog:

    \snippet doc/src/snippets/dialogs/dialogs.cpp 0

    \sa QDialogButtonBox, QTabWidget, QWidget, QProgressDialog,
        {fowler}{GUI Design Handbook: Dialogs, Standard}, {Extension Example},
        {Standard Dialogs Example}
*/

/*! \enum QDialog::DialogCode

    The value returned by a modal dialog.

    \value Accepted
    \value Rejected
*/

/*!
  \property QDialog::sizeGripEnabled
  \brief whether the size grip is enabled

  A QSizeGrip is placed in the bottom-right corner of the dialog when this
  property is enabled. By default, the size grip is disabled.
*/


/*!
  Constructs a dialog with parent \a parent.

  A dialog is always a top-level widget, but if it has a parent, its
  default location is centered on top of the parent. It will also
  share the parent's taskbar entry.

  The widget flags \a f are passed on to the QWidget constructor.
  If, for example, you don't want a What's This button in the title bar
  of the dialog, pass Qt::WindowTitleHint | Qt::WindowSystemMenuHint in \a f.

  \sa QWidget::setWindowFlags()
*/

QDialog::QDialog(QWidget *parent, Qt::WindowFlags f)
    : QWidget(*new QDialogPrivate, parent,
              f | ((f & Qt::WindowType_Mask) == 0 ? Qt::Dialog : Qt::WindowType(0)))
{
#ifdef Q_WS_WINCE
    if (!qt_wince_is_smartphone())
        setWindowFlags(windowFlags() | Qt::WindowOkButtonHint | QFlag(qt_wince_is_mobile() ? 0 : Qt::WindowCancelButtonHint));
#endif

#ifdef Q_WS_S60
    if (S60->avkonComponentsSupportTransparency) {
        bool noSystemBackground = testAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground); // also sets WA_NoSystemBackground
        setAttribute(Qt::WA_NoSystemBackground, noSystemBackground); // restore system background attribute
    }
#endif
}

#ifdef QT3_SUPPORT
/*!
    \overload
    \obsolete
*/
QDialog::QDialog(QWidget *parent, const char *name, bool modal, Qt::WindowFlags f)
    : QWidget(*new QDialogPrivate, parent,
              f
              | QFlag(modal ? Qt::WShowModal : Qt::WindowType(0))
              | QFlag((f & Qt::WindowType_Mask) == 0 ? Qt::Dialog : Qt::WindowType(0))
        )
{
    setObjectName(QString::fromAscii(name));
}
#endif

/*!
  \overload
  \internal
*/
QDialog::QDialog(QDialogPrivate &dd, QWidget *parent, Qt::WindowFlags f)
    : QWidget(dd, parent, f | ((f & Qt::WindowType_Mask) == 0 ? Qt::Dialog : Qt::WindowType(0)))
{
#ifdef Q_WS_WINCE
    if (!qt_wince_is_smartphone())
        setWindowFlags(windowFlags() | Qt::WindowOkButtonHint | QFlag(qt_wince_is_mobile() ? 0 : Qt::WindowCancelButtonHint));
#endif

#ifdef Q_WS_S60
    if (S60->avkonComponentsSupportTransparency) {
        bool noSystemBackground = testAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground); // also sets WA_NoSystemBackground
        setAttribute(Qt::WA_NoSystemBackground, noSystemBackground); // restore system background attribute
    }
#endif
}

/*!
  Destroys the QDialog, deleting all its children.
*/

QDialog::~QDialog()
{
    QT_TRY {
        // Need to hide() here, as our (to-be) overridden hide()
        // will not be called in ~QWidget.
        hide();
    } QT_CATCH(...) {
        // we're in the destructor - just swallow the exception
    }
}

/*!
  \internal
  This function is called by the push button \a pushButton when it
  becomes the default button. If \a pushButton is 0, the dialogs
  default default button becomes the default button. This is what a
  push button calls when it loses focus.
*/
void QDialogPrivate::setDefault(QPushButton *pushButton)
{
    Q_Q(QDialog);
    bool hasMain = false;
    QList<QPushButton*> list = q->findChildren<QPushButton*>();
    for (int i=0; i<list.size(); ++i) {
        QPushButton *pb = list.at(i);
        if (pb->window() == q) {
            if (pb == mainDef)
                hasMain = true;
            if (pb != pushButton)
                pb->setDefault(false);
        }
    }
    if (!pushButton && hasMain)
        mainDef->setDefault(true);
    if (!hasMain)
        mainDef = pushButton;
}

/*!
  \internal
  This function sets the default default push button to \a pushButton.
  This function is called by QPushButton::setDefault().
*/
void QDialogPrivate::setMainDefault(QPushButton *pushButton)
{
    mainDef = 0;
    setDefault(pushButton);
}

/*!
  \internal
  Hides the default button indicator. Called when non auto-default
  push button get focus.
 */
void QDialogPrivate::hideDefault()
{
    Q_Q(QDialog);
    QList<QPushButton*> list = q->findChildren<QPushButton*>();
    for (int i=0; i<list.size(); ++i) {
        list.at(i)->setDefault(false);
    }
}

void QDialogPrivate::resetModalitySetByOpen()
{
    Q_Q(QDialog);
    if (resetModalityTo != -1 && !q->testAttribute(Qt::WA_SetWindowModality)) {
        // open() changed the window modality and the user didn't touch it afterwards; restore it
        q->setWindowModality(Qt::WindowModality(resetModalityTo));
        q->setAttribute(Qt::WA_SetWindowModality, wasModalitySet);
#ifdef Q_WS_MAC
        Q_ASSERT(resetModalityTo != Qt::WindowModal);
        q->setParent(q->parentWidget(), Qt::Dialog);
#endif
    }
    resetModalityTo = -1;
}

#if defined(Q_WS_WINCE) || defined(Q_OS_SYMBIAN)
#ifdef Q_WS_WINCE_WM
void QDialogPrivate::_q_doneAction()
{
    //Done...
    QApplication::postEvent(q_func(), new QEvent(QEvent::OkRequest));
}
#endif

/*!
    \reimp
*/
bool QDialog::event(QEvent *e)
{
    bool result = QWidget::event(e);
#ifdef Q_WS_WINCE
    if (e->type() == QEvent::OkRequest) {
        accept();
        result = true;
     }
#elif defined(Q_WS_S60)
    if ((e->type() == QEvent::StyleChange) || (e->type() == QEvent::Resize )) {
        if (!testAttribute(Qt::WA_Moved)) {
            Qt::WindowStates state = windowState();
            adjustPosition(parentWidget());
            setAttribute(Qt::WA_Moved, false); // not really an explicit position
            if (state != windowState())
                setWindowState(state);
        }
    }
    // TODO is Symbian, non-S60 behaviour required?
#endif
    return result;
}
#endif

/*!
  In general returns the modal dialog's result code, \c Accepted or \c Rejected.

  \note When used from QMessageBox instance the result code type is \l QMessageBox::StandardButton

  Do not call this function if the dialog was constructed with the
  Qt::WA_DeleteOnClose attribute.
*/
int QDialog::result() const
{
    Q_D(const QDialog);
    return d->rescode;
}

/*!
  \fn void QDialog::setResult(int i)

  Sets the modal dialog's result code to \a i.

  \note We recommend that you use one of the values defined by
  QDialog::DialogCode.
*/
void QDialog::setResult(int r)
{
    Q_D(QDialog);
    d->rescode = r;
}

/*!
    \since 4.5

    Shows the dialog as a \l{QDialog#Modal Dialogs}{window modal dialog},
    returning immediately.

    \sa exec(), show(), result(), setWindowModality()
*/
void QDialog::open()
{
    Q_D(QDialog);

    Qt::WindowModality modality = windowModality();
    if (modality != Qt::WindowModal) {
        d->resetModalityTo = modality;
        d->wasModalitySet = testAttribute(Qt::WA_SetWindowModality);
        setWindowModality(Qt::WindowModal);
        setAttribute(Qt::WA_SetWindowModality, false);
#ifdef Q_WS_MAC
        setParent(parentWidget(), Qt::Sheet);
#endif
    }

    setResult(0);
    show();
}

/*!
    Shows the dialog as a \l{QDialog#Modal Dialogs}{modal dialog},
    blocking until the user closes it. The function returns a \l
    DialogCode result.

    If the dialog is \l{Qt::ApplicationModal}{application modal}, users cannot
    interact with any other window in the same application until they close
    the dialog. If the dialog is \l{Qt::ApplicationModal}{window modal}, only
    interaction with the parent window is blocked while the dialog is open.
    By default, the dialog is application modal.

    \sa open(), show(), result(), setWindowModality()
*/

int QDialog::exec()
{
    Q_D(QDialog);

    if (d->eventLoop) {
        qWarning("QDialog::exec: Recursive call detected");
        return -1;
    }

    bool deleteOnClose = testAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_DeleteOnClose, false);

    d->resetModalitySetByOpen();

    bool wasShowModal = testAttribute(Qt::WA_ShowModal);
    setAttribute(Qt::WA_ShowModal, true);
    setResult(0);

//On Windows Mobile we create an empty menu to hide the current menu
#ifdef Q_WS_WINCE_WM
#ifndef QT_NO_MENUBAR
    QMenuBar *menuBar = 0;
    if (!findChild<QMenuBar *>())
        menuBar = new QMenuBar(this);
    if (qt_wince_is_smartphone()) {
        QAction *doneAction = new QAction(tr("Done"), this);
        menuBar->setDefaultAction(doneAction);
        connect(doneAction, SIGNAL(triggered()), this, SLOT(_q_doneAction()));
    }
#endif //QT_NO_MENUBAR
#endif //Q_WS_WINCE_WM

    bool showSystemDialogFullScreen = false;
#ifdef Q_OS_SYMBIAN
    if (qobject_cast<QFileDialog *>(this) || qobject_cast<QFontDialog *>(this) ||
        qobject_cast<QWizard *>(this)) {
        showSystemDialogFullScreen = true;
    }
#endif // Q_OS_SYMBIAN

    if (showSystemDialogFullScreen) {
        setWindowFlags(windowFlags() | Qt::WindowSoftkeysVisibleHint);
        setWindowState(Qt::WindowFullScreen);
    }
    show();

#ifdef Q_WS_MAC
    d->mac_nativeDialogModalHelp();
#endif

    QEventLoop eventLoop;
    d->eventLoop = &eventLoop;
    QPointer<QDialog> guard = this;
    (void) eventLoop.exec(QEventLoop::DialogExec);
    if (guard.isNull())
        return QDialog::Rejected;
    d->eventLoop = 0;

    setAttribute(Qt::WA_ShowModal, wasShowModal);

    int res = result();
    if (deleteOnClose)
        delete this;
#ifdef Q_WS_WINCE_WM
#ifndef QT_NO_MENUBAR
    else if (menuBar)
        delete menuBar;
#endif //QT_NO_MENUBAR
#endif //Q_WS_WINCE_WM
    return res;
}


/*!
  Closes the dialog and sets its result code to \a r. If this dialog
  is shown with exec(), done() causes the local event loop to finish,
  and exec() to return \a r.

  As with QWidget::close(), done() deletes the dialog if the
  Qt::WA_DeleteOnClose flag is set. If the dialog is the application's
  main widget, the application terminates. If the dialog is the
  last window closed, the QApplication::lastWindowClosed() signal is
  emitted.

  \sa accept(), reject(), QApplication::activeWindow(), QApplication::quit()
*/

void QDialog::done(int r)
{
    Q_D(QDialog);
    hide();
    setResult(r);

    d->close_helper(QWidgetPrivate::CloseNoEvent);
    d->resetModalitySetByOpen();

    emit finished(r);
    if (r == Accepted)
        emit accepted();
    else if (r == Rejected)
        emit rejected();
}

/*!
  Hides the modal dialog and sets the result code to \c Accepted.

  \sa reject() done()
*/

void QDialog::accept()
{
    done(Accepted);
}

/*!
  Hides the modal dialog and sets the result code to \c Rejected.

  \sa accept() done()
*/

void QDialog::reject()
{
    done(Rejected);
}

/*! \reimp */
bool QDialog::eventFilter(QObject *o, QEvent *e)
{
    return QWidget::eventFilter(o, e);
}

/*****************************************************************************
  Event handlers
 *****************************************************************************/

#ifndef QT_NO_CONTEXTMENU
/*! \reimp */
void QDialog::contextMenuEvent(QContextMenuEvent *e)
{
#if defined(QT_NO_WHATSTHIS) || defined(QT_NO_MENU)
    Q_UNUSED(e);
#else
    QWidget *w = childAt(e->pos());
    if (!w) {
        w = rect().contains(e->pos()) ? this : 0;
        if (!w)
            return;
    }
    while (w && w->whatsThis().size() == 0 && !w->testAttribute(Qt::WA_CustomWhatsThis))
        w = w->isWindow() ? 0 : w->parentWidget();
    if (w) {
        QWeakPointer<QMenu> p = new QMenu(this);
        QAction *wt = p.data()->addAction(tr("What's This?"));
        if (p.data()->exec(e->globalPos()) == wt) {
            QHelpEvent e(QEvent::WhatsThis, w->rect().center(),
                         w->mapToGlobal(w->rect().center()));
            QApplication::sendEvent(w, &e);
        }
        delete p.data();
    }
#endif
}
#endif // QT_NO_CONTEXTMENU

/*! \reimp */
void QDialog::keyPressEvent(QKeyEvent *e)
{
    //   Calls reject() if Escape is pressed. Simulates a button
    //   click for the default button if Enter is pressed. Move focus
    //   for the arrow keys. Ignore the rest.
#ifdef Q_WS_MAC
    if(e->modifiers() == Qt::ControlModifier && e->key() == Qt::Key_Period) {
        reject();
    } else
#endif
    if (!e->modifiers() || (e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter)) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return: {
            QList<QPushButton*> list = findChildren<QPushButton*>();
            for (int i=0; i<list.size(); ++i) {
                QPushButton *pb = list.at(i);
                if (pb->isDefault() && pb->isVisible()) {
                    if (pb->isEnabled())
                        pb->click();
                    return;
                }
            }
        }
        break;
        case Qt::Key_Escape:
            reject();
            break;
        default:
            e->ignore();
            return;
        }
    } else {
        e->ignore();
    }
}

/*! \reimp */
void QDialog::closeEvent(QCloseEvent *e)
{
#ifndef QT_NO_WHATSTHIS
    if (isModal() && QWhatsThis::inWhatsThisMode())
        QWhatsThis::leaveWhatsThisMode();
#endif
    if (isVisible()) {
        QPointer<QObject> that = this;
        reject();
        if (that && isVisible())
            e->ignore();
    } else {
        e->accept();
    }
}

/*****************************************************************************
  Geometry management.
 *****************************************************************************/

/*! \reimp
*/

void QDialog::setVisible(bool visible)
{
    Q_D(QDialog);
    if (visible) {
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && !testAttribute(Qt::WA_WState_Hidden))
            return;

        if (!testAttribute(Qt::WA_Moved)) {
            Qt::WindowStates state = windowState();
            adjustPosition(parentWidget());
            setAttribute(Qt::WA_Moved, false); // not really an explicit position
            if (state != windowState())
                setWindowState(state);
        }
        QWidget::setVisible(visible);
        showExtension(d->doShowExtension);
        QWidget *fw = window()->focusWidget();
        if (!fw)
            fw = this;

        /*
          The following block is to handle a special case, and does not
          really follow propper logic in concern of autoDefault and TAB
          order. However, it's here to ease usage for the users. If a
          dialog has a default QPushButton, and first widget in the TAB
          order also is a QPushButton, then we give focus to the main
          default QPushButton. This simplifies code for the developers,
          and actually catches most cases... If not, then they simply
          have to use [widget*]->setFocus() themselves...
        */
        if (d->mainDef && fw->focusPolicy() == Qt::NoFocus) {
            QWidget *first = fw;
            while ((first = first->nextInFocusChain()) != fw && first->focusPolicy() == Qt::NoFocus)
                ;
            if (first != d->mainDef && qobject_cast<QPushButton*>(first))
                d->mainDef->setFocus();
        }
        if (!d->mainDef && isWindow()) {
            QWidget *w = fw;
            while ((w = w->nextInFocusChain()) != fw) {
                QPushButton *pb = qobject_cast<QPushButton *>(w);
                if (pb && pb->autoDefault() && pb->focusPolicy() != Qt::NoFocus) {
                    pb->setDefault(true);
                    break;
                }
            }
        }
        if (fw && !fw->hasFocus()) {
            QFocusEvent e(QEvent::FocusIn, Qt::TabFocusReason);
            QApplication::sendEvent(fw, &e);
        }

#ifndef QT_NO_ACCESSIBILITY
        QAccessible::updateAccessibility(this, 0, QAccessible::DialogStart);
#endif

    } else {
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && testAttribute(Qt::WA_WState_Hidden))
            return;

#ifndef QT_NO_ACCESSIBILITY
        if (isVisible())
            QAccessible::updateAccessibility(this, 0, QAccessible::DialogEnd);
#endif

        // Reimplemented to exit a modal event loop when the dialog is hidden.
        QWidget::setVisible(visible);
        if (d->eventLoop)
            d->eventLoop->exit();
    }
#ifdef Q_WS_WIN
    if (d->mainDef && isActiveWindow()) {
        BOOL snapToDefault = false;
        if (SystemParametersInfo(SPI_GETSNAPTODEFBUTTON, 0, &snapToDefault, 0)) {
            if (snapToDefault)
                QCursor::setPos(d->mainDef->mapToGlobal(d->mainDef->rect().center()));
        }
    }
#endif
}

/*!\reimp */
void QDialog::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !testAttribute(Qt::WA_Moved)) {
        Qt::WindowStates  state = windowState();
        adjustPosition(parentWidget());
        setAttribute(Qt::WA_Moved, false); // not really an explicit position
        if (state != windowState())
            setWindowState(state);
    }
}

/*! \internal */
void QDialog::adjustPosition(QWidget* w)
{
#ifdef Q_WS_X11
    // if the WM advertises that it will place the windows properly for us, let it do it :)
    if (X11->isSupportedByWM(ATOM(_NET_WM_FULL_PLACEMENT)))
        return;
#endif

#ifdef Q_OS_SYMBIAN
    if (symbianAdjustedPosition())
        //dialog has already been positioned
        return;
#endif

    QPoint p(0, 0);
    int extraw = 0, extrah = 0, scrn = 0;
    if (w)
        w = w->window();
    QRect desk;
    if (w) {
        scrn = QApplication::desktop()->screenNumber(w);
    } else if (QApplication::desktop()->isVirtualDesktop()) {
        scrn = QApplication::desktop()->screenNumber(QCursor::pos());
    } else {
        scrn = QApplication::desktop()->screenNumber(this);
    }
    desk = QApplication::desktop()->availableGeometry(scrn);

    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; (extraw == 0 || extrah == 0) && i < list.size(); ++i) {
        QWidget * current = list.at(i);
        if (current->isVisible()) {
            int framew = current->geometry().x() - current->x();
            int frameh = current->geometry().y() - current->y();

            extraw = qMax(extraw, framew);
            extrah = qMax(extrah, frameh);
        }
    }

    // sanity check for decoration frames. With embedding, we
    // might get extraordinary values
    if (extraw == 0 || extrah == 0 || extraw >= 10 || extrah >= 40) {
        extrah = 40;
        extraw = 10;
    }


    if (w) {
        // Use mapToGlobal rather than geometry() in case w might
        // be embedded in another application
        QPoint pp = w->mapToGlobal(QPoint(0,0));
        p = QPoint(pp.x() + w->width()/2,
                    pp.y() + w->height()/ 2);
    } else {
        // p = middle of the desktop
        p = QPoint(desk.x() + desk.width()/2, desk.y() + desk.height()/2);
    }

    // p = origin of this
    p = QPoint(p.x()-width()/2 - extraw,
                p.y()-height()/2 - extrah);


    if (p.x() + extraw + width() > desk.x() + desk.width())
        p.setX(desk.x() + desk.width() - width() - extraw);
    if (p.x() < desk.x())
        p.setX(desk.x());

    if (p.y() + extrah + height() > desk.y() + desk.height())
        p.setY(desk.y() + desk.height() - height() - extrah);
    if (p.y() < desk.y())
        p.setY(desk.y());

    move(p);
}

#if defined(Q_OS_SYMBIAN)
/*! \internal */
bool QDialog::symbianAdjustedPosition()
{
#if defined(Q_WS_S60)
    QPoint p;
    QPoint oldPos = pos();
    if (isFullScreen()) {
        p.setX(0);
        p.setY(0);
    } else if (isMaximized()) {
        TRect statusPaneRect = TRect();
        if (S60->screenHeightInPixels > S60->screenWidthInPixels) {
            AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EStatusPane, statusPaneRect);
        } else {
            AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EStaconTop, statusPaneRect);
            // In some native layouts, StaCon is not used. Try to fetch the status pane
            // height from StatusPane component.
            if (statusPaneRect.IsEmpty())
                AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EStatusPane, statusPaneRect);
        }

        p.setX(0);
        p.setY(statusPaneRect.Height());
    } else {
        // naive way to deduce screen orientation
        if (S60->screenHeightInPixels > S60->screenWidthInPixels) {
            int cbaHeight;
            TRect rect;
            AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EControlPane, rect);
            cbaHeight = rect.Height();
            p.setY(S60->screenHeightInPixels - height() - cbaHeight);
            p.setX(0);
        } else {
            const int scrollbarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
            TRect staConTopRect = TRect();
            AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EStaconTop, staConTopRect);
            if (staConTopRect.IsEmpty()) {
                TRect cbaRect = TRect();
                AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EControlPane, cbaRect);
                AknLayoutUtils::TAknCbaLocation cbaLocation = AknLayoutUtils::CbaLocation();
                switch (cbaLocation) {
                case AknLayoutUtils::EAknCbaLocationBottom:
                    p.setY(S60->screenHeightInPixels - height() - cbaRect.Height());
                    p.setX((S60->screenWidthInPixels - width()) >> 1);
                    break;
                case AknLayoutUtils::EAknCbaLocationRight:
                    p.setY((S60->screenHeightInPixels - height()) >> 1);
                    p.setX(qMax(0,S60->screenWidthInPixels - width() - scrollbarWidth - cbaRect.Width()));
                    break;
                case AknLayoutUtils::EAknCbaLocationLeft:
                    p.setY((S60->screenHeightInPixels - height()) >> 1);
                    p.setX(qMax(0,scrollbarWidth + cbaRect.Width()));
                    break;
                }
            } else {
                p.setY((S60->screenHeightInPixels - height()) >> 1);
                p.setX(qMax(0,S60->screenWidthInPixels - width()));
            }
        }
    }
    if (oldPos != p || p.y() < 0)
        move(p);
    return true;
#else
    // TODO - check positioning requirement for Symbian, non-s60
    return false;
#endif
}
#endif

/*!
    \obsolete

    If \a orientation is Qt::Horizontal, the extension will be displayed
    to the right of the dialog's main area. If \a orientation is
    Qt::Vertical, the extension will be displayed below the dialog's main
    area.

    Instead of using this functionality, we recommend that you simply call
    show() or hide() on the part of the dialog that you want to use as an
    extension. See the \l{Extension Example} for details.

    \sa setExtension()
*/
void QDialog::setOrientation(Qt::Orientation orientation)
{
    Q_D(QDialog);
    d->orientation = orientation;
}

/*!
    \obsolete

    Returns the dialog's extension orientation.

    Instead of using this functionality, we recommend that you simply call
    show() or hide() on the part of the dialog that you want to use as an
    extension. See the \l{Extension Example} for details.

    \sa extension()
*/
Qt::Orientation QDialog::orientation() const
{
    Q_D(const QDialog);
    return d->orientation;
}

/*!
    \obsolete

    Sets the widget, \a extension, to be the dialog's extension,
    deleting any previous extension. The dialog takes ownership of the
    extension. Note that if 0 is passed any existing extension will be
    deleted. This function must only be called while the dialog is hidden.

    Instead of using this functionality, we recommend that you simply call
    show() or hide() on the part of the dialog that you want to use as an
    extension. See the \l{Extension Example} for details.

    \sa showExtension(), setOrientation()
*/
void QDialog::setExtension(QWidget* extension)
{
    Q_D(QDialog);
    delete d->extension;
    d->extension = extension;

    if (!extension)
        return;

    if (extension->parentWidget() != this)
        extension->setParent(this);
    extension->hide();
}

/*!
    \obsolete

    Returns the dialog's extension or 0 if no extension has been
    defined.

    Instead of using this functionality, we recommend that you simply call
    show() or hide() on the part of the dialog that you want to use as an
    extension. See the \l{Extension Example} for details.

    \sa showExtension(), setOrientation()
*/
QWidget* QDialog::extension() const
{
    Q_D(const QDialog);
    return d->extension;
}


/*!
    \obsolete

    If \a showIt is true, the dialog's extension is shown; otherwise the
    extension is hidden.

    Instead of using this functionality, we recommend that you simply call
    show() or hide() on the part of the dialog that you want to use as an
    extension. See the \l{Extension Example} for details.

    \sa show(), setExtension(), setOrientation()
*/
void QDialog::showExtension(bool showIt)
{
    Q_D(QDialog);
    d->doShowExtension = showIt;
    if (!d->extension)
        return;
    if (!testAttribute(Qt::WA_WState_Visible))
        return;
    if (d->extension->isVisible() == showIt)
        return;

    if (showIt) {
        d->size = size();
        d->min = minimumSize();
        d->max = maximumSize();
        if (layout())
            layout()->setEnabled(false);
        QSize s(d->extension->sizeHint()
                 .expandedTo(d->extension->minimumSize())
                 .boundedTo(d->extension->maximumSize()));
        if (d->orientation == Qt::Horizontal) {
            int h = qMax(height(), s.height());
            d->extension->setGeometry(width(), 0, s.width(), h);
            setFixedSize(width() + s.width(), h);
        } else {
            int w = qMax(width(), s.width());
            d->extension->setGeometry(0, height(), w, s.height());
            setFixedSize(w, height() + s.height());
        }
        d->extension->show();
#ifndef QT_NO_SIZEGRIP
        const bool sizeGripEnabled = isSizeGripEnabled();
        setSizeGripEnabled(false);
        d->sizeGripEnabled = sizeGripEnabled;
#endif
    } else {
        d->extension->hide();
        // workaround for CDE window manager that won't shrink with (-1,-1)
        setMinimumSize(d->min.expandedTo(QSize(1, 1)));
        setMaximumSize(d->max);
        resize(d->size);
        if (layout())
            layout()->setEnabled(true);
#ifndef QT_NO_SIZEGRIP
        setSizeGripEnabled(d->sizeGripEnabled);
#endif
    }
}


/*! \reimp */
QSize QDialog::sizeHint() const
{
    Q_D(const QDialog);
    if (d->extension) {
        if (d->orientation == Qt::Horizontal)
            return QSize(QWidget::sizeHint().width(),
                        qMax(QWidget::sizeHint().height(),d->extension->sizeHint().height()));
        else
            return QSize(qMax(QWidget::sizeHint().width(), d->extension->sizeHint().width()),
                        QWidget::sizeHint().height());
    }
#if defined(Q_WS_S60)
    // if size is not fixed, try to adjust it according to S60 layoutting
    if (minimumSize() != maximumSize()) {
        // In S60, dialogs are always the width of screen (in portrait, regardless of current layout)
        return QSize(qMin(S60->screenHeightInPixels, S60->screenWidthInPixels), QWidget::sizeHint().height());
    } else {
        return QWidget::sizeHint();
    }
#else
    return QWidget::sizeHint();
#endif //Q_WS_S60
}


/*! \reimp */
QSize QDialog::minimumSizeHint() const
{
    Q_D(const QDialog);
    if (d->extension) {
        if (d->orientation == Qt::Horizontal)
            return QSize(QWidget::minimumSizeHint().width(),
                        qMax(QWidget::minimumSizeHint().height(), d->extension->minimumSizeHint().height()));
        else
            return QSize(qMax(QWidget::minimumSizeHint().width(), d->extension->minimumSizeHint().width()),
                        QWidget::minimumSizeHint().height());
    }

    return QWidget::minimumSizeHint();
}

/*!
    \property QDialog::modal
    \brief whether show() should pop up the dialog as modal or modeless

    By default, this property is false and show() pops up the dialog
    as modeless. Setting his property to true is equivalent to setting
    QWidget::windowModality to Qt::ApplicationModal.

    exec() ignores the value of this property and always pops up the
    dialog as modal.

    \sa QWidget::windowModality, show(), exec()
*/

void QDialog::setModal(bool modal)
{
    setAttribute(Qt::WA_ShowModal, modal);
}


bool QDialog::isSizeGripEnabled() const
{
#ifndef QT_NO_SIZEGRIP
    Q_D(const QDialog);
    return !!d->resizer;
#else
    return false;
#endif
}


void QDialog::setSizeGripEnabled(bool enabled)
{
#ifdef QT_NO_SIZEGRIP
    Q_UNUSED(enabled);
#else
    Q_D(QDialog);
#ifndef QT_NO_SIZEGRIP
    d->sizeGripEnabled = enabled;
    if (enabled && d->doShowExtension)
        return;
#endif
    if (!enabled != !d->resizer) {
        if (enabled) {
            d->resizer = new QSizeGrip(this);
            // adjustSize() processes all events, which is suboptimal
            d->resizer->resize(d->resizer->sizeHint());
            if (isRightToLeft())
                d->resizer->move(rect().bottomLeft() -d->resizer->rect().bottomLeft());
            else
                d->resizer->move(rect().bottomRight() -d->resizer->rect().bottomRight());
            d->resizer->raise();
            d->resizer->show();
        } else {
            delete d->resizer;
            d->resizer = 0;
        }
    }
#endif //QT_NO_SIZEGRIP
}



/*! \reimp */
void QDialog::resizeEvent(QResizeEvent *)
{
#ifndef QT_NO_SIZEGRIP
    Q_D(QDialog);
    if (d->resizer) {
        if (isRightToLeft())
            d->resizer->move(rect().bottomLeft() -d->resizer->rect().bottomLeft());
        else
            d->resizer->move(rect().bottomRight() -d->resizer->rect().bottomRight());
        d->resizer->raise();
    }
#endif
}

/*! \fn void QDialog::finished(int result)
    \since 4.1

    This signal is emitted when the dialog's \a result code has been
    set, either by the user or by calling done(), accept(), or
    reject().

    Note that this signal is \e not emitted when hiding the dialog
    with hide() or setVisible(false). This includes deleting the
    dialog while it is visible.

    \sa accepted(), rejected()
*/

/*! \fn void QDialog::accepted()
    \since 4.1

    This signal is emitted when the dialog has been accepted either by
    the user or by calling accept() or done() with the
    QDialog::Accepted argument.

    Note that this signal is \e not emitted when hiding the dialog
    with hide() or setVisible(false). This includes deleting the
    dialog while it is visible.

    \sa finished(), rejected()
*/

/*! \fn void QDialog::rejected()
    \since 4.1

    This signal is emitted when the dialog has been rejected either by
    the user or by calling reject() or done() with the
    QDialog::Rejected argument.

    Note that this signal is \e not emitted when hiding the dialog
    with hide() or setVisible(false). This includes deleting the
    dialog while it is visible.

    \sa finished(), accepted()
*/

QT_END_NAMESPACE
#include "moc_qdialog.cpp"
