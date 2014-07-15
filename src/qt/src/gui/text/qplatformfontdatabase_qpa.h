/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QPLATFORMFONTDATABASE_QPA_H
#define QPLATFORMFONTDATABASE_QPA_H

#include <QtCore/qconfig.h>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtGui/QFontDatabase>
#include <QtGui/private/qfont_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QWritingSystemsPrivate;

class Q_GUI_EXPORT QSupportedWritingSystems
{
public:

    QSupportedWritingSystems();
    QSupportedWritingSystems(const QSupportedWritingSystems &other);
    QSupportedWritingSystems &operator=(const QSupportedWritingSystems &other);
    ~QSupportedWritingSystems();

    void setSupported(QFontDatabase::WritingSystem, bool supported = true);
    bool supported(QFontDatabase::WritingSystem) const;

private:
    void detach();

    QWritingSystemsPrivate *d;

    friend Q_GUI_EXPORT bool operator==(const QSupportedWritingSystems &, const QSupportedWritingSystems &);
    friend Q_GUI_EXPORT bool operator!=(const QSupportedWritingSystems &, const QSupportedWritingSystems &);
};

Q_GUI_EXPORT bool operator==(const QSupportedWritingSystems &, const QSupportedWritingSystems &);
Q_GUI_EXPORT bool operator!=(const QSupportedWritingSystems &, const QSupportedWritingSystems &);

class QFontRequestPrivate;

class Q_GUI_EXPORT QPlatformFontDatabase
{
public:
    virtual void populateFontDatabase();
    virtual QFontEngine *fontEngine(const QFontDef &fontDef, QUnicodeTables::Script script, void *handle);
    virtual QStringList fallbacksForFamily(const QString family, const QFont::Style &style, const QFont::StyleHint &styleHint, const QUnicodeTables::Script &script) const;
    virtual QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName);
    virtual void releaseHandle(void *handle);

    virtual QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference);

    virtual QString fontDir() const;

    //callback
    static void registerQPF2Font(const QByteArray &dataArray, void *handle);
    static void registerFont(const QString &familyname, const QString &foundryname, QFont::Weight weight,
                             QFont::Style style, QFont::Stretch stretch, bool antialiased, bool scalable, int pixelSize,
                             const QSupportedWritingSystems &writingSystems, void *handle);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMFONTDATABASE_QPA_H
