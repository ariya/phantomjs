/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MINGW_MAKE_H
#define MINGW_MAKE_H

#include "winmakefile.h"

QT_BEGIN_NAMESPACE

class MingwMakefileGenerator : public Win32MakefileGenerator
{
public:
    MingwMakefileGenerator();
    ~MingwMakefileGenerator();
protected:
    QString escapeDependencyPath(const QString &path) const;
    QString getLibTarget();
    bool writeMakefile(QTextStream &);
    void init();
private:
    bool isWindowsShell() const;
    void writeMingwParts(QTextStream &);
    void writeIncPart(QTextStream &t);
    void writeLibsPart(QTextStream &t);
    void writeLibDirPart(QTextStream &t);
    void writeObjectsPart(QTextStream &t);
    void writeBuildRulesPart(QTextStream &t);
    void writeRcFilePart(QTextStream &t);
    void processPrlVariable(const QString &var, const QStringList &l);

    QStringList &findDependencies(const QString &file);
    
    QString preCompHeaderOut;

    virtual bool findLibraries();
    bool findLibraries(const QString &where);
    void fixTargetExt();

    bool init_flag;
    QString objectsLinkLine;
    QString quote;
};

inline MingwMakefileGenerator::~MingwMakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // MINGW_MAKE_H
