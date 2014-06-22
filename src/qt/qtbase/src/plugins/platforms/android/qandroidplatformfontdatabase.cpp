/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#include <QDir>

#include "qandroidplatformfontdatabase.h"

QString QAndroidPlatformFontDatabase::fontDir() const
{
    return QLatin1String("/system/fonts");
}

void QAndroidPlatformFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();

    if (!QFile::exists(fontpath)) {
        qFatal("QFontDatabase: Cannot find font directory %s - is Qt installed correctly?",
               qPrintable(fontpath));
    }

    QDir dir(fontpath, QLatin1String("*.ttf"));
    for (int i = 0; i < int(dir.count()); ++i) {
        const QByteArray file = QFile::encodeName(dir.absoluteFilePath(dir[i]));

        QSupportedWritingSystems supportedWritingSystems;
        QStringList families = addTTFile(QByteArray(), file, &supportedWritingSystems);

        extern int qt_script_for_writing_system(QFontDatabase::WritingSystem writingSystem);
        for (int i = 0; i < QFontDatabase::WritingSystemsCount; ++i) {
            if (i == QFontDatabase::Any || supportedWritingSystems.supported(QFontDatabase::WritingSystem(i))) {
                QChar::Script script = QChar::Script(qt_script_for_writing_system(QFontDatabase::WritingSystem(i)));
                m_fallbacks[script] += families;
            }
        }
    }
}

QStringList QAndroidPlatformFontDatabase::fallbacksForFamily(const QString &family,
                                                             QFont::Style style,
                                                             QFont::StyleHint styleHint,
                                                             QChar::Script script) const
{
    Q_UNUSED(family);
    Q_UNUSED(style);

    if (styleHint == QFont::Monospace)
        return QString(qgetenv("QT_ANDROID_FONTS_MONOSPACE")).split(";") + m_fallbacks[script];

    return QString(qgetenv("QT_ANDROID_FONTS")).split(";") + m_fallbacks[script];
}
