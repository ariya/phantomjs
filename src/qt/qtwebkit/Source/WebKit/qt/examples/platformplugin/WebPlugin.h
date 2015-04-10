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
#ifndef WEBPLUGIN_H
#define WEBPLUGIN_H

#include "qwebkitplatformplugin.h"
#include "WebNotificationPresenter.h"

#include <QDialog>
#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
#include <QVideoWidget>
#endif

QT_BEGIN_NAMESPACE
class QListWidgetItem;
class QListWidget;
QT_END_NAMESPACE

class Popup : public QDialog {
    Q_OBJECT
public:
    Popup(const QWebSelectData& data) : m_data(data) { setModal(true); }

Q_SIGNALS:
    void itemClicked(int idx);

protected Q_SLOTS:
    void onItemSelected(QListWidgetItem* item);

protected:
    void populateList();

    const QWebSelectData& m_data;
    QListWidget* m_list;
};


class SingleSelectionPopup : public Popup {
    Q_OBJECT
public:
    SingleSelectionPopup(const QWebSelectData& data);
};


class MultipleSelectionPopup : public Popup {
    Q_OBJECT
public:
    MultipleSelectionPopup(const QWebSelectData& data);
};


class WebPopup : public QWebSelectMethod {
    Q_OBJECT
public:
    WebPopup();
    ~WebPopup();

    virtual void show(const QWebSelectData& data);
    virtual void hide();
    virtual void setGeometry(const QRect&) { }
    virtual void setFont(const QFont&) { }

private Q_SLOTS:
    void popupClosed();
    void itemClicked(int idx);

private:
    Popup* m_popup;

    Popup* createPopup(const QWebSelectData& data);
    Popup* createSingleSelectionPopup(const QWebSelectData& data);
    Popup* createMultipleSelectionPopup(const QWebSelectData& data);
};

class TouchModifier : public QWebTouchModifier
{
    Q_OBJECT
public:
    unsigned hitTestPaddingForTouch(const PaddingDirection direction) const {
        // Use 10 as padding in each direction but Up.
        if (direction == QWebTouchModifier::Up)
            return 15;
        return 10;
    }
};

#if defined(WTF_USE_QT_MULTIMEDIA) && WTF_USE_QT_MULTIMEDIA
class FullScreenVideoWidget : public QVideoWidget {
    Q_OBJECT
public:
    FullScreenVideoWidget(QMediaPlayer*);
    virtual ~FullScreenVideoWidget() {}

Q_SIGNALS:
    void fullScreenClosed();

protected:
    bool event(QEvent*);
    void keyPressEvent(QKeyEvent*);

private:
    QMediaPlayer* m_mediaPlayer; // not owned
};

class FullScreenVideoHandler : public QWebFullScreenVideoHandler {
    Q_OBJECT
public:
    FullScreenVideoHandler();
    virtual ~FullScreenVideoHandler();
    bool requiresFullScreenForVideoPlayback() const;

public Q_SLOTS:
    void enterFullScreen(QMediaPlayer*);
    void exitFullScreen();

private:
    FullScreenVideoWidget* m_mediaWidget; // owned
};
#endif

class WebPlugin : public QObject, public QWebKitPlatformPlugin
{
    Q_OBJECT
    Q_INTERFACES(QWebKitPlatformPlugin)
    Q_PLUGIN_METADATA(IID "org.qt-project.QtWebKit.PlatformPluginInterface")
public:
    virtual bool supportsExtension(Extension extension) const;
    virtual QObject* createExtension(Extension extension) const;
};

#endif // WEBPLUGIN_H
