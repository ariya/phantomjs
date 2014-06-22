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

#include "qmakeevaluator.h"

#include "qmakeevaluator_p.h"
#include "qmakeglobals.h"
#include "qmakeparser.h"
#include "qmakevfs.h"
#include "ioutils.h"

#include <qbytearray.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qlist.h>
#include <qregexp.h>
#include <qset.h>
#include <qstringlist.h>
#include <qtextstream.h>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
# include <qjsondocument.h>
# include <qjsonobject.h>
# include <qjsonarray.h>
#endif
#ifdef PROEVALUATOR_THREAD_SAFE
# include <qthreadpool.h>
#endif

#ifdef Q_OS_UNIX
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
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

using namespace QMakeInternal;

QT_BEGIN_NAMESPACE

#define fL1S(s) QString::fromLatin1(s)

enum ExpandFunc {
    E_INVALID = 0, E_MEMBER, E_FIRST, E_LAST, E_SIZE, E_CAT, E_FROMFILE, E_EVAL, E_LIST,
    E_SPRINTF, E_FORMAT_NUMBER, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
    E_FIND, E_SYSTEM, E_UNIQUE, E_REVERSE, E_QUOTE, E_ESCAPE_EXPAND,
    E_UPPER, E_LOWER, E_TITLE, E_FILES, E_PROMPT, E_RE_ESCAPE, E_VAL_ESCAPE,
    E_REPLACE, E_SORT_DEPENDS, E_RESOLVE_DEPENDS, E_ENUMERATE_VARS,
    E_SHADOWED, E_ABSOLUTE_PATH, E_RELATIVE_PATH, E_CLEAN_PATH,
    E_SYSTEM_PATH, E_SHELL_PATH, E_SYSTEM_QUOTE, E_SHELL_QUOTE, E_GETENV
};

enum TestFunc {
    T_INVALID = 0, T_REQUIRES, T_GREATERTHAN, T_LESSTHAN, T_EQUALS,
    T_EXISTS, T_EXPORT, T_CLEAR, T_UNSET, T_EVAL, T_CONFIG, T_SYSTEM,
    T_DEFINED, T_CONTAINS, T_INFILE,
    T_COUNT, T_ISEMPTY, T_PARSE_JSON, T_INCLUDE, T_LOAD, T_DEBUG, T_LOG, T_MESSAGE, T_WARNING, T_ERROR, T_IF,
    T_MKPATH, T_WRITE_FILE, T_TOUCH, T_CACHE
};

void QMakeEvaluator::initFunctionStatics()
{
    static const struct {
        const char * const name;
        const ExpandFunc func;
    } expandInits[] = {
        { "member", E_MEMBER },
        { "first", E_FIRST },
        { "last", E_LAST },
        { "size", E_SIZE },
        { "cat", E_CAT },
        { "fromfile", E_FROMFILE },
        { "eval", E_EVAL },
        { "list", E_LIST },
        { "sprintf", E_SPRINTF },
        { "format_number", E_FORMAT_NUMBER },
        { "join", E_JOIN },
        { "split", E_SPLIT },
        { "basename", E_BASENAME },
        { "dirname", E_DIRNAME },
        { "section", E_SECTION },
        { "find", E_FIND },
        { "system", E_SYSTEM },
        { "unique", E_UNIQUE },
        { "reverse", E_REVERSE },
        { "quote", E_QUOTE },
        { "escape_expand", E_ESCAPE_EXPAND },
        { "upper", E_UPPER },
        { "lower", E_LOWER },
        { "title", E_TITLE },
        { "re_escape", E_RE_ESCAPE },
        { "val_escape", E_VAL_ESCAPE },
        { "files", E_FILES },
        { "prompt", E_PROMPT },
        { "replace", E_REPLACE },
        { "sort_depends", E_SORT_DEPENDS },
        { "resolve_depends", E_RESOLVE_DEPENDS },
        { "enumerate_vars", E_ENUMERATE_VARS },
        { "shadowed", E_SHADOWED },
        { "absolute_path", E_ABSOLUTE_PATH },
        { "relative_path", E_RELATIVE_PATH },
        { "clean_path", E_CLEAN_PATH },
        { "system_path", E_SYSTEM_PATH },
        { "shell_path", E_SHELL_PATH },
        { "system_quote", E_SYSTEM_QUOTE },
        { "shell_quote", E_SHELL_QUOTE },
        { "getenv", E_GETENV },
    };
    for (unsigned i = 0; i < sizeof(expandInits)/sizeof(expandInits[0]); ++i)
        statics.expands.insert(ProKey(expandInits[i].name), expandInits[i].func);

    static const struct {
        const char * const name;
        const TestFunc func;
    } testInits[] = {
        { "requires", T_REQUIRES },
        { "greaterThan", T_GREATERTHAN },
        { "lessThan", T_LESSTHAN },
        { "equals", T_EQUALS },
        { "isEqual", T_EQUALS },
        { "exists", T_EXISTS },
        { "export", T_EXPORT },
        { "clear", T_CLEAR },
        { "unset", T_UNSET },
        { "eval", T_EVAL },
        { "CONFIG", T_CONFIG },
        { "if", T_IF },
        { "isActiveConfig", T_CONFIG },
        { "system", T_SYSTEM },
        { "defined", T_DEFINED },
        { "contains", T_CONTAINS },
        { "infile", T_INFILE },
        { "count", T_COUNT },
        { "isEmpty", T_ISEMPTY },
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        { "parseJson", T_PARSE_JSON },
#endif
        { "load", T_LOAD },
        { "include", T_INCLUDE },
        { "debug", T_DEBUG },
        { "log", T_LOG },
        { "message", T_MESSAGE },
        { "warning", T_WARNING },
        { "error", T_ERROR },
        { "mkpath", T_MKPATH },
        { "write_file", T_WRITE_FILE },
        { "touch", T_TOUCH },
        { "cache", T_CACHE },
    };
    for (unsigned i = 0; i < sizeof(testInits)/sizeof(testInits[0]); ++i)
        statics.functions.insert(ProKey(testInits[i].name), testInits[i].func);
}

static bool isTrue(const ProString &_str, QString &tmp)
{
    const QString &str = _str.toQString(tmp);
    return !str.compare(statics.strtrue, Qt::CaseInsensitive) || str.toInt();
}

#if defined(Q_OS_WIN) && defined(PROEVALUATOR_FULL)
static QString windowsErrorCode()
{
    wchar_t *string = 0;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR)&string,
                  0,
                  NULL);
    QString ret = QString::fromWCharArray(string);
    LocalFree((HLOCAL)string);
    return ret;
}
#endif

static QString
quoteValue(const ProString &val)
{
    QString ret;
    ret.reserve(val.size());
    const QChar *chars = val.constData();
    bool quote = val.isEmpty();
    bool escaping = false;
    for (int i = 0, l = val.size(); i < l; i++) {
        QChar c = chars[i];
        ushort uc = c.unicode();
        if (uc < 32) {
            if (!escaping) {
                escaping = true;
                ret += QLatin1String("$$escape_expand(");
            }
            switch (uc) {
            case '\r':
                ret += QLatin1String("\\\\r");
                break;
            case '\n':
                ret += QLatin1String("\\\\n");
                break;
            case '\t':
                ret += QLatin1String("\\\\t");
                break;
            default:
                ret += QString::fromLatin1("\\\\x%1").arg(uc, 2, 16, QLatin1Char('0'));
                break;
            }
        } else {
            if (escaping) {
                escaping = false;
                ret += QLatin1Char(')');
            }
            switch (uc) {
            case '\\':
                ret += QLatin1String("\\\\");
                break;
            case '"':
                ret += QLatin1String("\\\"");
                break;
            case '\'':
                ret += QLatin1String("\\'");
                break;
            case '$':
                ret += QLatin1String("\\$");
                break;
            case '#':
                ret += QLatin1String("$${LITERAL_HASH}");
                break;
            case 32:
                quote = true;
                // fallthrough
            default:
                ret += c;
                break;
            }
        }
    }
    if (escaping)
        ret += QLatin1Char(')');
    if (quote) {
        ret.prepend(QLatin1Char('"'));
        ret.append(QLatin1Char('"'));
    }
    return ret;
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
static void addJsonValue(const QJsonValue &value, const QString &keyPrefix, ProValueMap *map);

static void insertJsonKeyValue(const QString &key, const QStringList &values, ProValueMap *map)
{
    map->insert(ProKey(key), ProStringList(values));
}

static void addJsonArray(const QJsonArray &array, const QString &keyPrefix, ProValueMap *map)
{
    QStringList keys;
    for (int i = 0; i < array.count(); ++i) {
        keys.append(QString::number(i));
        addJsonValue(array.at(i), keyPrefix + QString::number(i), map);
    }
    insertJsonKeyValue(keyPrefix + QLatin1String("_KEYS_"), keys, map);
}

static void addJsonObject(const QJsonObject &object, const QString &keyPrefix, ProValueMap *map)
{
    foreach (const QString &key, object.keys())
        addJsonValue(object.value(key), keyPrefix + key, map);

    insertJsonKeyValue(keyPrefix + QLatin1String("_KEYS_"), object.keys(), map);
}

static void addJsonValue(const QJsonValue &value, const QString &keyPrefix, ProValueMap *map)
{
    switch (value.type()) {
    case QJsonValue::Bool:
        insertJsonKeyValue(keyPrefix, QStringList() << (value.toBool() ? QLatin1String("true") : QLatin1String("false")), map);
        break;
    case QJsonValue::Double:
        insertJsonKeyValue(keyPrefix, QStringList() << QString::number(value.toDouble()), map);
        break;
    case QJsonValue::String:
        insertJsonKeyValue(keyPrefix, QStringList() << value.toString(), map);
        break;
    case QJsonValue::Array:
        addJsonArray(value.toArray(), keyPrefix + QLatin1Char('.'), map);
        break;
    case QJsonValue::Object:
        addJsonObject(value.toObject(), keyPrefix + QLatin1Char('.'), map);
        break;
    default:
        break;
    }
}

static QMakeEvaluator::VisitReturn parseJsonInto(const QByteArray &json, const QString &into, ProValueMap *value)
{
    QJsonDocument document = QJsonDocument::fromJson(json);
    if (document.isNull())
        return QMakeEvaluator::ReturnFalse;

    QString currentKey = into + QLatin1Char('.');

    // top-level item is either an array or object
    if (document.isArray())
        addJsonArray(document.array(), currentKey, value);
    else if (document.isObject())
        addJsonObject(document.object(), currentKey, value);
    else
        return QMakeEvaluator::ReturnFalse;

    return QMakeEvaluator::ReturnTrue;
}
#endif

QMakeEvaluator::VisitReturn
QMakeEvaluator::writeFile(const QString &ctx, const QString &fn, QIODevice::OpenMode mode,
                          const QString &contents)
{
    QString errStr;
    if (!m_vfs->writeFile(fn, mode, contents, &errStr)) {
        evalError(fL1S("Cannot write %1file %2: %3.")
                  .arg(ctx, QDir::toNativeSeparators(fn), errStr));
        return ReturnFalse;
    }
    m_parser->discardFileFromCache(fn);
    return ReturnTrue;
}

#ifndef QT_BOOTSTRAPPED
void QMakeEvaluator::runProcess(QProcess *proc, const QString &command) const
{
    proc->setWorkingDirectory(currentDirectory());
# ifdef PROEVALUATOR_SETENV
    if (!m_option->environment.isEmpty())
        proc->setProcessEnvironment(m_option->environment);
# endif
# ifdef Q_OS_WIN
    proc->setNativeArguments(QLatin1String("/v:off /s /c \"") + command + QLatin1Char('"'));
    proc->start(m_option->getEnv(QLatin1String("COMSPEC")), QStringList());
# else
    proc->start(QLatin1String("/bin/sh"), QStringList() << QLatin1String("-c") << command);
# endif
    proc->waitForFinished(-1);
}
#endif

QByteArray QMakeEvaluator::getCommandOutput(const QString &args) const
{
    QByteArray out;
#ifndef QT_BOOTSTRAPPED
    QProcess proc;
    runProcess(&proc, args);
    QByteArray errout = proc.readAllStandardError();
# ifdef PROEVALUATOR_FULL
    // FIXME: Qt really should have the option to set forwarding per channel
    fputs(errout.constData(), stderr);
# else
    if (!errout.isEmpty()) {
        if (errout.endsWith('\n'))
            errout.chop(1);
        m_handler->message(QMakeHandler::EvalError, QString::fromLocal8Bit(errout));
    }
# endif
    out = proc.readAllStandardOutput();
# ifdef Q_OS_WIN
    // FIXME: Qt's line end conversion on sequential files should really be fixed
    out.replace("\r\n", "\n");
# endif
#else
    if (FILE *proc = QT_POPEN(QString(QLatin1String("cd ")
                               + IoUtils::shellQuote(QDir::toNativeSeparators(currentDirectory()))
                               + QLatin1String(" && ") + args).toLocal8Bit().constData(), "r")) {
        while (!feof(proc)) {
            char buff[10 * 1024];
            int read_in = int(fread(buff, 1, sizeof(buff), proc));
            if (!read_in)
                break;
            out += QByteArray(buff, read_in);
        }
        QT_PCLOSE(proc);
    }
#endif
    return out;
}

void QMakeEvaluator::populateDeps(
        const ProStringList &deps, const ProString &prefix, const ProStringList &suffixes,
        QHash<ProKey, QSet<ProKey> > &dependencies, ProValueMap &dependees,
        ProStringList &rootSet) const
{
    foreach (const ProString &item, deps)
        if (!dependencies.contains(item.toKey())) {
            QSet<ProKey> &dset = dependencies[item.toKey()]; // Always create entry
            ProStringList depends;
            foreach (const ProString &suffix, suffixes)
                depends += values(ProKey(prefix + item + suffix));
            if (depends.isEmpty()) {
                rootSet << item;
            } else {
                foreach (const ProString &dep, depends) {
                    dset.insert(dep.toKey());
                    dependees[dep.toKey()] << item;
                }
                populateDeps(depends, prefix, suffixes, dependencies, dependees, rootSet);
            }
        }
}

ProStringList QMakeEvaluator::evaluateBuiltinExpand(
        int func_t, const ProKey &func, const ProStringList &args)
{
    ProStringList ret;

    traceMsg("calling built-in $$%s(%s)", dbgKey(func), dbgSepStrList(args));

    switch (func_t) {
    case E_BASENAME:
    case E_DIRNAME:
    case E_SECTION: {
        bool regexp = false;
        QString sep;
        ProString var;
        int beg = 0;
        int end = -1;
        if (func_t == E_SECTION) {
            if (args.count() != 3 && args.count() != 4) {
                evalError(fL1S("%1(var) section(var, sep, begin, end) requires"
                               " three or four arguments.").arg(func.toQString(m_tmp1)));
            } else {
                var = args[0];
                sep = args.at(1).toQString();
                beg = args.at(2).toQString(m_tmp2).toInt();
                if (args.count() == 4)
                    end = args.at(3).toQString(m_tmp2).toInt();
            }
        } else {
            if (args.count() != 1) {
                evalError(fL1S("%1(var) requires one argument.").arg(func.toQString(m_tmp1)));
            } else {
                var = args[0];
                regexp = true;
                sep = QLatin1String("[\\\\/]");
                if (func_t == E_DIRNAME)
                    end = -2;
                else
                    beg = -1;
            }
        }
        if (!var.isEmpty()) {
            if (regexp) {
                QRegExp sepRx(sep);
                foreach (const ProString &str, values(map(var))) {
                    const QString &rstr = str.toQString(m_tmp1).section(sepRx, beg, end);
                    ret << (rstr.isSharedWith(m_tmp1) ? str : ProString(rstr).setSource(str));
                }
            } else {
                foreach (const ProString &str, values(map(var))) {
                    const QString &rstr = str.toQString(m_tmp1).section(sep, beg, end);
                    ret << (rstr.isSharedWith(m_tmp1) ? str : ProString(rstr).setSource(str));
                }
            }
        }
        break;
    }
    case E_SPRINTF:
        if (args.count() < 1) {
            evalError(fL1S("sprintf(format, ...) requires at least one argument."));
        } else {
            QString tmp = args.at(0).toQString(m_tmp1);
            for (int i = 1; i < args.count(); ++i)
                tmp = tmp.arg(args.at(i).toQString(m_tmp2));
            ret << ProString(tmp);
        }
        break;
    case E_FORMAT_NUMBER:
        if (args.count() > 2) {
            evalError(fL1S("format_number(number[, options...]) requires one or two arguments."));
        } else {
            int ibase = 10;
            int obase = 10;
            int width = 0;
            bool zeropad = false;
            bool leftalign = false;
            enum { DefaultSign, PadSign, AlwaysSign } sign = DefaultSign;
            if (args.count() >= 2) {
                foreach (const ProString &opt, split_value_list(args.at(1).toQString(m_tmp2))) {
                    opt.toQString(m_tmp3);
                    if (m_tmp3.startsWith(QLatin1String("ibase="))) {
                        ibase = m_tmp3.mid(6).toInt();
                    } else if (m_tmp3.startsWith(QLatin1String("obase="))) {
                        obase = m_tmp3.mid(6).toInt();
                    } else if (m_tmp3.startsWith(QLatin1String("width="))) {
                        width = m_tmp3.mid(6).toInt();
                    } else if (m_tmp3 == QLatin1String("zeropad")) {
                        zeropad = true;
                    } else if (m_tmp3 == QLatin1String("padsign")) {
                        sign = PadSign;
                    } else if (m_tmp3 == QLatin1String("alwayssign")) {
                        sign = AlwaysSign;
                    } else if (m_tmp3 == QLatin1String("leftalign")) {
                        leftalign = true;
                    } else {
                        evalError(fL1S("format_number(): invalid format option %1.").arg(m_tmp3));
                        goto formfail;
                    }
                }
            }
            args.at(0).toQString(m_tmp3);
            if (m_tmp3.contains(QLatin1Char('.'))) {
                evalError(fL1S("format_number(): floats are currently not supported."));
                break;
            }
            bool ok;
            qlonglong num = m_tmp3.toLongLong(&ok, ibase);
            if (!ok) {
                evalError(fL1S("format_number(): malformed number %2 for base %1.")
                          .arg(ibase).arg(m_tmp3));
                break;
            }
            QString outstr;
            if (num < 0) {
                num = -num;
                outstr = QLatin1Char('-');
            } else if (sign == AlwaysSign) {
                outstr = QLatin1Char('+');
            } else if (sign == PadSign) {
                outstr = QLatin1Char(' ');
            }
            QString numstr = QString::number(num, obase);
            int space = width - outstr.length() - numstr.length();
            if (space <= 0) {
                outstr += numstr;
            } else if (leftalign) {
                outstr += numstr + QString(space, QLatin1Char(' '));
            } else if (zeropad) {
                outstr += QString(space, QLatin1Char('0')) + numstr;
            } else {
                outstr.prepend(QString(space, QLatin1Char(' ')));
                outstr += numstr;
            }
            ret += ProString(outstr);
        }
      formfail:
        break;
    case E_JOIN: {
        if (args.count() < 1 || args.count() > 4) {
            evalError(fL1S("join(var, glue, before, after) requires one to four arguments."));
        } else {
            QString glue;
            ProString before, after;
            if (args.count() >= 2)
                glue = args.at(1).toQString(m_tmp1);
            if (args.count() >= 3)
                before = args[2];
            if (args.count() == 4)
                after = args[3];
            const ProStringList &var = values(map(args.at(0)));
            if (!var.isEmpty()) {
                const ProFile *src = currentProFile();
                foreach (const ProString &v, var)
                    if (const ProFile *s = v.sourceFile()) {
                        src = s;
                        break;
                    }
                ret << ProString(before + var.join(glue) + after).setSource(src);
            }
        }
        break;
    }
    case E_SPLIT:
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("split(var, sep) requires one or two arguments."));
        } else {
            const QString &sep = (args.count() == 2) ? args.at(1).toQString(m_tmp1) : statics.field_sep;
            foreach (const ProString &var, values(map(args.at(0))))
                foreach (const QString &splt, var.toQString(m_tmp2).split(sep))
                    ret << (splt.isSharedWith(m_tmp2) ? var : ProString(splt).setSource(var));
        }
        break;
    case E_MEMBER:
        if (args.count() < 1 || args.count() > 3) {
            evalError(fL1S("member(var, start, end) requires one to three arguments."));
        } else {
            bool ok = true;
            const ProStringList &var = values(map(args.at(0)));
            int start = 0, end = 0;
            if (args.count() >= 2) {
                const QString &start_str = args.at(1).toQString(m_tmp1);
                start = start_str.toInt(&ok);
                if (!ok) {
                    if (args.count() == 2) {
                        int dotdot = start_str.indexOf(statics.strDotDot);
                        if (dotdot != -1) {
                            start = start_str.left(dotdot).toInt(&ok);
                            if (ok)
                                end = start_str.mid(dotdot+2).toInt(&ok);
                        }
                    }
                    if (!ok)
                        evalError(fL1S("member() argument 2 (start) '%2' invalid.")
                                  .arg(start_str));
                } else {
                    end = start;
                    if (args.count() == 3)
                        end = args.at(2).toQString(m_tmp1).toInt(&ok);
                    if (!ok)
                        evalError(fL1S("member() argument 3 (end) '%2' invalid.")
                                  .arg(args.at(2).toQString(m_tmp1)));
                }
            }
            if (ok) {
                if (start < 0)
                    start += var.count();
                if (end < 0)
                    end += var.count();
                if (start < 0 || start >= var.count() || end < 0 || end >= var.count()) {
                    //nothing
                } else if (start < end) {
                    for (int i = start; i <= end && var.count() >= i; i++)
                        ret.append(var[i]);
                } else {
                    for (int i = start; i >= end && var.count() >= i && i >= 0; i--)
                        ret += var[i];
                }
            }
        }
        break;
    case E_FIRST:
    case E_LAST:
        if (args.count() != 1) {
            evalError(fL1S("%1(var) requires one argument.").arg(func.toQString(m_tmp1)));
        } else {
            const ProStringList &var = values(map(args.at(0)));
            if (!var.isEmpty()) {
                if (func_t == E_FIRST)
                    ret.append(var[0]);
                else
                    ret.append(var.last());
            }
        }
        break;
    case E_SIZE:
        if (args.count() != 1)
            evalError(fL1S("size(var) requires one argument."));
        else
            ret.append(ProString(QString::number(values(map(args.at(0))).size())));
        break;
    case E_CAT:
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("cat(file, singleline=true) requires one or two arguments."));
        } else {
            const QString &file = args.at(0).toQString(m_tmp1);

            bool blob = false;
            bool lines = false;
            bool singleLine = true;
            if (args.count() > 1) {
                args.at(1).toQString(m_tmp2);
                if (!m_tmp2.compare(QLatin1String("false"), Qt::CaseInsensitive))
                    singleLine = false;
                else if (!m_tmp2.compare(QLatin1String("blob"), Qt::CaseInsensitive))
                    blob = true;
                else if (!m_tmp2.compare(QLatin1String("lines"), Qt::CaseInsensitive))
                    lines = true;
            }

            QFile qfile(resolvePath(m_option->expandEnvVars(file)));
            if (qfile.open(QIODevice::ReadOnly)) {
                QTextStream stream(&qfile);
                if (blob) {
                    ret += ProString(stream.readAll());
                } else {
                    while (!stream.atEnd()) {
                        if (lines) {
                            ret += ProString(stream.readLine());
                        } else {
                            ret += split_value_list(stream.readLine().trimmed());
                            if (!singleLine)
                                ret += ProString("\n");
                        }
                    }
                }
            }
        }
        break;
    case E_FROMFILE:
        if (args.count() != 2) {
            evalError(fL1S("fromfile(file, variable) requires two arguments."));
        } else {
            ProValueMap vars;
            QString fn = resolvePath(m_option->expandEnvVars(args.at(0).toQString(m_tmp1)));
            fn.detach();
            if (evaluateFileInto(fn, &vars, LoadProOnly) == ReturnTrue)
                ret = vars.value(map(args.at(1)));
        }
        break;
    case E_EVAL:
        if (args.count() != 1)
            evalError(fL1S("eval(variable) requires one argument."));
        else
            ret += values(map(args.at(0)));
        break;
    case E_LIST: {
        QString tmp;
        tmp.sprintf(".QMAKE_INTERNAL_TMP_variableName_%d", m_listCount++);
        ret = ProStringList(ProString(tmp));
        ProStringList lst;
        foreach (const ProString &arg, args)
            lst += split_value_list(arg.toQString(m_tmp1), arg.sourceFile()); // Relies on deep copy
        m_valuemapStack.top()[ret.at(0).toKey()] = lst;
        break; }
    case E_FIND:
        if (args.count() != 2) {
            evalError(fL1S("find(var, str) requires two arguments."));
        } else {
            QRegExp regx(args.at(1).toQString());
            int t = 0;
            foreach (const ProString &val, values(map(args.at(0)))) {
                if (regx.indexIn(val.toQString(m_tmp[t])) != -1)
                    ret += val;
                t ^= 1;
            }
        }
        break;
    case E_SYSTEM:
        if (!m_skipLevel) {
            if (args.count() < 1 || args.count() > 2) {
                evalError(fL1S("system(execute) requires one or two arguments."));
            } else {
                bool blob = false;
                bool lines = false;
                bool singleLine = true;
                if (args.count() > 1) {
                    args.at(1).toQString(m_tmp2);
                    if (!m_tmp2.compare(QLatin1String("false"), Qt::CaseInsensitive))
                        singleLine = false;
                    else if (!m_tmp2.compare(QLatin1String("blob"), Qt::CaseInsensitive))
                        blob = true;
                    else if (!m_tmp2.compare(QLatin1String("lines"), Qt::CaseInsensitive))
                        lines = true;
                }
                QByteArray bytes = getCommandOutput(args.at(0).toQString(m_tmp2));
                if (lines) {
                    QTextStream stream(bytes);
                    while (!stream.atEnd())
                        ret += ProString(stream.readLine());
                } else {
                    QString output = QString::fromLocal8Bit(bytes);
                    if (blob) {
                        ret += ProString(output);
                    } else {
                        output.replace(QLatin1Char('\t'), QLatin1Char(' '));
                        if (singleLine)
                            output.replace(QLatin1Char('\n'), QLatin1Char(' '));
                        ret += split_value_list(output);
                    }
                }
            }
        }
        break;
    case E_UNIQUE:
        if (args.count() != 1) {
            evalError(fL1S("unique(var) requires one argument."));
        } else {
            ret = values(map(args.at(0)));
            ret.removeDuplicates();
        }
        break;
    case E_REVERSE:
        if (args.count() != 1) {
            evalError(fL1S("reverse(var) requires one argument."));
        } else {
            ProStringList var = values(args.at(0).toKey());
            for (int i = 0; i < var.size() / 2; i++)
                qSwap(var[i], var[var.size() - i - 1]);
            ret += var;
        }
        break;
    case E_QUOTE:
        ret += args;
        break;
    case E_ESCAPE_EXPAND:
        for (int i = 0; i < args.size(); ++i) {
            QString str = args.at(i).toQString();
            QChar *i_data = str.data();
            int i_len = str.length();
            for (int x = 0; x < i_len; ++x) {
                if (*(i_data+x) == QLatin1Char('\\') && x < i_len-1) {
                    if (*(i_data+x+1) == QLatin1Char('\\')) {
                        ++x;
                    } else {
                        struct {
                            char in, out;
                        } mapped_quotes[] = {
                            { 'n', '\n' },
                            { 't', '\t' },
                            { 'r', '\r' },
                            { 0, 0 }
                        };
                        for (int i = 0; mapped_quotes[i].in; ++i) {
                            if (*(i_data+x+1) == QLatin1Char(mapped_quotes[i].in)) {
                                *(i_data+x) = QLatin1Char(mapped_quotes[i].out);
                                if (x < i_len-2)
                                    memmove(i_data+x+1, i_data+x+2, (i_len-x-2)*sizeof(QChar));
                                --i_len;
                                break;
                            }
                        }
                    }
                }
            }
            ret.append(ProString(QString(i_data, i_len)).setSource(args.at(i)));
        }
        break;
    case E_RE_ESCAPE:
        for (int i = 0; i < args.size(); ++i) {
            const QString &rstr = QRegExp::escape(args.at(i).toQString(m_tmp1));
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(i) : ProString(rstr).setSource(args.at(i)));
        }
        break;
    case E_VAL_ESCAPE:
        if (args.count() != 1) {
            evalError(fL1S("val_escape(var) requires one argument."));
        } else {
            const ProStringList &vals = values(args.at(0).toKey());
            ret.reserve(vals.size());
            foreach (const ProString &str, vals)
                ret += ProString(quoteValue(str));
        }
        break;
    case E_UPPER:
    case E_LOWER:
    case E_TITLE:
        for (int i = 0; i < args.count(); ++i) {
            QString rstr = args.at(i).toQString(m_tmp1);
            if (func_t == E_UPPER) {
                rstr = rstr.toUpper();
            } else {
                rstr = rstr.toLower();
                if (func_t == E_TITLE && rstr.length() > 0)
                    rstr[0] = rstr.at(0).toTitleCase();
            }
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(i) : ProString(rstr).setSource(args.at(i)));
        }
        break;
    case E_FILES:
        if (args.count() != 1 && args.count() != 2) {
            evalError(fL1S("files(pattern, recursive=false) requires one or two arguments."));
        } else {
            bool recursive = false;
            if (args.count() == 2)
                recursive = isTrue(args.at(1), m_tmp2);
            QStringList dirs;
            QString r = m_option->expandEnvVars(args.at(0).toQString(m_tmp1))
                        .replace(QLatin1Char('\\'), QLatin1Char('/'));
            QString pfx;
            if (IoUtils::isRelativePath(r)) {
                pfx = currentDirectory();
                if (!pfx.endsWith(QLatin1Char('/')))
                    pfx += QLatin1Char('/');
            }
            int slash = r.lastIndexOf(QLatin1Char('/'));
            if (slash != -1) {
                dirs.append(r.left(slash+1));
                r = r.mid(slash+1);
            } else {
                dirs.append(QString());
            }

            r.detach(); // Keep m_tmp out of QRegExp's cache
            QRegExp regex(r, Qt::CaseSensitive, QRegExp::Wildcard);
            for (int d = 0; d < dirs.count(); d++) {
                QString dir = dirs[d];
                QDir qdir(pfx + dir);
                for (int i = 0; i < (int)qdir.count(); ++i) {
                    if (qdir[i] == statics.strDot || qdir[i] == statics.strDotDot)
                        continue;
                    QString fname = dir + qdir[i];
                    if (IoUtils::fileType(pfx + fname) == IoUtils::FileIsDir) {
                        if (recursive)
                            dirs.append(fname + QLatin1Char('/'));
                    }
                    if (regex.exactMatch(qdir[i]))
                        ret += ProString(fname).setSource(currentProFile());
                }
            }
        }
        break;
#ifdef PROEVALUATOR_FULL
    case E_PROMPT: {
        if (args.count() != 1) {
            evalError(fL1S("prompt(question) requires one argument."));
//        } else if (currentFileName() == QLatin1String("-")) {
//            evalError(fL1S("prompt(question) cannot be used when '-o -' is used"));
        } else {
            QString msg = m_option->expandEnvVars(args.at(0).toQString(m_tmp1));
            if (!msg.endsWith(QLatin1Char('?')))
                msg += QLatin1Char('?');
            fprintf(stderr, "Project PROMPT: %s ", qPrintable(msg));

            QFile qfile;
            if (qfile.open(stdin, QIODevice::ReadOnly)) {
                QTextStream t(&qfile);
                ret = split_value_list(t.readLine());
            }
        }
        break; }
#endif
    case E_REPLACE:
        if (args.count() != 3 ) {
            evalError(fL1S("replace(var, before, after) requires three arguments."));
        } else {
            const QRegExp before(args.at(1).toQString());
            const QString &after(args.at(2).toQString(m_tmp2));
            foreach (const ProString &val, values(map(args.at(0)))) {
                QString rstr = val.toQString(m_tmp1);
                QString copy = rstr; // Force a detach on modify
                rstr.replace(before, after);
                ret << (rstr.isSharedWith(m_tmp1) ? val : ProString(rstr).setSource(val));
            }
        }
        break;
    case E_SORT_DEPENDS:
    case E_RESOLVE_DEPENDS:
        if (args.count() < 1 || args.count() > 3) {
            evalError(fL1S("%1(var, [prefix, [suffixes]]) requires one to three arguments.")
                      .arg(func.toQString(m_tmp1)));
        } else {
            QHash<ProKey, QSet<ProKey> > dependencies;
            ProValueMap dependees;
            ProStringList rootSet;
            ProStringList orgList = values(args.at(0).toKey());
            populateDeps(orgList, (args.count() < 2 ? ProString() : args.at(1)),
                         args.count() < 3 ? ProStringList(ProString(".depends"))
                                          : split_value_list(args.at(2).toQString(m_tmp2)),
                         dependencies, dependees, rootSet);
            for (int i = 0; i < rootSet.size(); ++i) {
                const ProString &item = rootSet.at(i);
                if ((func_t == E_RESOLVE_DEPENDS) || orgList.contains(item))
                    ret.prepend(item);
                foreach (const ProString &dep, dependees[item.toKey()]) {
                    QSet<ProKey> &dset = dependencies[dep.toKey()];
                    dset.remove(rootSet.at(i).toKey()); // *Don't* use 'item' - rootSet may have changed!
                    if (dset.isEmpty())
                        rootSet << dep;
                }
            }
        }
        break;
    case E_ENUMERATE_VARS: {
        QSet<ProString> keys;
        foreach (const ProValueMap &vmap, m_valuemapStack)
            for (ProValueMap::ConstIterator it = vmap.constBegin(); it != vmap.constEnd(); ++it)
                keys.insert(it.key());
        ret.reserve(keys.size());
        foreach (const ProString &key, keys)
            ret << key;
        break; }
    case E_SHADOWED:
        if (args.count() != 1) {
            evalError(fL1S("shadowed(path) requires one argument."));
        } else {
            QString rstr = m_option->shadowedPath(resolvePath(args.at(0).toQString(m_tmp1)));
            if (rstr.isEmpty())
                break;
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(0) : ProString(rstr).setSource(args.at(0)));
        }
        break;
    case E_ABSOLUTE_PATH:
        if (args.count() > 2) {
            evalError(fL1S("absolute_path(path[, base]) requires one or two arguments."));
        } else {
            QString rstr = QDir::cleanPath(
                    QDir(args.count() > 1 ? args.at(1).toQString(m_tmp2) : currentDirectory())
                    .absoluteFilePath(args.at(0).toQString(m_tmp1)));
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(0) : ProString(rstr).setSource(args.at(0)));
        }
        break;
    case E_RELATIVE_PATH:
        if (args.count() > 2) {
            evalError(fL1S("relative_path(path[, base]) requires one or two arguments."));
        } else {
            QDir baseDir(args.count() > 1 ? args.at(1).toQString(m_tmp2) : currentDirectory());
            QString rstr = baseDir.relativeFilePath(baseDir.absoluteFilePath(
                                args.at(0).toQString(m_tmp1)));
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(0) : ProString(rstr).setSource(args.at(0)));
        }
        break;
    case E_CLEAN_PATH:
        if (args.count() != 1) {
            evalError(fL1S("clean_path(path) requires one argument."));
        } else {
            QString rstr = QDir::cleanPath(args.at(0).toQString(m_tmp1));
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(0) : ProString(rstr).setSource(args.at(0)));
        }
        break;
    case E_SYSTEM_PATH:
        if (args.count() != 1) {
            evalError(fL1S("system_path(path) requires one argument."));
        } else {
            QString rstr = args.at(0).toQString(m_tmp1);
#ifdef Q_OS_WIN
            rstr.replace(QLatin1Char('/'), QLatin1Char('\\'));
#else
            rstr.replace(QLatin1Char('\\'), QLatin1Char('/'));
#endif
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(0) : ProString(rstr).setSource(args.at(0)));
        }
        break;
    case E_SHELL_PATH:
        if (args.count() != 1) {
            evalError(fL1S("shell_path(path) requires one argument."));
        } else {
            QString rstr = args.at(0).toQString(m_tmp1);
            if (m_dirSep.startsWith(QLatin1Char('\\')))
                rstr.replace(QLatin1Char('/'), QLatin1Char('\\'));
            else
                rstr.replace(QLatin1Char('\\'), QLatin1Char('/'));
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(0) : ProString(rstr).setSource(args.at(0)));
        }
        break;
    case E_SYSTEM_QUOTE:
        if (args.count() != 1) {
            evalError(fL1S("system_quote(arg) requires one argument."));
        } else {
            QString rstr = IoUtils::shellQuote(args.at(0).toQString(m_tmp1));
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(0) : ProString(rstr).setSource(args.at(0)));
        }
        break;
    case E_SHELL_QUOTE:
        if (args.count() != 1) {
            evalError(fL1S("shell_quote(arg) requires one argument."));
        } else {
            QString rstr = args.at(0).toQString(m_tmp1);
            if (m_dirSep.startsWith(QLatin1Char('\\')))
                rstr = IoUtils::shellQuoteWin(rstr);
            else
                rstr = IoUtils::shellQuoteUnix(rstr);
            ret << (rstr.isSharedWith(m_tmp1) ? args.at(0) : ProString(rstr).setSource(args.at(0)));
        }
        break;
    case E_GETENV:
        if (args.count() != 1) {
            evalError(fL1S("getenv(arg) requires one argument."));
        } else {
            const ProString &var = args.at(0);
            const ProString &val = ProString(m_option->getEnv(var.toQString(m_tmp1)));
            ret << val;
        }
        break;
    default:
        evalError(fL1S("Function '%1' is not implemented.").arg(func.toQString(m_tmp1)));
        break;
    }

    return ret;
}

QMakeEvaluator::VisitReturn QMakeEvaluator::evaluateBuiltinConditional(
        int func_t, const ProKey &function, const ProStringList &args)
{
    traceMsg("calling built-in %s(%s)", dbgKey(function), dbgSepStrList(args));

    switch (func_t) {
    case T_DEFINED: {
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("defined(function, [\"test\"|\"replace\"])"
                           " requires one or two arguments."));
            return ReturnFalse;
        }
        const ProKey &var = args.at(0).toKey();
        if (args.count() > 1) {
            if (args[1] == QLatin1String("test")) {
                return returnBool(m_functionDefs.testFunctions.contains(var));
            } else if (args[1] == QLatin1String("replace")) {
                return returnBool(m_functionDefs.replaceFunctions.contains(var));
            } else if (args[1] == QLatin1String("var")) {
                ProValueMap::Iterator it;
                return returnBool(findValues(var, &it));
            }
            evalError(fL1S("defined(function, type): unexpected type [%1].")
                      .arg(args.at(1).toQString(m_tmp1)));
            return ReturnFalse;
        }
        return returnBool(m_functionDefs.replaceFunctions.contains(var)
                          || m_functionDefs.testFunctions.contains(var));
    }
    case T_EXPORT: {
        if (args.count() != 1) {
            evalError(fL1S("export(variable) requires one argument."));
            return ReturnFalse;
        }
        const ProKey &var = map(args.at(0));
        for (ProValueMapStack::Iterator vmi = m_valuemapStack.end();
             --vmi != m_valuemapStack.begin(); ) {
            ProValueMap::Iterator it = (*vmi).find(var);
            if (it != (*vmi).end()) {
                if (it->constBegin() == statics.fakeValue.constBegin()) {
                    // This is stupid, but qmake doesn't propagate deletions
                    m_valuemapStack.first()[var] = ProStringList();
                } else {
                    m_valuemapStack.first()[var] = *it;
                }
                (*vmi).erase(it);
                while (--vmi != m_valuemapStack.begin())
                    (*vmi).remove(var);
                break;
            }
        }
        return ReturnTrue;
    }
    case T_INFILE:
        if (args.count() < 2 || args.count() > 3) {
            evalError(fL1S("infile(file, var, [values]) requires two or three arguments."));
        } else {
            ProValueMap vars;
            QString fn = resolvePath(m_option->expandEnvVars(args.at(0).toQString(m_tmp1)));
            fn.detach();
            VisitReturn ok = evaluateFileInto(fn, &vars, LoadProOnly);
            if (ok != ReturnTrue)
                return ok;
            if (args.count() == 2)
                return returnBool(vars.contains(map(args.at(1))));
            QRegExp regx;
            const QString &qry = args.at(2).toQString(m_tmp1);
            if (qry != QRegExp::escape(qry)) {
                QString copy = qry;
                copy.detach();
                regx.setPattern(copy);
            }
            int t = 0;
            foreach (const ProString &s, vars.value(map(args.at(1)))) {
                if ((!regx.isEmpty() && regx.exactMatch(s.toQString(m_tmp[t]))) || s == qry)
                    return ReturnTrue;
                t ^= 1;
            }
        }
        return ReturnFalse;
    case T_REQUIRES:
#ifdef PROEVALUATOR_FULL
        checkRequirements(args);
#endif
        return ReturnFalse; // Another qmake breakage
    case T_EVAL: {
            VisitReturn ret = ReturnFalse;
            ProFile *pro = m_parser->parsedProBlock(args.join(statics.field_sep),
                                                    m_current.pro->fileName(), m_current.line);
            if (pro) {
                if (m_cumulative || pro->isOk()) {
                    m_locationStack.push(m_current);
                    visitProBlock(pro, pro->tokPtr());
                    ret = ReturnTrue; // This return value is not too useful, but that's qmake
                    m_current = m_locationStack.pop();
                }
                pro->deref();
            }
            return ret;
        }
    case T_IF: {
        if (args.count() != 1) {
            evalError(fL1S("if(condition) requires one argument."));
            return ReturnFalse;
        }
        return returnBool(evaluateConditional(args.at(0).toQString(),
                                              m_current.pro->fileName(), m_current.line));
    }
    case T_CONFIG: {
        if (args.count() < 1 || args.count() > 2) {
            evalError(fL1S("CONFIG(config) requires one or two arguments."));
            return ReturnFalse;
        }
        if (args.count() == 1)
            return returnBool(isActiveConfig(args.at(0).toQString(m_tmp2)));
        const QStringList &mutuals = args.at(1).toQString(m_tmp2).split(QLatin1Char('|'));
        const ProStringList &configs = values(statics.strCONFIG);

        for (int i = configs.size() - 1; i >= 0; i--) {
            for (int mut = 0; mut < mutuals.count(); mut++) {
                if (configs[i] == mutuals[mut].trimmed())
                    return returnBool(configs[i] == args[0]);
            }
        }
        return ReturnFalse;
    }
    case T_CONTAINS: {
        if (args.count() < 2 || args.count() > 3) {
            evalError(fL1S("contains(var, val) requires two or three arguments."));
            return ReturnFalse;
        }

        const QString &qry = args.at(1).toQString(m_tmp1);
        QRegExp regx;
        if (qry != QRegExp::escape(qry)) {
            QString copy = qry;
            copy.detach();
            regx.setPattern(copy);
        }
        const ProStringList &l = values(map(args.at(0)));
        if (args.count() == 2) {
            int t = 0;
            for (int i = 0; i < l.size(); ++i) {
                const ProString &val = l[i];
                if ((!regx.isEmpty() && regx.exactMatch(val.toQString(m_tmp[t]))) || val == qry)
                    return ReturnTrue;
                t ^= 1;
            }
        } else {
            const QStringList &mutuals = args.at(2).toQString(m_tmp3).split(QLatin1Char('|'));
            for (int i = l.size() - 1; i >= 0; i--) {
                const ProString val = l[i];
                for (int mut = 0; mut < mutuals.count(); mut++) {
                    if (val == mutuals[mut].trimmed()) {
                        return returnBool((!regx.isEmpty()
                                           && regx.exactMatch(val.toQString(m_tmp2)))
                                          || val == qry);
                    }
                }
            }
        }
        return ReturnFalse;
    }
    case T_COUNT: {
        if (args.count() != 2 && args.count() != 3) {
            evalError(fL1S("count(var, count, op=\"equals\") requires two or three arguments."));
            return ReturnFalse;
        }
        int cnt = values(map(args.at(0))).count();
        int val = args.at(1).toQString(m_tmp1).toInt();
        if (args.count() == 3) {
            const ProString &comp = args.at(2);
            if (comp == QLatin1String(">") || comp == QLatin1String("greaterThan")) {
                return returnBool(cnt > val);
            } else if (comp == QLatin1String(">=")) {
                return returnBool(cnt >= val);
            } else if (comp == QLatin1String("<") || comp == QLatin1String("lessThan")) {
                return returnBool(cnt < val);
            } else if (comp == QLatin1String("<=")) {
                return returnBool(cnt <= val);
            } else if (comp == QLatin1String("equals") || comp == QLatin1String("isEqual")
                       || comp == QLatin1String("=") || comp == QLatin1String("==")) {
                // fallthrough
            } else {
                evalError(fL1S("Unexpected modifier to count(%2).").arg(comp.toQString(m_tmp1)));
                return ReturnFalse;
            }
        }
        return returnBool(cnt == val);
    }
    case T_GREATERTHAN:
    case T_LESSTHAN: {
        if (args.count() != 2) {
            evalError(fL1S("%1(variable, value) requires two arguments.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        const QString &rhs(args.at(1).toQString(m_tmp1)),
                      &lhs(values(map(args.at(0))).join(statics.field_sep));
        bool ok;
        int rhs_int = rhs.toInt(&ok);
        if (ok) { // do integer compare
            int lhs_int = lhs.toInt(&ok);
            if (ok) {
                if (func_t == T_GREATERTHAN)
                    return returnBool(lhs_int > rhs_int);
                return returnBool(lhs_int < rhs_int);
            }
        }
        if (func_t == T_GREATERTHAN)
            return returnBool(lhs > rhs);
        return returnBool(lhs < rhs);
    }
    case T_EQUALS:
        if (args.count() != 2) {
            evalError(fL1S("%1(variable, value) requires two arguments.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        return returnBool(values(map(args.at(0))).join(statics.field_sep)
                          == args.at(1).toQString(m_tmp1));
    case T_CLEAR: {
        if (args.count() != 1) {
            evalError(fL1S("%1(variable) requires one argument.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        ProValueMap *hsh;
        ProValueMap::Iterator it;
        const ProKey &var = map(args.at(0));
        if (!(hsh = findValues(var, &it)))
            return ReturnFalse;
        if (hsh == &m_valuemapStack.top())
            it->clear();
        else
            m_valuemapStack.top()[var].clear();
        return ReturnTrue;
    }
    case T_UNSET: {
        if (args.count() != 1) {
            evalError(fL1S("%1(variable) requires one argument.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        ProValueMap *hsh;
        ProValueMap::Iterator it;
        const ProKey &var = map(args.at(0));
        if (!(hsh = findValues(var, &it)))
            return ReturnFalse;
        if (m_valuemapStack.size() == 1)
            hsh->erase(it);
        else if (hsh == &m_valuemapStack.top())
            *it = statics.fakeValue;
        else
            m_valuemapStack.top()[var] = statics.fakeValue;
        return ReturnTrue;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    case T_PARSE_JSON: {
        if (args.count() != 2) {
            evalError(fL1S("parseJson(variable, into) requires two arguments."));
            return ReturnFalse;
        }

        QByteArray json = values(args.at(0).toKey()).join(QLatin1Char(' ')).toUtf8();
        QString parseInto = args.at(1).toQString(m_tmp2);
        return parseJsonInto(json, parseInto, &m_valuemapStack.top());
    }
#endif
    case T_INCLUDE: {
        if (args.count() < 1 || args.count() > 3) {
            evalError(fL1S("include(file, [into, [silent]]) requires one, two or three arguments."));
            return ReturnFalse;
        }
        QString parseInto;
        LoadFlags flags = 0;
        if (args.count() >= 2) {
            parseInto = args.at(1).toQString(m_tmp2);
            if (args.count() >= 3 && isTrue(args.at(2), m_tmp3))
                flags = LoadSilent;
        }
        QString fn = resolvePath(m_option->expandEnvVars(args.at(0).toQString(m_tmp1)));
        fn.detach();
        VisitReturn ok;
        if (parseInto.isEmpty()) {
            ok = evaluateFileChecked(fn, QMakeHandler::EvalIncludeFile, LoadProOnly | flags);
        } else {
            ProValueMap symbols;
            if ((ok = evaluateFileInto(fn, &symbols, LoadAll | flags)) == ReturnTrue) {
                ProValueMap newMap;
                for (ProValueMap::ConstIterator
                        it = m_valuemapStack.top().constBegin(),
                        end = m_valuemapStack.top().constEnd();
                        it != end; ++it) {
                    const QString &ky = it.key().toQString(m_tmp1);
                    if (!(ky.startsWith(parseInto) &&
                          (ky.length() == parseInto.length()
                           || ky.at(parseInto.length()) == QLatin1Char('.'))))
                        newMap[it.key()] = it.value();
                }
                for (ProValueMap::ConstIterator it = symbols.constBegin();
                     it != symbols.constEnd(); ++it) {
                    const QString &ky = it.key().toQString(m_tmp1);
                    if (!ky.startsWith(QLatin1Char('.')))
                        newMap.insert(ProKey(parseInto + QLatin1Char('.') + ky), it.value());
                }
                m_valuemapStack.top() = newMap;
            }
        }
        if (ok == ReturnFalse && (flags & LoadSilent))
            ok = ReturnTrue;
        return ok;
    }
    case T_LOAD: {
        bool ignore_error = false;
        if (args.count() == 2) {
            ignore_error = isTrue(args.at(1), m_tmp2);
        } else if (args.count() != 1) {
            evalError(fL1S("load(feature) requires one or two arguments."));
            return ReturnFalse;
        }
        VisitReturn ok = evaluateFeatureFile(m_option->expandEnvVars(args.at(0).toQString()),
                                             ignore_error);
        if (ok == ReturnFalse && ignore_error)
            ok = ReturnTrue;
        return ok;
    }
    case T_DEBUG: {
#ifdef PROEVALUATOR_DEBUG
        if (args.count() != 2) {
            evalError(fL1S("debug(level, message) requires two arguments."));
            return ReturnFalse;
        }
        int level = args.at(0).toInt();
        if (level <= m_debugLevel) {
            const QString &msg = m_option->expandEnvVars(args.at(1).toQString(m_tmp2));
            debugMsg(level, "Project DEBUG: %s", qPrintable(msg));
        }
#endif
        return ReturnTrue;
    }
    case T_LOG:
    case T_ERROR:
    case T_WARNING:
    case T_MESSAGE: {
        if (args.count() != 1) {
            evalError(fL1S("%1(message) requires one argument.")
                      .arg(function.toQString(m_tmp1)));
            return ReturnFalse;
        }
        const QString &msg = m_option->expandEnvVars(args.at(0).toQString(m_tmp2));
        if (!m_skipLevel) {
            if (func_t == T_LOG) {
#ifdef PROEVALUATOR_FULL
                fputs(msg.toLatin1().constData(), stderr);
#endif
            } else {
                m_handler->fileMessage(fL1S("Project %1: %2")
                                       .arg(function.toQString(m_tmp1).toUpper(), msg));
            }
        }
        return (func_t == T_ERROR && !m_cumulative) ? ReturnError : ReturnTrue;
    }
    case T_SYSTEM: {
        if (args.count() != 1) {
            evalError(fL1S("system(exec) requires one argument."));
            return ReturnFalse;
        }
#ifdef PROEVALUATOR_FULL
        if (m_cumulative) // Anything else would be insanity
            return ReturnFalse;
#ifndef QT_BOOTSTRAPPED
        QProcess proc;
        proc.setProcessChannelMode(QProcess::ForwardedChannels);
        runProcess(&proc, args.at(0).toQString(m_tmp2));
        return returnBool(proc.exitStatus() == QProcess::NormalExit && proc.exitCode() == 0);
#else
        return returnBool(system((QLatin1String("cd ")
                                  + IoUtils::shellQuote(QDir::toNativeSeparators(currentDirectory()))
                                  + QLatin1String(" && ") + args.at(0)).toLocal8Bit().constData()) == 0);
#endif
#else
        return ReturnTrue;
#endif
    }
    case T_ISEMPTY: {
        if (args.count() != 1) {
            evalError(fL1S("isEmpty(var) requires one argument."));
            return ReturnFalse;
        }
        return returnBool(values(map(args.at(0))).isEmpty());
    }
    case T_EXISTS: {
        if (args.count() != 1) {
            evalError(fL1S("exists(file) requires one argument."));
            return ReturnFalse;
        }
        const QString &file = resolvePath(m_option->expandEnvVars(args.at(0).toQString(m_tmp1)));

        // Don't use VFS here:
        // - it supports neither listing nor even directories
        // - it's unlikely that somebody would test for files they created themselves
        if (IoUtils::exists(file))
            return ReturnTrue;
        int slsh = file.lastIndexOf(QLatin1Char('/'));
        QString fn = file.mid(slsh+1);
        if (fn.contains(QLatin1Char('*')) || fn.contains(QLatin1Char('?'))) {
            QString dirstr = file.left(slsh+1);
            if (!QDir(dirstr).entryList(QStringList(fn)).isEmpty())
                return ReturnTrue;
        }

        return ReturnFalse;
    }
    case T_MKPATH: {
        if (args.count() != 1) {
            evalError(fL1S("mkpath(file) requires one argument."));
            return ReturnFalse;
        }
#ifdef PROEVALUATOR_FULL
        const QString &fn = resolvePath(args.at(0).toQString(m_tmp1));
        if (!QDir::current().mkpath(fn)) {
            evalError(fL1S("Cannot create directory %1.").arg(QDir::toNativeSeparators(fn)));
            return ReturnFalse;
        }
#endif
        return ReturnTrue;
    }
    case T_WRITE_FILE: {
        if (args.count() > 3) {
            evalError(fL1S("write_file(name, [content var, [append]]) requires one to three arguments."));
            return ReturnFalse;
        }
        QIODevice::OpenMode mode = QIODevice::Truncate;
        QString contents;
        if (args.count() >= 2) {
            const ProStringList &vals = values(args.at(1).toKey());
            if (!vals.isEmpty())
                contents = vals.join(fL1S("\n")) + QLatin1Char('\n');
            if (args.count() >= 3)
                if (!args.at(2).toQString(m_tmp1).compare(fL1S("append"), Qt::CaseInsensitive))
                    mode = QIODevice::Append;
        }
        return writeFile(QString(), resolvePath(args.at(0).toQString(m_tmp1)), mode, contents);
    }
    case T_TOUCH: {
        if (args.count() != 2) {
            evalError(fL1S("touch(file, reffile) requires two arguments."));
            return ReturnFalse;
        }
#ifdef PROEVALUATOR_FULL
        const QString &tfn = resolvePath(args.at(0).toQString(m_tmp1));
        const QString &rfn = resolvePath(args.at(1).toQString(m_tmp2));
#ifdef Q_OS_UNIX
        struct stat st;
        if (stat(rfn.toLocal8Bit().constData(), &st)) {
            evalError(fL1S("Cannot stat() reference file %1: %2.").arg(rfn, fL1S(strerror(errno))));
            return ReturnFalse;
        }
        struct utimbuf utb;
        utb.actime = time(0);
        utb.modtime = st.st_mtime;
        if (utime(tfn.toLocal8Bit().constData(), &utb)) {
            evalError(fL1S("Cannot touch %1: %2.").arg(tfn, fL1S(strerror(errno))));
            return ReturnFalse;
        }
#else
        HANDLE rHand = CreateFile((wchar_t*)rfn.utf16(),
                                  GENERIC_READ, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (rHand == INVALID_HANDLE_VALUE) {
            evalError(fL1S("Cannot open() reference file %1: %2.").arg(rfn, windowsErrorCode()));
            return ReturnFalse;
        }
        FILETIME ft;
        GetFileTime(rHand, 0, 0, &ft);
        CloseHandle(rHand);
        HANDLE wHand = CreateFile((wchar_t*)tfn.utf16(),
                                  GENERIC_WRITE, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (wHand == INVALID_HANDLE_VALUE) {
            evalError(fL1S("Cannot open() %1: %2.").arg(tfn, windowsErrorCode()));
            return ReturnFalse;
        }
        SetFileTime(wHand, 0, 0, &ft);
        CloseHandle(wHand);
#endif
#endif
        return ReturnTrue;
    }
    case T_CACHE: {
        if (args.count() > 3) {
            evalError(fL1S("cache(var, [set|add|sub] [transient] [super|stash], [srcvar]) requires one to three arguments."));
            return ReturnFalse;
        }
        bool persist = true;
        enum { TargetStash, TargetCache, TargetSuper } target = TargetCache;
        enum { CacheSet, CacheAdd, CacheSub } mode = CacheSet;
        ProKey srcvar;
        if (args.count() >= 2) {
            foreach (const ProString &opt, split_value_list(args.at(1).toQString(m_tmp2))) {
                opt.toQString(m_tmp3);
                if (m_tmp3 == QLatin1String("transient")) {
                    persist = false;
                } else if (m_tmp3 == QLatin1String("super")) {
                    target = TargetSuper;
                } else if (m_tmp3 == QLatin1String("stash")) {
                    target = TargetStash;
                } else if (m_tmp3 == QLatin1String("set")) {
                    mode = CacheSet;
                } else if (m_tmp3 == QLatin1String("add")) {
                    mode = CacheAdd;
                } else if (m_tmp3 == QLatin1String("sub")) {
                    mode = CacheSub;
                } else {
                    evalError(fL1S("cache(): invalid flag %1.").arg(m_tmp3));
                    return ReturnFalse;
                }
            }
            if (args.count() >= 3) {
                srcvar = args.at(2).toKey();
            } else if (mode != CacheSet) {
                evalError(fL1S("cache(): modes other than 'set' require a source variable."));
                return ReturnFalse;
            }
        }
        QString varstr;
        ProKey dstvar = args.at(0).toKey();
        if (!dstvar.isEmpty()) {
            if (srcvar.isEmpty())
                srcvar = dstvar;
            ProValueMap::Iterator srcvarIt;
            if (!findValues(srcvar, &srcvarIt)) {
                evalError(fL1S("Variable %1 is not defined.").arg(srcvar.toQString(m_tmp1)));
                return ReturnFalse;
            }
            // The caches for the host and target may differ (e.g., when we are manipulating
            // CONFIG), so we cannot compute a common new value for both.
            const ProStringList &diffval = *srcvarIt;
            ProStringList newval;
            bool changed = false;
            for (bool hostBuild = false; ; hostBuild = true) {
#ifdef PROEVALUATOR_THREAD_SAFE
                m_option->mutex.lock();
#endif
                QMakeBaseEnv *baseEnv =
                        m_option->baseEnvs.value(QMakeBaseKey(m_buildRoot, m_stashfile, hostBuild));
#ifdef PROEVALUATOR_THREAD_SAFE
                // It's ok to unlock this before locking baseEnv,
                // as we have no intention to initialize the env.
                m_option->mutex.unlock();
#endif
                do {
                    if (!baseEnv)
                        break;
#ifdef PROEVALUATOR_THREAD_SAFE
                    QMutexLocker locker(&baseEnv->mutex);
                    if (baseEnv->inProgress && baseEnv->evaluator != this) {
                        // The env is still in the works, but it may be already past the cache
                        // loading. So we need to wait for completion and amend it as usual.
                        QThreadPool::globalInstance()->releaseThread();
                        baseEnv->cond.wait(&baseEnv->mutex);
                        QThreadPool::globalInstance()->reserveThread();
                    }
                    if (!baseEnv->isOk)
                        break;
#endif
                    QMakeEvaluator *baseEval = baseEnv->evaluator;
                    const ProStringList &oldval = baseEval->values(dstvar);
                    if (mode == CacheSet) {
                        newval = diffval;
                    } else {
                        newval = oldval;
                        if (mode == CacheAdd)
                            newval += diffval;
                        else
                            removeEach(&newval, diffval);
                    }
                    if (oldval != newval) {
                        if (target != TargetStash || !m_stashfile.isEmpty()) {
                            baseEval->valuesRef(dstvar) = newval;
                            if (target == TargetSuper) {
                                do {
                                    if (dstvar == QLatin1String("QMAKEPATH")) {
                                        baseEval->m_qmakepath = newval.toQStringList();
                                        baseEval->updateMkspecPaths();
                                    } else if (dstvar == QLatin1String("QMAKEFEATURES")) {
                                        baseEval->m_qmakefeatures = newval.toQStringList();
                                    } else {
                                        break;
                                    }
                                    baseEval->updateFeaturePaths();
                                    if (hostBuild == m_hostBuild)
                                        m_featureRoots = baseEval->m_featureRoots;
                                } while (false);
                            }
                        }
                        changed = true;
                    }
                } while (false);
                if (hostBuild)
                    break;
            }
            // We assume that whatever got the cached value to be what it is now will do so
            // the next time as well, so we just skip the persisting if nothing changed.
            if (!persist || !changed)
                return ReturnTrue;
            varstr = dstvar.toQString();
            if (mode == CacheAdd)
                varstr += QLatin1String(" +=");
            else if (mode == CacheSub)
                varstr += QLatin1String(" -=");
            else
                varstr += QLatin1String(" =");
            if (diffval.count() == 1) {
                varstr += QLatin1Char(' ');
                varstr += quoteValue(diffval.at(0));
            } else if (!diffval.isEmpty()) {
                foreach (const ProString &vval, diffval) {
                    varstr += QLatin1String(" \\\n    ");
                    varstr += quoteValue(vval);
                }
            }
            varstr += QLatin1Char('\n');
        }
        QString fn;
        if (target == TargetSuper) {
            if (m_superfile.isEmpty()) {
                m_superfile = QDir::cleanPath(m_outputDir + QLatin1String("/.qmake.super"));
                printf("Info: creating super cache file %s\n", qPrintable(m_superfile));
                valuesRef(ProKey("_QMAKE_SUPER_CACHE_")) << ProString(m_superfile);
            }
            fn = m_superfile;
        } else if (target == TargetCache) {
            if (m_cachefile.isEmpty()) {
                m_cachefile = QDir::cleanPath(m_outputDir + QLatin1String("/.qmake.cache"));
                printf("Info: creating cache file %s\n", qPrintable(m_cachefile));
                valuesRef(ProKey("_QMAKE_CACHE_")) << ProString(m_cachefile);
                // We could update m_{source,build}Root and m_featureRoots here, or even
                // "re-home" our rootEnv, but this doesn't sound too useful - if somebody
                // wanted qmake to find something in the build directory, he could have
                // done so "from the outside".
                // The sub-projects will find the new cache all by themselves.
            }
            fn = m_cachefile;
        } else {
            fn = m_stashfile;
            if (fn.isEmpty())
                fn = QDir::cleanPath(m_outputDir + QLatin1String("/.qmake.stash"));
            if (!m_vfs->exists(fn)) {
                printf("Info: creating stash file %s\n", qPrintable(fn));
                valuesRef(ProKey("_QMAKE_STASH_")) << ProString(fn);
            }
        }
        return writeFile(fL1S("cache "), fn, QIODevice::Append, varstr);
    }
    default:
        evalError(fL1S("Function '%1' is not implemented.").arg(function.toQString(m_tmp1)));
        return ReturnFalse;
    }
}

QT_END_NAMESPACE
