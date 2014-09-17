/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qfiledialog.h"

#ifndef QT_NO_FILEDIALOG

#include <private/qfiledialog_p.h>
#if defined(Q_WS_S60) && !defined(SYMBIAN_VERSION_9_4) && !defined(SYMBIAN_VERSION_9_3) && !defined(SYMBIAN_VERSION_9_2)
#include <driveinfo.h>
#include <AknCommonDialogsDynMem.h>
#include <CAknMemorySelectionDialogMultiDrive.h>
#include <MAknFileFilter.h>
#endif
#include "private/qcore_symbian_p.h"

QT_BEGIN_NAMESPACE

extern QStringList qt_make_filter_list(const QString &filter); // defined in qfiledialog.cpp
extern QStringList qt_clean_filter_list(const QString &filter); // defined in qfiledialog.cpp

enum DialogMode { DialogOpen, DialogSave, DialogFolder };
#if defined(Q_WS_S60) && !defined(SYMBIAN_VERSION_9_4) && !defined(SYMBIAN_VERSION_9_3) && !defined(SYMBIAN_VERSION_9_2)
class CExtensionFilter : public CBase, public MAknFileFilter
{
public:
    void setFilter(const QString filter)
    {
        QStringList unparsedFiltersList = qt_make_filter_list(filter);
        QStringList filterList;
        filterRxList.clear();

        foreach (QString unparsedFilter, unparsedFiltersList) {
            filterList << qt_clean_filter_list(unparsedFilter);
        }
        foreach (QString currentFilter, filterList) {
            QRegExp filterRx(currentFilter, Qt::CaseInsensitive, QRegExp::Wildcard);
            filterRxList << filterRx;
        }
    }

    TBool Accept(const TDesC &/*aDriveAndPath*/, const TEntry &aEntry) const
    {
        //If no filter for files, all can be accepted
        if (filterRxList.isEmpty())
            return ETrue;

        if (aEntry.IsDir())
            return ETrue;

        foreach (QRegExp rx, filterRxList) {
            QString fileName = qt_TDesC2QString(aEntry.iName);
            if (rx.exactMatch(fileName))
                return ETrue;
        }

        return EFalse;
    }

private:
    QList<QRegExp> filterRxList;
};
#endif

static QString launchSymbianDialog(const QString dialogCaption, const QString startDirectory,
                                   const QString filter, DialogMode dialogMode)
{
    QString selection;
#if defined(Q_WS_S60) && !defined(SYMBIAN_VERSION_9_4) && !defined(SYMBIAN_VERSION_9_3) && !defined(SYMBIAN_VERSION_9_2)
    TFileName startFolder;
    if (!startDirectory.isEmpty()) {
        QString dir = QDir::toNativeSeparators(QFileDialogPrivate::workingDirectory(startDirectory));
        startFolder = qt_QString2TPtrC(dir);
    }
    TInt types = AknCommonDialogsDynMem::EMemoryTypeMMCExternal|
                 AknCommonDialogsDynMem::EMemoryTypeInternalMassStorage|
                 AknCommonDialogsDynMem::EMemoryTypePhone;

    TPtrC titlePtr(qt_QString2TPtrC(dialogCaption));
    TFileName target;
    bool select = false;
    int tryCount = 2;
    while (tryCount--) {
        TInt err(KErrNone);
        TRAP(err,
            if (dialogMode == DialogOpen) {
                CExtensionFilter* extensionFilter = new (ELeave) CExtensionFilter;
                CleanupStack::PushL(extensionFilter);
                extensionFilter->setFilter(filter);
                select = AknCommonDialogsDynMem::RunSelectDlgLD(types, target,
                         startFolder, 0, 0, titlePtr, extensionFilter);
                CleanupStack::PopAndDestroy(extensionFilter);
            } else if (dialogMode == DialogSave) {
                QString defaultFileName = QFileDialogPrivate::initialSelection(startDirectory);
                target = qt_QString2TPtrC(defaultFileName);
                select = AknCommonDialogsDynMem::RunSaveDlgLD(types, target,
                         startFolder, 0, 0, titlePtr);
            } else if (dialogMode == DialogFolder) {
                select = AknCommonDialogsDynMem::RunFolderSelectDlgLD(types, target, startFolder,
                            0, 0, titlePtr, NULL, NULL);
            }
        );

        if (err == KErrNone) {
            tryCount = 0;
        } else {
            // Symbian native file dialog doesn't allow accessing files outside C:/Data
            // It will always leave in that case, so default into QDir::homePath() in error cases.
            QString dir = QDir::toNativeSeparators(QDir::homePath());
            startFolder = qt_QString2TPtrC(dir);
        }
    }
    if (select) {
        QFileInfo fi(qt_TDesC2QString(target));
        selection = fi.absoluteFilePath();
    }
#endif
    return selection;
}

QString qtSymbianGetOpenFileName(const QString &caption,
                                 const QString &dir,
                                 const QString &filter)
{
    return launchSymbianDialog(caption, dir, filter, DialogOpen);
}

QStringList qtSymbianGetOpenFileNames(const QString &caption,
                                      const QString &dir,
                                      const QString &filter)
{
    QString fileName;
    fileName.append(launchSymbianDialog(caption, dir, filter, DialogOpen));
    QStringList fileList;
    fileList << fileName;

    return fileList;
}

QString qtSymbianGetSaveFileName(const QString &caption,
                                 const QString &dir)
{
    return launchSymbianDialog(caption, dir, QString(), DialogSave);
}

QString qtSymbianGetExistingDirectory(const QString &caption,
                                      const QString &dir)
{
    QString folderCaption;
    if (!caption.isEmpty()) {
        folderCaption.append(caption);
    } else {
        // Title for folder selection dialog is mandatory
        folderCaption.append(QFileDialog::tr("Find Directory"));
    }
    return launchSymbianDialog(folderCaption, dir, QString(), DialogFolder);
}

QT_END_NAMESPACE

#endif
