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

#include "meta.h"
#include "project.h"
#include "option.h"
#include <qdir.h>

QT_BEGIN_NAMESPACE

QHash<QString, ProValueMap> QMakeMetaInfo::cache_vars;

QMakeMetaInfo::QMakeMetaInfo(QMakeProject *_conf)
    : conf(_conf)
{

}


bool
QMakeMetaInfo::readLib(QString lib)
{
    clear();
    QString meta_file = findLib(lib);

    if(cache_vars.contains(meta_file)) {
        vars = cache_vars[meta_file];
        return true;
    }

    bool ret = false;
    if(!meta_file.isNull()) {
        if(meta_file.endsWith(Option::pkgcfg_ext)) {
            if((ret=readPkgCfgFile(meta_file)))
                meta_type = "pkgcfg";
        } else if(meta_file.endsWith(Option::libtool_ext)) {
            if((ret=readLibtoolFile(meta_file)))
                meta_type = "libtool";
        } else if(meta_file.endsWith(Option::prl_ext)) {
            QMakeProject proj;
            if (!proj.read(Option::normalizePath(meta_file), QMakeEvaluator::LoadProOnly))
                return false;
            meta_type = "qmake";
            vars = proj.variables();
            ret = true;
        } else {
            warn_msg(WarnLogic, "QMakeMetaInfo: unknown file format for %s",
                     QDir::toNativeSeparators(meta_file).toLatin1().constData());
        }
    }
    if(ret)
        cache_vars.insert(meta_file, vars);
    return ret;
}


void
QMakeMetaInfo::clear()
{
    vars.clear();
}


QString
QMakeMetaInfo::findLib(QString lib)
{
    if((lib[0] == '\'' || lib[0] == '"') &&
       lib[lib.length()-1] == lib[0])
    lib = lib.mid(1, lib.length()-2);
    lib = Option::normalizePath(lib);

    QString ret;
    QString extns[] = { Option::prl_ext, /*Option::pkgcfg_ext, Option::libtool_ext,*/ QString() };
    for(int extn = 0; !extns[extn].isNull(); extn++) {
        if(lib.endsWith(extns[extn]))
            ret = QFile::exists(lib) ? lib : QString();
    }
    if(ret.isNull()) {
        for(int extn = 0; !extns[extn].isNull(); extn++) {
            if(QFile::exists(lib + extns[extn])) {
                ret = lib + extns[extn];
                break;
            }
        }
    }
    if(ret.isNull()) {
        debug_msg(2, "QMakeMetaInfo: Cannot find info file for %s", lib.toLatin1().constData());
    } else {
        debug_msg(2, "QMakeMetaInfo: Found info file %s for %s", ret.toLatin1().constData(), lib.toLatin1().constData());
    }
    return ret;
}


bool
QMakeMetaInfo::readLibtoolFile(const QString &f)
{
    /* I can just run the .la through the .pro parser since they are compatible.. */
    QMakeProject proj;
    QString nf = Option::normalizePath(f);
    if (!proj.read(nf, QMakeEvaluator::LoadProOnly))
        return false;
    QString dirf = nf.section(QLatin1Char('/'), 0, -2);
    if(dirf == nf)
        dirf = "";
    else if(!dirf.isEmpty() && !dirf.endsWith(Option::output_dir))
        dirf += QLatin1Char('/');
    const ProValueMap &v = proj.variables();
    for (ProValueMap::ConstIterator it = v.begin(); it != v.end(); ++it) {
        ProStringList lst = it.value();
        if(lst.count() == 1 && (lst.first().startsWith("'") || lst.first().startsWith("\"")) &&
           lst.first().endsWith(QString(lst.first().at(0))))
            lst = ProStringList(lst.first().mid(1, lst.first().length() - 2));
        if(!vars.contains("QMAKE_PRL_TARGET") &&
           (it.key() == "dlname" || it.key() == "library_names" || it.key() == "old_library")) {
            ProString dir = v["libdir"].first();
            if ((dir.startsWith('\'') || dir.startsWith('"')) && dir.endsWith(dir.at(0)))
                dir = dir.mid(1, dir.length() - 2);
            dir = dir.trimmed();
            if(!dir.isEmpty() && !dir.endsWith(QLatin1Char('/')))
                dir += QLatin1Char('/');
            if(lst.count() == 1)
                lst = ProStringList(lst.first().toQString().split(" "));
            for (ProStringList::Iterator lst_it = lst.begin(); lst_it != lst.end(); ++lst_it) {
                bool found = false;
                QString dirs[] = { "", dir.toQString(), dirf, dirf + ".libs/", "(term)" };
                for(int i = 0; !found && dirs[i] != "(term)"; i++) {
                    if(QFile::exists(dirs[i] + (*lst_it))) {
                        QString targ = dirs[i] + (*lst_it);
                        if(QDir::isRelativePath(targ))
                            targ.prepend(qmake_getpwd() + QLatin1Char('/'));
                        vars["QMAKE_PRL_TARGET"] << targ;
                        found = true;
                    }
                }
                if(found)
                    break;
            }
        } else if(it.key() == "dependency_libs") {
            if(lst.count() == 1) {
                ProString dep = lst.first();
                if ((dep.startsWith('\'') || dep.startsWith('"')) && dep.endsWith(dep.at(0)))
                    dep = dep.mid(1, dep.length() - 2);
                lst = ProStringList(dep.trimmed().toQString().split(" "));
            }
            for (ProStringList::Iterator lit = lst.begin(); lit != lst.end(); ++lit) {
                if((*lit).startsWith("-R")) {
                    if(!conf->isEmpty("QMAKE_LFLAGS_RPATH"))
                        (*lit) = conf->first("QMAKE_LFLAGS_RPATH") + (*lit).mid(2);
                }
            }
            ProStringList &prlLibs = vars["QMAKE_PRL_LIBS"];
            foreach (const ProString &s, lst) {
                prlLibs.removeAll(s);
                prlLibs.append(s);
            }
        }
    }
    return true;
}

bool
QMakeMetaInfo::readPkgCfgFile(const QString &f)
{
    fprintf(stderr, "Must implement reading in pkg-config files (%s)!!!\n", f.toLatin1().constData());
    return false;
}

QT_END_NAMESPACE
