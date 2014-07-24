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

#include "metamakefile.h"
#include "qregexp.h"
#include "qdir.h"
#include "qdebug.h"
#include "makefile.h"
#include "project.h"
#include "cachekeys.h"

#define BUILDSMETATYPE 1
#define SUBDIRSMETATYPE 2

QT_BEGIN_NAMESPACE

MetaMakefileGenerator::~MetaMakefileGenerator()
{
    if(own_project)
        delete project;
}

#ifndef QT_QMAKE_PARSER_ONLY

class BuildsMetaMakefileGenerator : public MetaMakefileGenerator
{
private:
    bool init_flag;
    struct Build {
        QString name, build;
        MakefileGenerator *makefile;
    };
    QList<Build *> makefiles;
    void clearBuilds();
    MakefileGenerator *processBuild(const ProString &);

public:

    BuildsMetaMakefileGenerator(QMakeProject *p, const QString &n, bool op) : MetaMakefileGenerator(p, n, op), init_flag(false) { }
    virtual ~BuildsMetaMakefileGenerator() { clearBuilds(); }

    virtual bool init();
    virtual int type() const { return BUILDSMETATYPE; }
    virtual bool write();
};

void
BuildsMetaMakefileGenerator::clearBuilds()
{
    for(int i = 0; i < makefiles.count(); i++) {
        Build *build = makefiles[i];
        if(QMakeProject *p = build->makefile->projectFile()) {
            if(p != project)
                delete p;
        }
        delete build->makefile;
        delete build;
    }
    makefiles.clear();
}

bool
BuildsMetaMakefileGenerator::init()
{
    if(init_flag)
        return false;
    init_flag = true;

    const ProStringList &builds = project->values("BUILDS");
    bool use_single_build = builds.isEmpty();
    if(builds.count() > 1 && Option::output.fileName() == "-") {
        use_single_build = true;
        warn_msg(WarnLogic, "Cannot direct to stdout when using multiple BUILDS.");
    } else if(0 && !use_single_build && project->first("TEMPLATE") == "subdirs") {
        use_single_build = true;
        warn_msg(WarnLogic, "Cannot specify multiple builds with TEMPLATE subdirs.");
    }
    if(!use_single_build) {
        for(int i = 0; i < builds.count(); i++) {
            ProString build = builds[i];
            MakefileGenerator *makefile = processBuild(build);
            if(!makefile)
                return false;
            if(!makefile->supportsMetaBuild()) {
                warn_msg(WarnLogic, "QMAKESPEC does not support multiple BUILDS.");
                clearBuilds();
                use_single_build = true;
                break;
            } else {
                Build *b = new Build;
                b->name = name;
                if(builds.count() != 1)
                    b->build = build.toQString();
                b->makefile = makefile;
                makefiles += b;
            }
        }
    }
    if(use_single_build) {
        Build *build = new Build;
        build->name = name;
        build->makefile = createMakefileGenerator(project, false);
        if (build->makefile){
            makefiles += build;
        }else {
            delete build;
            return false;
        }
    }
    return true;
}

bool
BuildsMetaMakefileGenerator::write()
{
    Build *glue = 0;
    if(!makefiles.isEmpty() && !makefiles.first()->build.isNull()) {
        glue = new Build;
        glue->name = name;
        glue->makefile = createMakefileGenerator(project, true);
        makefiles += glue;
    }

    bool ret = true;
    const QString &output_name = Option::output.fileName();
    for(int i = 0; ret && i < makefiles.count(); i++) {
        Option::output.setFileName(output_name);
        Build *build = makefiles[i];

        bool using_stdout = false;
        if(build->makefile && (Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
                               Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT)
           && (!build->makefile->supportsMergedBuilds()
            || (build->makefile->supportsMergedBuilds() && (!glue || build == glue)))) {
            //open output
            if(!(Option::output.isOpen())) {
                if(Option::output.fileName() == "-") {
                    Option::output.setFileName("");
                    Option::output_dir = qmake_getpwd();
                    Option::output.open(stdout, QIODevice::WriteOnly | QIODevice::Text);
                    using_stdout = true;
                } else {
                    if(Option::output.fileName().isEmpty() &&
                       Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE)
                        Option::output.setFileName(project->first("QMAKE_MAKEFILE").toQString());
                    QString build_name = build->name;
                    if(!build->build.isEmpty()) {
                        if(!build_name.isEmpty())
                            build_name += ".";
                        build_name += build->build;
                    }
                    if(!build->makefile->openOutput(Option::output, build_name)) {
                        fprintf(stderr, "Failure to open file: %s\n",
                                Option::output.fileName().isEmpty() ? "(stdout)" :
                                Option::output.fileName().toLatin1().constData());
                        return false;
                    }
                }
            }
        } else {
           using_stdout = true; //kind of..
        }

        if(!build->makefile) {
            ret = false;
        } else if(build == glue) {
            ret = build->makefile->writeProjectMakefile();
        } else {
            ret = build->makefile->write();
            if (glue && glue->makefile->supportsMergedBuilds())
                ret = glue->makefile->mergeBuildProject(build->makefile);
        }
        if(!using_stdout) {
            Option::output.close();
            if(!ret)
                Option::output.remove();
        }
    }
    return ret;
}

MakefileGenerator
*BuildsMetaMakefileGenerator::processBuild(const ProString &build)
{
    if(project) {
        debug_msg(1, "Meta Generator: Parsing '%s' for build [%s].",
                  project->projectFile().toLatin1().constData(),build.toLatin1().constData());

        //initialize the base
        ProValueMap basevars;
        ProStringList basecfgs = project->values(ProKey(build + ".CONFIG"));
        basecfgs += build;
        basecfgs += "build_pass";
        basevars["BUILD_PASS"] = ProStringList(build);
        ProStringList buildname = project->values(ProKey(build + ".name"));
        basevars["BUILD_NAME"] = (buildname.isEmpty() ? ProStringList(build) : buildname);

        //create project
        QMakeProject *build_proj = new QMakeProject;
        build_proj->setExtraVars(basevars);
        build_proj->setExtraConfigs(basecfgs);

        build_proj->read(project->projectFile());

        //done
        return createMakefileGenerator(build_proj);
    }
    return 0;
}

class SubdirsMetaMakefileGenerator : public MetaMakefileGenerator
{
protected:
    bool init_flag;
    struct Subdir {
        Subdir() : makefile(0), indent(0) { }
        ~Subdir() { delete makefile; }
        QString input_dir;
        QString output_dir, output_file;
        MetaMakefileGenerator *makefile;
        int indent;
    };
    QList<Subdir *> subs;
    MakefileGenerator *processBuild(const QString &);

public:
    SubdirsMetaMakefileGenerator(QMakeProject *p, const QString &n, bool op) : MetaMakefileGenerator(p, n, op), init_flag(false) { }
    virtual ~SubdirsMetaMakefileGenerator();

    virtual bool init();
    virtual int type() const { return SUBDIRSMETATYPE; }
    virtual bool write();
};

bool
SubdirsMetaMakefileGenerator::init()
{
    if(init_flag)
        return false;
    init_flag = true;
    bool hasError = false;

    bool recurse = Option::recursive;
    if (recurse && project->isActiveConfig("dont_recurse"))
        recurse = false;
    if(recurse) {
        QString old_output_dir = Option::output_dir;
        QString old_output = Option::output.fileName();
        QString oldpwd = qmake_getpwd();
        QString thispwd = oldpwd;
        if(!thispwd.endsWith('/'))
           thispwd += '/';
        const ProStringList &subdirs = project->values("SUBDIRS");
        static int recurseDepth = -1;
        ++recurseDepth;
        for(int i = 0; i < subdirs.size(); ++i) {
            Subdir *sub = new Subdir;
            sub->indent = recurseDepth;

            QFileInfo subdir(subdirs.at(i).toQString());
            const ProKey fkey(subdirs.at(i) + ".file");
            if (!project->isEmpty(fkey)) {
                subdir = project->first(fkey).toQString();
            } else {
                const ProKey skey(subdirs.at(i) + ".subdir");
                if (!project->isEmpty(skey))
                    subdir = project->first(skey).toQString();
            }
            QString sub_name;
            if(subdir.isDir())
                subdir = QFileInfo(subdir.filePath() + "/" + subdir.fileName() + Option::pro_ext);
            else
                sub_name = subdir.baseName();
            if(!subdir.isRelative()) { //we can try to make it relative
                QString subdir_path = subdir.filePath();
                if(subdir_path.startsWith(thispwd))
                    subdir = QFileInfo(subdir_path.mid(thispwd.length()));
            }

            //handle sub project
            QMakeProject *sub_proj = new QMakeProject;
            for (int ind = 0; ind < sub->indent; ++ind)
                printf(" ");
            sub->input_dir = subdir.absolutePath();
            if(subdir.isRelative() && old_output_dir != oldpwd) {
                sub->output_dir = old_output_dir + (subdir.path() != "." ? "/" + subdir.path() : QString());
                printf("Reading %s [%s]\n", subdir.absoluteFilePath().toLatin1().constData(), sub->output_dir.toLatin1().constData());
            } else { //what about shadow builds?
                sub->output_dir = sub->input_dir;
                printf("Reading %s\n", subdir.absoluteFilePath().toLatin1().constData());
            }
            qmake_setpwd(sub->input_dir);
            Option::output_dir = sub->output_dir;
            bool tmpError = !sub_proj->read(subdir.fileName());
            if (!sub_proj->isEmpty("QMAKE_FAILED_REQUIREMENTS")) {
                fprintf(stderr, "Project file(%s) not recursed because all requirements not met:\n\t%s\n",
                        subdir.fileName().toLatin1().constData(),
                        sub_proj->values("QMAKE_FAILED_REQUIREMENTS").join(' ').toLatin1().constData());
                delete sub;
                delete sub_proj;
                Option::output_dir = old_output_dir;
                qmake_setpwd(oldpwd);
                continue;
            } else {
                hasError |= tmpError;
            }
            sub->makefile = MetaMakefileGenerator::createMetaGenerator(sub_proj, sub_name);
            if(0 && sub->makefile->type() == SUBDIRSMETATYPE) {
                subs.append(sub);
            } else {
                const QString output_name = Option::output.fileName();
                Option::output.setFileName(sub->output_file);
                hasError |= !sub->makefile->write();
                delete sub;
                qmakeClearCaches();
                sub = 0;
                Option::output.setFileName(output_name);
            }
            Option::output_dir = old_output_dir;
            qmake_setpwd(oldpwd);

        }
        --recurseDepth;
        Option::output.setFileName(old_output);
        Option::output_dir = old_output_dir;
        qmake_setpwd(oldpwd);
    }

    Subdir *self = new Subdir;
    self->input_dir = qmake_getpwd();
    self->output_dir = Option::output_dir;
    if(!recurse || (!Option::output.fileName().endsWith(Option::dir_sep) && !QFileInfo(Option::output).isDir()))
        self->output_file = Option::output.fileName();
    self->makefile = new BuildsMetaMakefileGenerator(project, name, false);
    self->makefile->init();
    subs.append(self);

    return !hasError;
}

bool
SubdirsMetaMakefileGenerator::write()
{
    bool ret = true;
    const QString &pwd = qmake_getpwd();
    const QString &output_dir = Option::output_dir;
    const QString &output_name = Option::output.fileName();
    for(int i = 0; ret && i < subs.count(); i++) {
        const Subdir *sub = subs.at(i);
        qmake_setpwd(sub->input_dir);
        Option::output_dir = QFileInfo(sub->output_dir).absoluteFilePath();
        Option::output.setFileName(sub->output_file);
        if(i != subs.count()-1) {
            for (int ind = 0; ind < sub->indent; ++ind)
                printf(" ");
            printf("Writing %s\n", QDir::cleanPath(Option::output_dir+"/"+
                                                   Option::output.fileName()).toLatin1().constData());
        }
        if (!(ret = sub->makefile->write()))
            break;
        //restore because I'm paranoid
        qmake_setpwd(pwd);
        Option::output.setFileName(output_name);
        Option::output_dir = output_dir;
    }
    return ret;
}

SubdirsMetaMakefileGenerator::~SubdirsMetaMakefileGenerator()
{
    for(int i = 0; i < subs.count(); i++)
        delete subs[i];
    subs.clear();
}

//Factory things
QT_BEGIN_INCLUDE_NAMESPACE
#include "unixmake.h"
#include "mingw_make.h"
#include "projectgenerator.h"
#include "pbuilder_pbx.h"
#include "msvc_nmake.h"
#include "msvc_vcproj.h"
#include "msvc_vcxproj.h"
#include "gbuild.h"
QT_END_INCLUDE_NAMESPACE

MakefileGenerator *
MetaMakefileGenerator::createMakefileGenerator(QMakeProject *proj, bool noIO)
{
    Option::postProcessProject(proj);

    MakefileGenerator *mkfile = NULL;
    if(Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT) {
        mkfile = new ProjectGenerator;
        mkfile->setProjectFile(proj);
        return mkfile;
    }

    ProString gen = proj->first("MAKEFILE_GENERATOR");
    if(gen.isEmpty()) {
        fprintf(stderr, "MAKEFILE_GENERATOR variable not set as a result of parsing : %s. Possibly qmake was not able to find files included using \"include(..)\" - enable qmake debugging to investigate more.\n",
                proj->projectFile().toLatin1().constData());
    } else if(gen == "UNIX") {
        mkfile = new UnixMakefileGenerator;
    } else if(gen == "MINGW") {
        mkfile = new MingwMakefileGenerator;
    } else if(gen == "PROJECTBUILDER" || gen == "XCODE") {
        mkfile = new ProjectBuilderMakefileGenerator;
    } else if(gen == "MSVC.NET") {
        if (proj->first("TEMPLATE").startsWith("vc"))
            mkfile = new VcprojGenerator;
        else
            mkfile = new NmakeMakefileGenerator;
    } else if(gen == "MSBUILD") {
        // Visual Studio >= v11.0
        if (proj->first("TEMPLATE").startsWith("vc"))
            mkfile = new VcxprojGenerator;
        else
            mkfile = new NmakeMakefileGenerator;
    } else if(gen == "GBUILD") {
        mkfile = new GBuildMakefileGenerator;
    } else {
        fprintf(stderr, "Unknown generator specified: %s\n", gen.toLatin1().constData());
    }
    if (mkfile) {
        mkfile->setNoIO(noIO);
        mkfile->setProjectFile(proj);
    }
    return mkfile;
}

MetaMakefileGenerator *
MetaMakefileGenerator::createMetaGenerator(QMakeProject *proj, const QString &name, bool op, bool *success)
{
    Option::postProcessProject(proj);

    MetaMakefileGenerator *ret = 0;
    if ((Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
         Option::qmake_mode == Option::QMAKE_GENERATE_PRL)) {
        if (proj->first("TEMPLATE").endsWith("subdirs"))
            ret = new SubdirsMetaMakefileGenerator(proj, name, op);
    }
    if (!ret)
        ret = new BuildsMetaMakefileGenerator(proj, name, op);
    bool res = ret->init();
    if (success)
        *success = res;
    return ret;
}

#endif // QT_QMAKE_PARSER_ONLY

QT_END_NAMESPACE
