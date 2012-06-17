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

#ifndef SYMMAKEFILE_H
#define SYMMAKEFILE_H

#include "initprojectdeploy_symbian.h"
#include "symbiancommon.h"
#include <makefile.h>

QT_BEGIN_NAMESPACE

#define BLD_INF_FILENAME "bld.inf"
#define MAKEFILE_DEPENDENCY_SEPARATOR " \\\n\t"
#define QT_EXTRA_INCLUDE_DIR "tmp"
#define MAKE_CACHE_NAME ".make.cache"
#define SYMBIAN_TEST_CONFIG "symbian_test"

class SymbianMakefileGenerator : public MakefileGenerator, public SymbianCommonGenerator
{
protected:
    QString platform;
    QString uid2;
    QString mmpFileName;
    QMap<QString, QStringList> sources;
    QMap<QString, QStringList> systeminclude;
    QMap<QString, QStringList> library;
    // (output file) (source , command)
    QMap<QString, QStringList> makmakeCommands;
    QStringList overriddenMmpKeywords;
    QDir outputDir;

    QString fixPathForMmp(const QString& origPath, const QDir& parentDir);
    QString absolutizePath(const QString& origPath);

    virtual bool writeMakefile(QTextStream &t);

    virtual void init();

    QString getTargetExtension();

    QString generateUID3();

    void initMmpVariables();
    void generateMmpFileName();
    void handleMmpRulesOverrides(QString &checkString,
                                 bool &inResourceBlock,
                                 QStringList &restrictedMmpKeywords,
                                 const QStringList &restrictableMmpKeywords,
                                 const QStringList &overridableMmpKeywords);
    void appendKeywordIfMatchFound(QStringList &list,
                                   const QStringList &keywordList,
                                   QString &checkString);

    void writeHeader(QTextStream &t);
    void writeBldInfContent(QTextStream& t,
                            bool addDeploymentExtension,
                            const QString &iconFile);

    static bool removeDuplicatedStrings(QStringList& stringList);

    void writeMmpFileHeader(QTextStream &t);
    void writeMmpFile(QString &filename, const SymbianLocalizationList &symbianLocalizationList);
    void writeMmpFileMacrosPart(QTextStream& t);
    void addMacro(QTextStream& t, const QString& value);
    void writeMmpFileTargetPart(QTextStream& t);
    void writeMmpFileResourcePart(QTextStream& t, const SymbianLocalizationList &symbianLocalizationList);
    void writeMmpFileSystemIncludePart(QTextStream& t);
    void writeMmpFileIncludePart(QTextStream& t);
    void writeMmpFileLibraryPart(QTextStream& t);
    void writeMmpFileCapabilityPart(QTextStream& t);
    void writeMmpFileConditionalOptions(QTextStream& t,
                                        const QString &optionType,
                                        const QString &optionTag,
                                        const QString &variableBase);
    void writeMmpFileSimpleOption(QTextStream& t,
                                  const QString &optionType,
                                  const QString &optionTag,
                                  const QString &options);
    void appendMmpFileOptions(QString &options, const QStringList &list);
    void writeMmpFileCompilerOptionPart(QTextStream& t);
    void writeMmpFileBinaryVersionPart(QTextStream& t);
    void writeMmpFileRulesPart(QTextStream& t);

    void appendIfnotExist(QStringList &list, QString value);
    void appendIfnotExist(QStringList &list, QStringList values);

    QString removeTrailingPathSeparators(QString &file);
    void generateCleanCommands(QTextStream& t,
                               const QStringList& toClean,
                               const QString& cmd,
                               const QString& cmdOptions,
                               const QString& itemPrefix,
                               const QString& itemSuffix);

    void generateDistcleanTargets(QTextStream& t);
    QString generateLocFileTarget(QTextStream& t, const QString& locCmd);

    // Subclass implements
    virtual void writeBldInfExtensionRulesPart(QTextStream& t, const QString &iconTargetFile) = 0;
    virtual void writeBldInfMkFilePart(QTextStream& t, bool addDeploymentExtension) = 0;
    virtual void writeMkFile(const QString& wrapperFileName, bool deploymentOnly) = 0;
    virtual void writeWrapperMakefile(QFile& wrapperFile, bool isPrimaryMakefile) = 0;
    virtual void appendAbldTempDirs(QStringList& sysincspaths, QString includepath) = 0;

public:

    SymbianMakefileGenerator();
    ~SymbianMakefileGenerator();
};

#endif // SYMMAKEFILE_H
