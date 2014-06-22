/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake application of the Qt Toolkit.
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
    void writeMingwParts(QTextStream &);
    void writeIncPart(QTextStream &t);
    void writeLibsPart(QTextStream &t);
    void writeObjectsPart(QTextStream &t);
    void writeBuildRulesPart(QTextStream &t);
    void writeRcFilePart(QTextStream &t);

    QStringList &findDependencies(const QString &file);

    QString preCompHeaderOut;

    virtual bool findLibraries();

    bool init_flag;
    QString objectsLinkLine;
    QString quote;
};

inline MingwMakefileGenerator::~MingwMakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // MINGW_MAKE_H
