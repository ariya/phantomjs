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

#include "qwinrtfiledialoghelper.h"
#include "qwinrtfileengine.h"

#include <QtCore/QEventLoop>
#include <QtCore/QMap>
#include <QtCore/QVector>
#include <QtCore/qfunctions_winrt.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.pickers.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Foundation::Collections;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::Storage::Pickers;

typedef IAsyncOperationCompletedHandler<StorageFile *> SingleFileHandler;
typedef IAsyncOperationCompletedHandler<IVectorView<StorageFile *> *> MultipleFileHandler;
typedef IAsyncOperationCompletedHandler<StorageFolder *> SingleFolderHandler;

QT_BEGIN_NAMESPACE

// Required for save file picker
class WindowsStringVector : public RuntimeClass<IVector<HSTRING>>
{
public:
    HRESULT __stdcall GetAt(quint32 index, HSTRING *item)
    {
        *item = impl.at(index);
        return S_OK;
    }
    HRESULT __stdcall get_Size(quint32 *size)
    {
        *size = impl.size();
        return S_OK;
    }
    HRESULT __stdcall GetView(IVectorView<HSTRING> **view)
    {
        *view = Q_NULLPTR;
        return E_NOTIMPL;
    }
    HRESULT __stdcall IndexOf(HSTRING value, quint32 *index, boolean *found)
    {
        *found = false;
        for (int i = 0; i < impl.size(); ++i) {
            qint32 result;
            HRESULT hr = WindowsCompareStringOrdinal(impl.at(i), value, &result);
            if (FAILED(hr))
                return hr;
            if (result == 0) {
                *index = quint32(i);
                *found = true;
                break;
            }
        }
        return S_OK;
    }
    HRESULT __stdcall SetAt(quint32 index, HSTRING item)
    {
        HSTRING newItem;
        HRESULT hr = WindowsDuplicateString(item, &newItem);
        if (FAILED(hr))
            return hr;
        impl[index] = newItem;
        return S_OK;
    }
    HRESULT __stdcall InsertAt(quint32 index, HSTRING item)
    {
        HSTRING newItem;
        HRESULT hr = WindowsDuplicateString(item, &newItem);
        if (FAILED(hr))
            return hr;
        impl.insert(index, newItem);
        return S_OK;
    }
    HRESULT __stdcall RemoveAt(quint32 index)
    {
        WindowsDeleteString(impl.takeAt(index));
        return S_OK;
    }
    HRESULT __stdcall Append(HSTRING item)
    {
        HSTRING newItem;
        HRESULT hr = WindowsDuplicateString(item, &newItem);
        if (FAILED(hr))
            return hr;
        impl.append(newItem);
        return S_OK;
    }
    HRESULT __stdcall RemoveAtEnd()
    {
        WindowsDeleteString(impl.takeLast());
        return S_OK;
    }
    HRESULT __stdcall Clear()
    {
        foreach (const HSTRING &item, impl)
            WindowsDeleteString(item);
        impl.clear();
        return S_OK;
    }
private:
    QVector<HSTRING> impl;
};

template<typename T>
static bool initializePicker(HSTRING runtimeId, T **picker, const QSharedPointer<QFileDialogOptions> &options)
{
    HRESULT hr;

    ComPtr<IInspectable> basePicker;
    hr = RoActivateInstance(runtimeId, &basePicker);
    RETURN_FALSE_IF_FAILED("Failed to instantiate file picker");
    hr = basePicker.Get()->QueryInterface(IID_PPV_ARGS(picker));
    RETURN_FALSE_IF_FAILED("Failed to cast file picker");

    if (options->isLabelExplicitlySet(QFileDialogOptions::Accept)) {
        const QString labelText = options->labelText(QFileDialogOptions::Accept);
        HStringReference labelTextRef(reinterpret_cast<const wchar_t *>(labelText.utf16()),
                                      labelText.length());
        hr = (*picker)->put_CommitButtonText(labelTextRef.Get());
        RETURN_FALSE_IF_FAILED("Failed to set commit button text");
    }

    return true;
}

template<typename T>
static bool initializeOpenPickerOptions(T *picker, const QSharedPointer<QFileDialogOptions> &options)
{
    HRESULT hr;
    hr = picker->put_ViewMode(options->viewMode() == QFileDialogOptions::Detail
                              ? PickerViewMode_Thumbnail : PickerViewMode_List);
    RETURN_FALSE_IF_FAILED("Failed to set picker view mode");

    ComPtr<IVector<HSTRING>> filters;
    hr = picker->get_FileTypeFilter(&filters);
    RETURN_FALSE_IF_FAILED("Failed to get file type filters list");
    foreach (const QString &namedFilter, options->nameFilters()) {
        foreach (const QString &filter, QPlatformFileDialogHelper::cleanFilterList(namedFilter)) {
            // Remove leading star
            const int offset = (filter.length() > 1 && filter.startsWith(QLatin1Char('*'))) ? 1 : 0;
            HStringReference filterRef(reinterpret_cast<const wchar_t *>(filter.utf16() + offset),
                                       filter.length() - offset);
            hr = filters->Append(filterRef.Get());
            if (FAILED(hr)) {
                qWarning("Failed to add named file filter \"%s\": %s",
                         qPrintable(filter), qPrintable(qt_error_string(hr)));
            }
        }
    }
    // The file dialog won't open with an empty list - add a default wildcard
    quint32 size;
    hr = filters->get_Size(&size);
    RETURN_FALSE_IF_FAILED("Failed to get file type filters list size");
    if (!size) {
        hr = filters->Append(HString::MakeReference(L"*").Get());
        RETURN_FALSE_IF_FAILED("Failed to add default wildcard to file type filters list");
    }

    return true;
}

class QWinRTFileDialogHelperPrivate
{
public:
    bool shown;
    QEventLoop loop;

    // Input
    QUrl directory;
    QUrl saveFileName;
    QString selectedNameFilter;

    // Output
    QList<QUrl> selectedFiles;
};

QWinRTFileDialogHelper::QWinRTFileDialogHelper()
    : QPlatformFileDialogHelper(), d_ptr(new QWinRTFileDialogHelperPrivate)
{
    Q_D(QWinRTFileDialogHelper);

    d->shown = false;
}

QWinRTFileDialogHelper::~QWinRTFileDialogHelper()
{
}

void QWinRTFileDialogHelper::exec()
{
    Q_D(QWinRTFileDialogHelper);

    if (!d->shown)
        show(Qt::Dialog, Qt::ApplicationModal, 0);
    d->loop.exec();
}

bool QWinRTFileDialogHelper::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags)
    Q_UNUSED(windowModality)
    Q_UNUSED(parent)
    Q_D(QWinRTFileDialogHelper);

    HRESULT hr;
    const QSharedPointer<QFileDialogOptions> dialogOptions = options();
    switch (dialogOptions->acceptMode()) {
    default:
    case QFileDialogOptions::AcceptOpen: {
        switch (dialogOptions->fileMode()) {
        case QFileDialogOptions::AnyFile:
        case QFileDialogOptions::ExistingFile:
        case QFileDialogOptions::ExistingFiles: {
            ComPtr<IFileOpenPicker> picker;
            if (!initializePicker(HString::MakeReference(RuntimeClass_Windows_Storage_Pickers_FileOpenPicker).Get(),
                                  picker.GetAddressOf(), dialogOptions)) {
                return false;
            }
            if (!initializeOpenPickerOptions(picker.Get(), dialogOptions))
                return false;

            if (dialogOptions->fileMode() == QFileDialogOptions::ExistingFiles) {
                ComPtr<IAsyncOperation<IVectorView<StorageFile *> *>> op;
                hr = picker->PickMultipleFilesAsync(&op);
                RETURN_FALSE_IF_FAILED("Failed to open multi file picker");
                hr = op->put_Completed(Callback<MultipleFileHandler>(this, &QWinRTFileDialogHelper::onMultipleFilesPicked).Get());
            } else {
                ComPtr<IAsyncOperation<StorageFile *>> op;
                hr = picker->PickSingleFileAsync(&op);
                RETURN_FALSE_IF_FAILED("Failed to open single file picker");
                hr = op->put_Completed(Callback<SingleFileHandler>(this, &QWinRTFileDialogHelper::onSingleFilePicked).Get());
            }
            RETURN_FALSE_IF_FAILED("Failed to attach file picker callback");
            break;
        }
        case QFileDialogOptions::Directory:
        case QFileDialogOptions::DirectoryOnly: {
            ComPtr<IFolderPicker> picker;
            if (!initializePicker(HString::MakeReference(RuntimeClass_Windows_Storage_Pickers_FolderPicker).Get(),
                                  picker.GetAddressOf(), dialogOptions)) {
                return false;
            }
            if (!initializeOpenPickerOptions(picker.Get(), dialogOptions))
                return false;

            ComPtr<IAsyncOperation<StorageFolder *>> op;
            hr = picker->PickSingleFolderAsync(&op);
            RETURN_FALSE_IF_FAILED("Failed to open folder picker");
            hr = op->put_Completed(Callback<SingleFolderHandler>(this, &QWinRTFileDialogHelper::onSingleFolderPicked).Get());
            RETURN_FALSE_IF_FAILED("Failed to attach folder picker callback");
            break;
        }
        }
        break;
    }
    case QFileDialogOptions::AcceptSave: {
        ComPtr<IFileSavePicker> picker;
        if (!initializePicker(HString::MakeReference(RuntimeClass_Windows_Storage_Pickers_FileSavePicker).Get(),
                              picker.GetAddressOf(), dialogOptions)) {
            return false;
        }

        ComPtr<IMap<HSTRING, IVector<HSTRING> *>> choices;
        hr = picker->get_FileTypeChoices(&choices);
        RETURN_FALSE_IF_FAILED("Failed to get file extension choices");
        foreach (const QString &namedFilter, dialogOptions->nameFilters()) {
            ComPtr<IVector<HSTRING>> entry = Make<WindowsStringVector>();
            foreach (const QString &filter, QPlatformFileDialogHelper::cleanFilterList(namedFilter)) {
                // Remove leading star
                const int offset = (filter.length() > 1 && filter.startsWith(QLatin1Char('*'))) ? 1 : 0;
                HStringReference filterRef(reinterpret_cast<const wchar_t *>(filter.utf16() + offset),
                                           filter.length() - offset);
                hr = entry->Append(filterRef.Get());
                if (FAILED(hr)) {
                    qWarning("Failed to add named file filter \"%s\": %s",
                             qPrintable(filter), qPrintable(qt_error_string(hr)));
                }
            }
            const int offset = namedFilter.indexOf(QLatin1String(" ("));
            const QString filterTitle = offset > 0 ? namedFilter.left(offset) : filterTitle;
            HStringReference namedFilterRef(reinterpret_cast<const wchar_t *>(filterTitle.utf16()),
                                            filterTitle.length());
            boolean replaced;
            hr = choices->Insert(namedFilterRef.Get(), entry.Get(), &replaced);
            RETURN_FALSE_IF_FAILED("Failed to insert file extension choice entry");
        }

        const QString suffix = dialogOptions->defaultSuffix();
        HStringReference nativeSuffix(reinterpret_cast<const wchar_t *>(suffix.utf16()),
                                      suffix.length());
        hr = picker->put_DefaultFileExtension(nativeSuffix.Get());
        RETURN_FALSE_IF_FAILED("Failed to set default file extension");

        const QString suggestedName = QFileInfo(d->saveFileName.toLocalFile()).fileName();
        HStringReference nativeSuggestedName(reinterpret_cast<const wchar_t *>(suggestedName.utf16()),
                                             suggestedName.length());
        hr = picker->put_SuggestedFileName(nativeSuggestedName.Get());
        RETURN_FALSE_IF_FAILED("Failed to set suggested file name");

        ComPtr<IAsyncOperation<StorageFile *>> op;
        hr = picker->PickSaveFileAsync(&op);
        RETURN_FALSE_IF_FAILED("Failed to open save file picker");
        hr = op->put_Completed(Callback<SingleFileHandler>(this, &QWinRTFileDialogHelper::onSingleFilePicked).Get());
        RETURN_FALSE_IF_FAILED("Failed to attach file picker callback");
        break;
    }
    }

    d->shown = true;
    return true;
}

void QWinRTFileDialogHelper::hide()
{
    Q_D(QWinRTFileDialogHelper);

    if (!d->shown)
        return;

    d->shown = false;
}

void QWinRTFileDialogHelper::setDirectory(const QUrl &directory)
{
    Q_D(QWinRTFileDialogHelper);
    d->directory = directory;
}

QUrl QWinRTFileDialogHelper::directory() const
{
    Q_D(const QWinRTFileDialogHelper);
    return d->directory;
}

void QWinRTFileDialogHelper::selectFile(const QUrl &saveFileName)
{
    Q_D(QWinRTFileDialogHelper);
    d->saveFileName = saveFileName;
}

QList<QUrl> QWinRTFileDialogHelper::selectedFiles() const
{
    Q_D(const QWinRTFileDialogHelper);
    return d->selectedFiles;
}

void QWinRTFileDialogHelper::selectNameFilter(const QString &selectedNameFilter)
{
    Q_D(QWinRTFileDialogHelper);
    d->selectedNameFilter = selectedNameFilter;
}

QString QWinRTFileDialogHelper::selectedNameFilter() const
{
    Q_D(const QWinRTFileDialogHelper);
    return d->selectedNameFilter;
}

HRESULT QWinRTFileDialogHelper::onSingleFilePicked(IAsyncOperation<StorageFile *> *args, AsyncStatus status)
{
    Q_D(QWinRTFileDialogHelper);

    QEventLoopLocker locker(&d->loop);
    d->shown = false;
    d->selectedFiles.clear();
    if (status == Canceled || status == Error) {
        emit reject();
        return S_OK;
    }

    HRESULT hr;
    ComPtr<IStorageFile> file;
    hr = args->GetResults(&file);
    Q_ASSERT_SUCCEEDED(hr);
    if (!file) {
        emit reject();
        return S_OK;
    }

    appendFile(file.Get());
    emit accept();
    return S_OK;
}

HRESULT QWinRTFileDialogHelper::onMultipleFilesPicked(IAsyncOperation<IVectorView<StorageFile *> *> *args, AsyncStatus status)
{
    Q_D(QWinRTFileDialogHelper);

    QEventLoopLocker locker(&d->loop);
    d->shown = false;
    d->selectedFiles.clear();
    if (status == Canceled || status == Error) {
        emit reject();
        return S_OK;
    }

    HRESULT hr;
    ComPtr<IVectorView<StorageFile *>> fileList;
    hr = args->GetResults(&fileList);
    RETURN_HR_IF_FAILED("Failed to get file list");

    quint32 size;
    hr = fileList->get_Size(&size);
    Q_ASSERT_SUCCEEDED(hr);
    if (!size) {
        emit reject();
        return S_OK;
    }
    for (quint32 i = 0; i < size; ++i) {
        ComPtr<IStorageFile> file;
        hr = fileList->GetAt(i, &file);
        Q_ASSERT_SUCCEEDED(hr);
        appendFile(file.Get());
    }

    emit accept();
    return S_OK;
}

HRESULT QWinRTFileDialogHelper::onSingleFolderPicked(IAsyncOperation<StorageFolder *> *args, AsyncStatus status)
{
    Q_D(QWinRTFileDialogHelper);

    QEventLoopLocker locker(&d->loop);
    d->shown = false;
    d->selectedFiles.clear();
    if (status == Canceled || status == Error) {
        emit reject();
        return S_OK;
    }

    HRESULT hr;
    ComPtr<IStorageFolder> folder;
    hr = args->GetResults(&folder);
    Q_ASSERT_SUCCEEDED(hr);
    if (!folder) {
        emit reject();
        return S_OK;
    }

    appendFile(folder.Get());
    emit accept();
    return S_OK;
}

void QWinRTFileDialogHelper::appendFile(IInspectable *file)
{
    Q_D(QWinRTFileDialogHelper);

    HRESULT hr;
    ComPtr<IStorageItem> item;
    hr = file->QueryInterface(IID_PPV_ARGS(&item));
    Q_ASSERT_SUCCEEDED(hr);

    HString path;
    hr = item->get_Path(path.GetAddressOf());
    Q_ASSERT_SUCCEEDED(hr);

    quint32 pathLen;
    const wchar_t *pathStr = path.GetRawBuffer(&pathLen);
    const QString filePath = QString::fromWCharArray(pathStr, pathLen);
    QWinRTFileEngineHandler::registerFile(filePath, item.Get());
    d->selectedFiles.append(QUrl::fromLocalFile(filePath));
}

QT_END_NAMESPACE
