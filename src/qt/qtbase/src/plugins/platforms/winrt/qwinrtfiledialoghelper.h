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

#ifndef QWINRTFILEDIALOGHELPER_H
#define QWINRTFILEDIALOGHELPER_H

#include <qpa/qplatformdialoghelper.h>
#include <QtCore/qt_windows.h>

struct IInspectable;
namespace ABI {
    namespace Windows {
        namespace Storage {
            class StorageFile;
            class StorageFolder;
            struct IStorageFile;
        }
        namespace Foundation {
            enum class AsyncStatus;
            template <typename T> struct IAsyncOperation;
            namespace Collections {
                template <typename T> struct IVectorView;
            }
        }
    }
}

QT_BEGIN_NAMESPACE

class QWinRTFileDialogHelperPrivate;
class QWinRTFileDialogHelper : public QPlatformFileDialogHelper
{
    Q_OBJECT
public:
    explicit QWinRTFileDialogHelper();
    ~QWinRTFileDialogHelper();

    void exec() Q_DECL_OVERRIDE;
    bool show(Qt::WindowFlags, Qt::WindowModality, QWindow *) Q_DECL_OVERRIDE;
    void hide() Q_DECL_OVERRIDE;

    bool defaultNameFilterDisables() const Q_DECL_OVERRIDE { return false; }
    void setDirectory(const QUrl &directory) Q_DECL_OVERRIDE;
    QUrl directory() const Q_DECL_OVERRIDE;
    void selectFile(const QUrl &saveFileName);
    QList<QUrl> selectedFiles() const Q_DECL_OVERRIDE;
    void setFilter() Q_DECL_OVERRIDE { }
    void selectNameFilter(const QString &selectedNameFilter) Q_DECL_OVERRIDE;
    QString selectedNameFilter() const;

private:
    HRESULT onSingleFilePicked(ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Storage::StorageFile *> *,
                               ABI::Windows::Foundation::AsyncStatus);
    HRESULT onMultipleFilesPicked(ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Storage::StorageFile *> *> *,
                                  ABI::Windows::Foundation::AsyncStatus);
    HRESULT onSingleFolderPicked(ABI::Windows::Foundation::IAsyncOperation<ABI::Windows::Storage::StorageFolder *> *,
                                 ABI::Windows::Foundation::AsyncStatus);
    void appendFile(IInspectable *);

    QScopedPointer<QWinRTFileDialogHelperPrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTFileDialogHelper)
};

QT_END_NAMESPACE

#endif // QWINRTFILEDIALOGHELPER_H
