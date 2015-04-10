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

#ifndef MAKEFILE_H
#define MAKEFILE_H

#include "option.h"
#include "project.h"
#include "makefiledeps.h"
#include <qtextstream.h>
#include <qlist.h>
#include <qhash.h>
#include <qfileinfo.h>

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#define QT_PCLOSE _pclose
#else
#define QT_POPEN popen
#define QT_PCLOSE pclose
#endif

struct ReplaceExtraCompilerCacheKey
{
    mutable uint hash;
    QString var, in, out, pwd;
    ReplaceExtraCompilerCacheKey(const QString &v, const QStringList &i, const QStringList &o);
    bool operator==(const ReplaceExtraCompilerCacheKey &f) const;
    inline uint hashCode() const {
        if(!hash)
            hash = qHash(var) ^ qHash(in) ^ qHash(out) /*^ qHash(pwd)*/;
        return hash;
    }
};
inline uint qHash(const ReplaceExtraCompilerCacheKey &f) { return f.hashCode(); }

struct ReplaceExtraCompilerCacheKey;

class MakefileGenerator : protected QMakeSourceFileInfo
{
    QString spec;
    bool init_opath_already, init_already, no_io;
    QHash<QString, bool> init_compiler_already;
    QString makedir, chkexists;
    QString build_args(const QString &outdir=QString());

    //internal caches
    mutable QHash<QString, QMakeLocalFileName> depHeuristicsCache;
    mutable QHash<QString, QStringList> dependsCache;
    mutable QHash<ReplaceExtraCompilerCacheKey, QString> extraCompilerVariablesCache;

protected:
    enum TARG_MODE { TARG_UNIX_MODE, TARG_MAC_MODE, TARG_WIN_MODE } target_mode;

    ProStringList createObjectList(const ProStringList &sources);

    //makefile style generator functions
    void writeObj(QTextStream &, const char *src);
    void writeInstalls(QTextStream &t, bool noBuild=false);
    void writeHeader(QTextStream &t);
    void writeSubDirs(QTextStream &t);
    void writeMakeQmake(QTextStream &t, bool noDummyQmakeAll = false);
    void writeExtraVariables(QTextStream &t);
    void writeExtraTargets(QTextStream &t);
    void writeExtraCompilerTargets(QTextStream &t);
    void writeExtraCompilerVariables(QTextStream &t);
    bool writeDummyMakefile(QTextStream &t);
    virtual bool writeStubMakefile(QTextStream &t);
    virtual bool writeMakefile(QTextStream &t);

    QString pkgConfigPrefix() const;
    QString pkgConfigFileName(bool fixify=true);
    QString pkgConfigFixPath(QString) const;
    void writePkgConfigFile();   // for pkg-config

    //generating subtarget makefiles
    struct SubTarget
    {
        QString name;
        QString in_directory, out_directory;
        QString profile, target, makefile;
        ProStringList depends;
    };
    enum SubTargetFlags {
        SubTargetInstalls=0x01,
        SubTargetOrdered=0x02,
        SubTargetSkipDefaultVariables=0x04,
        SubTargetSkipDefaultTargets=0x08,

        SubTargetsNoFlags=0x00
    };
    QList<MakefileGenerator::SubTarget*> findSubDirsSubTargets() const;
    void writeSubTargetCall(QTextStream &t,
            const QString &in_directory, const QString &in, const QString &out_directory, const QString &out,
            const QString &out_directory_cdin, const QString &makefilein);
    virtual void writeSubMakeCall(QTextStream &t, const QString &outDirectory_cdin,
                                  const QString &makeFileIn);
    void writeSubTargets(QTextStream &t, QList<SubTarget*> subtargets, int flags);

    //extra compiler interface
    bool verifyExtraCompiler(const ProString &c, const QString &f);
    virtual QString replaceExtraCompilerVariables(const QString &, const QStringList &, const QStringList &);
    inline QString replaceExtraCompilerVariables(const QString &val, const QString &in, const QString &out)
    { return replaceExtraCompilerVariables(val, QStringList(in), QStringList(out)); }

    //interface to the source file info
    QMakeLocalFileName fixPathForFile(const QMakeLocalFileName &, bool);
    QMakeLocalFileName findFileForDep(const QMakeLocalFileName &, const QMakeLocalFileName &);
    QFileInfo findFileInfo(const QMakeLocalFileName &);
    QMakeProject *project;

    //escape
    virtual QString unescapeFilePath(const QString &path) const;
    ProString unescapeFilePath(const ProString &path) const;
    virtual QStringList unescapeFilePaths(const QStringList &path) const;
    ProStringList unescapeFilePaths(const ProStringList &path) const;
    virtual QString escapeFilePath(const QString &path) const { return path; }
    ProString escapeFilePath(const ProString &path) const;
    QStringList escapeFilePaths(const QStringList &paths) const;
    ProStringList escapeFilePaths(const ProStringList &paths) const;
    virtual QString escapeDependencyPath(const QString &path) const { return escapeFilePath(path); }
    QStringList escapeDependencyPaths(const QStringList &paths) const;
    ProStringList escapeDependencyPaths(const ProStringList &paths) const;

    //initialization
    void verifyCompilers();
    virtual void init();
    void initOutPaths();
    struct Compiler
    {
        QString variable_in;
        enum CompilerFlag {
            CompilerNoFlags                 = 0x00,
            CompilerBuiltin                 = 0x01,
            CompilerNoCheckDeps             = 0x02,
            CompilerRemoveNoExist           = 0x04,
            CompilerAddInputsAsMakefileDeps = 0x08
        };
        uint flags, type;
    };
    void initCompiler(const Compiler &comp);
    enum VPATHFlag {
        VPATH_NoFlag             = 0x00,
        VPATH_WarnMissingFiles   = 0x01,
        VPATH_RemoveMissingFiles = 0x02,
        VPATH_NoFixify           = 0x04
    };
    ProStringList findFilesInVPATH(ProStringList l, uchar flags, const QString &var="");

    inline int findExecutable(const QStringList &cmdline)
    { int ret; canExecute(cmdline, &ret); return ret; }
    bool canExecute(const QStringList &cmdline, int *argv0) const;
    inline bool canExecute(const QString &cmdline) const
    { return canExecute(cmdline.split(' '), 0); }

    bool mkdir(const QString &dir) const;
    QString mkdir_p_asstring(const QString &dir, bool escape=true) const;

    QString specdir();

    //subclasses can use these to query information about how the generator was "run"
    QString buildArgs();
    QString fixifySpecdir(const QString &spec, const QString &outdir);

    virtual QStringList &findDependencies(const QString &file);
    virtual bool doDepends() const { return Option::mkfile::do_deps; }

    void filterIncludedFiles(const char *);
    virtual void processSources() {
        filterIncludedFiles("SOURCES");
        filterIncludedFiles("GENERATED_SOURCES");
    }

    //for installs
    virtual QString defaultInstall(const QString &);

    //for prl
    QString prlFileName(bool fixify=true);
    void writePrlFile();
    bool processPrlFile(QString &);
    virtual void processPrlFiles();
    virtual void writePrlFile(QTextStream &);

    //make sure libraries are found
    virtual bool findLibraries();

    //for retrieving values and lists of values
    virtual QString var(const ProKey &var) const;
    QString varGlue(const ProKey &var, const QString &before, const QString &glue, const QString &after) const;
    QString fileVarGlue(const ProKey &var, const QString &before, const QString &glue, const QString &after) const;
    QString varList(const ProKey &var) const;
    QString val(const ProStringList &varList) const;
    QString val(const QStringList &varList) const;
    QString valGlue(const QStringList &varList, const QString &before, const QString &glue, const QString &after) const;
    QString valGlue(const ProStringList &varList, const QString &before, const QString &glue, const QString &after) const;
    QString valList(const QStringList &varList) const;
    QString valList(const ProStringList &varList) const;

    QString filePrefixRoot(const QString &, const QString &);

    //file fixification to unify all file names into a single pattern
    enum FileFixifyType { FileFixifyAbsolute, FileFixifyRelative, FileFixifyDefault };
    QString fileFixify(const QString& file, const QString &out_dir=QString(),
                       const QString &in_dir=QString(), FileFixifyType fix=FileFixifyDefault, bool canon=true) const;
    inline QString fileFixify(const QString& file, FileFixifyType fix, bool canon=true) const
    { return fileFixify(file, QString(), QString(), fix, canon); }
    QStringList fileFixify(const QStringList& files, const QString &out_dir=QString(),
                           const QString &in_dir=QString(), FileFixifyType fix=FileFixifyDefault, bool canon=true) const;
    inline QStringList fileFixify(const QStringList& files, FileFixifyType fix, bool canon=true) const
    { return fileFixify(files, QString(), QString(), fix, canon); }

    QString installMetaFile(const ProKey &replace_rule, const QString &src, const QString &dst);

public:
    MakefileGenerator();
    virtual ~MakefileGenerator();
    QMakeProject *projectFile() const;
    void setProjectFile(QMakeProject *p);

    void setNoIO(bool o);
    bool noIO() const;

    inline bool exists(QString file) const { return fileInfo(file).exists(); }
    QFileInfo fileInfo(QString file) const;

    static MakefileGenerator *create(QMakeProject *);
    virtual bool write();
    virtual bool writeProjectMakefile();
    virtual bool supportsMetaBuild() { return true; }
    virtual bool supportsMergedBuilds() { return false; }
    virtual bool mergeBuildProject(MakefileGenerator * /*other*/) { return false; }
    virtual bool openOutput(QFile &, const QString &build) const;
    bool isWindowsShell() const { return Option::dir_sep == QLatin1String("\\"); }
    QString shellQuote(const QString &str);
};

inline void MakefileGenerator::setNoIO(bool o)
{ no_io = o; }

inline bool MakefileGenerator::noIO() const
{ return no_io; }

inline QString MakefileGenerator::defaultInstall(const QString &)
{ return QString(""); }

inline bool MakefileGenerator::findLibraries()
{ return true; }

inline MakefileGenerator::~MakefileGenerator()
{ }

QT_END_NAMESPACE

#endif // MAKEFILE_H
