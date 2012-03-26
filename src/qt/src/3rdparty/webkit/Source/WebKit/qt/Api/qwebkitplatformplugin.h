/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef QWEBKITPLATFORMPLUGIN_H
#define QWEBKITPLATFORMPLUGIN_H

/*
 *  Warning: The contents of this file is not  part of the public QtWebKit API
 *  and may be changed from version to version or even be completely removed.
*/

#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
#include <QMediaPlayer>
#endif
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtGui/QColor>

class QWebSelectData {
public:
    virtual ~QWebSelectData() {}

    enum ItemType { Option, Group, Separator };

    virtual ItemType itemType(int) const = 0;
    virtual QString itemText(int index) const = 0;
    virtual QString itemToolTip(int index) const = 0;
    virtual bool itemIsEnabled(int index) const = 0;
    virtual bool itemIsSelected(int index) const = 0;
    virtual int itemCount() const = 0;
    virtual bool multiple() const = 0;
    virtual QColor backgroundColor() const = 0;
    virtual QColor foregroundColor() const = 0;
    virtual QColor itemBackgroundColor(int index) const = 0;
    virtual QColor itemForegroundColor(int index) const = 0;
};

class QWebSelectMethod : public QObject {
    Q_OBJECT
public:
    virtual ~QWebSelectMethod() {}

    virtual void show(const QWebSelectData&) = 0;
    virtual void hide() = 0;

Q_SIGNALS:
    void selectItem(int index, bool allowMultiplySelections, bool shift);
    void didHide();
};

class QWebNotificationData {
public:
    virtual ~QWebNotificationData() {}

    virtual const QString title() const = 0;
    virtual const QString message() const = 0;
    virtual const QByteArray iconData() const = 0;
    virtual const QUrl openerPageUrl() const = 0;
};

class QWebNotificationPresenter : public QObject {
    Q_OBJECT
public:
    QWebNotificationPresenter() {}
    virtual ~QWebNotificationPresenter() {}

    virtual void showNotification(const QWebNotificationData*) = 0;
    
Q_SIGNALS:
    void notificationClosed();
    void notificationClicked();
};

class QWebHapticFeedbackPlayer: public QObject {
    Q_OBJECT
public:
    QWebHapticFeedbackPlayer() {}
    virtual ~QWebHapticFeedbackPlayer() {}

    enum HapticStrength {
        None, Weak, Medium, Strong
    };

    enum HapticEvent {
        Press, Release
    };

    virtual void playHapticFeedback(const HapticEvent, const QString& hapticType, const HapticStrength) = 0;
};

class QWebTouchModifier : public QObject {
    Q_OBJECT
public:
    virtual ~QWebTouchModifier() {}

    enum PaddingDirection {
        Up, Right, Down, Left
    };

    virtual unsigned hitTestPaddingForTouch(const PaddingDirection) const = 0;
};

#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
class QWebFullScreenVideoHandler : public QObject {
    Q_OBJECT
public:
    QWebFullScreenVideoHandler() {}
    virtual ~QWebFullScreenVideoHandler() {}
    virtual bool requiresFullScreenForVideoPlayback() const = 0;

Q_SIGNALS:
    void fullScreenClosed();

public Q_SLOTS:
    virtual void enterFullScreen(QMediaPlayer*) = 0;
    virtual void exitFullScreen() = 0;
};
#endif

class QWebKitPlatformPlugin {
public:
    virtual ~QWebKitPlatformPlugin() {}

    enum Extension {
        MultipleSelections,
        Notifications,
        Haptics,
        TouchInteraction,
        FullScreenVideoPlayer
    };

    virtual bool supportsExtension(Extension) const = 0;
    virtual QObject* createExtension(Extension) const = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QWebKitPlatformPlugin, "com.nokia.Qt.WebKit.PlatformPlugin/1.7");
QT_END_NAMESPACE

#endif // QWEBKITPLATFORMPLUGIN_H
