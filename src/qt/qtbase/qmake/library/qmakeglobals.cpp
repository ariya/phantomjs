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

#include "qmakeglobals.h"

#include "qmakeevaluator.h"
#include "ioutils.h"

#include <qbytearray.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qregexp.h>
#include <qset.h>
#include <qstack.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtextstream.h>
#ifdef PROEVALUATOR_THREAD_SAFE
# include <qthreadpool.h>
#endif

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/utsname.h>
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#define QT_PCLOSE _pclose
#else
#define QT_POPEN popen
#define QT_PCLOSE pclose
#endif

QT_BEGIN_NAMESPACE

#define fL1S(s) QString::fromLatin1(s)

namespace { // MSVC doesn't seem to know the semantics of "static" ...

static struct {
    QRegExp reg_variableName;
} statics;

}

static void initStatics()
{
    if (!statics.reg_variableName.isEmpty())
        return;

    statics.reg_variableName.setPattern(QLatin1String("\\$\\(.*\\)"));
    statics.reg_variableName.setMinimal(true);
}

QMakeGlobals::QMakeGlobals()
{
    initStatics();

    do_cache = true;

#ifdef PROEVALUATOR_DEBUG
    debugLevel = 0;
#endif
#ifdef Q_OS_WIN
    dirlist_sep = QLatin1Char(';');
    dir_sep = QLatin1Char('\\');
#else
    dirlist_sep = QLatin1Char(':');
    dir_sep = QLatin1Char('/');
#endif
}

QMakeGlobals::~QMakeGlobals()
{
    qDeleteAll(baseEnvs);
}

QString QMakeGlobals::cleanSpec(QMakeCmdLineParserState &state, const QString &spec)
{
    QString ret = QDir::cleanPath(spec);
    if (ret.contains(QLatin1Char('/'))) {
        QString absRet = QDir(state.pwd).absoluteFilePath(ret);
        if (QFile::exists(absRet))
            ret = QDir::cleanPath(absRet);
    }
    return ret;
}

QMakeGlobals::ArgumentReturn QMakeGlobals::addCommandLineArguments(
        QMakeCmdLineParserState &state, QStringList &args, int *pos)
{
    enum { ArgNone, ArgConfig, ArgSpec, ArgXSpec, ArgTmpl, ArgTmplPfx, ArgCache } argState = ArgNone;
    for (; *pos < args.count(); (*pos)++) {
        QString arg = args.at(*pos);
        switch (argState) {
        case ArgConfig:
            if (state.after)
                state.postconfigs << arg;
            else
                state.preconfigs << arg;
            break;
        case ArgSpec:
            qmakespec = args[*pos] = cleanSpec(state, arg);
            break;
        case ArgXSpec:
            xqmakespec = args[*pos] = cleanSpec(state, arg);
            break;
        case ArgTmpl:
            user_template = arg;
            break;
        case ArgTmplPfx:
            user_template_prefix = arg;
            break;
        case ArgCache:
            cachefile = args[*pos] = QDir::cleanPath(QDir(state.pwd).absoluteFilePath(arg));
            break;
        default:
            if (arg.startsWith(QLatin1Char('-'))) {
                if (arg == QLatin1String("-after"))
                    state.after = true;
                else if (arg == QLatin1String("-config"))
                    argState = ArgConfig;
                else if (arg == QLatin1String("-nocache"))
                    do_cache = false;
                else if (arg == QLatin1String("-cache"))
                    argState = ArgCache;
                else if (arg == QLatin1String("-platform") || arg == QLatin1String("-spec"))
                    argState = ArgSpec;
                else if (arg == QLatin1String("-xplatform") || arg == QLatin1String("-xspec"))
                    argState = ArgXSpec;
                else if (arg == QLatin1String("-template") || arg == QLatin1String("-t"))
                    argState = ArgTmpl;
                else if (arg == QLatin1String("-template_prefix") || arg == QLatin1String("-tp"))
                    argState = ArgTmplPfx;
                else if (arg == QLatin1String("-win32"))
                    dir_sep = QLatin1Char('\\');
                else if (arg == QLatin1String("-unix"))
                    dir_sep = QLatin1Char('/');
                else
                    return ArgumentUnknown;
            } else if (arg.contains(QLatin1Char('='))) {
                if (state.after)
                    state.postcmds << arg;
                else
                    state.precmds << arg;
            } else {
                return ArgumentUnknown;
            }
            continue;
        }
        argState = ArgNone;
    }
    if (argState != ArgNone)
        return ArgumentMalformed;
    return ArgumentsOk;
}

void QMakeGlobals::commitCommandLineArguments(QMakeCmdLineParserState &state)
{
    if (!state.preconfigs.isEmpty())
        state.precmds << (fL1S("CONFIG += ") + state.preconfigs.join(fL1S(" ")));
    precmds = state.precmds.join(fL1S("\n"));
    if (!state.postconfigs.isEmpty())
        state.postcmds << (fL1S("CONFIG += ") + state.postconfigs.join(fL1S(" ")));
    postcmds = state.postcmds.join(fL1S("\n"));

    if (xqmakespec.isEmpty())
        xqmakespec = qmakespec;
}

void QMakeGlobals::useEnvironment()
{
    if (xqmakespec.isEmpty())
        xqmakespec = getEnv(QLatin1String("XQMAKESPEC"));
    if (qmakespec.isEmpty()) {
        qmakespec = getEnv(QLatin1String("QMAKESPEC"));
        if (xqmakespec.isEmpty())
            xqmakespec = qmakespec;
    }
}

void QMakeGlobals::setCommandLineArguments(const QString &pwd, const QStringList &_args)
{
    QStringList args = _args;

    QMakeCmdLineParserState state(pwd);
    for (int pos = 0; pos < args.size(); pos++)
        addCommandLineArguments(state, args, &pos);
    commitCommandLineArguments(state);
    useEnvironment();
}

void QMakeGlobals::setDirectories(const QString &input_dir, const QString &output_dir)
{
    if (input_dir != output_dir && !output_dir.isEmpty()) {
        QString srcpath = input_dir;
        if (!srcpath.endsWith(QLatin1Char('/')))
            srcpath += QLatin1Char('/');
        QString dstpath = output_dir;
        if (!dstpath.endsWith(QLatin1Char('/')))
            dstpath += QLatin1Char('/');
        int srcLen = srcpath.length();
        int dstLen = dstpath.length();
        int lastSl = -1;
        while (++lastSl, --srcLen, --dstLen,
               srcLen && dstLen && srcpath.at(srcLen) == dstpath.at(dstLen))
            if (srcpath.at(srcLen) == QLatin1Char('/'))
                lastSl = 0;
        source_root = srcpath.left(srcLen + lastSl);
        build_root = dstpath.left(dstLen + lastSl);
    }
}

QString QMakeGlobals::shadowedPath(const QString &fileName) const
{
    if (source_root.isEmpty())
        return fileName;
    if (fileName.startsWith(source_root)
        && (fileName.length() == source_root.length()
            || fileName.at(source_root.length()) == QLatin1Char('/'))) {
        return build_root + fileName.mid(source_root.length());
    }
    return QString();
}

QStringList QMakeGlobals::splitPathList(const QString &val) const
{
    QStringList ret;
    if (!val.isEmpty()) {
        QDir bdir;
        QStringList vals = val.split(dirlist_sep);
        ret.reserve(vals.length());
        foreach (const QString &it, vals)
            ret << QDir::cleanPath(bdir.absoluteFilePath(it));
    }
    return ret;
}

QString QMakeGlobals::getEnv(const QString &var) const
{
#ifdef PROEVALUATOR_SETENV
    return environment.value(var);
#else
    return QString::fromLocal8Bit(qgetenv(var.toLocal8Bit().constData()));
#endif
}

QStringList QMakeGlobals::getPathListEnv(const QString &var) const
{
    return splitPathList(getEnv(var));
}

QString QMakeGlobals::expandEnvVars(const QString &str) const
{
    QString string = str;
    int rep;
    QRegExp reg_variableName = statics.reg_variableName; // Copy for thread safety
    while ((rep = reg_variableName.indexIn(string)) != -1)
        string.replace(rep, reg_variableName.matchedLength(),
                       getEnv(string.mid(rep + 2, reg_variableName.matchedLength() - 3)));
    return string;
}

#ifndef QT_BUILD_QMAKE
#ifdef PROEVALUATOR_INIT_PROPS
bool QMakeGlobals::initProperties()
{
    QByteArray data;
#ifndef QT_BOOTSTRAPPED
    QProcess proc;
    proc.start(qmake_abslocation, QStringList() << QLatin1String("-query"));
    if (!proc.waitForFinished())
        return false;
    data = proc.readAll();
#else
    if (FILE *proc = QT_POPEN(QString(QMakeInternal::IoUtils::shellQuote(qmake_abslocation)
                                      + QLatin1String(" -query")).toLocal8Bit(), "r")) {
        char buff[1024];
        while (!feof(proc))
            data.append(buff, int(fread(buff, 1, 1023, proc)));
        QT_PCLOSE(proc);
    }
#endif
    foreach (QByteArray line, data.split('\n')) {
        int off = line.indexOf(':');
        if (off < 0) // huh?
            continue;
        if (line.endsWith('\r'))
            line.chop(1);
        QString name = QString::fromLatin1(line.left(off));
        ProString value = ProString(QDir::fromNativeSeparators(
                    QString::fromLocal8Bit(line.mid(off + 1))));
        properties.insert(ProKey(name), value);
        if (name.startsWith(QLatin1String("QT_"))) {
            bool plain = !name.contains(QLatin1Char('/'));
            if (!plain) {
                if (!name.endsWith(QLatin1String("/get")))
                    continue;
                name.chop(4);
            }
            if (name.startsWith(QLatin1String("QT_INSTALL_"))) {
                if (plain) {
                    properties.insert(ProKey(name + QLatin1String("/raw")), value);
                    properties.insert(ProKey(name + QLatin1String("/get")), value);
                }
                properties.insert(ProKey(name + QLatin1String("/src")), value);
                if (name == QLatin1String("QT_INSTALL_PREFIX")
                    || name == QLatin1String("QT_INSTALL_DATA")
                    || name == QLatin1String("QT_INSTALL_BINS")) {
                    name.replace(3, 7, QLatin1String("HOST"));
                    if (plain) {
                        properties.insert(ProKey(name), value);
                        properties.insert(ProKey(name + QLatin1String("/get")), value);
                    }
                    properties.insert(ProKey(name + QLatin1String("/src")), value);
                }
            } else if (name.startsWith(QLatin1String("QT_HOST_"))) {
                if (plain)
                    properties.insert(ProKey(name + QLatin1String("/get")), value);
                properties.insert(ProKey(name + QLatin1String("/src")), value);
            }
        }
    }
    return true;
}
#else
void QMakeGlobals::setProperties(const QHash<QString, QString> &props)
{
    QHash<QString, QString>::ConstIterator it = props.constBegin(), eit = props.constEnd();
    for (; it != eit; ++it)
        properties.insert(ProKey(it.key()), ProString(it.value()));
}
#endif
#endif // QT_BUILD_QMAKE

QT_END_NAMESPACE
