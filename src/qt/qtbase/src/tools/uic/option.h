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

#ifndef OPTION_H
#define OPTION_H

#include <qstring.h>
#include <qdir.h>

QT_BEGIN_NAMESPACE

struct Option
{
    enum Generator
    {
        CppGenerator,
        JavaGenerator
    };

    unsigned int headerProtection : 1;
    unsigned int copyrightHeader : 1;
    unsigned int generateImplemetation : 1;
    unsigned int generateNamespace : 1;
    unsigned int autoConnection : 1;
    unsigned int dependencies : 1;
    unsigned int extractImages : 1;
    unsigned int limitXPM_LineLength : 1;
    unsigned int implicitIncludes: 1;
    Generator generator;

    QString inputFile;
    QString outputFile;
    QString qrcOutputFile;
    QString indent;
    QString prefix;
    QString postfix;
    QString translateFunction;
    QString includeFile;
#ifdef QT_UIC_JAVA_GENERATOR
    QString javaPackage;
    QString javaOutputDirectory;
#endif

    Option()
        : headerProtection(1),
          copyrightHeader(1),
          generateImplemetation(0),
          generateNamespace(1),
          autoConnection(1),
          dependencies(0),
          extractImages(0),
          limitXPM_LineLength(0),
          implicitIncludes(1),
          generator(CppGenerator),
          prefix(QLatin1String("Ui_"))
    { indent.fill(QLatin1Char(' '), 4); }

    QString messagePrefix() const
    {
        return inputFile.isEmpty() ?
               QString(QLatin1String("stdin")) :
               QDir::toNativeSeparators(inputFile);
    }
};

QT_END_NAMESPACE

#endif // OPTION_H
