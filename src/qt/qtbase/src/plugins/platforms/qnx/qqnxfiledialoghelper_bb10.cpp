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

#include "qqnxfiledialoghelper.h"

#include "qqnxfilepicker.h"
#include "qqnxbpseventfilter.h"
#include "qqnxscreen.h"
#include "qqnxintegration.h"

#include <QDebug>
#include <QEventLoop>
#include <QScreen>
#include <QTimer>
#include <QWindow>

#ifdef QQNXFILEDIALOGHELPER_DEBUG
#define qFileDialogHelperDebug qDebug
#else
#define qFileDialogHelperDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxFileDialogHelper::QQnxFileDialogHelper(const QQnxIntegration *integration)
    : QPlatformFileDialogHelper(),
      m_integration(integration),
      m_dialog(new QQnxFilePicker),
      m_acceptMode(QFileDialogOptions::AcceptOpen),
      m_selectedFilter()
{
    connect(m_dialog, &QQnxFilePicker::closed, this, &QQnxFileDialogHelper::emitSignals);
}

QQnxFileDialogHelper::~QQnxFileDialogHelper()
{
    delete m_dialog;
}

void QQnxFileDialogHelper::exec()
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;

    // Clear any previous results
    m_dialog->setDirectories(QStringList());

    QEventLoop loop;
    connect(m_dialog, SIGNAL(closed()), &loop, SLOT(quit()));
    loop.exec();
}

bool QQnxFileDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    Q_UNUSED(flags);
    Q_UNUSED(parent);
    Q_UNUSED(modality);

    qFileDialogHelperDebug() << Q_FUNC_INFO;

    // Create dialog
    const QSharedPointer<QFileDialogOptions> &opts = options();
    if (opts->acceptMode() == QFileDialogOptions::AcceptOpen) {
        // Select one or many files?
        const QQnxFilePicker::Mode mode = (opts->fileMode() == QFileDialogOptions::ExistingFiles)
            ? QQnxFilePicker::PickerMultiple : QQnxFilePicker::Picker;

        m_dialog->setMode(mode);

        // Set the actual list of extensions
        if (!opts->nameFilters().isEmpty())
            setNameFilters(opts->nameFilters());
        else
            setNameFilter(tr("All files (*.*)"));
    } else {
        const QQnxFilePicker::Mode mode = (opts->initiallySelectedFiles().count() >= 2)
            ? QQnxFilePicker::SaverMultiple : QQnxFilePicker::Saver;

        m_dialog->setMode(mode);

        if (!opts->initiallySelectedFiles().isEmpty()) {
            QStringList files;
            Q_FOREACH ( const QUrl &url, opts->initiallySelectedFiles() )
                files.append(url.toLocalFile());
            m_dialog->setDefaultSaveFileNames(files);
        }
    }

    // Cache the accept mode so we know which functions to use to get the results back
    m_acceptMode = opts->acceptMode();
    m_dialog->setTitle(opts->windowTitle());
    m_dialog->open();

    return true;
}

void QQnxFileDialogHelper::hide()
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    m_dialog->close();
}

bool QQnxFileDialogHelper::defaultNameFilterDisables() const
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    return false;
}

void QQnxFileDialogHelper::setDirectory(const QUrl &directory)
{
    m_dialog->addDirectory(directory.toLocalFile());
}

QUrl QQnxFileDialogHelper::directory() const
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    if (!m_dialog->directories().isEmpty())
        return QUrl::fromLocalFile(m_dialog->directories().first());

    return QUrl();
}

void QQnxFileDialogHelper::selectFile(const QUrl &fileName)
{
    m_dialog->addDefaultSaveFileName(fileName.toLocalFile());
}

QList<QUrl> QQnxFileDialogHelper::selectedFiles() const
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    QList<QUrl> urls;
    QStringList files = m_dialog->selectedFiles();
    Q_FOREACH (const QString &file, files)
        urls.append(QUrl::fromLocalFile(file));
    return urls;
}

void QQnxFileDialogHelper::setFilter()
{
    // No native api to support setting a filter from QDir::Filters
    qFileDialogHelperDebug() << Q_FUNC_INFO;
}

void QQnxFileDialogHelper::selectNameFilter(const QString &filter)
{
    qFileDialogHelperDebug() << Q_FUNC_INFO << "filter =" << filter;
    setNameFilter(filter);
}

QString QQnxFileDialogHelper::selectedNameFilter() const
{
    // For now there is no way for the user to change the selected filter
    // so this just reflects what the developer has set programmatically.
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    return m_selectedFilter;
}

void QQnxFileDialogHelper::emitSignals()
{
    if (m_dialog->selectedFiles().isEmpty())
        Q_EMIT reject();
    else
        Q_EMIT accept();
}

void QQnxFileDialogHelper::setNameFilter(const QString &filter)
{
    qFileDialogHelperDebug() << Q_FUNC_INFO << "filter =" << filter;

    setNameFilters(QPlatformFileDialogHelper::cleanFilterList(filter));
}

void QQnxFileDialogHelper::setNameFilters(const QStringList &filters)
{
    qFileDialogHelperDebug() << Q_FUNC_INFO << "filters =" << filters;

    Q_ASSERT(!filters.isEmpty());

    m_dialog->setFilters(filters);
    m_selectedFilter = filters.first();
}

QT_END_NAMESPACE
