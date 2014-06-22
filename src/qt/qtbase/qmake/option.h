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

#ifndef OPTION_H
#define OPTION_H

#include <qmakeglobals.h>
#include <qmakevfs.h>
#include <qmakeparser.h>
#include <qmakeevaluator.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qlibraryinfo.h>

QT_BEGIN_NAMESPACE

#define QMAKE_VERSION_STR "3.0"

QString qmake_getpwd();
bool qmake_setpwd(const QString &p);

#define debug_msg if(Option::debug_level) debug_msg_internal
void debug_msg_internal(int level, const char *fmt, ...); //don't call directly, use debug_msg
enum QMakeWarn {
    WarnNone    = 0x00,
    WarnParser  = 0x01,
    WarnLogic   = 0x02,
    WarnDeprecated = 0x04,
    WarnAll     = 0xFF
};
void warn_msg(QMakeWarn t, const char *fmt, ...);

class QMakeProject;

class EvalHandler : public QMakeHandler {
public:
    void message(int type, const QString &msg, const QString &fileName, int lineNo);

    void fileMessage(const QString &msg);

    void aboutToEval(ProFile *, ProFile *, EvalFileType);
    void doneWithEval(ProFile *);
};

struct Option
{
    static EvalHandler evalHandler;

    static QMakeGlobals *globals;
    static ProFileCache *proFileCache;
    static QMakeVfs *vfs;
    static QMakeParser *parser;

    //simply global convenience
    static QString libtool_ext;
    static QString pkgcfg_ext;
    static QString prf_ext;
    static QString prl_ext;
    static QString ui_ext;
    static QStringList h_ext;
    static QStringList cpp_ext;
    static QStringList c_ext;
    static QString cpp_moc_ext;
    static QString obj_ext;
    static QString lex_ext;
    static QString yacc_ext;
    static QString h_moc_mod;
    static QString lex_mod;
    static QString yacc_mod;
    static QString dir_sep;
    static QString pro_ext;
    static QString res_ext;
    static char field_sep;

    enum CmdLineFlags {
        QMAKE_CMDLINE_SUCCESS       = 0x00,
        QMAKE_CMDLINE_SHOW_USAGE    = 0x01,
        QMAKE_CMDLINE_BAIL          = 0x02,
        QMAKE_CMDLINE_ERROR         = 0x04
    };

    //both of these must be called..
    static int init(int argc=0, char **argv=0); //parse cmdline
    static void prepareProject(const QString &pfile);
    static bool postProcessProject(QMakeProject *);

    enum StringFixFlags {
        FixNone                 = 0x00,
        FixEnvVars              = 0x01,
        FixPathCanonicalize     = 0x02,
        FixPathToLocalSeparators  = 0x04,
        FixPathToTargetSeparators = 0x08,
        FixPathToNormalSeparators = 0x10
    };
    static QString fixString(QString string, uchar flags);

    //and convenience functions
    inline static QString fixPathToLocalOS(const QString &in, bool fix_env=true, bool canonical=true)
    {
        uchar flags = FixPathToLocalSeparators;
        if(fix_env)
            flags |= FixEnvVars;
        if(canonical)
            flags |= FixPathCanonicalize;
        return fixString(in, flags);
    }
    inline static QString fixPathToTargetOS(const QString &in, bool fix_env=true, bool canonical=true)
    {
        uchar flags = FixPathToTargetSeparators;
        if(fix_env)
            flags |= FixEnvVars;
        if(canonical)
            flags |= FixPathCanonicalize;
        return fixString(in, flags);
    }
    inline static QString normalizePath(const QString &in, bool fix_env=true, bool canonical=true)
    {
        uchar flags = FixPathToNormalSeparators;
        if (fix_env)
            flags |= FixEnvVars;
        if (canonical)
            flags |= FixPathCanonicalize;
        return fixString(in, flags);
    }

    inline static bool hasFileExtension(const QString &str, const QStringList &extensions)
    {
        foreach (const QString &ext, extensions)
            if (str.endsWith(ext))
                return true;
        return false;
    }

    //global qmake mode, can only be in one mode per invocation!
    enum QMAKE_MODE { QMAKE_GENERATE_NOTHING,
                      QMAKE_GENERATE_PROJECT, QMAKE_GENERATE_MAKEFILE, QMAKE_GENERATE_PRL,
                      QMAKE_SET_PROPERTY, QMAKE_UNSET_PROPERTY, QMAKE_QUERY_PROPERTY };
    static QMAKE_MODE qmake_mode;

    //all modes
    static QFile output;
    static QString output_dir;
    static int debug_level;
    static int warn_level;
    static bool recursive;

    //QMAKE_*_PROPERTY options
    struct prop {
        static QStringList properties;
    };

    //QMAKE_GENERATE_PROJECT options
    struct projfile {
        static bool do_pwd;
        static QStringList project_dirs;
    };

    //QMAKE_GENERATE_MAKEFILE options
    struct mkfile {
        static bool do_deps;
        static bool do_mocs;
        static bool do_dep_heuristics;
        static bool do_preprocess;
        static bool do_stub_makefile;
        static int cachefile_depth;
        static QStringList project_files;
    };

private:
    static int parseCommandLine(QStringList &args, QMakeCmdLineParserState &state);
};

inline QString fixEnvVariables(const QString &x) { return Option::fixString(x, Option::FixEnvVars); }
inline QStringList splitPathList(const QString &paths) { return paths.isEmpty() ? QStringList() : paths.split(Option::globals->dirlist_sep); }

QT_END_NAMESPACE

#endif // OPTION_H
