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

/*
  config.h
*/

#ifndef CONFIG_H
#define CONFIG_H

#include <qmap.h>
#include <qset.h>
#include <qstringlist.h>
#include <qstack.h>
#include <qpair.h>
#include "location.h"

QT_BEGIN_NAMESPACE

/*
  This struct contains all the information for
  one config variable found in a qdocconf file.
 */
struct ConfigVar {
    bool plus_;
    QString name_;
    QStringList values_;
    QString currentPath_;
    Location location_;

  ConfigVar() : plus_(false) { }

  ConfigVar(const QString& name, const QStringList& values, const QString& dir)
    : plus_(true), name_(name), values_(values), currentPath_(dir) { }

  ConfigVar(const QString& name, const QStringList& values, const QString& dir, const Location& loc)
    : plus_(false), name_(name), values_(values), currentPath_(dir), location_(loc) { }
};

/*
  In this multimap, the key is a config variable name.
 */
typedef QMultiMap<QString, ConfigVar> ConfigVarMultimap;

class Config
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::Config)

public:
    Config(const QString& programName);
    ~Config();

    void load(const QString& fileName);
    void setStringList(const QString& var, const QStringList& values);

    const QString& programName() const { return prog; }
    const Location& location() const { return loc; }
    const Location& lastLocation() const { return lastLocation_; }
    bool getBool(const QString& var) const;
    int getInt(const QString& var) const;
    QString getOutputDir() const;
    QSet<QString> getOutputFormats() const;
    QString getString(const QString& var) const;
    QSet<QString> getStringSet(const QString& var) const;
    QStringList getStringList(const QString& var) const;
    QStringList getCanonicalPathList(const QString& var) const;
    QStringList getCleanPathList(const QString& var) const;
    QStringList getPathList(const QString& var) const;
    QRegExp getRegExp(const QString& var) const;
    QList<QRegExp> getRegExpList(const QString& var) const;
    QSet<QString> subVars(const QString& var) const;
    void subVarsAndValues(const QString& var, ConfigVarMultimap& t) const;
    QStringList getAllFiles(const QString& filesVar,
                            const QString& dirsVar,
                            const QSet<QString> &excludedDirs = QSet<QString>(),
                            const QSet<QString> &excludedFiles = QSet<QString>());
    QString getIncludeFilePath(const QString& fileName) const;
    QStringList getExampleQdocFiles(const QSet<QString> &excludedDirs, const QSet<QString> &excludedFiles);
    QStringList getExampleImageFiles(const QSet<QString> &excludedDirs, const QSet<QString> &excludedFiles);

    static QStringList getFilesHere(const QString& dir,
                                    const QString& nameFilter,
                                    const Location &location = Location(),
                                    const QSet<QString> &excludedDirs = QSet<QString>(),
                                    const QSet<QString> &excludedFiles = QSet<QString>());
    static QString findFile(const Location& location,
                            const QStringList &files,
                            const QStringList& dirs,
                            const QString& fileName,
                            QString& userFriendlyFilePath);
    static QString findFile(const Location &location,
                            const QStringList &files,
                            const QStringList &dirs,
                            const QString &fileBase,
                            const QStringList &fileExtensions,
                            QString &userFriendlyFilePath);
    static QString copyFile(const Location& location,
                            const QString& sourceFilePath,
                            const QString& userFriendlySourceFilePath,
                            const QString& targetDirPath);
    static int numParams(const QString& value);
    static bool removeDirContents(const QString& dir);
    static void pushWorkingDir(const QString& dir);
    static QString popWorkingDir();

    QT_STATIC_CONST QString dot;

    static bool generateExamples;
    static QString installDir;
    static QString overrideOutputDir;
    static QSet<QString> overrideOutputFormats;

private:
    static bool isMetaKeyChar(QChar ch);
    void load(Location location, const QString& fileName);

    QString prog;
    Location loc;
    Location lastLocation_;
    ConfigVarMultimap   configVars_;

    static QMap<QString, QString> uncompressedFiles;
    static QMap<QString, QString> extractedDirs;
    static int numInstances;
    static QStack<QString> workingDirs_;
    static QMap<QString, QStringList> includeFilesMap_;
};

struct ConfigStrings
{
    static QString ALIAS;
    static QString BASE;
    static QString BASEDIR;
    static QString BUILDVERSION;
    static QString CODEINDENT;
    static QString CPPCLASSESPAGE;
    static QString DEFINES;
    static QString DEPENDS;
    static QString DESCRIPTION;
    static QString EDITION;
    static QString ENDHEADER;
    static QString EXAMPLEDIRS;
    static QString EXAMPLES;
    static QString EXAMPLESINSTALLPATH;
    static QString EXCLUDEDIRS;
    static QString EXCLUDEFILES;
    static QString EXTRAIMAGES;
    static QString FALSEHOODS;
    static QString FORMATTING;
    static QString GENERATEINDEX;
    static QString HEADERDIRS;
    static QString HEADERS;
    static QString HEADERSCRIPTS;
    static QString HEADERSTYLES;
    static QString HOMEPAGE;
    static QString IGNOREDIRECTIVES;
    static QString IGNORETOKENS;
    static QString IMAGEDIRS;
    static QString IMAGES;
    static QString INDEXES;
    static QString LANDINGPAGE;
    static QString LANGUAGE;
    static QString MACRO;
    static QString MANIFESTMETA;
    static QString NATURALLANGUAGE;
    static QString NAVIGATION;
    static QString NOLINKERRORS;
    static QString OBSOLETELINKS;
    static QString OUTPUTDIR;
    static QString OUTPUTENCODING;
    static QString OUTPUTLANGUAGE;
    static QString OUTPUTFORMATS;
    static QString OUTPUTPREFIXES;
    static QString PROJECT;
    static QString REDIRECTDOCUMENTATIONTODEVNULL;
    static QString QHP;
    static QString QUOTINGINFORMATION;
    static QString SCRIPTDIRS;
    static QString SCRIPTS;
    static QString SHOWINTERNAL;
    static QString SOURCEDIRS;
    static QString SOURCEENCODING;
    static QString SOURCES;
    static QString SPURIOUS;
    static QString STYLEDIRS;
    static QString STYLE;
    static QString STYLES;
    static QString STYLESHEETS;
    static QString SYNTAXHIGHLIGHTING;
    static QString TEMPLATEDIR;
    static QString TABSIZE;
    static QString TAGFILE;
    static QString TRANSLATORS;
    static QString URL;
    static QString VERSION;
    static QString VERSIONSYM;
    static QString FILEEXTENSIONS;
    static QString IMAGEEXTENSIONS;
    static QString QMLONLY;
    static QString QMLTYPESPAGE;
};

#define CONFIG_ALIAS ConfigStrings::ALIAS
#define CONFIG_BASE ConfigStrings::BASE
#define CONFIG_BASEDIR ConfigStrings::BASEDIR
#define CONFIG_BUILDVERSION ConfigStrings::BUILDVERSION
#define CONFIG_CODEINDENT ConfigStrings::CODEINDENT
#define CONFIG_CPPCLASSESPAGE ConfigStrings::CPPCLASSESPAGE
#define CONFIG_DEFINES ConfigStrings::DEFINES
#define CONFIG_DEPENDS ConfigStrings::DEPENDS
#define CONFIG_DESCRIPTION ConfigStrings::DESCRIPTION
#define CONFIG_EDITION ConfigStrings::EDITION
#define CONFIG_ENDHEADER ConfigStrings::ENDHEADER
#define CONFIG_EXAMPLEDIRS ConfigStrings::EXAMPLEDIRS
#define CONFIG_EXAMPLES ConfigStrings::EXAMPLES
#define CONFIG_EXAMPLESINSTALLPATH ConfigStrings::EXAMPLESINSTALLPATH
#define CONFIG_EXCLUDEDIRS ConfigStrings::EXCLUDEDIRS
#define CONFIG_EXCLUDEFILES ConfigStrings::EXCLUDEFILES
#define CONFIG_EXTRAIMAGES ConfigStrings::EXTRAIMAGES
#define CONFIG_FALSEHOODS ConfigStrings::FALSEHOODS
#define CONFIG_FORMATTING ConfigStrings::FORMATTING
#define CONFIG_GENERATEINDEX ConfigStrings::GENERATEINDEX
#define CONFIG_HEADERDIRS ConfigStrings::HEADERDIRS
#define CONFIG_HEADERS ConfigStrings::HEADERS
#define CONFIG_HEADERSCRIPTS ConfigStrings::HEADERSCRIPTS
#define CONFIG_HEADERSTYLES ConfigStrings::HEADERSTYLES
#define CONFIG_HOMEPAGE ConfigStrings::HOMEPAGE
#define CONFIG_IGNOREDIRECTIVES ConfigStrings::IGNOREDIRECTIVES
#define CONFIG_IGNORETOKENS ConfigStrings::IGNORETOKENS
#define CONFIG_IMAGEDIRS ConfigStrings::IMAGEDIRS
#define CONFIG_IMAGES ConfigStrings::IMAGES
#define CONFIG_INDEXES ConfigStrings::INDEXES
#define CONFIG_LANDINGPAGE ConfigStrings::LANDINGPAGE
#define CONFIG_LANGUAGE ConfigStrings::LANGUAGE
#define CONFIG_MACRO ConfigStrings::MACRO
#define CONFIG_MANIFESTMETA ConfigStrings::MANIFESTMETA
#define CONFIG_NATURALLANGUAGE ConfigStrings::NATURALLANGUAGE
#define CONFIG_NAVIGATION ConfigStrings::NAVIGATION
#define CONFIG_NOLINKERRORS ConfigStrings::NOLINKERRORS
#define CONFIG_OBSOLETELINKS ConfigStrings::OBSOLETELINKS
#define CONFIG_OUTPUTDIR ConfigStrings::OUTPUTDIR
#define CONFIG_OUTPUTENCODING ConfigStrings::OUTPUTENCODING
#define CONFIG_OUTPUTLANGUAGE ConfigStrings::OUTPUTLANGUAGE
#define CONFIG_OUTPUTFORMATS ConfigStrings::OUTPUTFORMATS
#define CONFIG_OUTPUTPREFIXES ConfigStrings::OUTPUTPREFIXES
#define CONFIG_PROJECT ConfigStrings::PROJECT
#define CONFIG_REDIRECTDOCUMENTATIONTODEVNULL ConfigStrings::REDIRECTDOCUMENTATIONTODEVNULL
#define CONFIG_QHP ConfigStrings::QHP
#define CONFIG_QUOTINGINFORMATION ConfigStrings::QUOTINGINFORMATION
#define CONFIG_SCRIPTDIRS ConfigStrings::SCRIPTDIRS
#define CONFIG_SCRIPTS ConfigStrings::SCRIPTS
#define CONFIG_SHOWINTERNAL ConfigStrings::SHOWINTERNAL
#define CONFIG_SOURCEDIRS ConfigStrings::SOURCEDIRS
#define CONFIG_SOURCEENCODING ConfigStrings::SOURCEENCODING
#define CONFIG_SOURCES ConfigStrings::SOURCES
#define CONFIG_SPURIOUS ConfigStrings::SPURIOUS
#define CONFIG_STYLEDIRS ConfigStrings::STYLEDIRS
#define CONFIG_STYLE ConfigStrings::STYLE
#define CONFIG_STYLES ConfigStrings::STYLES
#define CONFIG_STYLESHEETS ConfigStrings::STYLESHEETS
#define CONFIG_SYNTAXHIGHLIGHTING ConfigStrings::SYNTAXHIGHLIGHTING
#define CONFIG_TEMPLATEDIR ConfigStrings::TEMPLATEDIR
#define CONFIG_TABSIZE ConfigStrings::TABSIZE
#define CONFIG_TAGFILE ConfigStrings::TAGFILE
#define CONFIG_TRANSLATORS ConfigStrings::TRANSLATORS
#define CONFIG_URL ConfigStrings::URL
#define CONFIG_VERSION ConfigStrings::VERSION
#define CONFIG_VERSIONSYM ConfigStrings::VERSIONSYM
#define CONFIG_FILEEXTENSIONS ConfigStrings::FILEEXTENSIONS
#define CONFIG_IMAGEEXTENSIONS ConfigStrings::IMAGEEXTENSIONS
#define CONFIG_QMLONLY ConfigStrings::QMLONLY
#define CONFIG_QMLTYPESPAGE ConfigStrings::QMLTYPESPAGE

QT_END_NAMESPACE

#endif
