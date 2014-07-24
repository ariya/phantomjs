/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qcolordialog.h"
#include "qfontdialog.h"
#include "qfiledialog.h"

#include "qevent.h"
#include "qdesktopwidget.h"
#include "qpushbutton.h"
#include "qapplication.h"
#include "qlayout.h"
#include "qsizegrip.h"
#include "qwhatsthis.h"
#include "qmenu.h"
#include "qcursor.h"
#include "qmessagebox.h"
#include "qerrormessage.h"
#include <qpa/qplatformtheme.h>
#include "private/qdialog_p.h"
#include "private/qguiapplication_p.h"
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif

QT_BEGIN_NAMESPACE

static inline int themeDialogType(const QDialog *dialog)
{
#ifndef QT_NO_FILEDIALOG
    if (qobject_cast<const QFileDialog *>(dialog))
        return QPlatformTheme::FileDialog;
#endif
#ifndef QT_NO_COLORDIALOG
    if (qobject_cast<const QColorDialog *>(dialog))
        return QPlatformTheme::ColorDialog;
#endif
#ifndef QT_NO_FONTDIALOG
    if (qobject_cast<const QFontDialog *>(dialog))
        return QPlatformTheme::FontDialog;
#endif
#ifndef QT_NO_MESSAGEBOX
    if (qobject_cast<const QMessageBox *>(dialog))
        return QPlatformTheme::MessageDialog;
#endif
#ifndef QT_NO_ERRORMESSAGE
    if (qobject_cast<const QErrorMessage *>(dialog))
        return QPlatformTheme::MessageDialog;
#endif
    return -1;
}

QPlatformDialogHelper *QDialogPrivate::platformHelper() const
{
    // Delayed creation of the platform, ensuring that
    // that qobject_cast<> on the dialog works in the plugin.
    if (!m_platformHelperCreated) {
        m_platformHelperCreated = true;
        QDialogPrivate *ncThis = const_cast<QDialogPrivate *>(this);
        QDialog *dialog = ncThis->q_func();
        const int type = themeDialogType(dialog);
        if (type >= 0) {
            m_platformHelper = QGuiApplicationPrivate::platformTheme()
                    ->createPlatformDialogHelper(static_cast<QPlatformTheme::DialogType>(type));
            if (m_platformHelper) {
                QObject::connect(m_platformHelper, SIGNAL(accept()), dialog, SLOT(accept()));
                QObject::connect(m_platformHelper, SIGNAL(reject()), dialog, SLOT(reject()));
                ncThis->initHelper(m_platformHelper);
            }
        }
    }
    return m_platformHelper;
}

bool QDialogPrivate::canBeNativeDialog() const
{
    QDialogPrivate *ncThis = const_cast<QDialogPrivate *>(this);
    QDialog *dialog = ncThis->q_func();
    const int type = themeDialogType(dialog);
    if (type >= 0)
        return QGuiApplicationPrivate::platformTheme()
                ->usePlatformNativeDialog(static_cast<QPlatformTheme::DialogType>(type));
    return false;
}

QWindow *QDialogPrivate::parentWindow() const
{
    if (const QWidget *parent = q_func()->nativeParentWidget())
        return parent->windowHandle();
    return 0;
}

bool QDialogPrivate::setNativeDialogVisible(bool visible)
{
    if (QPlatformDialogHelper *helper = platformHelper()) {
        if (visible) {
            Q_Q(QDialog);
            helperPrepareShow(helper);
            nativeDialogInUse = helper->show(q->windowFlags(), q->windowModality(), parentWindow());
        } else {
            helper->hide();
        }
    }
    return nativeDialogInUse;
}

QVariant QDialogPrivate::styleHint(QPlatformDialogHelper::StyleHint hint) const
{
    if (const QPlatformDialogHelper *helper = platformHelper())
        return helper->styleHint(hint);
    return QPlatformDialogHelper::defaultStyleHint(hint);
}

void QDialogPrivate::deletePlatformHelper()
{
    delete m_platformHelper;
    m_platformHelper = 0;
    m_platformHelperCreated = false;
    nativeDialogInUse = false;
}

/*!
    \class QDialog
    \brief The QDialog class is the base class of dialog windows.

    \ingroup dialog-classes
    \ingroup abstractwidgets
    \inmodule QtWidgets

    A dialog window is a top-level window mostly used for short-term
    tasks and brief communications with the user. QDialogs may be
    modal or modeless. QDialogs can
    provide a \l{#return}{return value}, and they can have \l{#default}{default buttons}. QDialogs can also have a QSizeGrip in their
    lower-right corner, using setSizeGripEnabled().

    Note that QDialog (and any other widget that has type \c Qt::Dialog) uses
    the parent widget slightly differently from other classes in Qt. A dialog is
    always a top-level widget, but if it has a parent, its default location is
    centered on top of the parent's top-level widget (if it is not top-level
    itself). It will also share the parent's taskbar entry.

    Use the overload of the QWidget::setParent() function to change
    the ownership of a QDialog widget. This function allows you to
    explicitly set the window flags of the reparented widget; using
    the overloaded function will clear the window flags specifying the
    window-system properties for the widget (in particular it will
    reset the Qt::Dialog flag).

    \section1 Modal Dialogs

    A \b{modal} dialog is a dialog that blocks input to other
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
    provide a useful \l{#return}{return value}. Typically,
    to get the dialog to close and return the appropriate value, we
    connect a default button, e.g. \uicontrol OK, to the accept() slot and a
    \uicontrol Cancel button to the reject() slot.
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

    A \b{modeless} dialog is a dialog that operates
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
    will be called. This will cause the window to close: The \l{QCloseEvent}{close event} cannot be \l{QCloseEvent::ignore()}{ignored}.

    \section1 Extensibility

    Extensibility is the ability to show the dialog in two ways: a
    partial dialog that shows the most commonly used options, and a
    full dialog that shows all the options. Typically an extensible
    dialog will initially appear as a partial dialog, but with a
    \uicontrol More toggle button. If the user presses the \uicontrol More button down,
    the dialog is expanded. The \l{Extension Example} shows how to achieve
    extensible dialogs using Qt.

    \target return
    \section1 Return Value (Modal Dialogs)

    Modal dialogs are often used in situations where a return value is
    required, e.g. to indicate whether the user pressed \uicontrol OK or
    \uicontrol Cancel. A dialog can be closed by calling the accept() or the
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

    \snippet dialogs/dialogs.cpp 1

    A modeless dialog:

    \snippet dialogs/dialogs.cpp 0

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
}

/*!
  \overload
  \internal
*/
QDialog::QDialog(QDialogPrivate &dd, QWidget *parent, Qt::WindowFlags f)
    : QWidget(dd, parent, f | ((f & Qt::WindowType_Mask) == 0 ? Qt::Dialog : Qt::WindowType(0)))
{
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
#ifdef Q_OS_MAC
        Q_ASSERT(resetModalityTo != Qt::WindowModal);
        q->setParent(q->parentWidget(), Qt::Dialog);
#endif
    }
    resetModalityTo = -1;
}

#if defined(Q_OS_WINCE)
#ifdef Q_OS_WINCE_WM
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
#ifdef Q_OS_WINCE
    if (e->type() == QEvent::OkRequest) {
        accept();
        result = true;
     }
#endif
    return result;
}
#endif

/*!
  In general returns the modal dialog's result code, \c Accepted or
  \c Rejected.

  \note When called on a QMessageBox instance, the returned value is a
  value of the \l QMessageBox::StandardButton enum.

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
#ifdef Q_OS_MAC
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

    show();

    QPointer<QDialog> guard = this;
    if (d->nativeDialogInUse) {
        d->platformHelper()->exec();
    } else {
        QEventLoop eventLoop;
        d->eventLoop = &eventLoop;
        (void) eventLoop.exec(QEventLoop::DialogExec);
    }
    if (guard.isNull())
        return QDialog::Rejected;
    d->eventLoop = 0;

    setAttribute(Qt::WA_ShowModal, wasShowModal);

    int res = result();
    if (d->nativeDialogInUse)
        d->helperDone(static_cast<QDialog::DialogCode>(res), d->platformHelper());
    if (deleteOnClose)
        delete this;
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

  \sa reject(), done()
*/

void QDialog::accept()
{
    done(Accepted);
}

/*!
  Hides the modal dialog and sets the result code to \c Rejected.

  \sa accept(), done()
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
        QPointer<QMenu> p = new QMenu(this);
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
#ifdef Q_OS_MAC
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
    if (!testAttribute(Qt::WA_DontShowOnScreen) && d->canBeNativeDialog() && d->setNativeDialogVisible(visible))
        return;

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
        QAccessibleEvent event(this, QAccessible::DialogStart);
        QAccessible::updateAccessibility(&event);
#endif

    } else {
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && testAttribute(Qt::WA_WState_Hidden))
            return;

#ifndef QT_NO_ACCESSIBILITY
        if (isVisible()) {
            QAccessibleEvent event(this, QAccessible::DialogEnd);
            QAccessible::updateAccessibility(&event);
        }
#endif

        // Reimplemented to exit a modal event loop when the dialog is hidden.
        QWidget::setVisible(visible);
        if (d->eventLoop)
            d->eventLoop->exit();
    }

    const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme();
    if (d->mainDef && isActiveWindow()
        && theme->themeHint(QPlatformTheme::DialogSnapToDefaultButton).toBool())
        QCursor::setPos(d->mainDef->mapToGlobal(d->mainDef->rect().center()));
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

    if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        if (theme->themeHint(QPlatformTheme::WindowAutoPlacement).toBool())
            return;
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
        // Use pos() if the widget is embedded into a native window
        QPoint pp;
        if (w->windowHandle() && w->windowHandle()->property("_q_embedded_native_parent_handle").value<WId>())
            pp = w->pos();
        else
            pp = w->mapToGlobal(QPoint(0,0));
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
    return QWidget::sizeHint();
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

    By default, this property is \c false and show() pops up the dialog
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
