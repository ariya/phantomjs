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

#ifndef SYMMAKE_SBSV2_H
#define SYMMAKE_SBSV2_H

#include <symmake.h>

QT_BEGIN_NAMESPACE

class SymbianSbsv2MakefileGenerator : public SymbianMakefileGenerator
{
protected:

    // Inherited from parent
    virtual void writeBldInfExtensionRulesPart(QTextStream& t, const QString &iconTargetFile);
    virtual void writeBldInfMkFilePart(QTextStream& t, bool addDeploymentExtension);
    virtual void writeMkFile(const QString& wrapperFileName, bool deploymentOnly);
    virtual void writeWrapperMakefile(QFile& wrapperFile, bool isPrimaryMakefile);
    virtual void appendAbldTempDirs(QStringList& sysincspaths, QString includepath);
    virtual bool isForSymbianSbsv2() const { return true; } // FIXME: killme - i'm ugly!

public:

    SymbianSbsv2MakefileGenerator();
    ~SymbianSbsv2MakefileGenerator();

private:
    void exportFlm();
    void findGcceVersions(QStringList *gcceVersionList, QString *defaultVersion);
    void findRvctVersions(QStringList *rvctVersionList, QString *defaultVersion);
    void findInstalledCompilerVersions(const QString &matchExpression,
                                       const QString &versionPrefix,
                                       QStringList *versionList);
    QString configClause(const QString &platform,
                         const QString &build,
                         const QString &compilerVersion,
                         const QString &clauseTemplate);

    void writeSbsDeploymentList(const DeploymentList& depList, QTextStream& t);

    QString extraTargetsCache;
    QString extraCompilersCache;
};

#endif // SYMMAKE_SBSV2_H
