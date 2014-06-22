/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxbpseventfilter.h"
#include "qqnxscreen.h"
#include "qqnxintegration.h"

#include <QDebug>
#include <QEventLoop>
#include <QScreen>
#include <QTimer>
#include <QWindow>

#if defined(QQNXFILEDIALOGHELPER_DEBUG)
#define qFileDialogHelperDebug qDebug
#else
#define qFileDialogHelperDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxFileDialogHelper::QQnxFileDialogHelper(const QQnxIntegration *integration)
    : QPlatformFileDialogHelper(),
      m_integration(integration),
      m_dialog(0),
      m_acceptMode(QFileDialogOptions::AcceptOpen),
      m_selectedFilter(),
      m_result(QPlatformDialogHelper::Rejected),
      m_paths()
{
}

QQnxFileDialogHelper::~QQnxFileDialogHelper()
{
    if (m_dialog)
        dialog_destroy(m_dialog);
}

bool QQnxFileDialogHelper::handleEvent(bps_event_t *event)
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;

    // Check dialog event response type (OK vs CANCEL)
    // CANCEL => index = 0
    //     OK => index = 1
    int index = dialog_event_get_selected_index(event);
    qFileDialogHelperDebug() << "Index =" << index;
    if (index == 1) {
        m_result = QPlatformDialogHelper::Accepted;

        if (m_acceptMode == QFileDialogOptions::AcceptOpen) {
            // File open dialog

            // ###TODO Check that this actually gets multiple paths and cleans up properly
            char **filePaths = 0;
            int pathCount = 0;
            int result = dialog_event_get_filebrowse_filepaths(event, &filePaths, &pathCount);
            if (result != BPS_SUCCESS) {
                qWarning() << "Could not get paths from native file dialog";
                return false;
            }

            for (int i = 0; i < pathCount; ++i) {
                QString path = QFile::decodeName(filePaths[i]);
                m_paths.append(QUrl::fromLocalFile(path));
                qFileDialogHelperDebug() << "path =" << path;
            }

            bps_free(filePaths);
        } else {
            // File save dialog
            const char *filePath = dialog_event_get_filesave_filepath(event);
            QString path = QFile::decodeName(filePath);
            qFileDialogHelperDebug() << "path =" << path;
            m_paths.append(QUrl::fromLocalFile(path));
        }
    } else { // Cancel
        m_result = QPlatformDialogHelper::Rejected;
    }

    Q_EMIT dialogClosed();

    return true;
}

void QQnxFileDialogHelper::exec()
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;

    // Clear any previous results
    m_paths.clear();

    QEventLoop loop;
    connect(this, SIGNAL(dialogClosed()), &loop, SLOT(quit()));
    loop.exec();

    if (m_result == QPlatformDialogHelper::Accepted)
        Q_EMIT accept();
    else
        Q_EMIT reject();
}

bool QQnxFileDialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    Q_UNUSED(flags);
    qFileDialogHelperDebug() << Q_FUNC_INFO;

    QQnxBpsEventFilter *eventFilter = m_integration->bpsEventFilter();
    // We *really* need the bps event filter ;)
    if (!eventFilter)
        return false;

    // Native dialogs can only handle application modal use cases so far
    if (modality != Qt::ApplicationModal)
        return false;

    // Tear down any existing dialog and start again as dialog mode may have changed
    if (m_dialog) {
        dialog_destroy(m_dialog);
        m_dialog = 0;
    }

    // Create dialog
    const QSharedPointer<QFileDialogOptions> &opts = options();
    if (opts->acceptMode() == QFileDialogOptions::AcceptOpen) {
        if (dialog_create_filebrowse(&m_dialog) != BPS_SUCCESS) {
            qWarning("dialog_create_filebrowse failed");
            return false;
        }

        // Select one or many files?
        bool multiSelect = (opts->fileMode() == QFileDialogOptions::ExistingFiles);
        dialog_set_filebrowse_multiselect(m_dialog, multiSelect);

        // Set the actual list of extensions
        if (!opts->nameFilters().isEmpty()) {
            qFileDialogHelperDebug() << "nameFilters =" << opts->nameFilters();
            setNameFilter(opts->nameFilters().first());
        } else {
            QString defaultNameFilter = QStringLiteral("*.*");
            setNameFilter(defaultNameFilter);
        }
    } else {
        if (dialog_create_filesave(&m_dialog) != BPS_SUCCESS) {
            qWarning("dialog_create_filesave failed");
            return false;
        }

        // Maybe pre-select a filename
        if (!opts->initiallySelectedFiles().isEmpty()) {
            QString fileName = opts->initiallySelectedFiles().first().toLocalFile();
            dialog_set_filesave_filename(m_dialog, QFile::encodeName(fileName).constData());
        }

        // Add OK and Cancel buttons. We add the buttons in the order "CANCEL" followed by "OK
        // such that they have indices matching the buttons on the file open dialog which
        // is automatically populated with buttons.
        if (dialog_add_button(m_dialog, tr("CANCEL").toLocal8Bit().constData(), true, 0, true) != BPS_SUCCESS) {
            qWarning("dialog_add_button failed");
            return false;
        }

        if (dialog_add_button(m_dialog, tr("OK").toLocal8Bit().constData(), true, 0, true) != BPS_SUCCESS) {
            qWarning("dialog_add_button failed");
            return false;
        }
    }

    // Cache the accept mode so we know which functions to use to get the results back
    m_acceptMode = opts->acceptMode();

    // Set the libscreen window group and common properties

    QQnxScreen *nativeScreen = parent ? static_cast<QQnxScreen *>(parent->screen()->handle()) :
                                        m_integration->primaryDisplay();
    Q_ASSERT(nativeScreen);
    dialog_set_group_id(m_dialog, nativeScreen->windowGroupName());
    dialog_set_title_text(m_dialog, opts->windowTitle().toLocal8Bit().constData());

    // Register ourselves for dialog domain events from bps
    eventFilter->registerForDialogEvents(this);

    // Show the dialog
    dialog_show(m_dialog);

    return true;
}

void QQnxFileDialogHelper::hide()
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    if (!m_dialog)
        return;
    dialog_cancel(m_dialog);
}

bool QQnxFileDialogHelper::defaultNameFilterDisables() const
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    return false;
}

void QQnxFileDialogHelper::setDirectory(const QUrl &directory)
{
    qFileDialogHelperDebug() << Q_FUNC_INFO << "directory =" << directory;
    // No native API for setting the directory(!). The best we can do is to
    // set it as the file name but even then only with a file save dialog.
    if (m_dialog && m_acceptMode == QFileDialogOptions::AcceptSave)
        dialog_set_filesave_filename(m_dialog, QFile::encodeName(directory.toLocalFile()).constData());
}

QUrl QQnxFileDialogHelper::directory() const
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    return m_paths.first();
}

void QQnxFileDialogHelper::selectFile(const QUrl &fileName)
{
    qFileDialogHelperDebug() << Q_FUNC_INFO << "filename =" << fileName;
    if (m_dialog && m_acceptMode == QFileDialogOptions::AcceptSave)
        dialog_set_filesave_filename(m_dialog, QFile::encodeName(fileName.toLocalFile()).constData());
}

QList<QUrl> QQnxFileDialogHelper::selectedFiles() const
{
    qFileDialogHelperDebug() << Q_FUNC_INFO;
    return m_paths;
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

void QQnxFileDialogHelper::setNameFilter(const QString &filter)
{
    qFileDialogHelperDebug() << Q_FUNC_INFO << "filter =" << filter;
    setNameFilters(QPlatformFileDialogHelper::cleanFilterList(filter));
}

void QQnxFileDialogHelper::setNameFilters(const QStringList &filters)
{
    qFileDialogHelperDebug() << Q_FUNC_INFO << "filters =" << filters;

    Q_ASSERT(!filters.isEmpty());

    char **globs = new char*[filters.size()];
    for (int i = 0; i < filters.size(); ++i) {
        QByteArray glob = filters.at(i).toLocal8Bit();
        globs[i] = new char[glob.length()];
        strcpy(globs[i], glob.constData());
    }

    // Set the filters
    dialog_set_filebrowse_filter(m_dialog, const_cast<const char**>(globs), filters.size());
    m_selectedFilter = filters.first();

    // Cleanup
    for (int i = 0; i < filters.size(); ++i)
        delete[] globs[i];
    delete[] globs;
}

QT_END_NAMESPACE
