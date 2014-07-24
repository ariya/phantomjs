/***************************************************************************
**
** Copyright (C) 2013 Research In Motion
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

#include "qqnxfilepicker.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QMimeDatabase>
#include <QUrl>
#include <private/qppsobject_p.h>

#include <bps/navigator.h>
#include <bps/navigator_invoke.h>

#include <errno.h>

#ifdef QQNXFILEPICKER_DEBUG
#define qFilePickerDebug qDebug
#else
#define qFilePickerDebug QT_NO_QDEBUG_MACRO
#endif

static const char s_filePickerTarget[] = "sys.filepicker.target";

QQnxFilePicker::QQnxFilePicker(QObject *parent)
    : QObject(parent)
    , m_invocationHandle(0)
    , m_mode(QQnxFilePicker::Picker)
    , m_title(tr("Pick a file"))
{
    QCoreApplication::eventDispatcher()->installNativeEventFilter(this);
}

QQnxFilePicker::~QQnxFilePicker()
{
    cleanup();

    QCoreApplication::eventDispatcher()->removeNativeEventFilter(this);
}

void QQnxFilePicker::open()
{
    if (m_invocationHandle)
        return;

    // Clear any previous results
    m_selectedFiles.clear();

    int errorCode = BPS_SUCCESS;

    errorCode = navigator_invoke_invocation_create(&m_invocationHandle);
    if (errorCode != BPS_SUCCESS) {
        qWarning() << "QQnxFilePicker: unable to create invocation:" << strerror(errno);
        return;
    }

    errorCode = navigator_invoke_invocation_set_target(m_invocationHandle, s_filePickerTarget);

    if (errorCode != BPS_SUCCESS) {
        cleanup();
        qWarning() << "QQnxFilePicker: unable to set target:" << strerror(errno);
        return;
    }

    errorCode = navigator_invoke_invocation_set_action(m_invocationHandle, "bb.action.OPEN");
    if (errorCode != BPS_SUCCESS) {
        cleanup();
        qWarning() << "QQnxFilePicker: unable to set action:" << strerror(errno);
        return;
    }

    errorCode = navigator_invoke_invocation_set_type(m_invocationHandle, "application/vnd.blackberry.file_picker");
    if (errorCode != BPS_SUCCESS) {
        cleanup();
        qWarning() << "QQnxFilePicker: unable to set mime type:" << strerror(errno);
        return;
    }

    QVariantMap map;
    map[QStringLiteral("Type")] = filePickerType();
    map[QStringLiteral("Mode")] = modeToString(m_mode);
    map[QStringLiteral("Title")] = m_title;
    map[QStringLiteral("ViewMode")] = QStringLiteral("Default");
    map[QStringLiteral("SortBy")] = QStringLiteral("Default");
    map[QStringLiteral("SortOrder")] = QStringLiteral("Default");
    map[QStringLiteral("ImageCrop")] = false;
    map[QStringLiteral("AllowOverwrite")] = false;

    if (!m_defaultSaveFileNames.isEmpty())
        map[QStringLiteral("DefaultFileNames")] = m_defaultSaveFileNames.join(QLatin1Char(','));
    if (!m_filters.isEmpty())
        map[QStringLiteral("Filter")] = m_filters.join(QLatin1Char(';'));

    QByteArray ppsData;
#if defined(Q_OS_BLACKBERRY_TABLET)
    QJsonDocument document;
    document.setObject(QJsonObject::fromVariantMap(map));
    ppsData = document.toJson(QJsonDocument::Compact);
#else
    ppsData = QPpsObject::encode(map);
#endif

    errorCode = navigator_invoke_invocation_set_data(m_invocationHandle, ppsData.constData(), ppsData.size());
    if (errorCode != BPS_SUCCESS) {
        cleanup();
        qWarning() << "QQnxFilePicker: unable to set data:" << strerror(errno);
        return;
    }

    navigator_invoke_invocation_send(m_invocationHandle);
}

void QQnxFilePicker::close()
{
    navigator_card_close_child();
    cleanup();
}

bool QQnxFilePicker::nativeEventFilter(const QByteArray&, void *message, long*)
{
    bps_event_t * const event = static_cast<bps_event_t*>(message);
    if (!event)
        return false;

    if (bps_event_get_code(event) == NAVIGATOR_INVOKE_TARGET_RESULT) {
        const char *id = navigator_event_get_id(event);
        const char *err = navigator_event_get_err(event);
        qFilePickerDebug("received invocation response: id=%s err=%s", id, err);
    } else if (bps_event_get_code(event) == NAVIGATOR_CHILD_CARD_CLOSED) {
        const char *data = navigator_event_get_card_closed_data(event);
        qFilePickerDebug("received data: data='%s'", data);
        handleFilePickerResponse(data);
    }

    return false; // do not drop the event
}

void QQnxFilePicker::setMode(QQnxFilePicker::Mode mode)
{
    m_mode = mode;
}

void QQnxFilePicker::setDefaultSaveFileNames(const QStringList &fileNames)
{
    m_defaultSaveFileNames = fileNames;
}

void QQnxFilePicker::addDefaultSaveFileName(const QString &fileName)
{
    m_defaultSaveFileNames.append(fileName);
}

void QQnxFilePicker::setDirectories(const QStringList &directories)
{
    m_directories = directories;
}

void QQnxFilePicker::addDirectory(const QString &directory)
{
    m_directories.append(directory);
}

void QQnxFilePicker::setFilters(const QStringList &filters)
{
    m_filters = filters;
}

void QQnxFilePicker::setTitle(const QString &title)
{
    m_title = title;
}

QQnxFilePicker::Mode QQnxFilePicker::mode() const
{
    return m_mode;
}

QStringList QQnxFilePicker::defaultSaveFileNames() const
{
    return m_defaultSaveFileNames;
}

QStringList QQnxFilePicker::directories() const
{
    return m_directories;
}

QStringList QQnxFilePicker::filters() const
{
    return m_filters;
}

QStringList QQnxFilePicker::selectedFiles() const
{
    return m_selectedFiles;
}

QString QQnxFilePicker::title() const
{
    return m_title;
}

void QQnxFilePicker::cleanup()
{
    if (m_invocationHandle) {
        navigator_invoke_invocation_destroy(m_invocationHandle);
        m_invocationHandle = 0;
    }
}

void QQnxFilePicker::handleFilePickerResponse(const char *data)
{
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(data, &jsonError);

    if (jsonError.error != QJsonParseError::NoError) {
        qFilePickerDebug() << "Error parsing FilePicker response: "
                 << jsonError.errorString();
        Q_EMIT closed();
        cleanup();
        return;
    }

    // The response is a list of Json objects.
    const QVariantList array = document.array().toVariantList();

    foreach (const QVariant &variant, array) {
        const QJsonObject object = QJsonObject::fromVariantMap(variant.toMap());
        const QUrl url(object.value(QStringLiteral("uri")).toString());
        const QString localFile = url.toLocalFile(); // strip "file://"

        if (!localFile.isEmpty())
            m_selectedFiles << localFile;

        qFilePickerDebug() << "FilePicker uri response:" << localFile;
    }

    Q_EMIT closed();
    cleanup();
}

QString QQnxFilePicker::filePickerType() const
{
    bool images = false;
    bool video = false;
    bool music = false;
    QMimeDatabase mimeDb;
    for (int i = 0; i < m_filters.count(); i++) {
        QList<QMimeType> mimeTypes = mimeDb.mimeTypesForFileName(m_filters.at(i));
        if (mimeTypes.isEmpty())
            return QStringLiteral("Other");

        if (mimeTypes.first().name().startsWith(QLatin1String("image")))
            images = true;
        else if (mimeTypes.first().name().startsWith(QLatin1String("audio")))
            music = true;
        else if (mimeTypes.first().name().startsWith(QLatin1String("video")))
            video = true;
        else
            return QStringLiteral("Other");
    }

    if (!video && !music)
        return QStringLiteral("Picture");

    if (!images && !music)
        return QStringLiteral("Video");

    if (!images && !video)
        return QStringLiteral("Music");

    return QStringLiteral("Other");
}

QString QQnxFilePicker::modeToString(QQnxFilePicker::Mode mode) const
{
    switch (mode) {
    case Picker:
        return QStringLiteral("Picker");
    case Saver:
        return QStringLiteral("Saver");
    case PickerMultiple:
        return QStringLiteral("PickerMultiple");
    case SaverMultiple:
        return QStringLiteral("SaverMultiple");
    }

    return QStringLiteral("Picker");
}
