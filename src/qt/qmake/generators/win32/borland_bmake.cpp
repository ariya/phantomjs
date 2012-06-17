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

#include "borland_bmake.h"
#include "option.h"
#include <qdir.h>
#include <qregexp.h>
#include <time.h>

QT_BEGIN_NAMESPACE

BorlandMakefileGenerator::BorlandMakefileGenerator() : Win32MakefileGenerator(), init_flag(false)
{

}

bool
BorlandMakefileGenerator::writeMakefile(QTextStream &t)
{
    writeHeader(t);
    if(!project->values("QMAKE_FAILED_REQUIREMENTS").isEmpty()) {
        QStringList &qut = project->values("QMAKE_EXTRA_TARGETS");
        for(QStringList::ConstIterator it = qut.begin(); it != qut.end(); ++it)
            t << *it << " ";
        t << "all first clean:" << "\n\t"
          << "@echo \"Some of the required modules ("
          << var("QMAKE_FAILED_REQUIREMENTS") << ") are not available.\"" << "\n\t"
          << "@echo \"Skipped.\"" << endl << endl;
        return true;
    }

    if(project->first("TEMPLATE") == "app" ||
       project->first("TEMPLATE") == "lib" ||
       project->first("TEMPLATE") == "aux") {
        writeBorlandParts(t);
        return MakefileGenerator::writeMakefile(t);
    }
    else if(project->first("TEMPLATE") == "subdirs") {
        writeSubDirs(t);
        return true;
    }
    return false;
}

void
BorlandMakefileGenerator::writeBorlandParts(QTextStream &t)
{
    t << "!if !$d(BCB)" << endl;
    t << "BCB = $(MAKEDIR)\\.." << endl;
    t << "!endif" << endl << endl;

    writeStandardParts(t);
}

void
BorlandMakefileGenerator::init()
{
    if(init_flag)
        return;
    init_flag = true;

    /* this should probably not be here, but I'm using it to wrap the .t files */
    if (project->first("TEMPLATE") == "app") {
        project->values("QMAKE_APP_FLAG").append("1");
    } else if(project->first("TEMPLATE") == "lib"){
        project->values("QMAKE_LIB_FLAG").append("1");
    } else if(project->first("TEMPLATE") == "subdirs") {
        MakefileGenerator::init();
        if(project->isEmpty("QMAKE_COPY_FILE"))
            project->values("QMAKE_COPY_FILE").append("$(COPY)");
        if(project->isEmpty("QMAKE_COPY_DIR"))
            project->values("QMAKE_COPY_DIR").append("xcopy /s /q /y /i");
        if(project->isEmpty("QMAKE_INSTALL_FILE"))
            project->values("QMAKE_INSTALL_FILE").append("$(COPY_FILE)");
        if(project->isEmpty("QMAKE_INSTALL_PROGRAM"))
            project->values("QMAKE_INSTALL_PROGRAM").append("$(COPY_FILE)");
        if(project->isEmpty("QMAKE_INSTALL_DIR"))
            project->values("QMAKE_INSTALL_DIR").append("$(COPY_DIR)");
        if(project->values("MAKEFILE").isEmpty())
            project->values("MAKEFILE").append("Makefile");
        return;
    }

    processVars();

    project->values("QMAKE_LIBS") += project->values("LIBS");

    MakefileGenerator::init();

    if (project->isActiveConfig("dll") || !project->values("QMAKE_APP_FLAG").isEmpty()) {
        // bcc does not generate a .tds file for static libs
        QString tdsPostfix;
        if (!project->values("VERSION").isEmpty())
            tdsPostfix = project->first("TARGET_VERSION_EXT");
        tdsPostfix += ".tds";
        project->values("QMAKE_CLEAN").append(project->first("DESTDIR") + project->first("TARGET") + tdsPostfix);
    }
}

void BorlandMakefileGenerator::writeBuildRulesPart(QTextStream &t)
{
    if (project->first("TEMPLATE") == "aux") {
        t << "first:" << endl;
        return;
    }

    t << "first: all" << endl;
    t << "all: " << fileFixify(Option::output.fileName()) << " " << varGlue("ALL_DEPS"," "," "," ") << " $(DESTDIR_TARGET)" << endl << endl;
    t << "$(DESTDIR_TARGET): " << var("PRE_TARGETDEPS") << " $(OBJECTS) " << var("POST_TARGETDEPS");
    if(!project->isEmpty("QMAKE_PRE_LINK"))
        t << "\n\t" <<var("QMAKE_PRE_LINK");
    if(project->isActiveConfig("staticlib")) {
        t << "\n\t-$(DEL_FILE) $(DESTDIR_TARGET)"
	      << "\n\t" << "$(LIB) $(DESTDIR_TARGET) @&&|" << " \n+"
	      << project->values("OBJECTS").join(" \\\n+") << " \\\n+"
	      << project->values("OBJMOC").join(" \\\n+");
    } else {
        t << "\n\t" << "$(LINK) @&&|" << "\n\t"
	      << "$(LFLAGS) $(OBJECTS) $(OBJMOC),$(DESTDIR_TARGET),,$(LIBS),$(DEF_FILE),$(RES_FILE)";
    }
    t << endl << "|";
    if(!project->isEmpty("QMAKE_POST_LINK"))
        t << "\n\t" <<var("QMAKE_POST_LINK");
    t << endl;
}

void BorlandMakefileGenerator::writeCleanParts(QTextStream &t)
{
    t << "clean: "
      << varGlue("OBJECTS","\n\t-$(DEL_FILE) ","\n\t-$(DEL_FILE) ","")
      << varGlue("QMAKE_CLEAN","\n\t-$(DEL_FILE) ","\n\t-$(DEL_FILE) ","\n")
      << varGlue("CLEAN_FILES","\n\t-$(DEL_FILE) ","\n\t-$(DEL_FILE) ","\n");

    if(!project->isEmpty("IMAGES"))
        t << varGlue("QMAKE_IMAGE_COLLECTION", "\n\t-$(DEL_FILE) ", "\n\t-$(DEL_FILE) ", "");
    t << endl;

    t << "distclean: clean"
      << "\n\t-$(DEL_FILE) $(DESTDIR_TARGET)"
      << endl << endl;
}

QT_END_NAMESPACE
