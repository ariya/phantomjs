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

#ifndef SYMBIANCOMMON_H
#define SYMBIANCOMMON_H

#include <project.h>
#include <makefile.h>
#include "initprojectdeploy_symbian.h"

#define PRINT_FILE_CREATE_ERROR(filename) fprintf(stderr, "Error: Could not create '%s'\n", qPrintable(filename));

class SymbianLocalization
{
public:
    QString qtLanguageCode;
    QString symbianLanguageCode;
    QString shortCaption;
    QString longCaption;
    QString pkgDisplayName;
    QString installerPkgDisplayName;
};

typedef QList<SymbianLocalization> SymbianLocalizationList;
typedef QListIterator<SymbianLocalization> SymbianLocalizationListIterator;

class SymbianCommonGenerator
{
public:
    enum TargetType {
        TypeExe,
        TypeDll,
        TypeLib,
        TypePlugin,
        TypeSubdirs
    };


    SymbianCommonGenerator(MakefileGenerator *generator);

    virtual void init();

protected:

    QString removePathSeparators(QString &file);
    void removeSpecialCharacters(QString& str);
    void generatePkgFile(const QString &iconFile,
                         bool epocBuild,
                         const SymbianLocalizationList &symbianLocalizationList);
    bool containsStartWithItem(const QChar &c, const QStringList& src);

    void writeRegRssFile(QMap<QString, QStringList> &useritems);
    void writeRegRssList(QTextStream &t, QStringList &userList,
                         const QString &listTag,
                         const QString &listItem);
    void writeRssFile(QString &numberOfIcons, QString &iconfile);
    void writeLocFile(const SymbianLocalizationList &symbianLocalizationList);
    void readRssRules(QString &numberOfIcons,
                      QString &iconFile,
                      QMap<QString, QStringList> &userRssRules);

    void writeCustomDefFile();

    void parseTsFiles(SymbianLocalizationList *symbianLocalizationList);
    void fillQt2SymbianLocalizationList(SymbianLocalizationList *symbianLocalizationList);

    void parsePreRules(const QString &deploymentVariable,
                       const QString &variableSuffix,
                       QStringList *rawRuleList,
                       QStringList *languageRuleList,
                       QStringList *headerRuleList,
                       QStringList *vendorRuleList);
    void parsePostRules(const QString &deploymentVariable,
                        const QString &variableSuffix,
                        QStringList *rawRuleList);
    bool parseTsContent(const QString &tsFilename, SymbianLocalization *loc);
    QString generatePkgNameForHeader(const SymbianLocalizationList &symbianLocalizationList,
                                     const QString &defaultName,
                                     bool isForSmartInstaller);
    void addLocalizedResourcesToDeployment(const QString &deploymentFilesVar,
                                           const SymbianLocalizationList &symbianLocalizationList);
    QString generateLocFileName();


protected:
    MakefileGenerator *generator;

    QStringList generatedFiles;
    QStringList generatedDirs;
    QString fixedTarget;
    QString privateDirUid;
    QString uid3;
    TargetType targetType;
};

#endif // SYMBIANCOMMON_H
