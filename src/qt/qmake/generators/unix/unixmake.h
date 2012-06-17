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

#ifndef UNIXMAKE_H
#define UNIXMAKE_H

#include "makefile.h"

QT_BEGIN_NAMESPACE

class UnixMakefileGenerator : public MakefileGenerator
{
    bool init_flag, include_deps;
    QString libtoolFileName(bool fixify=true);
    void writeLibtoolFile();     // for libtool
    void writePrlFile(QTextStream &);

public:
    UnixMakefileGenerator();
    ~UnixMakefileGenerator();

protected:
    virtual bool doPrecompiledHeaders() const { return project->isActiveConfig("precompile_header"); }
    virtual bool doDepends() const { return !include_deps && !Option::mkfile::do_stub_makefile && MakefileGenerator::doDepends(); }
    virtual QString defaultInstall(const QString &);
    virtual void processPrlVariable(const QString &, const QStringList &);
    virtual void processPrlFiles();

    virtual bool findLibraries();
    virtual QString escapeFilePath(const QString &path) const;
    virtual QStringList &findDependencies(const QString &);
    virtual void init();

    void writeMakeParts(QTextStream &);
    bool writeMakefile(QTextStream &);

private:
    void init2();
};

inline UnixMakefileGenerator::~UnixMakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // UNIXMAKE_H
