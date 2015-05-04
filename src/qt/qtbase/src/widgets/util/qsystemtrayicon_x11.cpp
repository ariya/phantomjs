/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlabel.h"
#include "qpainter.h"
#include "qpixmap.h"
#include "qbitmap.h"
#include "qevent.h"
#include "qapplication.h"
#include "qlist.h"
#include "qmenu.h"
#include "qtimer.h"
#include "qsystemtrayicon_p.h"
#include "qpaintengine.h"
#include <qwindow.h>
#include <qguiapplication.h>
#include <qscreen.h>
#include <qbackingstore.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformsystemtrayicon.h>
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>
#include <qdebug.h>

#ifndef QT_NO_SYSTEMTRAYICON
QT_BEGIN_NAMESPACE

static inline unsigned long locateSystemTray()
{
    return (unsigned long)QGuiApplication::platformNativeInterface()->nativeResourceForScreen(QByteArrayLiteral("traywindow"), QGuiApplication::primaryScreen());
}

// System tray widget. Could be replaced by a QWindow with
// a backing store if it did not need tooltip handling.
class QSystemTrayIconSys : public QWidget
{
    Q_OBJECT
public:
    explicit QSystemTrayIconSys(QSystemTrayIcon *q);

    inline void updateIcon() { update(); }
    inline QSystemTrayIcon *systemTrayIcon() const { return q; }

    QRect globalGeometry() const;

protected:
    virtual void mousePressEvent(QMouseEvent *ev);
    virtual void mouseDoubleClickEvent(QMouseEvent *ev);
    virtual bool event(QEvent *);
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void moveEvent(QMoveEvent *);

private slots:
    void systemTrayWindowChanged(QScreen *screen);

private:
    bool addToTray();

    QSystemTrayIcon *q;
};

QSystemTrayIconSys::QSystemTrayIconSys(QSystemTrayIcon *qIn)
    : QWidget(0, Qt::Window | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint)
    , q(qIn)
{
    setObjectName(QStringLiteral("QSystemTrayIconSys"));
    setToolTip(q->toolTip());
    setAttribute(Qt::WA_AlwaysShowToolTips, true);
    setAttribute(Qt::WA_QuitOnClose, false);
    const QSize size(22, 22); // Gnome, standard size
    setGeometry(QRect(QPoint(0, 0), size));
    setMinimumSize(size);

    // We need two different behaviors depending on whether the X11 visual for the system tray
    // (a) exists and (b) supports an alpha channel, i.e. is 32 bits.
    // If we have a visual that has an alpha channel, we can paint this widget with a transparent
    // background and it will work.
    // However, if there's no alpha channel visual, in order for transparent tray icons to work,
    // we do not have a transparent background on the widget, but call xcb_clear_region before
    // painting the icon
    bool hasAlphaChannel = false;
    QMetaObject::invokeMethod(QGuiApplication::platformNativeInterface(),
                              "systrayVisualHasAlphaChannel", Qt::DirectConnection,
                              Q_RETURN_ARG(bool, hasAlphaChannel));
    setAttribute(Qt::WA_TranslucentBackground, hasAlphaChannel);

    addToTray();
}

bool QSystemTrayIconSys::addToTray()
{
    if (!locateSystemTray())
        return false;

    createWinId();
    setMouseTracking(true);

    bool requestResult = false;
    if (!QMetaObject::invokeMethod(QGuiApplication::platformNativeInterface(),
                                   "requestSystemTrayWindowDock", Qt::DirectConnection,
                                   Q_RETURN_ARG(bool, requestResult),
                                   Q_ARG(const QWindow *, windowHandle()))
            || !requestResult) {
        qWarning("requestSystemTrayWindowDock failed.");
        return false;
    }
    show();
    return true;
}

void QSystemTrayIconSys::systemTrayWindowChanged(QScreen *)
{
    if (locateSystemTray()) {
        addToTray();
    } else {
        QBalloonTip::hideBalloon();
        hide(); // still no luck
        destroy();
    }
}

QRect QSystemTrayIconSys::globalGeometry() const
{
    QRect result;
    if (!QMetaObject::invokeMethod(QGuiApplication::platformNativeInterface(),
                                   "systemTrayWindowGlobalGeometry", Qt::DirectConnection,
                                   Q_RETURN_ARG(QRect, result),
                                   Q_ARG(const QWindow *, windowHandle()))
        || !result.isValid()) {
        qWarning("systemTrayWindowGlobalGeometry failed.");
    }
    return result;
}

void QSystemTrayIconSys::mousePressEvent(QMouseEvent *ev)
{
    QPoint globalPos = ev->globalPos();
#ifndef QT_NO_CONTEXTMENU
    if (ev->button() == Qt::RightButton && q->contextMenu())
        q->contextMenu()->popup(globalPos);
#endif

    if (QBalloonTip::isBalloonVisible()) {
        emit q->messageClicked();
        QBalloonTip::hideBalloon();
    }

    if (ev->button() == Qt::LeftButton)
        emit q->activated(QSystemTrayIcon::Trigger);
    else if (ev->button() == Qt::RightButton)
        emit q->activated(QSystemTrayIcon::Context);
    else if (ev->button() == Qt::MidButton)
        emit q->activated(QSystemTrayIcon::MiddleClick);
}

void QSystemTrayIconSys::mouseDoubleClickEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        emit q->activated(QSystemTrayIcon::DoubleClick);
}

bool QSystemTrayIconSys::event(QEvent *e)
{
    switch (e->type()) {
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        return QApplication::sendEvent(q, e);
#endif
    default:
        break;
    }
    return QWidget::event(e);
}

void QSystemTrayIconSys::paintEvent(QPaintEvent *)
{
    const QRect rect(QPoint(0, 0), geometry().size());
    QPainter painter(this);

    // If we have Qt::WA_TranslucentBackground set, during widget creation
    // we detected the systray visual supported an alpha channel
    if (testAttribute(Qt::WA_TranslucentBackground)) {
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        painter.fillRect(rect, Qt::transparent);
    } else {
        QMetaObject::invokeMethod(QGuiApplication::platformNativeInterface(),
                                    "clearRegion", Qt::DirectConnection,
                                    Q_ARG(const QWindow *, windowHandle()),
                                    Q_ARG(const QRect&, rect)
                                 );
    }
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    q->icon().paint(&painter, rect);
}

void QSystemTrayIconSys::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    if (QBalloonTip::isBalloonVisible())
        QBalloonTip::updateBalloonPosition(globalGeometry().center());
}

void QSystemTrayIconSys::resizeEvent(QResizeEvent *event)
{
    update();
    QWidget::resizeEvent(event);
    if (QBalloonTip::isBalloonVisible())
        QBalloonTip::updateBalloonPosition(globalGeometry().center());
}
////////////////////////////////////////////////////////////////////////////

QSystemTrayIconPrivate::QSystemTrayIconPrivate()
    : sys(0),
      qpa_sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon()),
      visible(false)
{
}

QSystemTrayIconPrivate::~QSystemTrayIconPrivate()
{
    delete qpa_sys;
}

void QSystemTrayIconPrivate::install_sys()
{
    if (qpa_sys) {
        install_sys_qpa();
        return;
    }
    Q_Q(QSystemTrayIcon);
    if (!sys && locateSystemTray()) {
        sys = new QSystemTrayIconSys(q);
        QObject::connect(QGuiApplication::platformNativeInterface(), SIGNAL(systemTrayWindowChanged(QScreen*)),
                         sys, SLOT(systemTrayWindowChanged(QScreen*)));
    }
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
    if (qpa_sys)
        return geometry_sys_qpa();
    if (!sys)
        return QRect();
    return sys->globalGeometry();
}

void QSystemTrayIconPrivate::remove_sys()
{
    if (qpa_sys) {
        remove_sys_qpa();
        return;
    }
    if (!sys)
        return;
    QBalloonTip::hideBalloon();
    sys->hide(); // this should do the trick, but...
    delete sys; // wm may resize system tray only for DestroyEvents
    sys = 0;
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
    if (qpa_sys) {
        updateIcon_sys_qpa();
        return;
    }
    if (sys)
        sys->updateIcon();
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
    if (qpa_sys)
        updateMenu_sys_qpa();
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
    if (qpa_sys) {
        updateToolTip_sys_qpa();
        return;
    }
    if (!sys)
        return;
#ifndef QT_NO_TOOLTIP
    sys->setToolTip(toolTip);
#endif
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{
    QScopedPointer<QPlatformSystemTrayIcon> sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon());
    if (sys)
        return sys->isSystemTrayAvailable();

    // no QPlatformSystemTrayIcon so fall back to default xcb platform behavior
    const QString platform = QGuiApplication::platformName();
    if (platform.compare(QLatin1String("xcb"), Qt::CaseInsensitive) == 0)
       return locateSystemTray();
    return false;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
    QScopedPointer<QPlatformSystemTrayIcon> sys(QGuiApplicationPrivate::platformTheme()->createPlatformSystemTrayIcon());
    if (sys)
        return sys->supportsMessages();

    // no QPlatformSystemTrayIcon so fall back to default xcb platform behavior
    return true;
}

void QSystemTrayIconPrivate::showMessage_sys(const QString &message, const QString &title,
                                   QSystemTrayIcon::MessageIcon icon, int msecs)
{
    if (qpa_sys) {
        showMessage_sys_qpa(message, title, icon, msecs);
        return;
    }
    if (!sys)
        return;
    QBalloonTip::showBalloon(icon, message, title, sys->systemTrayIcon(),
                             sys->globalGeometry().center(),
                             msecs);
}

QT_END_NAMESPACE

#include "qsystemtrayicon_x11.moc"

#endif //QT_NO_SYSTEMTRAYICON
