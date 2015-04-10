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

#include "qwinrtservices.h"
#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#include <windows.system.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::System;

QT_BEGIN_NAMESPACE

QWinRTServices::QWinRTServices()
{
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Foundation_Uri).Get(), &m_uriFactory);
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_StorageFile).Get(), &m_fileFactory);
    GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_System_Launcher).Get(), &m_launcher);
}

QWinRTServices::~QWinRTServices()
{
    if (m_uriFactory)
        m_uriFactory->Release();

    if (m_fileFactory)
        m_fileFactory->Release();

    if (m_launcher)
        m_launcher->Release();
}

bool QWinRTServices::openUrl(const QUrl &url)
{
    if (!(m_uriFactory && m_launcher))
        return QPlatformServices::openUrl(url);

    IUriRuntimeClass *uri;
    QString urlString = url.toString(); HSTRING uriString; HSTRING_HEADER header;
    WindowsCreateStringReference((const wchar_t*)urlString.utf16(), urlString.length(), &header, &uriString);
    m_uriFactory->CreateUri(uriString, &uri);
    if (!uri)
        return false;

    IAsyncOperation<bool> *launchOp;
    m_launcher->LaunchUriAsync(uri, &launchOp);
    uri->Release();
    if (!launchOp)
        return false;

    boolean result = false;
    while (launchOp->GetResults(&result) == E_ILLEGAL_METHOD_CALL)
        QCoreApplication::processEvents();
    launchOp->Release();

    return result;
}

bool QWinRTServices::openDocument(const QUrl &url)
{
    if (!(m_fileFactory && m_launcher))
        return QPlatformServices::openDocument(url);

    const QString pathString = QDir::toNativeSeparators(url.toLocalFile());
    HSTRING_HEADER header; HSTRING path;
    WindowsCreateStringReference((const wchar_t*)pathString.utf16(), pathString.length(), &header, &path);
    IAsyncOperation<StorageFile*> *fileOp;
    m_fileFactory->GetFileFromPathAsync(path, &fileOp);
    if (!fileOp)
        return false;

    IStorageFile *file = nullptr;
    while (fileOp->GetResults(&file) == E_ILLEGAL_METHOD_CALL)
        QCoreApplication::processEvents();
    fileOp->Release();
    if (!file)
        return false;

    IAsyncOperation<bool> *launchOp;
    m_launcher->LaunchFileAsync(file, &launchOp);
    if (!launchOp)
        return false;

    boolean result = false;
    while (launchOp->GetResults(&result) == E_ILLEGAL_METHOD_CALL)
        QCoreApplication::processEvents();
    launchOp->Release();

    return result;
}

QT_END_NAMESPACE
