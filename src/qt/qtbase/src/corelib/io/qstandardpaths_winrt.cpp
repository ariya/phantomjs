/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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
    case ConfigLocation: // same as DataLocation, on Windows
    case GenericConfigLocation: // same as GenericDataLocation, on Windows
    case DataLocation:
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
        HSTRING path;
        if (FAILED(settingsFolderItem->get_Path(&path)))
            break;
        result = convertCharArray(WindowsGetStringRawBuffer(path, nullptr));
        if (isTestModeEnabled())
            result += QLatin1String("/qttest");
        break;
    }
    case CacheLocation:
        return writableLocation(DataLocation) + QLatin1String("/cache");

    case GenericCacheLocation:
        return writableLocation(GenericDataLocation) + QLatin1String("/cache");

    case RuntimeLocation:
    case HomeLocation:
        result = QDir::homePath();
        break;

    case TempLocation:
        result = QDir::tempPath();
        break;
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
