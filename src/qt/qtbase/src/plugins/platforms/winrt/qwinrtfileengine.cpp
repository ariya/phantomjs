/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qwinrtfileengine.h"

#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/qfunctions_winrt.h>

#include <wrl.h>
#include <windows.storage.h>
#include <robuffer.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Streams;

typedef IAsyncOperationCompletedHandler<IRandomAccessStream *> StreamCompletedHandler;
typedef IAsyncOperationWithProgressCompletedHandler<IBuffer *, UINT32> StreamReadCompletedHandler;

QT_BEGIN_NAMESPACE

#define RETURN_AND_SET_ERROR_IF_FAILED(error, ret) \
    setError(error, qt_error_string(hr)); \
    if (FAILED(hr)) \
        return ret;

Q_GLOBAL_STATIC(QWinRTFileEngineHandler, handlerInstance)

class QWinRTFileEngineHandlerPrivate
{
public:
    QHash<QString, ComPtr<IStorageItem>> files;
};

class QWinRTFileEnginePrivate
{
public:
    QWinRTFileEnginePrivate(const QString &fileName, IStorageItem *file)
        : fileName(fileName), file(file)
    {
        HRESULT hr;
        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_Streams_Buffer).Get(),
                                    IID_PPV_ARGS(&bufferFactory));
        Q_ASSERT_SUCCEEDED(hr);

        lastSeparator = fileName.size() - 1;
        for (int i = lastSeparator; i >= 0; --i) {
            if (fileName.at(i).unicode() == '/' || fileName.at(i).unicode() == '\\') {
                lastSeparator = i;
                break;
            }
        }

        firstDot = fileName.size();
        for (int i = lastSeparator; i > fileName.size(); ++i) {
            if (fileName.at(i).unicode() == '.') {
                firstDot = i;
                break;
            }
        }
    }

    ComPtr<IBufferFactory> bufferFactory;

    QString fileName;
    int lastSeparator;
    int firstDot;
    ComPtr<IStorageItem> file;
    ComPtr<IRandomAccessStream> stream;

    qint64 pos;

private:
    QWinRTFileEngineHandler *q_ptr;
    Q_DECLARE_PUBLIC(QWinRTFileEngineHandler)
};


QWinRTFileEngineHandler::QWinRTFileEngineHandler()
    : d_ptr(new QWinRTFileEngineHandlerPrivate)
{
}

QWinRTFileEngineHandler::~QWinRTFileEngineHandler()
{
}

void QWinRTFileEngineHandler::registerFile(const QString &fileName, IStorageItem *file)
{
    handlerInstance->d_func()->files.insert(QDir::cleanPath(fileName), file);
}

IStorageItem *QWinRTFileEngineHandler::registeredFile(const QString &fileName)
{
    return handlerInstance->d_func()->files.value(fileName).Get();
}

QAbstractFileEngine *QWinRTFileEngineHandler::create(const QString &fileName) const
{
    Q_D(const QWinRTFileEngineHandler);

    QHash<QString, ComPtr<IStorageItem>>::const_iterator file = d->files.find(fileName);
    if (file != d->files.end())
        return new QWinRTFileEngine(fileName, file.value().Get());

    return Q_NULLPTR;
}

static HRESULT getDestinationFolder(const QString &fileName, const QString newFileName,
                                    IStorageItem *file, IStorageFolder **folder)
{
    HRESULT hr;
    ComPtr<IAsyncOperation<StorageFolder *>> op;
    QFileInfo newFileInfo(newFileName);
#ifndef Q_OS_WINPHONE
    QFileInfo fileInfo(fileName);
    if (fileInfo.dir() == newFileInfo.dir()) {
        ComPtr<IStorageItem2> item;
        hr = file->QueryInterface(IID_PPV_ARGS(&item));
        Q_ASSERT_SUCCEEDED(hr);

        hr = item->GetParentAsync(&op);
    } else
#else
    Q_UNUSED(fileName);
    Q_UNUSED(file)
#endif
    {
        ComPtr<IStorageFolderStatics> folderFactory;
        hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_StorageFolder).Get(),
                                    IID_PPV_ARGS(&folderFactory));
        Q_ASSERT_SUCCEEDED(hr);

        const QString newFilePath = QDir::toNativeSeparators(newFileInfo.absolutePath());
        HStringReference nativeNewFilePath(reinterpret_cast<LPCWSTR>(newFilePath.utf16()),
                                           newFilePath.length());
        hr = folderFactory->GetFolderFromPathAsync(nativeNewFilePath.Get(), &op);
    }
    if (FAILED(hr))
        return hr;
    return QWinRTFunctions::await(op, folder);
}

QWinRTFileEngine::QWinRTFileEngine(const QString &fileName, IStorageItem *file)
    : d_ptr(new QWinRTFileEnginePrivate(fileName, file))
{
}

QWinRTFileEngine::~QWinRTFileEngine()
{
}

bool QWinRTFileEngine::open(QIODevice::OpenMode openMode)
{
    Q_D(QWinRTFileEngine);

    FileAccessMode fileAccessMode = (openMode & QIODevice::WriteOnly)
            ? FileAccessMode_ReadWrite : FileAccessMode_Read;

    HRESULT hr;
    ComPtr<IStorageFile> file;
    hr = d->file.As(&file);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::OpenError, false);

    ComPtr<IAsyncOperation<IRandomAccessStream *>> op;
    hr = file->OpenAsync(fileAccessMode, &op);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::OpenError, false);

    hr = QWinRTFunctions::await(op, d->stream.GetAddressOf());
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::OpenError, false);

    return SUCCEEDED(hr);
}

bool QWinRTFileEngine::close()
{
    Q_D(QWinRTFileEngine);

    if (!d->stream)
        return false;

    ComPtr<IClosable> closable;
    HRESULT hr = d->stream.As(&closable);
    Q_ASSERT_SUCCEEDED(hr);

    hr = closable->Close();
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::UnspecifiedError, false);
    d->stream.Reset();
    return SUCCEEDED(hr);
}

qint64 QWinRTFileEngine::size() const
{
    Q_D(const QWinRTFileEngine);

    if (!d->stream)
        return 0;

    UINT64 size;
    HRESULT hr;
    hr = d->stream->get_Size(&size);
    RETURN_IF_FAILED("Failed to get file size", return 0);

    return qint64(size);
}

qint64 QWinRTFileEngine::pos() const
{
    Q_D(const QWinRTFileEngine);
    return d->pos;
}

bool QWinRTFileEngine::seek(qint64 pos)
{
    Q_D(QWinRTFileEngine);

    if (!d->stream)
        return false;

    HRESULT hr = d->stream->Seek(pos);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::PositionError, false);
    d->pos = pos;
    return SUCCEEDED(hr);
}

bool QWinRTFileEngine::remove()
{
    Q_D(QWinRTFileEngine);

    ComPtr<IAsyncAction> op;
    HRESULT hr = d->file->DeleteAsync(StorageDeleteOption_Default, &op);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::RemoveError, false);

    hr = QWinRTFunctions::await(op);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::RemoveError, false);
    return SUCCEEDED(hr);
}

bool QWinRTFileEngine::copy(const QString &newName)
{
    Q_D(QWinRTFileEngine);

    HRESULT hr;
    ComPtr<IStorageFolder> destinationFolder;
    hr = getDestinationFolder(d->fileName, newName, d->file.Get(), destinationFolder.GetAddressOf());
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::CopyError, false);

    ComPtr<IStorageFile> file;
    hr = d->file.As(&file);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::CopyError, false);

    const QString destinationName = QFileInfo(newName).fileName();
    HStringReference nativeDestinationName(reinterpret_cast<LPCWSTR>(destinationName.utf16()), destinationName.length());
    ComPtr<IAsyncOperation<StorageFile *>> op;
    hr = file->CopyOverloadDefaultOptions(destinationFolder.Get(), nativeDestinationName.Get(), &op);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::CopyError, false);

    ComPtr<IStorageFile> newFile;
    hr = QWinRTFunctions::await(op, newFile.GetAddressOf());
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::CopyError, false);
    return SUCCEEDED(hr);
}

bool QWinRTFileEngine::rename(const QString &newName)
{
    Q_D(QWinRTFileEngine);

    HRESULT hr;
    ComPtr<IStorageFolder> destinationFolder;
    hr = getDestinationFolder(d->fileName, newName, d->file.Get(), destinationFolder.GetAddressOf());
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::RenameError, false);

    const QString destinationName = QFileInfo(newName).fileName();
    HStringReference nativeDestinationName(reinterpret_cast<LPCWSTR>(destinationName.utf16()), destinationName.length());
    ComPtr<IAsyncAction> op;
    hr = d->file->RenameAsyncOverloadDefaultOptions(nativeDestinationName.Get(), &op);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::RenameError, false);
    return SUCCEEDED(hr);
}

bool QWinRTFileEngine::renameOverwrite(const QString &newName)
{
    Q_D(QWinRTFileEngine);

    HRESULT hr;
    ComPtr<IStorageFolder> destinationFolder;
    hr = getDestinationFolder(d->fileName, newName, d->file.Get(), destinationFolder.GetAddressOf());
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::RenameError, false);

    const QString destinationName = QFileInfo(newName).fileName();
    HStringReference nativeDestinationName(reinterpret_cast<LPCWSTR>(destinationName.utf16()), destinationName.length());
    ComPtr<IAsyncAction> op;
    hr = d->file->RenameAsync(nativeDestinationName.Get(), NameCollisionOption_ReplaceExisting, &op);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::RenameError, false);
    return SUCCEEDED(hr);
}

QAbstractFileEngine::FileFlags QWinRTFileEngine::fileFlags(FileFlags type) const
{
    Q_D(const QWinRTFileEngine);

    FileFlags flags = ExistsFlag|ReadOwnerPerm|ReadUserPerm|WriteOwnerPerm|WriteUserPerm;

    HRESULT hr;
    FileAttributes attributes;
    hr = d->file->get_Attributes(&attributes);
    RETURN_IF_FAILED("Failed to get file attributes", return flags);
    if (attributes & FileAttributes_ReadOnly)
        flags ^= WriteUserPerm;
    if (attributes & FileAttributes_Directory)
        flags |= DirectoryType;
    else
        flags |= FileType;

    return type & flags;
}

bool QWinRTFileEngine::setPermissions(uint perms)
{
    Q_UNUSED(perms);
    Q_UNIMPLEMENTED();
    return false;
}

QString QWinRTFileEngine::fileName(FileName type) const
{
    Q_D(const QWinRTFileEngine);

    switch (type) {
    default:
    case DefaultName:
    case AbsoluteName:
    case CanonicalName:
        break;
    case BaseName:
        return d->lastSeparator < 0
                ? d->fileName : d->fileName.mid(d->lastSeparator, d->firstDot - d->lastSeparator);
    case PathName:
    case AbsolutePathName:
    case CanonicalPathName:
        return d->fileName.mid(0, d->lastSeparator);
    case LinkName:
    case BundleName:
        return QString();
    }
    return d->fileName;
}

QDateTime QWinRTFileEngine::fileTime(FileTime type) const
{
    Q_D(const QWinRTFileEngine);

    HRESULT hr;
    DateTime dateTime = { 0 };
    switch (type) {
    case CreationTime:
        hr = d->file->get_DateCreated(&dateTime);
        RETURN_IF_FAILED("Failed to get file creation time", return QDateTime());
        break;
    case ModificationTime:
    case AccessTime: {
        ComPtr<IAsyncOperation<FileProperties::BasicProperties *>> op;
        hr = d->file->GetBasicPropertiesAsync(&op);
        RETURN_IF_FAILED("Failed to initiate file properties", return QDateTime());
        ComPtr<FileProperties::IBasicProperties> properties;
        hr = QWinRTFunctions::await(op, properties.GetAddressOf());
        RETURN_IF_FAILED("Failed to get file properties", return QDateTime());
        hr = type == ModificationTime ? properties->get_DateModified(&dateTime)
                                      : properties->get_ItemDate(&dateTime);
        RETURN_IF_FAILED("Failed to get file date", return QDateTime());
    }
        break;
    }

    SYSTEMTIME systemTime;
    FileTimeToSystemTime((const FILETIME *)&dateTime, &systemTime);
    QDate date(systemTime.wYear, systemTime.wMonth, systemTime.wDay);
    QTime time(systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);
    return QDateTime(date, time);
}

qint64 QWinRTFileEngine::read(char *data, qint64 maxlen)
{
    Q_D(QWinRTFileEngine);

    if (!d->stream)
        return -1;

    ComPtr<IInputStream> stream;
    HRESULT hr = d->stream.As(&stream);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::ReadError, -1);

    UINT32 length = qBound(quint64(0), quint64(maxlen), quint64(UINT_MAX));
    ComPtr<IBuffer> buffer;
    hr = d->bufferFactory->Create(length, &buffer);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::ReadError, -1);

    ComPtr<IAsyncOperationWithProgress<IBuffer *, UINT32>> op;
    hr = stream->ReadAsync(buffer.Get(), length, InputStreamOptions_None, &op);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::ReadError, -1);

    hr = QWinRTFunctions::await(op, buffer.GetAddressOf());
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::ReadError, -1);

    hr = buffer->get_Length(&length);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::ReadError, -1);

    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteArrayAccess;
    hr = buffer.As(&byteArrayAccess);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::ReadError, -1);

    byte *bytes;
    hr = byteArrayAccess->Buffer(&bytes);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::ReadError, -1);
    memcpy(data, bytes, length);
    return qint64(length);
}

qint64 QWinRTFileEngine::write(const char *data, qint64 maxlen)
{
    Q_D(QWinRTFileEngine);

    if (!d->stream)
        return -1;

    ComPtr<IOutputStream> stream;
    HRESULT hr = d->stream.As(&stream);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);

    UINT32 length = qBound(quint64(0), quint64(maxlen), quint64(UINT_MAX));
    ComPtr<IBuffer> buffer;
    hr = d->bufferFactory->Create(length, &buffer);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);
    hr = buffer->put_Length(length);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);

    ComPtr<Windows::Storage::Streams::IBufferByteAccess> byteArrayAccess;
    hr = buffer.As(&byteArrayAccess);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);

    byte *bytes;
    hr = byteArrayAccess->Buffer(&bytes);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);
    memcpy(bytes, data, length);

    ComPtr<IAsyncOperationWithProgress<UINT32, UINT32>> op;
    hr = stream->WriteAsync(buffer.Get(), &op);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);

    hr = QWinRTFunctions::await(op, &length);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);

    ComPtr<IAsyncOperation<bool>> flushOp;
    hr = stream->FlushAsync(&flushOp);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);
    boolean flushed;
    hr = QWinRTFunctions::await(flushOp, &flushed);
    RETURN_AND_SET_ERROR_IF_FAILED(QFileDevice::WriteError, -1);

    return qint64(length);
}

QT_END_NAMESPACE
