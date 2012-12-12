/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "winmakefile.h"
#include "option.h"
#include "project.h"
#include "meta.h"
#include <qtextstream.h>
#include <qstring.h>
#include <qhash.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qdir.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

Win32MakefileGenerator::Win32MakefileGenerator() : MakefileGenerator()
{
}

int
Win32MakefileGenerator::findHighestVersion(const QString &d, const QString &stem, const QString &ext)
{
    QString bd = Option::fixPathToLocalOS(d, true);
    if(!exists(bd))
        return -1;

    QString dllStem = stem + QTDLL_POSTFIX;
    QMakeMetaInfo libinfo;
    bool libInfoRead = libinfo.readLib(bd + Option::dir_sep + dllStem);

    // If the library, for which we're trying to find the highest version
    // number, is a static library
    if (libInfoRead && libinfo.values("QMAKE_PRL_CONFIG").contains("staticlib"))
        return -1;

    if(!project->values("QMAKE_" + stem.toUpper() + "_VERSION_OVERRIDE").isEmpty())
        return project->values("QMAKE_" + stem.toUpper() + "_VERSION_OVERRIDE").first().toInt();

    int biggest=-1;
    if(!project->isActiveConfig("no_versionlink")) {
        static QHash<QString, QStringList> dirEntryListCache;
        QStringList entries = dirEntryListCache.value(bd);
        if (entries.isEmpty()) {
            QDir dir(bd);
            entries = dir.entryList();
            dirEntryListCache.insert(bd, entries);
        }

        QRegExp regx(QString("((lib)?%1([0-9]*)).(%2|prl)$").arg(dllStem).arg(ext), Qt::CaseInsensitive);
        for(QStringList::Iterator it = entries.begin(); it != entries.end(); ++it) {
            if(regx.exactMatch((*it))) {
				if (!regx.cap(3).isEmpty()) {
					bool ok = true;
					int num = regx.cap(3).toInt(&ok);
					biggest = qMax(biggest, (!ok ? -1 : num));
				}
			}
        }
    }
    if(libInfoRead
       && !libinfo.values("QMAKE_PRL_CONFIG").contains("staticlib")
       && !libinfo.isEmpty("QMAKE_PRL_VERSION"))
       biggest = qMax(biggest, libinfo.first("QMAKE_PRL_VERSION").replace(".", "").toInt());
    return biggest;
}

bool
Win32MakefileGenerator::findLibraries(const QString &where)
{
    QStringList &l = project->values(where);
    QList<QMakeLocalFileName> dirs;
    {
        QStringList &libpaths = project->values("QMAKE_LIBDIR");
        for(QStringList::Iterator libpathit = libpaths.begin();
            libpathit != libpaths.end(); ++libpathit)
            dirs.append(QMakeLocalFileName((*libpathit)));
    }
    for(QStringList::Iterator it = l.begin(); it != l.end();) {
        QChar quote;
        bool modified_opt = false, remove = false;
        QString opt = (*it).trimmed();
        if((opt[0] == '\'' || opt[0] == '"') && opt[(int)opt.length()-1] == opt[0]) {
            quote = opt[0];
            opt = opt.mid(1, opt.length()-2);
        }
        if(opt.startsWith("/LIBPATH:")) {
            dirs.append(QMakeLocalFileName(opt.mid(9)));
        } else if(opt.startsWith("-L") || opt.startsWith("/L")) {
            QString libpath = opt.mid(2);
            QMakeLocalFileName l(libpath);
            if(!dirs.contains(l)) {
                dirs.append(l);
                modified_opt = true;
                if (!quote.isNull()) {
                    libpath = quote + libpath + quote;
                    quote = QChar();
                }
                (*it) = "/LIBPATH:" + libpath;
            } else {
                remove = true;
            }
        } else if(opt.startsWith("-l") || opt.startsWith("/l")) {
            QString lib = opt.right(opt.length() - 2), out;
            if(!lib.isEmpty()) {
                QString suffix;
                if(!project->isEmpty("QMAKE_" + lib.toUpper() + "_SUFFIX"))
                    suffix = project->first("QMAKE_" + lib.toUpper() + "_SUFFIX");
                for(QList<QMakeLocalFileName>::Iterator it = dirs.begin();
                    it != dirs.end(); ++it) {
                    QString extension;
                    int ver = findHighestVersion((*it).local(), lib);
                    if(ver > 0)
                        extension += QString::number(ver);
                    extension += suffix;
                    extension += ".lib";
                    if(QMakeMetaInfo::libExists((*it).local() + Option::dir_sep + lib) ||
                       exists((*it).local() + Option::dir_sep + lib + extension)) {
                        out = (*it).real() + Option::dir_sep + lib + extension;
                        if (out.contains(QLatin1Char(' '))) {
                            out.prepend(QLatin1Char('\"'));
                            out.append(QLatin1Char('\"'));
                        }
                        break;
                    }
                }
            }
            if(out.isEmpty())
                out = lib + ".lib";
            modified_opt = true;
            (*it) = out;
        } else if(!exists(Option::fixPathToLocalOS(opt))) {
            QList<QMakeLocalFileName> lib_dirs;
            QString file = opt;
            int slsh = file.lastIndexOf(Option::dir_sep);
            if(slsh != -1) {
                lib_dirs.append(QMakeLocalFileName(file.left(slsh+1)));
                file = file.right(file.length() - slsh - 1);
            } else {
                lib_dirs = dirs;
            }
            if(file.endsWith(".lib")) {
                file = file.left(file.length() - 4);
                if(!file.at(file.length()-1).isNumber()) {
                    QString suffix;
                    if(!project->isEmpty("QMAKE_" + file.section(Option::dir_sep, -1).toUpper() + "_SUFFIX"))
                        suffix = project->first("QMAKE_" + file.section(Option::dir_sep, -1).toUpper() + "_SUFFIX");
                    for(QList<QMakeLocalFileName>::Iterator dep_it = lib_dirs.begin(); dep_it != lib_dirs.end(); ++dep_it) {
                        QString lib_tmpl(file + "%1" + suffix + ".lib");
                        int ver = findHighestVersion((*dep_it).local(), file);
                        if(ver != -1) {
                            if(ver)
                                lib_tmpl = lib_tmpl.arg(ver);
                            else
                                lib_tmpl = lib_tmpl.arg("");
                            if(slsh != -1) {
                                QString dir = (*dep_it).real();
                                if(!dir.endsWith(Option::dir_sep))
                                    dir += Option::dir_sep;
                                lib_tmpl.prepend(dir);
                            }
                            modified_opt = true;
                            (*it) = lib_tmpl;
                            break;
                        }
                    }
                }
            }
        }
        if(remove) {
            it = l.erase(it);
        } else {
            if(!quote.isNull() && modified_opt)
                (*it) = quote + (*it) + quote;
            ++it;
        }
    }
    return true;
}

void
Win32MakefileGenerator::processPrlFiles()
{
    QHash<QString, bool> processed;
    QList<QMakeLocalFileName> libdirs;
    {
        QStringList &libpaths = project->values("QMAKE_LIBDIR");
        for(QStringList::Iterator libpathit = libpaths.begin(); libpathit != libpaths.end(); ++libpathit)
            libdirs.append(QMakeLocalFileName((*libpathit)));
    }
    for(bool ret = false; true; ret = false) {
        //read in any prl files included..
        QStringList l_out;
        QString where = "QMAKE_LIBS";
        if(!project->isEmpty("QMAKE_INTERNAL_PRL_LIBS"))
            where = project->first("QMAKE_INTERNAL_PRL_LIBS");
        QStringList l = project->values(where);
        for(QStringList::Iterator it = l.begin(); it != l.end(); ++it) {
            QString opt = (*it).trimmed();
            if((opt[0] == '\'' || opt[0] == '"') && opt[(int)opt.length()-1] == opt[0])
                opt = opt.mid(1, opt.length()-2);
            if(opt.startsWith("/")) {
                if(opt.startsWith("/LIBPATH:")) {
                    QMakeLocalFileName l(opt.mid(9));
                    if(!libdirs.contains(l))
                        libdirs.append(l);
                }
            } else if(!processed.contains(opt)) {
                if(processPrlFile(opt)) {
                    processed.insert(opt, true);
                    ret = true;
                } else if(QDir::isRelativePath(opt) || opt.startsWith("-l")) {
                    QString tmp;
                    if (opt.startsWith("-l"))
                        tmp = opt.mid(2);
                    else
                        tmp = opt;
                    for(QList<QMakeLocalFileName>::Iterator it = libdirs.begin(); it != libdirs.end(); ++it) {
                        QString prl = (*it).local() + Option::dir_sep + tmp;
                        // the original is used as the key
                        QString orgprl = prl;
                        if(processed.contains(prl)) {
                            break;
                        } else if(processPrlFile(prl)) {
                            processed.insert(orgprl, true);
                            ret = true;
                            break;
                        }
                    }
                }
            }
            if(!opt.isEmpty())
                l_out.append(opt);
        }
        if(ret)
            l = l_out;
        else
            break;
    }
}


void Win32MakefileGenerator::processVars()
{
    //If the TARGET looks like a path split it into DESTDIR and the resulting TARGET
    if(!project->isEmpty("TARGET")) {
        QString targ = project->first("TARGET");
        int slsh = qMax(targ.lastIndexOf('/'), targ.lastIndexOf(Option::dir_sep));
        if(slsh != -1) {
            if(project->isEmpty("DESTDIR"))
                project->values("DESTDIR").append("");
            else if(project->first("DESTDIR").right(1) != Option::dir_sep)
                project->values("DESTDIR") = QStringList(project->first("DESTDIR") + Option::dir_sep);
            project->values("DESTDIR") = QStringList(project->first("DESTDIR") + targ.left(slsh+1));
            project->values("TARGET") = QStringList(targ.mid(slsh+1));
        }
    }

    project->values("QMAKE_ORIG_TARGET") = project->values("TARGET");
    if (project->isEmpty("QMAKE_PROJECT_NAME"))
        project->values("QMAKE_PROJECT_NAME") = project->values("QMAKE_ORIG_TARGET");
    else if (project->first("TEMPLATE").startsWith("vc"))
        project->values("MAKEFILE") = project->values("QMAKE_PROJECT_NAME");

    if (!project->values("QMAKE_INCDIR").isEmpty())
        project->values("INCLUDEPATH") += project->values("QMAKE_INCDIR");

    if (!project->values("VERSION").isEmpty()) {
        QStringList l = project->first("VERSION").split('.');
        if (l.size() > 0)
            project->values("VER_MAJ").append(l[0]);
        if (l.size() > 1)
            project->values("VER_MIN").append(l[1]);
    }

    // TARGET_VERSION_EXT will be used to add a version number onto the target name
    if (project->values("TARGET_VERSION_EXT").isEmpty()
        && !project->values("VER_MAJ").isEmpty()
        && project->values("QMAKE_SYMBIAN_SHLIB").isEmpty())
        project->values("TARGET_VERSION_EXT").append(project->values("VER_MAJ").first());

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

    fixTargetExt();
    processRcFileVar();
    processFileTagsVar();

    QStringList &incDir = project->values("INCLUDEPATH");
    for(QStringList::Iterator incDir_it = incDir.begin(); incDir_it != incDir.end(); ++incDir_it) {
    if(!(*incDir_it).isEmpty())
        (*incDir_it) = Option::fixPathToTargetOS((*incDir_it), false, false);
    }
    QStringList &libDir = project->values("QMAKE_LIBDIR");
    for(QStringList::Iterator libDir_it = libDir.begin(); libDir_it != libDir.end(); ++libDir_it) {
    if(!(*libDir_it).isEmpty())
        (*libDir_it) = Option::fixPathToTargetOS((*libDir_it), false, false);
    }

    if (project->values("TEMPLATE").contains("app")) {
        project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_APP");
        project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_APP");
        project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_APP");
    } else if (project->values("TEMPLATE").contains("lib") && project->isActiveConfig("dll")) {
        if(!project->isActiveConfig("plugin") || !project->isActiveConfig("plugin_no_share_shlib_cflags")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_SHLIB");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_SHLIB");
        }
        if (project->isActiveConfig("plugin")) {
            project->values("QMAKE_CFLAGS") += project->values("QMAKE_CFLAGS_PLUGIN");
            project->values("QMAKE_CXXFLAGS") += project->values("QMAKE_CXXFLAGS_PLUGIN");
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_PLUGIN");
        } else {
            project->values("QMAKE_LFLAGS") += project->values("QMAKE_LFLAGS_SHLIB");
        }
    }
}

void Win32MakefileGenerator::fixTargetExt()
{
    if (project->isEmpty("QMAKE_EXTENSION_STATICLIB"))
        project->values("QMAKE_EXTENSION_STATICLIB").append("lib");
    if (project->isEmpty("QMAKE_EXTENSION_SHLIB"))
        project->values("QMAKE_EXTENSION_SHLIB").append("dll");

    if (!project->values("QMAKE_APP_FLAG").isEmpty()) {
        project->values("TARGET_EXT").append(".exe");
    } else if (project->isActiveConfig("shared")) {
        project->values("TARGET_EXT").append(project->first("TARGET_VERSION_EXT") + "."
                + project->first("QMAKE_EXTENSION_SHLIB"));
        project->values("TARGET").first() = project->first("QMAKE_PREFIX_SHLIB") + project->first("TARGET");
    } else {
        project->values("TARGET_EXT").append("." + project->first("QMAKE_EXTENSION_STATICLIB"));
        project->values("TARGET").first() = project->first("QMAKE_PREFIX_STATICLIB") + project->first("TARGET");
    }
}

void Win32MakefileGenerator::processRcFileVar()
{
    if (Option::qmake_mode == Option::QMAKE_GENERATE_NOTHING)
        return;

    if (((!project->values("VERSION").isEmpty())
        && project->values("RC_FILE").isEmpty()
        && project->values("RES_FILE").isEmpty()
        && !project->isActiveConfig("no_generated_target_info")
        && (project->isActiveConfig("shared") || !project->values("QMAKE_APP_FLAG").isEmpty()))
        || !project->values("QMAKE_WRITE_DEFAULT_RC").isEmpty()){

        QByteArray rcString;
        QTextStream ts(&rcString, QFile::WriteOnly);

        QStringList vers = project->values("VERSION").first().split(".");
        for (int i = vers.size(); i < 4; i++)
            vers += "0";
        QString versionString = vers.join(".");

        QString companyName;
        if (!project->values("QMAKE_TARGET_COMPANY").isEmpty())
            companyName = project->values("QMAKE_TARGET_COMPANY").join(" ");

        QString description;
        if (!project->values("QMAKE_TARGET_DESCRIPTION").isEmpty())
            description = project->values("QMAKE_TARGET_DESCRIPTION").join(" ");

        QString copyright;
        if (!project->values("QMAKE_TARGET_COPYRIGHT").isEmpty())
            copyright = project->values("QMAKE_TARGET_COPYRIGHT").join(" ");

        QString productName;
        if (!project->values("QMAKE_TARGET_PRODUCT").isEmpty())
            productName = project->values("QMAKE_TARGET_PRODUCT").join(" ");
        else
            productName = project->values("TARGET").first();

        QString originalName = project->values("TARGET").first() + project->values("TARGET_EXT").first();
        int rcLang = project->intValue("RC_LANG", 1033);            // default: English(USA)
        int rcCodePage = project->intValue("RC_CODEPAGE", 1200);    // default: Unicode

        ts << "# if defined(UNDER_CE)" << endl;
        ts << "#  include <winbase.h>" << endl;
        ts << "# else" << endl;
        ts << "#  include <winver.h>" << endl;
        ts << "# endif" << endl;
        ts << endl;
        ts << "VS_VERSION_INFO VERSIONINFO" << endl;
        ts << "\tFILEVERSION " << QString(versionString).replace(".", ",") << endl;
        ts << "\tPRODUCTVERSION " << QString(versionString).replace(".", ",") << endl;
        ts << "\tFILEFLAGSMASK 0x3fL" << endl;
        ts << "#ifdef _DEBUG" << endl;
        ts << "\tFILEFLAGS VS_FF_DEBUG" << endl;
        ts << "#else" << endl;
        ts << "\tFILEFLAGS 0x0L" << endl;
        ts << "#endif" << endl;
        ts << "\tFILEOS VOS__WINDOWS32" << endl;
        if (project->isActiveConfig("shared"))
            ts << "\tFILETYPE VFT_DLL" << endl;
        else
            ts << "\tFILETYPE VFT_APP" << endl;
        ts << "\tFILESUBTYPE 0x0L" << endl;
        ts << "\tBEGIN" << endl;
        ts << "\t\tBLOCK \"StringFileInfo\"" << endl;
        ts << "\t\tBEGIN" << endl;
        ts << "\t\t\tBLOCK \""
           << QString("%1%2").arg(rcLang, 4, 16, QLatin1Char('0')).arg(rcCodePage, 4, 16, QLatin1Char('0'))
           << "\"" << endl;
        ts << "\t\t\tBEGIN" << endl;
        ts << "\t\t\t\tVALUE \"CompanyName\", \"" << companyName << "\\0\"" << endl;
        ts << "\t\t\t\tVALUE \"FileDescription\", \"" <<  description << "\\0\"" << endl;
        ts << "\t\t\t\tVALUE \"FileVersion\", \"" << versionString << "\\0\"" << endl;
        ts << "\t\t\t\tVALUE \"LegalCopyright\", \"" << copyright << "\\0\"" << endl;
        ts << "\t\t\t\tVALUE \"OriginalFilename\", \"" << originalName << "\\0\"" << endl;
        ts << "\t\t\t\tVALUE \"ProductName\", \"" << productName << "\\0\"" << endl;
        ts << "\t\t\tEND" << endl;
        ts << "\t\tEND" << endl;
        ts << "\t\tBLOCK \"VarFileInfo\"" << endl;
        ts << "\t\tBEGIN" << endl;
        ts << "\t\t\tVALUE \"Translation\", "
           << QString("0x%1").arg(rcLang, 4, 16, QLatin1Char('0'))
           << ", " << QString("%1").arg(rcCodePage, 4) << endl;
        ts << "\t\tEND" << endl;
        ts << "\t\tBLOCK \"VarFileInfo\"" << endl;
        ts << "\t\tBEGIN" << endl;
        ts << "\t\t\tVALUE \"Translation\", 0x409, 1200" << endl;
        ts << "\t\tEND" << endl;
        ts << "\tEND" << endl;
        ts << "/* End of Version info */" << endl;
        ts << endl;

        ts.flush();


        QString rcFilename = project->values("OUT_PWD").first()
                           + "/"
                           + project->values("TARGET").first()
                           + "_resource"
                           + ".rc";
        QFile rcFile(QDir::cleanPath(rcFilename));

        bool writeRcFile = true;
        if (rcFile.exists() && rcFile.open(QFile::ReadOnly)) {
            writeRcFile = rcFile.readAll() != rcString;
            rcFile.close();
        }
        if (writeRcFile) {
	    bool ok;
	    ok = rcFile.open(QFile::WriteOnly);
	    if (!ok) {
		// The file can't be opened... try creating the containing
		// directory first (needed for clean shadow builds)
		QDir().mkpath(QFileInfo(rcFile).path());
		ok = rcFile.open(QFile::WriteOnly);
	    }
	    if (!ok) {
		::fprintf(stderr, "Cannot open for writing: %s", rcFile.fileName().toLatin1().constData());
		::exit(1);
	    }
	    rcFile.write(rcString);
	    rcFile.close();
        }
        if (project->values("QMAKE_WRITE_DEFAULT_RC").isEmpty())
            project->values("RC_FILE").insert(0, rcFile.fileName());
    }
    if (!project->values("RC_FILE").isEmpty()) {
        if (!project->values("RES_FILE").isEmpty()) {
            fprintf(stderr, "Both rc and res file specified.\n");
            fprintf(stderr, "Please specify one of them, not both.");
            exit(1);
        }
        QString resFile = project->values("RC_FILE").first();

        // if this is a shadow build then use the absolute path of the rc file
        if (Option::output_dir != qmake_getpwd()) {
            QFileInfo fi(resFile);
            project->values("RC_FILE").first() = fi.absoluteFilePath();
        }

        resFile.replace(".rc", Option::res_ext);
        project->values("RES_FILE").prepend(fileInfo(resFile).fileName());
        if (!project->values("OBJECTS_DIR").isEmpty()) {
            QString resDestDir;
            if (project->isActiveConfig("staticlib"))
                resDestDir = fileInfo(project->first("DESTDIR")).absoluteFilePath();
            else
                resDestDir = project->first("OBJECTS_DIR");
            resDestDir.append(Option::dir_sep);
            project->values("RES_FILE").first().prepend(resDestDir);
        }
        project->values("RES_FILE").first() = Option::fixPathToTargetOS(project->values("RES_FILE").first(), false, false);
	project->values("POST_TARGETDEPS") += project->values("RES_FILE");
        project->values("CLEAN_FILES") += project->values("RES_FILE");
    }
}

void Win32MakefileGenerator::processFileTagsVar()
{
    QStringList tags;
    tags << "SOURCES" << "GENERATED_SOURCES" << "DEF_FILE" << "RC_FILE"
         << "TARGET" << "QMAKE_LIBS" << "DESTDIR" << "DLLDESTDIR" << "INCLUDEPATH";
    if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
        const QStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
        for(QStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it)
            tags += project->values((*it)+".input");
    }

    //clean path
    QStringList &filetags = project->values("QMAKE_FILETAGS");
    for(int i = 0; i < tags.size(); ++i)
        filetags += Option::fixPathToTargetOS(tags.at(i), false);
}

void Win32MakefileGenerator::writeCleanParts(QTextStream &t)
{
    t << "clean: compiler_clean " << var("CLEAN_DEPS");
    {
        const char *clean_targets[] = { "OBJECTS", "QMAKE_CLEAN", "CLEAN_FILES", 0 };
        for(int i = 0; clean_targets[i]; ++i) {
            const QStringList &list = project->values(clean_targets[i]);
            const QString del_statement("-$(DEL_FILE)");
            if(project->isActiveConfig("no_delete_multiple_files")) {
                for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
                    t << "\n\t" << del_statement << " " << escapeFilePath((*it));
            } else {
                QString files, file;
                const int commandlineLimit = 2047; // NT limit, expanded
                for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
                    file = " " + escapeFilePath((*it));
                    if(del_statement.length() + files.length() +
                       qMax(fixEnvVariables(file).length(), file.length()) > commandlineLimit) {
                        t << "\n\t" << del_statement << files;
                        files.clear();
                    }
                    files += file;
                }
                if(!files.isEmpty())
                    t << "\n\t" << del_statement << files;
            }
        }
    }
    t << endl << endl;

    t << "distclean: clean";
    {
        const char *clean_targets[] = { "QMAKE_DISTCLEAN", 0 };
        for(int i = 0; clean_targets[i]; ++i) {
            const QStringList &list = project->values(clean_targets[i]);
            const QString del_statement("-$(DEL_FILE)");
            if(project->isActiveConfig("no_delete_multiple_files")) {
                for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it)
                    t << "\n\t" << del_statement << " " << escapeFilePath((*it));
            } else {
                QString files, file;
                const int commandlineLimit = 2047; // NT limit, expanded
                for(QStringList::ConstIterator it = list.begin(); it != list.end(); ++it) {
                    file = " " + escapeFilePath((*it));
                    if(del_statement.length() + files.length() +
                       qMax(fixEnvVariables(file).length(), file.length()) > commandlineLimit) {
                        t << "\n\t" << del_statement << files;
                        files.clear();
                    }
                    files += file;
                }
                if(!files.isEmpty())
                    t << "\n\t" << del_statement << files;
            }
        }
    }
    t << "\n\t-$(DEL_FILE) $(DESTDIR_TARGET)" << endl;
    {
        QString ofile = Option::fixPathToTargetOS(fileFixify(Option::output.fileName()));
        if(!ofile.isEmpty())
            t << "\t-$(DEL_FILE) " << ofile << endl;
    }
    t << endl;
}

void Win32MakefileGenerator::writeIncPart(QTextStream &t)
{
    t << "INCPATH       = ";

    const QStringList &incs = project->values("INCLUDEPATH");
    for(int i = 0; i < incs.size(); ++i) {
        QString inc = incs.at(i);
        inc.replace(QRegExp("\\\\$"), "");
        inc.replace(QRegExp("\""), "");
        if(!inc.isEmpty())
            t << "-I" << "\"" << inc << "\" ";
    }
    t << "-I\"" << specdir() << "\""
      << endl;
}

void Win32MakefileGenerator::writeStandardParts(QTextStream &t)
{
    t << "####### Compiler, tools and options" << endl << endl;
    t << "CC            = " << var("QMAKE_CC") << endl;
    t << "CXX           = " << var("QMAKE_CXX") << endl;
    t << "DEFINES       = "
      << varGlue("PRL_EXPORT_DEFINES","-D"," -D"," ")
      << varGlue("DEFINES","-D"," -D","") << endl;
    t << "CFLAGS        = " << var("QMAKE_CFLAGS") << " $(DEFINES)" << endl;
    t << "CXXFLAGS      = " << var("QMAKE_CXXFLAGS") << " $(DEFINES)" << endl;

    writeIncPart(t);
    writeLibsPart(t);

    t << "QMAKE         = " << var("QMAKE_QMAKE") << endl;
    t << "IDC           = " << (project->isEmpty("QMAKE_IDC") ? QString("idc") :
                              Option::fixPathToTargetOS(var("QMAKE_IDC"), false)) << endl;
    t << "IDL           = " << (project->isEmpty("QMAKE_IDL") ? QString("midl") :
                              Option::fixPathToTargetOS(var("QMAKE_IDL"), false)) << endl;
    t << "ZIP           = " << var("QMAKE_ZIP") << endl;
    t << "DEF_FILE      = " << varList("DEF_FILE") << endl;
    t << "RES_FILE      = " << varList("RES_FILE") << endl; // Not on mingw, can't see why not though...
    t << "COPY          = " << var("QMAKE_COPY") << endl;
    t << "SED           = " << var("QMAKE_STREAM_EDITOR") << endl;
    t << "COPY_FILE     = " << var("QMAKE_COPY_FILE") << endl;
    t << "COPY_DIR      = " << var("QMAKE_COPY_DIR") << endl;
    t << "DEL_FILE      = " << var("QMAKE_DEL_FILE") << endl;
    t << "DEL_DIR       = " << var("QMAKE_DEL_DIR") << endl;
    t << "MOVE          = " << var("QMAKE_MOVE") << endl;
    t << "CHK_DIR_EXISTS= " << var("QMAKE_CHK_DIR_EXISTS") << endl;
    t << "MKDIR         = " << var("QMAKE_MKDIR") << endl;
    t << "INSTALL_FILE    = " << var("QMAKE_INSTALL_FILE") << endl;
    t << "INSTALL_PROGRAM = " << var("QMAKE_INSTALL_PROGRAM") << endl;
    t << "INSTALL_DIR     = " << var("QMAKE_INSTALL_DIR") << endl;
    t << endl;

    t << "####### Output directory" << endl << endl;
    if(!project->values("OBJECTS_DIR").isEmpty())
        t << "OBJECTS_DIR   = " << var("OBJECTS_DIR").replace(QRegExp("\\\\$"),"") << endl;
    else
        t << "OBJECTS_DIR   = . " << endl;
    t << endl;

    t << "####### Files" << endl << endl;
    t << "SOURCES       = " << valList(escapeFilePaths(project->values("SOURCES")))
      << " " << valList(escapeFilePaths(project->values("GENERATED_SOURCES"))) << endl;

    // do this here so we can set DEST_TARGET to be the complete path to the final target if it is needed.
    QString orgDestDir = var("DESTDIR");
    QString destDir = Option::fixPathToTargetOS(orgDestDir, false);
    if (!destDir.isEmpty() && (orgDestDir.endsWith('/') || orgDestDir.endsWith(Option::dir_sep)))
        destDir += Option::dir_sep;
    QString target = QString(project->first("TARGET")+project->first("TARGET_EXT"));
    target.remove("\"");
    project->values("DEST_TARGET").prepend(destDir + target);

    writeObjectsPart(t);

    writeExtraCompilerVariables(t);
    writeExtraVariables(t);

    t << "DIST          = " << varList("DISTFILES") << endl;
    t << "QMAKE_TARGET  = " << var("QMAKE_ORIG_TARGET") << endl;
    // The comment is important to maintain variable compatibility with Unix
    // Makefiles, while not interpreting a trailing-slash as a linebreak
    t << "DESTDIR        = " << escapeFilePath(destDir) << " #avoid trailing-slash linebreak" << endl;
    t << "TARGET         = " << escapeFilePath(target) << endl;
    t << "DESTDIR_TARGET = " << escapeFilePath(var("DEST_TARGET")) << endl;
    t << endl;

    t << "####### Implicit rules" << endl << endl;
    writeImplicitRulesPart(t);

    t << "####### Build rules" << endl << endl;
    writeBuildRulesPart(t);

    if(project->isActiveConfig("shared") && !project->values("DLLDESTDIR").isEmpty()) {
        QStringList dlldirs = project->values("DLLDESTDIR");
        for (QStringList::Iterator dlldir = dlldirs.begin(); dlldir != dlldirs.end(); ++dlldir) {
            t << "\t" << "-$(COPY_FILE) \"$(DESTDIR_TARGET)\" " << Option::fixPathToTargetOS(*dlldir, false) << endl;
        }
    }
    t << endl;

    writeRcFilePart(t);

    writeMakeQmake(t);

    QStringList dist_files = fileFixify(Option::mkfile::project_files);
    if(!project->isEmpty("QMAKE_INTERNAL_INCLUDED_FILES"))
        dist_files += project->values("QMAKE_INTERNAL_INCLUDED_FILES");
    if(!project->isEmpty("TRANSLATIONS"))
        dist_files << var("TRANSLATIONS");
    if(!project->isEmpty("FORMS")) {
        QStringList &forms = project->values("FORMS");
        for(QStringList::Iterator formit = forms.begin(); formit != forms.end(); ++formit) {
            QString ui_h = fileFixify((*formit) + Option::h_ext.first());
            if(exists(ui_h))
                dist_files << ui_h;
        }
    }
    t << "dist:" << "\n\t"
      << "$(ZIP) " << var("QMAKE_ORIG_TARGET") << ".zip " << "$(SOURCES) $(DIST) "
      << dist_files.join(" ") << " " << var("TRANSLATIONS") << " ";
    if(!project->isEmpty("QMAKE_EXTRA_COMPILERS")) {
        const QStringList &quc = project->values("QMAKE_EXTRA_COMPILERS");
        for(QStringList::ConstIterator it = quc.begin(); it != quc.end(); ++it) {
            const QStringList &inputs = project->values((*it)+".input");
            for(QStringList::ConstIterator input = inputs.begin(); input != inputs.end(); ++input) {
                t << (*input) << " ";
            }
        }
    }
    t << endl << endl;

    writeCleanParts(t);
    writeExtraTargets(t);
    writeExtraCompilerTargets(t);
    t << endl << endl;
}

void Win32MakefileGenerator::writeLibsPart(QTextStream &t)
{
    if(project->isActiveConfig("staticlib") && project->first("TEMPLATE") == "lib") {
        t << "LIBAPP        = " << var("QMAKE_LIB") << endl;
        t << "LIBFLAGS      = " << var("QMAKE_LIBFLAGS") << endl;
    } else {
        t << "LINK          = " << var("QMAKE_LINK") << endl;
        t << "LFLAGS        = ";
        if(!project->values("QMAKE_LIBDIR").isEmpty())
            writeLibDirPart(t);
        t << var("QMAKE_LFLAGS") << endl;
        t << "LIBS          = " << var("QMAKE_LIBS") << " " << var("QMAKE_LIBS_PRIVATE") << endl;
    }
}

void Win32MakefileGenerator::writeLibDirPart(QTextStream &t)
{
    QStringList libDirs = project->values("QMAKE_LIBDIR");
    for (int i = 0; i < libDirs.size(); ++i)
        libDirs[i].remove("\"");
    t << valGlue(libDirs,"-L\"","\" -L\"","\"") << " ";
}

void Win32MakefileGenerator::writeObjectsPart(QTextStream &t)
{
    t << "OBJECTS       = " << valList(escapeDependencyPaths(project->values("OBJECTS"))) << endl;
}

void Win32MakefileGenerator::writeImplicitRulesPart(QTextStream &t)
{
    t << ".SUFFIXES:";
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
        t << " " << (*cppit);
    for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
        t << " " << (*cit);
    t << endl << endl;
    for(QStringList::Iterator cppit = Option::cpp_ext.begin(); cppit != Option::cpp_ext.end(); ++cppit)
        t << (*cppit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CXX_IMP") << endl << endl;
    for(QStringList::Iterator cit = Option::c_ext.begin(); cit != Option::c_ext.end(); ++cit)
        t << (*cit) << Option::obj_ext << ":\n\t" << var("QMAKE_RUN_CC_IMP") << endl << endl;
}

void Win32MakefileGenerator::writeBuildRulesPart(QTextStream &)
{
}

void Win32MakefileGenerator::writeRcFilePart(QTextStream &t)
{
    if(!project->values("RC_FILE").isEmpty()) {
        const QString res_file = project->first("RES_FILE"),
                       rc_file = fileFixify(project->first("RC_FILE"));
        // The resource tool needs to have the same defines passed in as the compiler, since you may
        // use these defines in the .rc file itself. Also, we need to add the _DEBUG define manually
        // since the compiler defines this symbol by itself, and we use it in the automatically
        // created rc file when VERSION is define the .pro file.
        t << res_file << ": " << rc_file << "\n\t"
          << var("QMAKE_RC") << (project->isActiveConfig("debug") ? " -D_DEBUG" : "") << " $(DEFINES) -fo " << res_file << " " << rc_file;
        t << endl << endl;
    }
}

QString Win32MakefileGenerator::getLibTarget()
{
    return QString(project->first("TARGET") + project->first("TARGET_VERSION_EXT") + ".lib");
}

QString Win32MakefileGenerator::defaultInstall(const QString &t)
{
    if((t != "target" && t != "dlltarget") ||
       (t == "dlltarget" && (project->first("TEMPLATE") != "lib" || !project->isActiveConfig("shared"))) ||
        project->first("TEMPLATE") == "subdirs")
       return QString();

    const QString root = "$(INSTALL_ROOT)";
    QStringList &uninst = project->values(t + ".uninstall");
    QString ret;
    QString targetdir = Option::fixPathToTargetOS(project->first(t + ".path"), false);
    targetdir = fileFixify(targetdir, FileFixifyAbsolute);
    if(targetdir.right(1) != Option::dir_sep)
        targetdir += Option::dir_sep;

    if(t == "target" && project->first("TEMPLATE") == "lib") {
        if(project->isActiveConfig("create_prl") && !project->isActiveConfig("no_install_prl") &&
           !project->isEmpty("QMAKE_INTERNAL_PRL_FILE")) {
            QString dst_prl = Option::fixPathToTargetOS(project->first("QMAKE_INTERNAL_PRL_FILE"));
            int slsh = dst_prl.lastIndexOf(Option::dir_sep);
            if(slsh != -1)
                dst_prl = dst_prl.right(dst_prl.length() - slsh - 1);
            dst_prl = filePrefixRoot(root, targetdir + dst_prl);
            ret += "-$(INSTALL_FILE) \"" + project->first("QMAKE_INTERNAL_PRL_FILE") + "\" \"" + dst_prl + "\"";
            if(!uninst.isEmpty())
                uninst.append("\n\t");
            uninst.append("-$(DEL_FILE) \"" + dst_prl + "\"");
        }
        if(project->isActiveConfig("create_pc")) {
            QString dst_pc = pkgConfigFileName(false);
            if (!dst_pc.isEmpty()) {
                dst_pc = filePrefixRoot(root, targetdir + dst_pc);
                const QString dst_pc_dir = Option::fixPathToTargetOS(fileInfo(dst_pc).path(), false);
                if (!dst_pc_dir.isEmpty()) {
                    if (!ret.isEmpty())
                        ret += "\n\t";
                    ret += mkdir_p_asstring(dst_pc_dir, true);
                }
                if(!ret.isEmpty())
                    ret += "\n\t";
                const QString replace_rule("QMAKE_PKGCONFIG_INSTALL_REPLACE");
                if (project->isEmpty(replace_rule)
                    || project->isActiveConfig("no_sed_meta_install")
                    || project->isEmpty("QMAKE_STREAM_EDITOR")) {
                    ret += "-$(INSTALL_FILE) \"" + pkgConfigFileName(true) + "\" \"" + dst_pc + "\"";
                } else {
                    ret += "-$(SED)";
                    QStringList replace_rules = project->values(replace_rule);
                    for (int r = 0; r < replace_rules.size(); ++r) {
                        const QString match = project->first(replace_rules.at(r) + ".match"),
                                    replace = project->first(replace_rules.at(r) + ".replace");
                        if (!match.isEmpty() /*&& match != replace*/)
                            ret += " -e \"s," + match + "," + replace + ",g\"";
                    }
                    ret += " \"" + pkgConfigFileName(true) + "\" >\"" + dst_pc + "\"";
                }
                if(!uninst.isEmpty())
                    uninst.append("\n\t");
                uninst.append("-$(DEL_FILE) \"" + dst_pc + "\"");
            }
        }
        if(project->isActiveConfig("shared") && !project->isActiveConfig("plugin")) {
            QString lib_target = getLibTarget();
            lib_target.remove('"');
            QString src_targ = (project->isEmpty("DESTDIR") ? QString("$(DESTDIR)") : project->first("DESTDIR")) + lib_target;
            QString dst_targ = filePrefixRoot(root, fileFixify(targetdir + lib_target, FileFixifyAbsolute));
            if(!ret.isEmpty())
                ret += "\n\t";
            ret += QString("-$(INSTALL_FILE)") + " \"" + src_targ + "\" \"" + dst_targ + "\"";
            if(!uninst.isEmpty())
                uninst.append("\n\t");
            uninst.append("-$(DEL_FILE) \"" + dst_targ + "\"");
        }
    }

    if(t == "dlltarget" || project->values(t + ".CONFIG").indexOf("no_dll") == -1) {
        QString src_targ = "$(DESTDIR_TARGET)";
        QString dst_targ = filePrefixRoot(root, fileFixify(targetdir + "$(TARGET)", FileFixifyAbsolute));
        if(!ret.isEmpty())
            ret += "\n\t";
        ret += QString("-$(INSTALL_FILE)") + " \"" + src_targ + "\" \"" + dst_targ + "\"";
        if(!uninst.isEmpty())
            uninst.append("\n\t");
        uninst.append("-$(DEL_FILE) \"" + dst_targ + "\"");
    }
    return ret;
}

QString Win32MakefileGenerator::escapeFilePath(const QString &path) const
{
    QString ret = path;
    if(!ret.isEmpty()) {
        ret = unescapeFilePath(ret);
        if(ret.contains(" "))
            ret = "\"" + ret + "\"";
        debug_msg(2, "EscapeFilePath: %s -> %s", path.toLatin1().constData(), ret.toLatin1().constData());
    }
    return ret;
}

QT_END_NAMESPACE
