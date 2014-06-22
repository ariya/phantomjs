/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "cesdkhandler.h"

#include <qfile.h>
#include <qdebug.h>
#include <qxmlstream.h>

QT_BEGIN_NAMESPACE

CeSdkInfo::CeSdkInfo() : m_major(0) , m_minor(0)
{
}

CeSdkHandler::CeSdkHandler()
{
}

bool CeSdkHandler::parse()
{
    // look at the file at %VCInstallDir%/vcpackages/WCE.VCPlatform.config
    // and scan through all installed sdks...
    m_list.clear();
    m_vcInstallDir = QString::fromLatin1(qgetenv("VCInstallDir"));
    if (m_vcInstallDir.isEmpty())
        return false;

    QDir vStudioDir(m_vcInstallDir);
    if (!vStudioDir.cd(QLatin1String("vcpackages")))
        return false;

    QFile configFile(vStudioDir.absoluteFilePath(QLatin1String("WCE.VCPlatform.config")));
    if (!configFile.open(QIODevice::ReadOnly))
        return false;

    QString currentElement;
    CeSdkInfo currentItem;
    QXmlStreamReader xml(&configFile);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            currentElement = xml.name().toString();
            if (currentElement == QLatin1String("Platform")) {
                currentItem = CeSdkInfo();
            } else if (currentElement == QLatin1String("Directories")) {
                QXmlStreamAttributes attr = xml.attributes();
                currentItem.m_include = fixPaths(attr.value(QLatin1String("Include")).toString());
                currentItem.m_lib = fixPaths(attr.value(QLatin1String("Library")).toString());
                currentItem.m_bin = fixPaths(attr.value(QLatin1String("Path")).toString());
            }
        } else if (xml.isEndElement()) {
            if (xml.name().toString() == QLatin1String("Platform"))
                m_list.append(currentItem);
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (currentElement == QLatin1String("PlatformName"))
                currentItem.m_name = xml.text().toString();
            else if (currentElement == QLatin1String("OSMajorVersion"))
                currentItem.m_major = xml.text().toString().toInt();
            else if (currentElement == QLatin1String("OSMinorVersion"))
                currentItem.m_minor = xml.text().toString().toInt();
        }
    }

    if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
        return false;
    }

    return m_list.size() > 0 ? true : false;
}

QString CeSdkHandler::fixPaths(QString path) const
{
    QRegExp searchStr(QLatin1String("(\\$\\(\\w+\\))"));
    QString fixedString = path;
    for (int index = fixedString.indexOf(searchStr, 0);
         index >= 0;
         index = fixedString.indexOf(searchStr, index)) {
        const QString capture = searchStr.cap(0);
        fixedString.replace(index, capture.length(), capture.toUpper());
        index += capture.length(); // don't count the zero terminator
        fixedString.insert(index, '\\'); // the configuration file lacks a directory separator for env vars
        ++index;
    }
    return fixedString;
}

QT_END_NAMESPACE
