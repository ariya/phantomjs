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

#include "tools.h"
#include "environment.h"

#include <qdir.h>
#include <qfile.h>
#include <qbytearray.h>
#include <qstringlist.h>

#include <iostream>

std::ostream &operator<<(std::ostream &s, const QString &val);
using namespace std;

void Tools::checkLicense(QMap<QString,QString> &dictionary,
                         const QString &sourcePath, const QString &buildPath)
{
    QString tpLicense = sourcePath + "/LICENSE.PREVIEW.COMMERCIAL";
    if (QFile::exists(tpLicense)) {
        dictionary["EDITION"] = "Preview";
        dictionary["LICENSE FILE"] = tpLicense;
        return;
    }

    const QString licenseChecker =
        QDir::toNativeSeparators(sourcePath + "/bin/licheck.exe");

    if (QFile::exists(licenseChecker)) {
        const QString qMakeSpec =
            QDir::toNativeSeparators(dictionary.value("QMAKESPEC"));
        const QString xQMakeSpec =
            QDir::toNativeSeparators(dictionary.value("XQMAKESPEC"));

        QString command = QString("%1 %2 %3 %4 %5 %6")
            .arg(licenseChecker,
                 dictionary.value("LICENSE_CONFIRMED", "no"),
                 QDir::toNativeSeparators(sourcePath),
                 QDir::toNativeSeparators(buildPath),
                 qMakeSpec, xQMakeSpec);

        int returnValue = 0;
        QString licheckOutput = Environment::execute(command, &returnValue);

        if (returnValue) {
            dictionary["DONE"] = "error";
        } else {
            foreach (const QString &var, licheckOutput.split('\n'))
                dictionary[var.section('=', 0, 0).toUpper()] = var.section('=', 1, 1);
        }
    } else {
        cout << endl << "Error: Could not find licheck.exe" << endl
             << "Try re-installing." << endl << endl;
        dictionary["DONE"] = "error";
    }
}

