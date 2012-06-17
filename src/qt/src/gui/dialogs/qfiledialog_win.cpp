/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfiledialog.h"

#ifndef QT_NO_FILEDIALOG

#include <private/qfiledialog_p.h>
#include <qapplication.h>
#include <private/qapplication_p.h>
#include <qt_windows.h>
#include <qglobal.h>
#include <qregexp.h>
#include <qbuffer.h>
#include <qdir.h>
#include <qstringlist.h>
#include <private/qsystemlibrary_p.h>
#include "qfiledialog_win_p.h"

#ifdef Q_WS_WINCE
#include <commdlg.h>
bool qt_priv_ptr_valid = false;
#else
//we have to declare them here because they're not present for all SDK/compilers
static const IID   QT_IID_IFileOpenDialog  = {0xd57c7288, 0xd4ad, 0x4768, {0xbe, 0x02, 0x9d, 0x96, 0x95, 0x32, 0xd9, 0x60} };
static const IID   QT_IID_IShellItem       = {0x43826d1e, 0xe718, 0x42ee, {0xbc, 0x55, 0xa1, 0xe2, 0x61, 0xc3, 0x7b, 0xfe} };
static const CLSID QT_CLSID_FileOpenDialog = {0xdc1c5a9c, 0xe88a, 0x4dde, {0xa5, 0xa1, 0x60, 0xf8, 0x2a, 0x20, 0xae, 0xf7} };
#endif


typedef qt_LPITEMIDLIST (WINAPI *PtrSHBrowseForFolder)(qt_BROWSEINFO*);
static PtrSHBrowseForFolder ptrSHBrowseForFolder = 0;
typedef BOOL (WINAPI *PtrSHGetPathFromIDList)(qt_LPITEMIDLIST, LPWSTR);
static PtrSHGetPathFromIDList ptrSHGetPathFromIDList = 0;
typedef HRESULT (WINAPI *PtrSHGetMalloc)(LPMALLOC *);
static PtrSHGetMalloc ptrSHGetMalloc = 0;


QT_BEGIN_NAMESPACE

static void qt_win_resolve_libs()
{
    static bool triedResolve = false;
    if (!triedResolve) {
#if !defined(Q_WS_WINCE)
        QSystemLibrary lib(QLatin1String("shell32"));
        ptrSHBrowseForFolder = (PtrSHBrowseForFolder)lib.resolve("SHBrowseForFolderW");
        ptrSHGetPathFromIDList = (PtrSHGetPathFromIDList)lib.resolve("SHGetPathFromIDListW");
        ptrSHGetMalloc = (PtrSHGetMalloc)lib.resolve("SHGetMalloc");
#else
        // CE stores them in a different lib and does not use unicode version
        QSystemLibrary lib(QLatin1String("Ceshell"));
        ptrSHBrowseForFolder = (PtrSHBrowseForFolder)lib.resolve("SHBrowseForFolder");
        ptrSHGetPathFromIDList = (PtrSHGetPathFromIDList)lib.resolve("SHGetPathFromIDList");
        ptrSHGetMalloc = (PtrSHGetMalloc)lib.resolve("SHGetMalloc");
        if (ptrSHBrowseForFolder && ptrSHGetPathFromIDList && ptrSHGetMalloc)
            qt_priv_ptr_valid = true;
#endif

        triedResolve = true;
    }
}

extern const char* qt_file_dialog_filter_reg_exp; // defined in qfiledialog.cpp
extern QStringList qt_make_filter_list(const QString &filter);

const int maxNameLen = 1023;
const int maxMultiLen = 65535;

// Returns the wildcard part of a filter.
static QString qt_win_extract_filter(const QString &rawFilter)
{
    QString result = rawFilter;
    QRegExp r(QString::fromLatin1(qt_file_dialog_filter_reg_exp));
    int index = r.indexIn(result);
    if (index >= 0)
        result = r.cap(2);
    QStringList list = result.split(QLatin1Char(' '));
    for(QStringList::iterator it = list.begin(); it < list.end(); ++it) {
        if (*it == QLatin1String("*")) {
            *it = QLatin1String("*.*");
            break;
        }
    }
    return list.join(QLatin1String(";"));
}

static QStringList qt_win_make_filters_list(const QString &filter)
{
    QString f(filter);

    if (f.isEmpty())
        f = QFileDialog::tr("All Files (*.*)");

    return qt_make_filter_list(f);
}

// Makes a NUL-oriented Windows filter from a Qt filter.
static QString qt_win_filter(const QString &filter, bool hideFiltersDetails)
{
    QStringList filterLst = qt_win_make_filters_list(filter);
    QStringList::Iterator it = filterLst.begin();
    QString winfilters;
    QRegExp r(QString::fromLatin1(qt_file_dialog_filter_reg_exp));
    for (; it != filterLst.end(); ++it) {
        QString subfilter = *it;
        if (!subfilter.isEmpty()) {
            if (hideFiltersDetails) {
                int index = r.indexIn(subfilter);
                if (index >= 0)
                    winfilters += r.cap(1);
            } else {
                winfilters += subfilter;
            }
            winfilters += QChar();
            winfilters += qt_win_extract_filter(subfilter);
            winfilters += QChar();
        }
    }
    winfilters += QChar();
    return winfilters;
}

static QString qt_win_selected_filter(const QString &filter, DWORD idx)
{
    return qt_win_make_filters_list(filter).at((int)idx - 1);
}

static QString tFilters, tTitle, tInitDir;

static OPENFILENAME* qt_win_make_OFN(QWidget *parent,
                                     const QString& initialSelection,
                                     const QString& initialDirectory,
                                     const QString& title,
                                     const QString& filters,
                                     QFileDialog::FileMode mode,
                                     QFileDialog::Options options)
{
    if (parent)
        parent = parent->window();
    else
        parent = QApplication::activeWindow();

    tInitDir = QDir::toNativeSeparators(initialDirectory);
    tFilters = filters;
    tTitle = title;
    QString initSel = QDir::toNativeSeparators(initialSelection);
    if (!initSel.isEmpty()) {
        initSel.remove(QLatin1Char('<'));
        initSel.remove(QLatin1Char('>'));
        initSel.remove(QLatin1Char('\"'));
        initSel.remove(QLatin1Char('|'));
    }

    int maxLen = mode == QFileDialog::ExistingFiles ? maxMultiLen : maxNameLen;
    wchar_t *tInitSel = new wchar_t[maxLen + 1];
    if (initSel.length() > 0 && initSel.length() <= maxLen)
        memcpy(tInitSel, initSel.utf16(), (initSel.length()+1)*sizeof(QChar));
    else
        tInitSel[0] = 0;

    OPENFILENAME* ofn = new OPENFILENAME;
    memset(ofn, 0, sizeof(OPENFILENAME));

    ofn->lStructSize = sizeof(OPENFILENAME);
    ofn->hwndOwner = parent ? parent->winId() : 0;
    ofn->lpstrFilter = (wchar_t*)tFilters.utf16();
    ofn->lpstrFile = tInitSel;
    ofn->nMaxFile = maxLen;
    ofn->lpstrInitialDir = (wchar_t*)tInitDir.utf16();
    ofn->lpstrTitle = (wchar_t*)tTitle.utf16();
    ofn->Flags = (OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST);
    if (mode == QFileDialog::ExistingFile ||
         mode == QFileDialog::ExistingFiles)
        ofn->Flags |= (OFN_FILEMUSTEXIST);
    if (mode == QFileDialog::ExistingFiles)
        ofn->Flags |= (OFN_ALLOWMULTISELECT);
    if (!(options & QFileDialog::DontConfirmOverwrite))
        ofn->Flags |= OFN_OVERWRITEPROMPT;

    return ofn;
}

static void qt_win_clean_up_OFN(OPENFILENAME **ofn)
{
    delete [] (*ofn)->lpstrFile;
    delete *ofn;
    *ofn = 0;
}

extern void qt_win_eatMouseMove();

QString qt_win_get_open_file_name(const QFileDialogArgs &args,
                                  QString *initialDirectory,
                                  QString *selectedFilter)
{
    QString result;

    QString isel = args.selection;

    if (initialDirectory && initialDirectory->left(5) == QLatin1String("file:"))
        initialDirectory->remove(0, 5);
    QFileInfo fi(*initialDirectory);

    if (initialDirectory && !fi.isDir()) {
        *initialDirectory = fi.absolutePath();
        if (isel.isEmpty())
            isel = fi.fileName();
    }

    if (!fi.exists())
        *initialDirectory = QDir::homePath();

    DWORD selFilIdx = 0;

    int idx = 0;
    if (selectedFilter) {
        QStringList filterLst = qt_win_make_filters_list(args.filter);
        idx = filterLst.indexOf(*selectedFilter);
    }

    QDialog modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(args.parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);

    bool hideFiltersDetails = args.options & QFileDialog::HideNameFilterDetails;
    OPENFILENAME* ofn = qt_win_make_OFN(args.parent, args.selection,
                                        args.directory, args.caption,
                                        qt_win_filter(args.filter, hideFiltersDetails),
                                        QFileDialog::ExistingFile,
                                        args.options);
    if (idx)
        ofn->nFilterIndex = idx + 1;
    if (GetOpenFileName(ofn)) {
        result = QString::fromWCharArray(ofn->lpstrFile);
        selFilIdx = ofn->nFilterIndex;
    }
    qt_win_clean_up_OFN(&ofn);

    QApplicationPrivate::leaveModal(&modal_widget);

    qt_win_eatMouseMove();

    if (result.isEmpty())
        return result;

    fi = result;
    *initialDirectory = fi.path();
    if (selectedFilter)
        *selectedFilter = qt_win_selected_filter(args.filter, selFilIdx);
    return fi.absoluteFilePath();
}

QString qt_win_get_save_file_name(const QFileDialogArgs &args,
                                  QString *initialDirectory,
                                  QString *selectedFilter)
{
    QString result;

    QString isel = args.selection;
    if (initialDirectory && initialDirectory->left(5) == QLatin1String("file:"))
        initialDirectory->remove(0, 5);
    QFileInfo fi(*initialDirectory);

    if (initialDirectory && !fi.isDir()) {
        *initialDirectory = fi.absolutePath();
        if (isel.isEmpty())
            isel = fi.fileName();
    }

    if (!fi.exists())
        *initialDirectory = QDir::homePath();

    DWORD selFilIdx = 0;

    int idx = 0;
    if (selectedFilter) {
        QStringList filterLst = qt_win_make_filters_list(args.filter);
        idx = filterLst.indexOf(*selectedFilter);
    }

    QDialog modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(args.parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);
    bool hideFiltersDetails = args.options & QFileDialog::HideNameFilterDetails;
    // This block is used below for the lpstrDefExt member.
    // Note that the current MSDN docs document this member wrong.
    // It should rather be documented as "the default extension if no extension was given and if the
    // current filter does not have a extension (e.g (*)). If the current filter have an extension, use
    // the extension of the current filter"
    QString defaultSaveExt;
    if (selectedFilter && !selectedFilter->isEmpty()) {
        defaultSaveExt = qt_win_extract_filter(*selectedFilter);
        // make sure we only have the extension
        int firstDot = defaultSaveExt.indexOf(QLatin1Char('.'));
        if (firstDot != -1) {
            defaultSaveExt.remove(0, firstDot + 1);
        } else {
            defaultSaveExt.clear();
        }
    }

    OPENFILENAME *ofn = qt_win_make_OFN(args.parent, args.selection,
                                        args.directory, args.caption,
                                        qt_win_filter(args.filter, hideFiltersDetails),
                                        QFileDialog::AnyFile,
                                        args.options);

    ofn->lpstrDefExt = (wchar_t*)defaultSaveExt.utf16();

    if (idx)
        ofn->nFilterIndex = idx + 1;
    if (GetSaveFileName(ofn)) {
        result = QString::fromWCharArray(ofn->lpstrFile);
        selFilIdx = ofn->nFilterIndex;
    }
    qt_win_clean_up_OFN(&ofn);

#if defined(Q_WS_WINCE)
    int semIndex = result.indexOf(QLatin1Char(';'));
    if (semIndex >= 0)
        result = result.left(semIndex);
#endif

    QApplicationPrivate::leaveModal(&modal_widget);

    qt_win_eatMouseMove();

    if (result.isEmpty())
        return result;

    fi = result;
    *initialDirectory = fi.path();
    if (selectedFilter)
        *selectedFilter = qt_win_selected_filter(args.filter, selFilIdx);
    return fi.absoluteFilePath();
}


#ifndef Q_WS_WINCE

typedef HRESULT (WINAPI *PtrSHCreateItemFromParsingName)(PCWSTR pszPath, IBindCtx *pbc, REFIID riid, void **ppv);
static PtrSHCreateItemFromParsingName pSHCreateItemFromParsingName = 0;

static bool qt_win_set_IFileDialogOptions(IFileDialog *pfd,
                                          const QString& initialSelection,
                                          const QString& initialDirectory,
                                          const QString& title,
                                          const QStringList& filterLst,
                                          QFileDialog::FileMode mode,
                                          QFileDialog::Options options)
{
    if (!pSHCreateItemFromParsingName) {
        // This function is available only in Vista & above.
        QSystemLibrary shellLib(QLatin1String("Shell32"));
        pSHCreateItemFromParsingName = (PtrSHCreateItemFromParsingName)
            shellLib.resolve("SHCreateItemFromParsingName");
        if (!pSHCreateItemFromParsingName)
            return false;
    }
    HRESULT hr;
    QString winfilters;
    int numFilters = 0;
    quint32 currentOffset = 0;
    QList<quint32> offsets;
    QStringList::ConstIterator it = filterLst.begin();
    // Create the native filter string and save offset to each entry.
    for (; it != filterLst.end(); ++it) {
        QString subfilter = *it;
        if (!subfilter.isEmpty()) {
            offsets<<currentOffset;
            //Here the COMMON_ITEM_DIALOG API always add the details for the filter (e.g. *.txt)
            //so we don't need to handle the flag HideNameFilterDetails.
            winfilters += subfilter; // The name of the filter.
            winfilters += QChar();
            currentOffset += subfilter.size()+1;
            offsets<<currentOffset;
            QString spec = qt_win_extract_filter(subfilter);
            winfilters += spec; // The actual filter spec.
            winfilters += QChar();
            currentOffset += spec.size()+1;
            numFilters++;
        }
    }
    // Add the filters to the file dialog.
    if (numFilters) {
        wchar_t *szData = (wchar_t*)winfilters.utf16();
        qt_COMDLG_FILTERSPEC *filterSpec = new qt_COMDLG_FILTERSPEC[numFilters];
        for(int i = 0; i<numFilters; i++) {
            filterSpec[i].pszName = szData+offsets[i*2];
            filterSpec[i].pszSpec = szData+offsets[(i*2)+1];
        }
        hr = pfd->SetFileTypes(numFilters, filterSpec);
        delete []filterSpec;
    }
    // Set the starting folder.
    tInitDir = QDir::toNativeSeparators(initialDirectory);
    if (!tInitDir.isEmpty()) {
        IShellItem *psiDefaultFolder;
        hr = pSHCreateItemFromParsingName((wchar_t*)tInitDir.utf16(), NULL, QT_IID_IShellItem, 
            reinterpret_cast<void**>(&psiDefaultFolder));

        if (SUCCEEDED(hr)) {
            hr = pfd->SetFolder(psiDefaultFolder);
            psiDefaultFolder->Release();
        }
    }
    // Set the currently selected file.
    QString initSel = QDir::toNativeSeparators(initialSelection);
    if (!initSel.isEmpty()) {
        initSel.remove(QLatin1Char('<'));
        initSel.remove(QLatin1Char('>'));
        initSel.remove(QLatin1Char('\"'));
        initSel.remove(QLatin1Char('|'));
    }
    if (!initSel.isEmpty()) {
        hr = pfd->SetFileName((wchar_t*)initSel.utf16());
    }
    // Set the title for the file dialog.
    if (!title.isEmpty()) {
        hr = pfd->SetTitle((wchar_t*)title.utf16());
    }
    // Set other flags for the dialog.
    DWORD newOptions;
    hr = pfd->GetOptions(&newOptions);
    if (SUCCEEDED(hr)) {
        newOptions |= FOS_NOCHANGEDIR;
        if (mode == QFileDialog::ExistingFile ||
             mode == QFileDialog::ExistingFiles)
            newOptions |= (FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
        if (mode == QFileDialog::ExistingFiles)
            newOptions |= FOS_ALLOWMULTISELECT;
        if (!(options & QFileDialog::DontConfirmOverwrite))
            newOptions |= FOS_OVERWRITEPROMPT;
        hr = pfd->SetOptions(newOptions);
    }
    return SUCCEEDED(hr);
}

static QStringList qt_win_CID_get_open_file_names(const QFileDialogArgs &args,
                                       QString *initialDirectory,
                                       const QStringList &filterList,
                                       QString *selectedFilter,
                                       int selectedFilterIndex)
{
    QStringList result;
    QDialog modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(args.parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);
    // Multiple selection is allowed only in IFileOpenDialog.
    IFileOpenDialog *pfd = 0;
    HRESULT hr = CoCreateInstance(QT_CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, QT_IID_IFileOpenDialog, 
        reinterpret_cast<void**>(&pfd));

    if (SUCCEEDED(hr)) {
        qt_win_set_IFileDialogOptions(pfd, args.selection,
                                      args.directory, args.caption,
                                      filterList, QFileDialog::ExistingFiles,
                                      args.options);
        // Set the currently selected filter (one-based index).
        hr = pfd->SetFileTypeIndex(selectedFilterIndex+1);
        QWidget *parentWindow = args.parent;
        if (parentWindow)
            parentWindow = parentWindow->window();
        else
            parentWindow = QApplication::activeWindow();
        // Show the file dialog.
        hr = pfd->Show(parentWindow ? parentWindow->winId() : 0);
        if (SUCCEEDED(hr)) {
            // Retrieve the results.
            IShellItemArray *psiaResults;
            hr = pfd->GetResults(&psiaResults);
            if (SUCCEEDED(hr)) {
                DWORD numItems = 0;
                psiaResults->GetCount(&numItems);
                for (DWORD i = 0; i<numItems; i++) {
                    IShellItem *psi = 0;
                    hr = psiaResults->GetItemAt(i, &psi);
                    if (SUCCEEDED(hr)) {
                        // Retrieve the file name from shell item.
                        wchar_t *pszPath;
                        hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                        if (SUCCEEDED(hr)) {
                            QString fileName = QString::fromWCharArray(pszPath);
                            result.append(fileName);
                            CoTaskMemFree(pszPath);
                        }
                        psi->Release(); // Free the current item.
                    }
                }
                psiaResults->Release(); // Free the array of items.
            }
        }
    }
    QApplicationPrivate::leaveModal(&modal_widget);

    qt_win_eatMouseMove();

    if (!result.isEmpty()) {
        // Retrieve the current folder name.
        IShellItem *psi = 0;
        hr = pfd->GetFolder(&psi);
        if (SUCCEEDED(hr)) {
            wchar_t *pszPath;
            hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
            if (SUCCEEDED(hr)) {
                *initialDirectory = QString::fromWCharArray(pszPath);
                CoTaskMemFree(pszPath);
            }
            psi->Release();
        }
        // Retrieve the currently selected filter.
        if (selectedFilter) {
            quint32 filetype = 0;
            hr = pfd->GetFileTypeIndex(&filetype);
            if (SUCCEEDED(hr) && filetype && filetype <= (quint32)filterList.length()) {
                // This is a one-based index, not zero-based.
                *selectedFilter = filterList[filetype-1];
            }
        }
    }
    if (pfd)
        pfd->Release();
    return result;
}

QString qt_win_CID_get_existing_directory(const QFileDialogArgs &args)
{
    QString result;
    QDialog modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(args.parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);

    IFileOpenDialog *pfd = 0;
    HRESULT hr = CoCreateInstance(QT_CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                                  QT_IID_IFileOpenDialog, reinterpret_cast<void**>(&pfd));

    if (SUCCEEDED(hr)) {
        qt_win_set_IFileDialogOptions(pfd, args.selection,
                                      args.directory, args.caption,
                                      QStringList(), QFileDialog::ExistingFiles,
                                      args.options);

        // Set the FOS_PICKFOLDERS flag
        DWORD newOptions;
        hr = pfd->GetOptions(&newOptions);
        newOptions |= (FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
        if (SUCCEEDED(hr) && SUCCEEDED((hr = pfd->SetOptions(newOptions)))) {
            QWidget *parentWindow = args.parent;
            if (parentWindow)
                parentWindow = parentWindow->window();
            else
                parentWindow = QApplication::activeWindow();

            // Show the file dialog.
            hr = pfd->Show(parentWindow ? parentWindow->winId() : 0);
            if (SUCCEEDED(hr)) {
                // Retrieve the result
                IShellItem *psi = 0;
                hr = pfd->GetResult(&psi);
                if (SUCCEEDED(hr)) {
                    // Retrieve the file name from shell item.
                    wchar_t *pszPath;
                    hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    if (SUCCEEDED(hr)) {
                        result = QString::fromWCharArray(pszPath);
                        CoTaskMemFree(pszPath);
                    }
                    psi->Release(); // Free the current item.
                }
            }
        }
    }
    QApplicationPrivate::leaveModal(&modal_widget);

    qt_win_eatMouseMove();

    if (pfd)
        pfd->Release();
    return result;
}

#endif

QStringList qt_win_get_open_file_names(const QFileDialogArgs &args,
                                       QString *initialDirectory,
                                       QString *selectedFilter)
{
    QFileInfo fi;
    QDir dir;

    if (initialDirectory && initialDirectory->left(5) == QLatin1String("file:"))
        initialDirectory->remove(0, 5);
    fi = QFileInfo(*initialDirectory);

    if (initialDirectory && !fi.isDir()) {
        *initialDirectory = fi.absolutePath();
    }

    if (!fi.exists())
        *initialDirectory = QDir::homePath();

    DWORD selFilIdx = 0;

    QStringList filterLst = qt_win_make_filters_list(args.filter);
    int idx = 0;
    if (selectedFilter) {
        idx = filterLst.indexOf(*selectedFilter);
    }
    // Windows Vista (& above) allows users to search from file dialogs. If user selects
    // multiple files belonging to different folders from these search results, the
    // GetOpenFileName() will return only one folder name for all the files. To retrieve
    // the correct path for all selected files, we have to use Common Item Dialog interfaces.
#ifndef Q_WS_WINCE
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based))
        return qt_win_CID_get_open_file_names(args, initialDirectory, filterLst, selectedFilter, idx);
#endif

    QStringList result;
    QDialog modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(args.parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);

    bool hideFiltersDetails = args.options & QFileDialog::HideNameFilterDetails;
    OPENFILENAME* ofn = qt_win_make_OFN(args.parent, args.selection,
                                        args.directory, args.caption,
                                        qt_win_filter(args.filter, hideFiltersDetails),
                                        QFileDialog::ExistingFiles,
                                        args.options);
    if (idx)
        ofn->nFilterIndex = idx + 1;
    if (GetOpenFileName(ofn)) {
        QString fileOrDir = QString::fromWCharArray(ofn->lpstrFile);
        selFilIdx = ofn->nFilterIndex;
        int offset = fileOrDir.length() + 1;
        if (ofn->lpstrFile[offset] == 0) {
            // Only one file selected; has full path
            fi.setFile(fileOrDir);
            QString res = fi.absoluteFilePath();
            if (!res.isEmpty())
                result.append(res);
        }
        else {
            // Several files selected; first string is path
            dir.setPath(fileOrDir);
            QString f;
            while(!(f = QString::fromWCharArray(ofn->lpstrFile + offset)).isEmpty()) {
                fi.setFile(dir, f);
                QString res = fi.absoluteFilePath();
                if (!res.isEmpty())
                    result.append(res);
                offset += f.length() + 1;
            }
        }
    }
    qt_win_clean_up_OFN(&ofn);

    QApplicationPrivate::leaveModal(&modal_widget);

    qt_win_eatMouseMove();

    if (!result.isEmpty()) {
        *initialDirectory = fi.path();    // only save the path if there is a result
        if (selectedFilter)
            *selectedFilter = qt_win_selected_filter(args.filter, selFilIdx);
    }
    return result;
}

// MFC Directory Dialog. Contrib: Steve Williams (minor parts from Scott Powers)

static int __stdcall winGetExistDirCallbackProc(HWND hwnd,
                                                UINT uMsg,
                                                LPARAM lParam,
                                                LPARAM lpData)
{
    if (uMsg == BFFM_INITIALIZED && lpData != 0) {
        QString *initDir = (QString *)(lpData);
        if (!initDir->isEmpty()) {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, LPARAM(initDir->utf16()));
        }
    } else if (uMsg == BFFM_SELCHANGED) {
        qt_win_resolve_libs();
        if (ptrSHGetPathFromIDList) {
            wchar_t path[MAX_PATH];
            ptrSHGetPathFromIDList(qt_LPITEMIDLIST(lParam), path);
            QString tmpStr = QString::fromWCharArray(path);
            if (!tmpStr.isEmpty())
                SendMessage(hwnd, BFFM_ENABLEOK, 1, 1);
            else
                SendMessage(hwnd, BFFM_ENABLEOK, 0, 0);
            SendMessage(hwnd, BFFM_SETSTATUSTEXT, 1, LPARAM(path));
        }
    }
    return 0;
}

QString qt_win_get_existing_directory(const QFileDialogArgs &args)
{
#ifndef Q_WS_WINCE
    if (QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based))
        return qt_win_CID_get_existing_directory(args);
#endif

    QString currentDir = QDir::currentPath();
    QString result;
    QWidget *parent = args.parent;
    if (parent)
        parent = parent->window();
    else
        parent = QApplication::activeWindow();
    if (parent)
        parent->createWinId();

    QDialog modal_widget;
    modal_widget.setAttribute(Qt::WA_NoChildEventsForParent, true);
    modal_widget.setParent(parent, Qt::Window);
    QApplicationPrivate::enterModal(&modal_widget);

    QString initDir = QDir::toNativeSeparators(args.directory);
    wchar_t path[MAX_PATH];
    wchar_t initPath[MAX_PATH];
    initPath[0] = 0;
    path[0] = 0;
    tTitle = args.caption;

    qt_BROWSEINFO bi;

    Q_ASSERT(!parent ||parent->testAttribute(Qt::WA_WState_Created));
    bi.hwndOwner = (parent ? parent->winId() : 0);
    bi.pidlRoot = NULL;
    //### This does not seem to be respected? - the dialog always displays "Browse for folder"
    bi.lpszTitle = (wchar_t*)tTitle.utf16();
    bi.pszDisplayName = initPath;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_NEWDIALOGSTYLE;
    bi.lpfn = winGetExistDirCallbackProc;
    bi.lParam = LPARAM(&initDir);

    qt_win_resolve_libs();
    if (ptrSHBrowseForFolder) {
        qt_LPITEMIDLIST pItemIDList = ptrSHBrowseForFolder(&bi);
        if (pItemIDList) {
            ptrSHGetPathFromIDList(pItemIDList, path);
            IMalloc *pMalloc;
            if (ptrSHGetMalloc(&pMalloc) == NOERROR) {
                pMalloc->Free(pItemIDList);
                pMalloc->Release();
                result = QString::fromWCharArray(path);
            }
        }
    }
    tTitle = QString();

    QApplicationPrivate::leaveModal(&modal_widget);

    qt_win_eatMouseMove();

    if (!result.isEmpty())
        result.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return result;
}


QT_END_NAMESPACE

#endif
