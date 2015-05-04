/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qstandardpaths.h"

#include <qdir.h>
#include <private/qsystemlibrary_p.h>
#include <qcoreapplication.h>
#include <qstringlist.h>

#include <qt_windows.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#include <Windows.ApplicationModel.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::ApplicationModel;

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

static QString convertCharArray(const wchar_t *path)
{
    return QDir::fromNativeSeparators(QString::fromWCharArray(path));
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    QString result;

    switch (type) {
    case ConfigLocation: // same as AppLocalDataLocation, on Windows
    case GenericConfigLocation: // same as GenericDataLocation, on Windows
    case AppDataLocation:
    case AppLocalDataLocation:
    case GenericDataLocation: {
        ComPtr<IApplicationDataStatics> applicationDataStatics;
        if (FAILED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &applicationDataStatics)))
            break;
        ComPtr<IApplicationData> applicationData;
        if (FAILED(applicationDataStatics->get_Current(&applicationData)))
            break;
        ComPtr<IStorageFolder> settingsFolder;
        if (FAILED(applicationData->get_LocalFolder(&settingsFolder)))
            break;
        ComPtr<IStorageItem> settingsFolderItem;
        if (FAILED(settingsFolder.As(&settingsFolderItem)))
            break;
        HString path;
        if (FAILED(settingsFolderItem->get_Path(path.GetAddressOf())))
            break;
        result = convertCharArray(path.GetRawBuffer(nullptr));
        if (isTestModeEnabled())
            result += QLatin1String("/qttest");
        break;
    }
    case CacheLocation:
        return writableLocation(AppLocalDataLocation) + QLatin1String("/cache");

    case GenericCacheLocation:
        return writableLocation(GenericDataLocation) + QLatin1String("/cache");

    case TempLocation:
        result = QDir::tempPath();
        break;

    case ApplicationsLocation:
    case DesktopLocation:
    case FontsLocation:
    case HomeLocation:
    case RuntimeLocation:
        // these are read-only
        break;

    case DocumentsLocation:
    case MusicLocation:
    case MoviesLocation:
    case PicturesLocation:
    case DownloadLocation:
    default:
        Q_UNIMPLEMENTED();
    }
    return result;

}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    return QStringList(writableLocation(type));
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
