/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxnavigatorpps.h"

#include <QDebug>
#include <private/qcore_unix_p.h>

#if defined(QQNXNAVIGATOR_DEBUG)
#define qNavigatorDebug qDebug
#else
#define qNavigatorDebug QT_NO_QDEBUG_MACRO
#endif

static const char *navigatorControlPath = "/pps/services/navigator/control";
static const int ppsBufferSize = 4096;

QT_BEGIN_NAMESPACE

QQnxNavigatorPps::QQnxNavigatorPps(QObject *parent)
    : QQnxAbstractNavigator(parent)
    , m_fd(-1)
{
}

QQnxNavigatorPps::~QQnxNavigatorPps()
{
    // close connection to navigator
    if (m_fd != -1)
        qt_safe_close(m_fd);
}

bool QQnxNavigatorPps::openPpsConnection()
{
    if (m_fd != -1)
        return true;

    // open connection to navigator
    errno = 0;
    m_fd = qt_safe_open(navigatorControlPath, O_RDWR);
    if (m_fd == -1) {
        qWarning("QQNX: failed to open navigator pps, errno=%d", errno);
        return false;
    }

    qNavigatorDebug() << Q_FUNC_INFO << "successfully connected to Navigator. fd=" << m_fd;

    return true;
}

bool QQnxNavigatorPps::requestInvokeUrl(const QByteArray &encodedUrl)
{
    if (!openPpsConnection())
        return false;

    return sendPpsMessage("invoke", encodedUrl);
}

bool QQnxNavigatorPps::sendPpsMessage(const QByteArray &message, const QByteArray &data)
{
    QByteArray ppsMessage = "msg::" + message;

    if (!data.isEmpty())
        ppsMessage += "\ndat::" + data;

    ppsMessage += "\n";

    qNavigatorDebug() << Q_FUNC_INFO << "sending PPS message:\n" << ppsMessage;

    // send pps message to navigator
    errno = 0;
    int bytes = qt_safe_write(m_fd, ppsMessage.constData(), ppsMessage.size());
    if (bytes == -1)
        qFatal("QQNX: failed to write navigator pps, errno=%d", errno);

    // allocate buffer for pps data
    char buffer[ppsBufferSize];

    // attempt to read pps data
    do {
        errno = 0;
        bytes = qt_safe_read(m_fd, buffer, ppsBufferSize - 1);
        if (bytes == -1)
            qFatal("QQNX: failed to read navigator pps, errno=%d", errno);
    } while (bytes == 0);

    // ensure data is null terminated
    buffer[bytes] = '\0';

    qNavigatorDebug() << Q_FUNC_INFO << "received PPS message:\n" << buffer;

    // process received message
    QByteArray ppsData(buffer);
    QHash<QByteArray, QByteArray> responseFields;
    parsePPS(ppsData, responseFields);

    if (responseFields.contains("res") && responseFields.value("res") == message) {
        if (responseFields.contains("err")) {
            qCritical() << "navigator responded with error: " << responseFields.value("err");
            return false;
        }
    }

    return true;
}

void QQnxNavigatorPps::parsePPS(const QByteArray &ppsData, QHash<QByteArray, QByteArray> &messageFields)
{
    qNavigatorDebug() << Q_FUNC_INFO << "data=" << ppsData;

    // tokenize pps data into lines
    QList<QByteArray> lines = ppsData.split('\n');

    // validate pps object
    if (lines.size() == 0 || lines.at(0) != "@control")
        qFatal("QQNX: unrecognized pps object, data=%s", ppsData.constData());

    // parse pps object attributes and extract values
    for (int i = 1; i < lines.size(); i++) {

        // tokenize current attribute
        const QByteArray &attr = lines.at(i);

        qNavigatorDebug() << Q_FUNC_INFO << "attr=" << attr;

        int firstColon = attr.indexOf(':');
        if (firstColon == -1) {
            // abort - malformed attribute
            continue;
        }

        int secondColon = attr.indexOf(':', firstColon + 1);
        if (secondColon == -1) {
            // abort - malformed attribute
            continue;
        }

        QByteArray key = attr.left(firstColon);
        QByteArray value = attr.mid(secondColon + 1);

        qNavigatorDebug() << Q_FUNC_INFO << "key=" << key;
        qNavigatorDebug() << Q_FUNC_INFO << "val=" << value;
        messageFields[key] = value;
    }
}

QT_END_NAMESPACE
