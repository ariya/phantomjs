/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qxcbxsettings.h"

#include <QtCore/QByteArray>

#include <X11/extensions/XIproto.h>

QT_BEGIN_NAMESPACE
/* Implementation of http://standards.freedesktop.org/xsettings-spec/xsettings-0.5.html */

enum XSettingsType {
    XSettingsTypeInteger = 0,
    XSettingsTypeString = 1,
    XSettingsTypeColor = 2
};

class QXcbXSettingsCallback
{
public:
    QXcbXSettings::PropertyChangeFunc func;
    void *handle;
};

class QXcbXSettingsPropertyValue
{
public:
    QXcbXSettingsPropertyValue()
        : last_change_serial(-1)
    {}

    void updateValue(QXcbScreen *screen, const QByteArray &name, const QVariant &value, int last_change_serial)
    {
        if (last_change_serial <= this->last_change_serial)
            return;
        this->value = value;
        this->last_change_serial = last_change_serial;
        QLinkedList<QXcbXSettingsCallback>::const_iterator it = callback_links.begin();
        for (;it != callback_links.end();++it) {
            it->func(screen,name,value,it->handle);
        }
    }

    void addCallback(QXcbXSettings::PropertyChangeFunc func, void *handle)
    {
        QXcbXSettingsCallback callback;
        callback.func = func;
        callback.handle = handle;
        callback_links.append(callback);
    }

    QVariant value;
    int last_change_serial;
    QLinkedList<QXcbXSettingsCallback> callback_links;

};

class QXcbXSettingsPrivate
{
public:
    QXcbXSettingsPrivate(QXcbScreen *screen)
        : screen(screen)
        , initialized(false)
    {
    }

    QByteArray getSettings()
    {
        QXcbConnectionGrabber connectionGrabber(screen->connection());

        int offset = 0;
        QByteArray settings;
        xcb_atom_t _xsettings_atom = screen->connection()->atom(QXcbAtom::_XSETTINGS_SETTINGS);
        while (1) {
            xcb_get_property_cookie_t get_prop_cookie =
                    xcb_get_property_unchecked(screen->xcb_connection(),
                                               false,
                                               x_settings_window,
                                               _xsettings_atom,
                                               _xsettings_atom,
                                               offset/4,
                                               8192);
            xcb_get_property_reply_t *reply = xcb_get_property_reply(screen->xcb_connection(), get_prop_cookie, NULL);
            bool more = false;
            if (!reply)
                return settings;

            settings += QByteArray((const char *)xcb_get_property_value(reply), xcb_get_property_value_length(reply));
            offset += xcb_get_property_value_length(reply);
            more = reply->bytes_after != 0;

            free(reply);

            if (!more)
                break;
        }

        return settings;
    }

    static int round_to_nearest_multiple_of_4(int value)
    {
        int remainder = value % 4;
        if (!remainder)
            return value;
        return value + 4 - remainder;
    }

    void populateSettings(const QByteArray &xSettings)
    {
        if (xSettings.length() < 12)
            return;
        // we ignore byteorder for now
        char byteOrder = xSettings.at(1);
        Q_UNUSED(byteOrder);
        uint number_of_settings = *reinterpret_cast<const uint *>(xSettings.mid(8,4).constData());

        const char *data = xSettings.constData() + 12;
        size_t offset = 0;
        for (uint i = 0; i < number_of_settings; i++) {
            int local_offset = 0;
            XSettingsType type = static_cast<XSettingsType>(*reinterpret_cast<const quint8 *>(data + offset));
            local_offset += 2;

            quint16 name_len = *reinterpret_cast<const quint16 *>(data + offset + local_offset);
            local_offset += 2;

            QByteArray name(data + offset + local_offset, name_len);
            local_offset += round_to_nearest_multiple_of_4(name_len);

            int last_change_serial = *reinterpret_cast<const int *>(data + offset + local_offset);
            Q_UNUSED(last_change_serial);
            local_offset += 4;

            QVariant value;
            if (type == XSettingsTypeString) {
                int value_length = *reinterpret_cast<const int *>(data + offset + local_offset);
                local_offset+=4;
                QByteArray value_string(data + offset + local_offset, value_length);
                value.setValue(value_string);
                local_offset += round_to_nearest_multiple_of_4(value_length);
            } else if (type == XSettingsTypeInteger) {
                int value_length = *reinterpret_cast<const int *>(data + offset + local_offset);
                local_offset += 4;
                value.setValue(value_length);
            } else if (type == XSettingsTypeColor) {
                quint16 red = *reinterpret_cast<const quint16 *>(data + offset + local_offset);
                local_offset += 2;
                quint16 green = *reinterpret_cast<const quint16 *>(data + offset + local_offset);
                local_offset += 2;
                quint16 blue = *reinterpret_cast<const quint16 *>(data + offset + local_offset);
                local_offset += 2;
                quint16 alpha= *reinterpret_cast<const quint16 *>(data + offset + local_offset);
                local_offset += 2;
                QColor color_value(red,green,blue,alpha);
                value.setValue(color_value);
            }
            offset += local_offset;
            settings[name].updateValue(screen,name,value,last_change_serial);
        }

    }

    QXcbScreen *screen;
    xcb_window_t x_settings_window;
    QMap<QByteArray, QXcbXSettingsPropertyValue> settings;
    bool initialized;
};


QXcbXSettings::QXcbXSettings(QXcbScreen *screen)
    : d_ptr(new QXcbXSettingsPrivate(screen))
{
    QByteArray settings_atom_for_screen("_XSETTINGS_S");
    settings_atom_for_screen.append(QByteArray::number(screen->screenNumber()));
    xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(screen->xcb_connection(),
                                                           true,
                                                           settings_atom_for_screen.length(),
                                                           settings_atom_for_screen.constData());
    xcb_generic_error_t *error = 0;
    xcb_intern_atom_reply_t *atom_reply = xcb_intern_atom_reply(screen->xcb_connection(),atom_cookie,&error);
    if (error) {
        free(error);
        return;
    }
    xcb_atom_t selection_owner_atom = atom_reply->atom;
    free(atom_reply);

    xcb_get_selection_owner_cookie_t selection_cookie =
            xcb_get_selection_owner(screen->xcb_connection(), selection_owner_atom);

    xcb_get_selection_owner_reply_t *selection_result =
            xcb_get_selection_owner_reply(screen->xcb_connection(), selection_cookie, &error);
    if (error) {
        free(error);
        return;
    }

    d_ptr->x_settings_window = selection_result->owner;
    free(selection_result);
    if (!d_ptr->x_settings_window) {
        return;
    }

    const uint32_t event = XCB_CW_EVENT_MASK;
    const uint32_t event_mask[] = { XCB_EVENT_MASK_STRUCTURE_NOTIFY|XCB_EVENT_MASK_PROPERTY_CHANGE };
    xcb_change_window_attributes(screen->xcb_connection(),d_ptr->x_settings_window,event,event_mask);

    d_ptr->populateSettings(d_ptr->getSettings());
    d_ptr->initialized = true;
}

bool QXcbXSettings::initialized() const
{
    Q_D(const QXcbXSettings);
    return d->initialized;
}

void QXcbXSettings::handlePropertyNotifyEvent(const xcb_property_notify_event_t *event)
{
    Q_D(QXcbXSettings);
    if (event->window != d->x_settings_window)
        return;
    d->populateSettings(d->getSettings());
}

void QXcbXSettings::registerCallbackForProperty(const QByteArray &property, QXcbXSettings::PropertyChangeFunc func, void *handle)
{
    Q_D(QXcbXSettings);
    d->settings[property].addCallback(func,handle);
}

void QXcbXSettings::removeCallbackForHandle(const QByteArray &property, void *handle)
{
    Q_D(QXcbXSettings);
    QXcbXSettingsPropertyValue &value = d->settings[property];
    QLinkedList<QXcbXSettingsCallback>::iterator it = value.callback_links.begin();
    while (it != value.callback_links.end()) {
        if (it->handle == handle)
            it = value.callback_links.erase(it);
        else
            ++it;
    }
}

void QXcbXSettings::removeCallbackForHandle(void *handle)
{
    Q_D(QXcbXSettings);
    for (QMap<QByteArray, QXcbXSettingsPropertyValue>::const_iterator it = d->settings.cbegin();
         it != d->settings.cend(); ++it) {
        removeCallbackForHandle(it.key(),handle);
    }
}

QVariant QXcbXSettings::setting(const QByteArray &property) const
{
    Q_D(const QXcbXSettings);
    return d->settings.value(property).value;
}

QT_END_NAMESPACE
