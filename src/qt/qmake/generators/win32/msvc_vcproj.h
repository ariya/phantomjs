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

#ifndef MSVC_VCPROJ_H
#define MSVC_VCPROJ_H

#include "winmakefile.h"
#include "msvc_objectmodel.h"

QT_BEGIN_NAMESPACE

enum Target {
    Application,
    SharedLib,
    StaticLib
};

struct QUuid;
class VcprojGenerator : public Win32MakefileGenerator
{
    bool init_flag;
    bool is64Bit;
    bool writeVcprojParts(QTextStream &);

    bool writeMakefile(QTextStream &);
    bool writeProjectMakefile();

    QString findTemplate(QString file);
    void init();

public:
    VcprojGenerator();
    ~VcprojGenerator();

    QString defaultMakefile() const;
    QString precompH, precompHFilename, precompCPP,
            precompObj, precompPch;
    bool autogenPrecompCPP;
    static bool hasBuiltinCompiler(const QString &file);

    QMap<QString, QStringList> extraCompilerSources;
    QMap<QString, QStringList> extraCompilerOutputs;
    bool usePCH;
    VCProjectWriter *projectWriter;

protected:
    virtual VCProjectWriter *createProjectWriter();
    virtual bool doDepends() const { return false; } //never necesary
    virtual void processSources() { filterIncludedFiles("SOURCES"); filterIncludedFiles("GENERATED_SOURCES"); }
    virtual QString replaceExtraCompilerVariables(const QString &, const QStringList &, const QStringList &);
    inline QString replaceExtraCompilerVariables(const QString &val, const QString &in, const QString &out)
    { return MakefileGenerator::replaceExtraCompilerVariables(val, in, out); }
    virtual bool supportsMetaBuild() { return true; }
    virtual bool supportsMergedBuilds() { return true; }
    virtual bool mergeBuildProject(MakefileGenerator *other);

    virtual bool openOutput(QFile &file, const QString &build) const;
    virtual bool findLibraries();
    virtual void outputVariables();
    QString fixFilename(QString ofile) const;

    void initOld();
    virtual void initProject();
    void initConfiguration();
    void initCompilerTool();
    void initLinkerTool();
    void initLibrarianTool();
    void initResourceTool();
    void initIDLTool();
    void initCustomBuildTool();
    void initPreBuildEventTools();
    void initPostBuildEventTools();
    void initDeploymentTool();
    void initPreLinkEventTools();
    void initRootFiles();
    void initSourceFiles();
    void initHeaderFiles();
    void initGeneratedFiles();
    void initTranslationFiles();
    void initFormFiles();
    void initResourceFiles();
    void initLexYaccFiles();
    void initExtraCompilerOutputs();

    void writeSubDirs(QTextStream &t); // Called from VCXProj backend
    QUuid getProjectUUID(const QString &filename=QString()); // Called from VCXProj backend

    Target projectTarget;

    // Used for single project
    VCProjectSingleConfig vcProject;

    // Holds all configurations for glue (merged) project
    QList<VcprojGenerator*> mergedProjects;

private:
    QStringList collectSubDirs(QMakeProject *proj);
    QUuid increaseUUID(const QUuid &id);
    friend class VCFilter;
};

inline QString VcprojGenerator::defaultMakefile() const
{
    return project->first("TARGET") + project->first("VCPROJ_EXTENSION");
}

inline bool VcprojGenerator::findLibraries()
{
    return Win32MakefileGenerator::findLibraries("MSVCPROJ_LIBS");
}

QT_END_NAMESPACE

#endif // MSVC_VCPROJ_H
