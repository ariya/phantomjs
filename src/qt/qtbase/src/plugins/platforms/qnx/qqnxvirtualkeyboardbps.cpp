/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxvirtualkeyboardbps.h"

#include <QDebug>

#include <bps/event.h>
#include <bps/locale.h>
#include <bps/virtualkeyboard.h>
#if defined(Q_OS_BLACKBERRY)
#include <bbndk.h>
#endif

#if defined(QQNXVIRTUALKEYBOARD_DEBUG)
#define qVirtualKeyboardDebug qDebug
#else
#define qVirtualKeyboardDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxVirtualKeyboardBps::QQnxVirtualKeyboardBps(QObject *parent)
    : QQnxAbstractVirtualKeyboard(parent)
{
    if (locale_request_events(0) != BPS_SUCCESS)
        qWarning("QQNX: Failed to register for locale events");

    if (virtualkeyboard_request_events(0) != BPS_SUCCESS)
        qWarning("QQNX: Failed to register for virtual keyboard events");

    int height = 0;
    if (virtualkeyboard_get_height(&height) != BPS_SUCCESS)
        qWarning("QQNX: Failed to get virtual keyboard height");

    setHeight(height);
}

bool QQnxVirtualKeyboardBps::handleEvent(bps_event_t *event)
{
    const int eventDomain = bps_event_get_domain(event);
    if (eventDomain == locale_get_domain())
        return handleLocaleEvent(event);

    if (eventDomain == virtualkeyboard_get_domain())
        return handleVirtualKeyboardEvent(event);

    return false;
}

bool QQnxVirtualKeyboardBps::showKeyboard()
{
    qVirtualKeyboardDebug() << Q_FUNC_INFO << "current visibility=" << isVisible();

    // They keyboard's mode is global between applications, we have to set it each time
    if ( !isVisible() )
        applyKeyboardOptions();

    virtualkeyboard_show();
    return true;
}

bool QQnxVirtualKeyboardBps::hideKeyboard()
{
    qVirtualKeyboardDebug() << Q_FUNC_INFO << "current visibility=" << isVisible();
    virtualkeyboard_hide();
    return true;
}

void QQnxVirtualKeyboardBps::applyKeyboardOptions()
{
    virtualkeyboard_layout_t layout = keyboardLayout();
    virtualkeyboard_enter_t enter = enterKey();

    qVirtualKeyboardDebug() << Q_FUNC_INFO << "mode=" << keyboardMode() << "enterKey=" << enterKeyType();

    virtualkeyboard_change_options(layout, enter);
}

virtualkeyboard_layout_t QQnxVirtualKeyboardBps::keyboardLayout() const
{
    switch (keyboardMode()) {
    case Url:
        return VIRTUALKEYBOARD_LAYOUT_URL;
    case Email:
        return VIRTUALKEYBOARD_LAYOUT_EMAIL;
    case Web:
        return VIRTUALKEYBOARD_LAYOUT_WEB;
    case NumPunc:
        return VIRTUALKEYBOARD_LAYOUT_NUM_PUNC;
    case Number:
        return VIRTUALKEYBOARD_LAYOUT_NUMBER;
    case Symbol:
        return VIRTUALKEYBOARD_LAYOUT_SYMBOL;
    case Phone:
        return VIRTUALKEYBOARD_LAYOUT_PHONE;
    case Pin:
        return VIRTUALKEYBOARD_LAYOUT_PIN;
    case Password:
        return VIRTUALKEYBOARD_LAYOUT_PASSWORD;
#if defined(Q_OS_BLACKBERRY)
#if BBNDK_VERSION_AT_LEAST(10, 2, 1)
    case Alphanumeric:
        return VIRTUALKEYBOARD_LAYOUT_ALPHANUMERIC;
#endif
#endif
    case Default: // fall through
    default:
        return VIRTUALKEYBOARD_LAYOUT_DEFAULT;
    }

    return VIRTUALKEYBOARD_LAYOUT_DEFAULT;
}

virtualkeyboard_enter_t QQnxVirtualKeyboardBps::enterKey() const
{
    switch (enterKeyType()) {
    case Connect:
        return VIRTUALKEYBOARD_ENTER_CONNECT;
    case Done:
        return VIRTUALKEYBOARD_ENTER_DONE;
    case Go:
        return VIRTUALKEYBOARD_ENTER_GO;
    case Join:
        return VIRTUALKEYBOARD_ENTER_JOIN;
    case Next:
        return VIRTUALKEYBOARD_ENTER_NEXT;
    case Search:
        return VIRTUALKEYBOARD_ENTER_SEARCH;
    case Send:
        return VIRTUALKEYBOARD_ENTER_SEND;
    case Submit:
        return VIRTUALKEYBOARD_ENTER_SUBMIT;
    case Default: // fall through
    default:
        return VIRTUALKEYBOARD_ENTER_DEFAULT;
    }

    return VIRTUALKEYBOARD_ENTER_DEFAULT;
}

bool QQnxVirtualKeyboardBps::handleLocaleEvent(bps_event_t *event)
{
    if (bps_event_get_code(event) == LOCALE_INFO) {
        const QString language = QString::fromLatin1(locale_event_get_language(event));
        const QString country  = QString::fromLatin1(locale_event_get_country(event));
        const QLocale newLocale(language + QLatin1Char('_') + country);

        qVirtualKeyboardDebug() << Q_FUNC_INFO << "current locale" << locale() << "new locale=" << newLocale;
        setLocale(newLocale);
        return true;
    }

    qVirtualKeyboardDebug() << Q_FUNC_INFO << "Unhandled locale event. code=" << bps_event_get_code(event);

    return false;
}

bool QQnxVirtualKeyboardBps::handleVirtualKeyboardEvent(bps_event_t *event)
{
    switch (bps_event_get_code(event)) {
    case VIRTUALKEYBOARD_EVENT_VISIBLE:
        qVirtualKeyboardDebug() << Q_FUNC_INFO << "EVENT VISIBLE: current visibility=" << isVisible();
        setVisible(true);
        break;

    case VIRTUALKEYBOARD_EVENT_HIDDEN:
        qVirtualKeyboardDebug() << Q_FUNC_INFO << "EVENT HIDDEN: current visibility=" << isVisible();
        setVisible(false);
        break;

    case VIRTUALKEYBOARD_EVENT_INFO: {
        const int newHeight = virtualkeyboard_event_get_height(event);
        qVirtualKeyboardDebug() << Q_FUNC_INFO << "EVENT INFO: current height=" << height() << "new height=" << newHeight;
        setHeight(newHeight);
        break;
    }

    default:
        qVirtualKeyboardDebug() << Q_FUNC_INFO << "Unhandled virtual keyboard event. code=" << bps_event_get_code(event);
        return false;
    }

    return true;
}

QT_END_NAMESPACE
