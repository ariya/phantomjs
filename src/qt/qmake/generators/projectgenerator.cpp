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

#include "projectgenerator.h"
#include "option.h"
#include <qdatetime.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

QT_BEGIN_NAMESPACE

QString project_builtin_regx() //calculate the builtin regular expression..
{
    QString ret;
    QStringList builtin_exts;
    builtin_exts << Option::c_ext << Option::ui_ext << Option::yacc_ext << Option::lex_ext << ".ts" << ".xlf" << ".qrc";
    builtin_exts += Option::h_ext + Option::cpp_ext;
    for(int i = 0; i < builtin_exts.size(); ++i) {
        if(!ret.isEmpty())
            ret += "; ";
        ret += QString("*") + builtin_exts[i];
    }
    return ret;
}

ProjectGenerator::ProjectGenerator() : MakefileGenerator(), init_flag(false)
{
}

void
ProjectGenerator::init()
{
    if(init_flag)
        return;
    int file_count = 0;
    init_flag = true;
    verifyCompilers();

    project->read(QMakeProject::ReadFeatures);
    project->variables()["CONFIG"].clear();

    QMap<QString, QStringList> &v = project->variables();
    QString templ = Option::user_template.isEmpty() ? QString("app") : Option::user_template;
    if(!Option::user_template_prefix.isEmpty())
        templ.prepend(Option::user_template_prefix);
    v["TEMPLATE_ASSIGN"] += templ;

    //figure out target
    if(Option::output.fileName() == "-")
        v["TARGET_ASSIGN"] = QStringList("unknown");
    else
        v["TARGET_ASSIGN"] = QStringList(QFileInfo(Option::output).baseName());

    //the scary stuff
    if(project->first("TEMPLATE_ASSIGN") != "subdirs") {
        QString builtin_regex = project_builtin_regx();
        QStringList dirs = Option::projfile::project_dirs;
        if(Option::projfile::do_pwd) {
            if(!v["INCLUDEPATH"].contains("."))
                v["INCLUDEPATH"] += ".";
            dirs.prepend(qmake_getpwd());
        }

        for(int i = 0; i < dirs.count(); ++i) {
            QString dir, regex, pd = dirs.at(i);
            bool add_depend = false;
            if(exists(pd)) {
                QFileInfo fi(fileInfo(pd));
                if(fi.isDir()) {
                    dir = pd;
                    add_depend = true;
                    if(dir.right(1) != Option::dir_sep)
                        dir += Option::dir_sep;
                    if(Option::recursive == Option::QMAKE_RECURSIVE_YES) {
                        QStringList files = QDir(dir).entryList(QDir::Files);
                        for(int i = 0; i < (int)files.count(); i++) {
                            if(files[i] != "." && files[i] != "..")
                                dirs.append(dir + files[i] + QDir::separator() + builtin_regex);
                        }
                    }
                    regex = builtin_regex;
                } else {
                    QString file = pd;
                    int s = file.lastIndexOf(Option::dir_sep);
                    if(s != -1)
                        dir = file.left(s+1);
                    if(addFile(file)) {
                        add_depend = true;
                        file_count++;
                    }
                }
            } else { //regexp
                regex = pd;
            }
            if(!regex.isEmpty()) {
                int s = regex.lastIndexOf(Option::dir_sep);
                if(s != -1) {
                    dir = regex.left(s+1);
                    regex = regex.right(regex.length() - (s+1));
                }
                if(Option::recursive == Option::QMAKE_RECURSIVE_YES) {
                    QStringList entries = QDir(dir).entryList(QDir::Dirs);
                    for(int i = 0; i < (int)entries.count(); i++) {
                        if(entries[i] != "." && entries[i] != "..") {
                            dirs.append(dir + entries[i] + QDir::separator() + regex);
                        }
                    }
                }
                QStringList files = QDir(dir).entryList(QDir::nameFiltersFromString(regex));
                for(int i = 0; i < (int)files.count(); i++) {
                    QString file = dir + files[i];
                    if (addFile(file)) {
                        add_depend = true;
                        file_count++;
                    }
                }
            }
            if(add_depend && !dir.isEmpty() && !v["DEPENDPATH"].contains(dir, Qt::CaseInsensitive)) {
                QFileInfo fi(fileInfo(dir));
                if(fi.absoluteFilePath() != qmake_getpwd())
                    v["DEPENDPATH"] += fileFixify(dir);
            }
        }
    }
    if(!file_count) { //shall we try a subdir?
        QStringList knownDirs = Option::projfile::project_dirs;
        if(Option::projfile::do_pwd)
            knownDirs.prepend(".");
        const QString out_file = fileFixify(Option::output.fileName());
        for(int i = 0; i < knownDirs.count(); ++i) {
            QString pd = knownDirs.at(i);
            if(exists(pd)) {
                QString newdir = pd;
                QFileInfo fi(fileInfo(newdir));
                if(fi.isDir()) {
                    newdir = fileFixify(newdir);
                    QStringList &subdirs = v["SUBDIRS"];
                    if(exists(fi.filePath() + QDir::separator() + fi.fileName() + Option::pro_ext) &&
                       !subdirs.contains(newdir, Qt::CaseInsensitive)) {
                        subdirs.append(newdir);
                    } else {
                        QStringList profiles = QDir(newdir).entryList(QStringList("*" + Option::pro_ext), QDir::Files);
                        for(int i = 0; i < (int)profiles.count(); i++) {
                            QString nd = newdir;
                            if(nd == ".")
                                nd = "";
                            else if(!nd.isEmpty() && !nd.endsWith(QString(QChar(QDir::separator()))))
                                nd += QDir::separator();
                            nd += profiles[i];
                            fileFixify(nd);
                            if(profiles[i] != "." && profiles[i] != ".." &&
                               !subdirs.contains(nd, Qt::CaseInsensitive) && !out_file.endsWith(nd))
                                subdirs.append(nd);
                        }
                    }
                    if(Option::recursive == Option::QMAKE_RECURSIVE_YES) {
                        QStringList dirs = QDir(newdir).entryList(QDir::Dirs);
                        for(int i = 0; i < (int)dirs.count(); i++) {
                            QString nd = fileFixify(newdir + QDir::separator() + dirs[i]);
                            if(dirs[i] != "." && dirs[i] != ".." && !knownDirs.contains(nd, Qt::CaseInsensitive))
                                knownDirs.append(nd);
                        }
                    }
                }
            } else { //regexp
                QString regx = pd, dir;
                int s = regx.lastIndexOf(Option::dir_sep);
                if(s != -1) {
                    dir = regx.left(s+1);
                    regx = regx.right(regx.length() - (s+1));
                }
                QStringList files = QDir(dir).entryList(QDir::nameFiltersFromString(regx), QDir::Dirs);
                QStringList &subdirs = v["SUBDIRS"];
                for(int i = 0; i < (int)files.count(); i++) {
                    QString newdir(dir + files[i]);
                    QFileInfo fi(fileInfo(newdir));
                    if(fi.fileName() != "." && fi.fileName() != "..") {
                        newdir = fileFixify(newdir);
                        if(exists(fi.filePath() + QDir::separator() + fi.fileName() + Option::pro_ext) &&
                           !subdirs.contains(newdir)) {
                           subdirs.append(newdir);
                        } else {
                            QStringList profiles = QDir(newdir).entryList(QStringList("*" + Option::pro_ext), QDir::Files);
                            for(int i = 0; i < (int)profiles.count(); i++) {
                                QString nd = newdir + QDir::separator() + files[i];
                                fileFixify(nd);
                                if(files[i] != "." && files[i] != ".." && !subdirs.contains(nd, Qt::CaseInsensitive)) {
                                    if(newdir + files[i] != Option::output_dir + Option::output.fileName())
                                        subdirs.append(nd);
                                }
                            }
                        }
                        if(Option::recursive == Option::QMAKE_RECURSIVE_YES
                           && !knownDirs.contains(newdir, Qt::CaseInsensitive))
                            knownDirs.append(newdir);
                    }
                }
            }
        }
        v["TEMPLATE_ASSIGN"] = QStringList("subdirs");
        return;
    }

    //setup deplist
    QList<QMakeLocalFileName> deplist;
    {
        const QStringList &d = v["DEPENDPATH"];
        for(int i = 0; i < d.size(); ++i)
            deplist.append(QMakeLocalFileName(d[i]));
    }
    setDependencyPaths(deplist);

    QStringList &h = v["HEADERS"];
    bool no_qt_files = true;
    QString srcs[] = { "SOURCES", "YACCSOURCES", "LEXSOURCES", "FORMS", QString() };
    for(int i = 0; !srcs[i].isNull(); i++) {
        const QStringList &l = v[srcs[i]];
        QMakeSourceFileInfo::SourceFileType type = QMakeSourceFileInfo::TYPE_C;
        QMakeSourceFileInfo::addSourceFiles(l, QMakeSourceFileInfo::SEEK_DEPS, type);
        for(int i = 0; i < l.size(); ++i) {
            QStringList tmp = QMakeSourceFileInfo::dependencies(l[i]);
            if(!tmp.isEmpty()) {
                for(int dep_it = 0; dep_it < tmp.size(); ++dep_it) {
                    QString dep = tmp[dep_it];
                    dep = fixPathToQmake(dep);
                    QString file_dir = dep.section(Option::dir_sep, 0, -2),
                        file_no_path = dep.section(Option::dir_sep, -1);
                    if(!file_dir.isEmpty()) {
                        for(int inc_it = 0; inc_it < deplist.size(); ++inc_it) {
                            QMakeLocalFileName inc = deplist[inc_it];
                            if(inc.local() == file_dir && !v["INCLUDEPATH"].contains(inc.real(), Qt::CaseInsensitive))
                                v["INCLUDEPATH"] += inc.real();
                        }
                    }
                    if(no_qt_files && file_no_path.indexOf(QRegExp("^q[a-z_0-9].h$")) != -1)
                        no_qt_files = false;
                    QString h_ext;
                    for(int hit = 0; hit < Option::h_ext.size(); ++hit) {
                        if(dep.endsWith(Option::h_ext.at(hit))) {
                            h_ext = Option::h_ext.at(hit);
                            break;
                        }
                    }
                    if(!h_ext.isEmpty()) {
                        for(int cppit = 0; cppit < Option::cpp_ext.size(); ++cppit) {
                            QString src(dep.left(dep.length() - h_ext.length()) +
                                        Option::cpp_ext.at(cppit));
                            if(exists(src)) {
                                QStringList &srcl = v["SOURCES"];
                                if(!srcl.contains(src, Qt::CaseInsensitive))
                                    srcl.append(src);
                            }
                        }
                    } else if(dep.endsWith(Option::lex_ext) &&
                              file_no_path.startsWith(Option::lex_mod)) {
                        addConfig("lex_included");
                    }
                    if(!h.contains(dep, Qt::CaseInsensitive))
                        h += dep;
                }
            }
        }
    }

    //strip out files that are actually output from internal compilers (ie temporary files)
    const QStringList &quc = project->variables()["QMAKE_EXTRA_COMPILERS"];
    for(QStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
        QString tmp_out = project->variables()[(*it) + ".output"].first();
        if(tmp_out.isEmpty())
            continue;

        QStringList var_out = project->variables()[(*it) + ".variable_out"];
        bool defaults = var_out.isEmpty();
        for(int i = 0; i < var_out.size(); ++i) {
            QString v = var_out.at(i);
            if(v.startsWith("GENERATED_")) {
                defaults = true;
                break;
            }
        }
        if(defaults) {
            var_out << "SOURCES";
            var_out << "HEADERS";
            var_out << "FORMS";
        }
        const QStringList &tmp = project->variables()[(*it) + ".input"];
        for(QStringList::ConstIterator it2 = tmp.begin(); it2 != tmp.end(); ++it2) {
            QStringList &inputs = project->variables()[(*it2)];
            for(QStringList::Iterator input = inputs.begin(); input != inputs.end(); ++input) {
                QString path = replaceExtraCompilerVariables(tmp_out, (*input), QString());
                path = fixPathToQmake(path).section('/', -1);
                for(int i = 0; i < var_out.size(); ++i) {
                    QString v = var_out.at(i);
                    QStringList &list = project->variables()[v];
                    for(int src = 0; src < list.size(); ) {
                        if(list[src] == path || list[src].endsWith("/" + path))
                            list.removeAt(src);
                        else
                            ++src;
                    }
                }
            }
        }
    }
}

bool
ProjectGenerator::writeMakefile(QTextStream &t)
{
    t << "######################################################################" << endl;
    t << "# Automatically generated by qmake (" << qmake_version() << ") " << QDateTime::currentDateTime().toString() << endl;
    t << "######################################################################" << endl << endl;
    if(!Option::user_configs.isEmpty())
        t << "CONFIG += " << Option::user_configs.join(" ") << endl;
    int i;
    for(i = 0; i < Option::before_user_vars.size(); ++i)
        t << Option::before_user_vars[i] << endl;
    t << getWritableVar("TEMPLATE_ASSIGN", false);
    if(project->first("TEMPLATE_ASSIGN") == "subdirs") {
        t << endl << "# Directories" << "\n"
          << getWritableVar("SUBDIRS");
    } else {
        t << getWritableVar("TARGET_ASSIGN")
          << getWritableVar("CONFIG", false)
          << getWritableVar("CONFIG_REMOVE", false)
          << getWritableVar("DEPENDPATH")
          << getWritableVar("INCLUDEPATH") << endl;

        t << "# Input" << "\n";
        t << getWritableVar("HEADERS")
          << getWritableVar("FORMS")
          << getWritableVar("LEXSOURCES")
          << getWritableVar("YACCSOURCES")
          << getWritableVar("SOURCES")
          << getWritableVar("RESOURCES")
          << getWritableVar("TRANSLATIONS");
    }
    for(i = 0; i < Option::after_user_vars.size(); ++i)
        t << Option::after_user_vars[i] << endl;
    return true;
}

bool
ProjectGenerator::addConfig(const QString &cfg, bool add)
{
    QString where = "CONFIG";
    if(!add)
        where = "CONFIG_REMOVE";
    if(!project->variables()[where].contains(cfg)) {
        project->variables()[where] += cfg;
        return true;
    }
    return false;
}

bool
ProjectGenerator::addFile(QString file)
{
    file = fileFixify(file, qmake_getpwd());
    QString dir;
    int s = file.lastIndexOf(Option::dir_sep);
    if(s != -1)
        dir = file.left(s+1);
    if(file.mid(dir.length(), Option::h_moc_mod.length()) == Option::h_moc_mod)
        return false;

    QString where;
    for(int cppit = 0; cppit < Option::cpp_ext.size(); ++cppit) {
        if(file.endsWith(Option::cpp_ext[cppit])) {
            where = "SOURCES";
            break;
        }
    }
    if(where.isEmpty()) {
        for(int hit = 0; hit < Option::h_ext.size(); ++hit)
            if(file.endsWith(Option::h_ext.at(hit))) {
                where = "HEADERS";
                break;
            }
    }
    if(where.isEmpty()) {
        for(int cit = 0; cit < Option::c_ext.size(); ++cit) {
            if(file.endsWith(Option::c_ext[cit])) {
                where = "SOURCES";
                break;
            }
        }
    }
    if(where.isEmpty()) {
        if(file.endsWith(Option::ui_ext))
            where = "FORMS";
        else if(file.endsWith(Option::lex_ext))
            where = "LEXSOURCES";
        else if(file.endsWith(Option::yacc_ext))
            where = "YACCSOURCES";
        else if(file.endsWith(".ts") || file.endsWith(".xlf"))
            where = "TRANSLATIONS";
        else if(file.endsWith(".qrc"))
            where = "RESOURCES";
    }

    QString newfile = fixPathToQmake(fileFixify(file));

    QStringList &endList = project->variables()[where];
    if(!endList.contains(newfile, Qt::CaseInsensitive)) {
        endList += newfile;
        return true;
    }
    return false;
}

QString
ProjectGenerator::getWritableVar(const QString &v, bool)
{
    QStringList &vals = project->variables()[v];
    if(vals.isEmpty())
        return "";

    // If values contain spaces, ensure that they are quoted
    for(QStringList::iterator it = vals.begin(); it != vals.end(); ++it) {
        if ((*it).contains(' ') && !(*it).startsWith(' '))
            *it = '\"' + *it + '\"';
    }

    QString ret;
    if(v.endsWith("_REMOVE"))
        ret = v.left(v.length() - 7) + " -= ";
    else if(v.endsWith("_ASSIGN"))
        ret = v.left(v.length() - 7) + " = ";
    else
        ret = v + " += ";
    QString join = vals.join(" ");
    if(ret.length() + join.length() > 80) {
        QString spaces;
        for(int i = 0; i < ret.length(); i++)
            spaces += " ";
        join = vals.join(" \\\n" + spaces);
    }
    return ret + join + "\n";
}

bool
ProjectGenerator::openOutput(QFile &file, const QString &build) const
{
    QString outdir;
    if(!file.fileName().isEmpty()) {
        QFileInfo fi(fileInfo(file.fileName()));
        if(fi.isDir())
            outdir = fi.path() + QDir::separator();
    }
    if(!outdir.isEmpty() || file.fileName().isEmpty()) {
        QString dir = qmake_getpwd();
        int s = dir.lastIndexOf('/');
        if(s != -1)
            dir = dir.right(dir.length() - (s + 1));
        file.setFileName(outdir + dir + Option::pro_ext);
    }
    return MakefileGenerator::openOutput(file, build);
}


QString
ProjectGenerator::fixPathToQmake(const QString &file)
{
    QString ret = file;
    if(Option::dir_sep != QLatin1String("/"))
        ret = ret.replace(Option::dir_sep, QLatin1String("/"));
    return ret;
}

QT_END_NAMESPACE
