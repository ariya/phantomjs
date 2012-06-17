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

#include "project.h"
#include "property.h"
#include "option.h"
#include "cachekeys.h"
#include "metamakefile.h"
#include <qnamespace.h>
#include <qdebug.h>
#include <qregexp.h>
#include <qdir.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

QT_BEGIN_NAMESPACE

// for Borland, main is defined to qMain which breaks qmake
#undef main
#ifdef Q_OS_MAC
#endif

/* This is to work around lame implementation on Darwin. It has been noted that the getpwd(3) function
   is much too slow, and called much too often inside of Qt (every fileFixify). With this we use a locally
   cached copy because I can control all the times it is set (because Qt never sets the pwd under me).
*/
static QString pwd;
QString qmake_getpwd()
{
    if(pwd.isNull())
        pwd = QDir::currentPath();
    return pwd;
}
bool qmake_setpwd(const QString &p)
{
    if(QDir::setCurrent(p)) {
        pwd = QDir::currentPath();
        return true;
    }
    return false;
}

int runQMake(int argc, char **argv)
{
    // stderr is unbuffered by default, but stdout buffering depends on whether
    // there is a terminal attached. Buffering can make output from stderr and stdout
    // appear out of sync, so force stdout to be unbuffered as well.
    // This is particularly important for things like QtCreator and scripted builds.
    setvbuf(stdout, (char *)NULL, _IONBF, 0);

    // parse command line
    int ret = Option::init(argc, argv);
    if(ret != Option::QMAKE_CMDLINE_SUCCESS) {
        if ((ret & Option::QMAKE_CMDLINE_ERROR) != 0)
            return 1;
        return 0;
    }

    QString oldpwd = qmake_getpwd();
#ifdef Q_WS_WIN
    if(!(oldpwd.length() == 3 && oldpwd[0].isLetter() && oldpwd.endsWith(":/")))
#endif
    {
        if(oldpwd.right(1) != QString(QChar(QDir::separator())))
            oldpwd += QDir::separator();
    }
    Option::output_dir = oldpwd; //for now this is the output dir

    if(Option::output.fileName() != "-") {
        QFileInfo fi(Option::output);
        QString dir;
        if(fi.isDir()) {
            dir = fi.filePath();
        } else {
            QString tmp_dir = fi.path();
            if(!tmp_dir.isEmpty() && QFile::exists(tmp_dir))
                dir = tmp_dir;
        }
        if(!dir.isNull() && dir != ".")
            Option::output_dir = dir;
        if(QDir::isRelativePath(Option::output_dir))
            Option::output_dir.prepend(oldpwd);
        Option::output_dir = QDir::cleanPath(Option::output_dir);
    }

    QMakeProperty prop;
    if(Option::qmake_mode == Option::QMAKE_QUERY_PROPERTY ||
       Option::qmake_mode == Option::QMAKE_SET_PROPERTY ||
       Option::qmake_mode == Option::QMAKE_UNSET_PROPERTY)
        return prop.exec() ? 0 : 101;

    QMakeProject project(&prop);
    int exit_val = 0;
    QStringList files;
    if(Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT)
        files << "(*hack*)"; //we don't even use files, but we do the for() body once
    else
        files = Option::mkfile::project_files;
    for(QStringList::Iterator pfile = files.begin(); pfile != files.end(); pfile++) {
        if(Option::qmake_mode == Option::QMAKE_GENERATE_MAKEFILE ||
           Option::qmake_mode == Option::QMAKE_GENERATE_PRL) {
            QString fn = Option::fixPathToLocalOS((*pfile));
            if(!QFile::exists(fn)) {
                fprintf(stderr, "Cannot find file: %s.\n", fn.toLatin1().constData());
                exit_val = 2;
                continue;
            }

            //setup pwd properly
            debug_msg(1, "Resetting dir to: %s", oldpwd.toLatin1().constData());
            qmake_setpwd(oldpwd); //reset the old pwd
            int di = fn.lastIndexOf(QDir::separator());
            if(di != -1) {
                debug_msg(1, "Changing dir to: %s", fn.left(di).toLatin1().constData());
                if(!qmake_setpwd(fn.left(di)))
                    fprintf(stderr, "Cannot find directory: %s\n", fn.left(di).toLatin1().constData());
                fn = fn.right(fn.length() - di - 1);
            }

            // read project..
            if(!project.read(fn)) {
                fprintf(stderr, "Error processing project file: %s\n",
                        fn == "-" ? "(stdin)" : (*pfile).toLatin1().constData());
                exit_val = 3;
                continue;
            }
            if(Option::mkfile::do_preprocess) //no need to create makefile
                continue;
        }

        bool success = true;
        MetaMakefileGenerator *mkfile = MetaMakefileGenerator::createMetaGenerator(&project, QString(), false, &success);
        if (!success)
            exit_val = 3;

        if(mkfile && !mkfile->write(oldpwd)) {
            if(Option::qmake_mode == Option::QMAKE_GENERATE_PROJECT)
                fprintf(stderr, "Unable to generate project file.\n");
            else
                fprintf(stderr, "Unable to generate makefile for: %s\n", (*pfile).toLatin1().constData());
            exit_val = 5;
        }
        delete mkfile;
        mkfile = NULL;
    }
    qmakeClearCaches();
    return exit_val;
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    return QT_PREPEND_NAMESPACE(runQMake)(argc, argv);
}
