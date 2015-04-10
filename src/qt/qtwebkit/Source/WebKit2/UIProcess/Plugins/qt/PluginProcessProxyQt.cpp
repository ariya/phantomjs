/*
 * Copyright (C) 2011 Nokia Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PluginProcessProxy.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ProcessExecutablePath.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QProcess>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QVariant>
#include <WebCore/FileSystem.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebKit {

class PluginProcessCreationParameters;

void PluginProcessProxy::platformGetLaunchOptions(ProcessLauncher::LaunchOptions& launchOptions, const PluginProcessAttributes& pluginProcessAttributes)
{
    launchOptions.extraInitializationData.add("plugin-path", pluginProcessAttributes.moduleInfo.path);
}

void PluginProcessProxy::platformInitializePluginProcess(PluginProcessCreationParameters&)
{
}

static PassOwnPtr<QFile> cacheFile()
{
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cachePath.isEmpty())
        return PassOwnPtr<QFile>();

    // This should match the path set through WKContextSetDiskCacheDirectory.
    cachePath.append(QDir::separator()).append(QStringLiteral(".QtWebKit")).append(QDir::separator());
    QString cacheFilePath = cachePath % QStringLiteral("plugin_meta_data.json");

    QDir::root().mkpath(cachePath);
    return adoptPtr(new QFile(cacheFilePath));
}

static void removeCacheFile()
{
    if (OwnPtr<QFile> file = cacheFile())
        file->remove();
}

struct ReadResult {
    enum Tag {
        Empty,
        Error,
        Success
    };
};

static ReadResult::Tag readMetaDataFromCacheFile(QJsonDocument& result)
{
    OwnPtr<QFile> file = cacheFile();
    if (!file || !file->open(QFile::ReadOnly))
        return ReadResult::Empty;
    QByteArray data = file->readAll();
    if (data.isEmpty())
        return ReadResult::Empty;

    QJsonParseError error;
    result = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !result.isArray()) {
        // Corrupted file.
        file->remove();
        return ReadResult::Error;
    }

    return ReadResult::Success;
}

static void writeToCacheFile(const QJsonArray& array)
{
    OwnPtr<QFile> file = cacheFile();
    if (file && file->open(QFile::WriteOnly | QFile::Truncate))
        // Don't care about write error here. We will detect it later.
        file->write(QJsonDocument(array).toJson());
}

static void appendToCacheFile(const QJsonObject& object)
{
    QJsonDocument jsonDocument;
    ReadResult::Tag result = readMetaDataFromCacheFile(jsonDocument);
    if (result == ReadResult::Error)
        return;
    if (result == ReadResult::Empty)
        jsonDocument.setArray(QJsonArray());

    QJsonArray array = jsonDocument.array();
    array.append(object);
    writeToCacheFile(array);
}

struct MetaDataResult {
    enum Tag {
        NotAvailable,
        Unloadable,
        Available
    };
};

static MetaDataResult::Tag tryReadPluginMetaDataFromCacheFile(const QString& canonicalPluginPath, RawPluginMetaData& result)
{
    QJsonDocument jsonDocument;
    if (readMetaDataFromCacheFile(jsonDocument) != ReadResult::Success)
        return MetaDataResult::NotAvailable;

    QJsonArray array = jsonDocument.array();
    QDateTime pluginLastModified = QFileInfo(canonicalPluginPath).lastModified();
    for (QJsonArray::const_iterator i = array.constBegin(); i != array.constEnd(); ++i) {
        QJsonValue item = *i;
        if (!item.isObject()) {
            removeCacheFile();
            return MetaDataResult::NotAvailable;
        }

        QJsonObject object = item.toObject();
        if (object.value(QStringLiteral("path")).toString() == canonicalPluginPath) {
            QString timestampString = object.value(QStringLiteral("timestamp")).toString();
            if (timestampString.isEmpty()) {
                removeCacheFile();
                return MetaDataResult::NotAvailable;
            }
            QDateTime timestamp = QDateTime::fromString(timestampString);
            if (timestamp < pluginLastModified) {
                // Out of date data for this plugin => remove it from the file.
                array.removeAt(i.i);
                writeToCacheFile(array);
                return MetaDataResult::NotAvailable;
            }

            if (object.contains(QLatin1String("unloadable")))
                return MetaDataResult::Unloadable;

            // Match.
            result.name = object.value(QStringLiteral("name")).toString();
            result.description = object.value(QStringLiteral("description")).toString();
            result.mimeDescription = object.value(QStringLiteral("mimeDescription")).toString();
            if (result.mimeDescription.isEmpty()) {
                // Only the mime description is mandatory.
                // Don't trust in the cache file if it is empty.
                removeCacheFile();
                return MetaDataResult::NotAvailable;
            }

            return MetaDataResult::Available;
        }
    }

    return MetaDataResult::NotAvailable;
}

bool PluginProcessProxy::scanPlugin(const String& pluginPath, RawPluginMetaData& result)
{
    QFileInfo pluginFileInfo(pluginPath);
    if (!pluginFileInfo.exists())
        return false;

    MetaDataResult::Tag metaDataResult = tryReadPluginMetaDataFromCacheFile(pluginFileInfo.canonicalFilePath(), result);
    if (metaDataResult == MetaDataResult::Available)
        return true;
    if (metaDataResult == MetaDataResult::Unloadable)
        return false;

    // Scan the plugin via the plugin process.
    QString commandLine = QString(executablePathOfPluginProcess()) % QLatin1Char(' ')
                          % QStringLiteral("-scanPlugin") % QLatin1Char(' ') % pluginFileInfo.canonicalFilePath();
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.start(commandLine);

    bool ranSuccessfully = process.waitForFinished()
                           && process.exitStatus() == QProcess::NormalExit
                           && process.exitCode() == EXIT_SUCCESS;
    if (ranSuccessfully) {
        QByteArray outputBytes = process.readAll();
        ASSERT(!(outputBytes.size() % sizeof(UChar)));

        String output(reinterpret_cast<const UChar*>(outputBytes.constData()), outputBytes.size() / sizeof(UChar));
        Vector<String> lines;
        output.split(UChar('\n'), true, lines);
        ASSERT(lines.size() == 4 && lines.last().isEmpty());

        result.name.swap(lines[0]);
        result.description.swap(lines[1]);
        result.mimeDescription.swap(lines[2]);
    } else
        process.kill();

    QVariantMap map;
    map[QStringLiteral("path")] = QString(pluginFileInfo.canonicalFilePath());
    map[QStringLiteral("timestamp")] = QDateTime::currentDateTime().toString();

    if (!ranSuccessfully || result.mimeDescription.isEmpty()) {
        // We failed getting the meta data in some way. Cache this information, so we don't
        // need to rescan such plugins every time. We will retry it once the plugin is updated.

        map[QStringLiteral("unloadable")] = QStringLiteral("true");
        appendToCacheFile(QJsonObject::fromVariantMap(map));
        return false;
    }

    map[QStringLiteral("name")] = QString(result.name);
    map[QStringLiteral("description")] = QString(result.description);
    map[QStringLiteral("mimeDescription")] = QString(result.mimeDescription);
    appendToCacheFile(QJsonObject::fromVariantMap(map));
    return true;
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
