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

#include "qsplashscreen.h"

#ifndef QT_NO_SPLASHSCREEN

#include "qapplication.h"
#include "qdesktopwidget.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qtextdocument.h"
#include "qtextcursor.h"
#include <QtCore/qdebug.h>
#include <private/qwidget_p.h>

QT_BEGIN_NAMESPACE

class QSplashScreenPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QSplashScreen)
public:
    QPixmap pixmap;
    QString currStatus;
    QColor currColor;
    int currAlign;

    inline QSplashScreenPrivate();
};

/*!
   \class QSplashScreen
   \brief The QSplashScreen widget provides a splash screen that can
   be shown during application startup.

   A splash screen is a widget that is usually displayed when an
   application is being started. Splash screens are often used for
   applications that have long start up times (e.g. database or
   networking applications that take time to establish connections) to
   provide the user with feedback that the application is loading.

   The splash screen appears in the center of the screen. It may be
   useful to add the Qt::WindowStaysOnTopHint to the splash widget's
   window flags if you want to keep it above all the other windows on
   the desktop.

   Some X11 window managers do not support the "stays on top" flag. A
   solution is to set up a timer that periodically calls raise() on
   the splash screen to simulate the "stays on top" effect.

   The most common usage is to show a splash screen before the main
   widget is displayed on the screen. This is illustrated in the
   following code snippet in which a splash screen is displayed and
   some initialization tasks are performed before the application's
   main window is shown:

   \snippet doc/src/snippets/qsplashscreen/main.cpp 0
   \dots
   \snippet doc/src/snippets/qsplashscreen/main.cpp 1

   The user can hide the splash screen by clicking on it with the
   mouse. Since the splash screen is typically displayed before the
   event loop has started running, it is necessary to periodically
   call QApplication::processEvents() to receive the mouse clicks.

   It is sometimes useful to update the splash screen with messages,
   for example, announcing connections established or modules loaded
   as the application starts up:

   \snippet doc/src/snippets/code/src_gui_widgets_qsplashscreen.cpp 0

   QSplashScreen supports this with the showMessage() function. If you
   wish to do your own drawing you can get a pointer to the pixmap
   used in the splash screen with pixmap().  Alternatively, you can
   subclass QSplashScreen and reimplement drawContents().
*/

/*!
    Construct a splash screen that will display the \a pixmap.

    There should be no need to set the widget flags, \a f, except
    perhaps Qt::WindowStaysOnTopHint.
*/
QSplashScreen::QSplashScreen(const QPixmap &pixmap, Qt::WindowFlags f)
    : QWidget(*(new QSplashScreenPrivate()), 0, Qt::SplashScreen | Qt::FramelessWindowHint | f)
{
    setPixmap(pixmap);  // Does an implicit repaint
}

/*!
    \overload

    This function allows you to specify a parent for your splashscreen. The
    typical use for this constructor is if you have a multiple screens and
    prefer to have the splash screen on a different screen than your primary
    one. In that case pass the proper desktop() as the \a parent.
*/
QSplashScreen::QSplashScreen(QWidget *parent, const QPixmap &pixmap, Qt::WindowFlags f)
    : QWidget(*new QSplashScreenPrivate, parent, Qt::SplashScreen | f)
{
    d_func()->pixmap = pixmap;
    setPixmap(d_func()->pixmap);  // Does an implicit repaint
}

/*!
  Destructor.
*/
QSplashScreen::~QSplashScreen()
{
}

/*!
    \reimp
*/
void QSplashScreen::mousePressEvent(QMouseEvent *)
{
    hide();
}

/*!
    This overrides QWidget::repaint(). It differs from the standard
    repaint function in that it also calls QApplication::flush() to
    ensure the updates are displayed, even when there is no event loop
    present.
*/
void QSplashScreen::repaint()
{
    QWidget::repaint();
    QApplication::flush();
}

/*!
    \fn QSplashScreen::messageChanged(const QString &message)

    This signal is emitted when the message on the splash screen
    changes. \a message is the new message and is a null-string
    when the message has been removed.

    \sa showMessage(), clearMessage()
*/



/*!
    Draws the \a message text onto the splash screen with color \a
    color and aligns the text according to the flags in \a alignment.

    To make sure the splash screen is repainted immediately, you can
    call \l{QCoreApplication}'s
    \l{QCoreApplication::}{processEvents()} after the call to
    showMessage(). You usually want this to make sure that the message
    is kept up to date with what your application is doing (e.g.,
    loading files).

    \sa Qt::Alignment, clearMessage()
*/
void QSplashScreen::showMessage(const QString &message, int alignment,
                                const QColor &color)
{
    Q_D(QSplashScreen);
    d->currStatus = message;
    d->currAlign = alignment;
    d->currColor = color;
    emit messageChanged(d->currStatus);
    repaint();
}

/*!
    Removes the message being displayed on the splash screen

    \sa showMessage()
 */
void QSplashScreen::clearMessage()
{
    d_func()->currStatus.clear();
    emit messageChanged(d_func()->currStatus);
    repaint();
}

/*!
    Makes the splash screen wait until the widget \a mainWin is displayed
    before calling close() on itself.
*/
void QSplashScreen::finish(QWidget *mainWin)
{
    if (mainWin) {
#if defined(Q_WS_X11)
        extern void qt_x11_wait_for_window_manager(QWidget *mainWin, bool);
        qt_x11_wait_for_window_manager(mainWin, false);
#endif
    }
    close();
}

/*!
    Sets the pixmap that will be used as the splash screen's image to
    \a pixmap.
*/
void QSplashScreen::setPixmap(const QPixmap &pixmap)
{
    Q_D(QSplashScreen);

    d->pixmap = pixmap;
    setAttribute(Qt::WA_TranslucentBackground, pixmap.hasAlpha());

    QRect r(QPoint(), d->pixmap.size());
    resize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());
    if (isVisible())
        repaint();
}

/*!
    Returns the pixmap that is used in the splash screen. The image
    does not have any of the text drawn by showMessage() calls.
*/
const QPixmap QSplashScreen::pixmap() const
{
    return d_func()->pixmap;
}

/*!
    \internal
*/
inline QSplashScreenPrivate::QSplashScreenPrivate() : currAlign(Qt::AlignLeft)
{
}

/*!
    Draw the contents of the splash screen using painter \a painter.
    The default implementation draws the message passed by showMessage().
    Reimplement this function if you want to do your own drawing on
    the splash screen.
*/
void QSplashScreen::drawContents(QPainter *painter)
{
    Q_D(QSplashScreen);
    painter->setPen(d->currColor);
    QRect r = rect().adjusted(5, 5, -5, -5);
    if (Qt::mightBeRichText(d->currStatus)) {
        QTextDocument doc;
#ifdef QT_NO_TEXTHTMLPARSER
        doc.setPlainText(d->currStatus);
#else
        doc.setHtml(d->currStatus);
#endif
        doc.setTextWidth(r.width());
        QTextCursor cursor(&doc);
        cursor.select(QTextCursor::Document);
        QTextBlockFormat fmt;
        fmt.setAlignment(Qt::Alignment(d->currAlign));
        cursor.mergeBlockFormat(fmt);
        painter->save();
        painter->translate(r.topLeft());
        doc.drawContents(painter);
        painter->restore();
    } else {
        painter->drawText(r, d->currAlign, d->currStatus);
    }
}

/*!
    \fn void QSplashScreen::message(const QString &message, int alignment,
                                    const QColor &color)
    \compat

    Use showMessage() instead.
*/

/*!
    \fn void QSplashScreen::clear()
    \compat

    Use clearMessage() instead.
*/

/*! \reimp */
bool QSplashScreen::event(QEvent *e)
{
    if (e->type() == QEvent::Paint) {
        Q_D(QSplashScreen);
        QPainter painter(this);
        if (!d->pixmap.isNull())
            painter.drawPixmap(QPoint(), d->pixmap);
        drawContents(&painter);
    }
    return QWidget::event(e);
}

QT_END_NAMESPACE

#endif //QT_NO_SPLASHSCREEN
