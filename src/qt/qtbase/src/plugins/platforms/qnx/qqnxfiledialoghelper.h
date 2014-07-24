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

#ifndef QQNXFILEDIALOGHELPER_H
#define QQNXFILEDIALOGHELPER_H

#include <qpa/qplatformdialoghelper.h>


QT_BEGIN_NAMESPACE

class QQnxIntegration;

#if defined(Q_OS_BLACKBERRY_TABLET)
#include <bps/dialog.h>
#define NativeDialogPtr dialog_instance_t
#else
class QQnxFilePicker;
#define NativeDialogPtr QQnxFilePicker *
#endif

class QQnxFileDialogHelper : public QPlatformFileDialogHelper
{
    Q_OBJECT
public:
    explicit QQnxFileDialogHelper(const QQnxIntegration *);
    ~QQnxFileDialogHelper();

#if defined(Q_OS_BLACKBERRY_TABLET)
    bool handleEvent(bps_event_t *event);
#endif

    void exec();

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent);
    void hide();

    bool defaultNameFilterDisables() const;
    void setDirectory(const QUrl &directory) Q_DECL_OVERRIDE;
    QUrl directory() const Q_DECL_OVERRIDE;
    void selectFile(const QUrl &fileName) Q_DECL_OVERRIDE;
    QList<QUrl> selectedFiles() const Q_DECL_OVERRIDE;
    void setFilter();
    void selectNameFilter(const QString &filter);
    QString selectedNameFilter() const;

    NativeDialogPtr nativeDialog() const { return m_dialog; }

Q_SIGNALS:
    void dialogClosed();

private Q_SLOTS:
    void emitSignals();

private:
    void setNameFilter(const QString &filter);
    void setNameFilters(const QStringList &filters);

    const QQnxIntegration *m_integration;
    NativeDialogPtr m_dialog;
    QFileDialogOptions::AcceptMode m_acceptMode;
    QString m_selectedFilter;

#if defined(Q_OS_BLACKBERRY_TABLET)
    QPlatformDialogHelper::DialogCode m_result;
    QList<QUrl> m_paths;
#endif
};

QT_END_NAMESPACE

#endif // QQNXFILEDIALOGHELPER_H
