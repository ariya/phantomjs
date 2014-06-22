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

#ifndef QQNXFILEPICKER_H
#define QQNXFILEPICKER_H

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QStringList>

struct navigator_invoke_invocation_t;

class QQnxFilePicker : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    explicit QQnxFilePicker(QObject *parent = 0);
    ~QQnxFilePicker();

    enum Mode {
        Picker,
        Saver,
        PickerMultiple,
        SaverMultiple
    };

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;

    void setMode(Mode mode);
    void setDefaultSaveFileNames(const QStringList &fileNames);
    void addDefaultSaveFileName(const QString &fileName);
    void setDirectories(const QStringList &directories);
    void addDirectory(const QString &directory);
    void setFilters(const QStringList &filters);
    void setTitle(const QString &title);

    Mode mode() const;

    QStringList defaultSaveFileNames() const;
    QStringList directories() const;
    QStringList filters() const;
    QStringList selectedFiles() const;

    QString title() const;

Q_SIGNALS:
    void closed();

public Q_SLOTS:
    void open();
    void close();

private:
    void cleanup();
    void handleFilePickerResponse(const char *data);
    QString filePickerType() const;

    QString modeToString(Mode mode) const;

    navigator_invoke_invocation_t *m_invocationHandle;

    Mode m_mode;

    QStringList m_defaultSaveFileNames;
    QStringList m_directories;
    QStringList m_filters;
    QStringList m_selectedFiles;

    QString m_title;
};

#endif // QQNXFILEPICKER_H
