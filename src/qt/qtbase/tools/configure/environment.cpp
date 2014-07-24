/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "environment.h"

#include <qdebug.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstandardpaths.h>

#include <process.h>
#include <errno.h>
#include <iostream>

//#define CONFIGURE_DEBUG_EXECUTE
//#define CONFIGURE_DEBUG_CP_DIR

using namespace std;

#ifdef Q_OS_WIN32
#include <qt_windows.h>
#endif

#include <windows/registry_p.h> // from tools/shared

QT_BEGIN_NAMESPACE

struct CompilerInfo{
    Compiler compiler;
    const char *compilerStr;
    const char *regKey;
    const char *executable;
} compiler_info[] = {
    // The compilers here are sorted in a reversed-preferred order
    {CC_BORLAND, "Borland C++",                                                    0, "bcc32.exe"},
    {CC_MINGW,   "MinGW (Minimalist GNU for Windows)",                             0, "g++.exe"},
    {CC_INTEL,   "Intel(R) C++ Compiler for 32-bit applications",                  0, "icl.exe"}, // xilink.exe, xilink5.exe, xilink6.exe, xilib.exe
    {CC_NET2003, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2003 (7.1)",  "Software\\Microsoft\\VisualStudio\\7.1\\Setup\\VC\\ProductDir", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2003, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2003 (7.1)",  "Software\\Wow6432Node\\Microsoft\\VisualStudio\\7.1\\Setup\\VC\\ProductDir", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2005, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2005 (8.0)",  "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\8.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2005, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2005 (8.0)",  "Software\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7\\8.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2008, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2008 (9.0)",  "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\9.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2008, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2008 (9.0)",  "Software\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7\\9.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2010, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2010 (10.0)", "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\10.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2010, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2010 (10.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7\\10.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2012, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2012 (11.0)", "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\11.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2012, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2012 (11.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7\\11.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2013, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2013 (12.0)", "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\12.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2013, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2013 (12.0)", "Software\\Wow6432Node\\Microsoft\\VisualStudio\\SxS\\VC7\\12.0", "cl.exe"}, // link.exe, lib.exe
    {CC_UNKNOWN, "Unknown", 0, 0},
};


// Initialize static variables
Compiler Environment::detectedCompiler = CC_UNKNOWN;

/*!
    Returns the pointer to the CompilerInfo for a \a compiler.
*/
CompilerInfo *Environment::compilerInfo(Compiler compiler)
{
    int i = 0;
    while(compiler_info[i].compiler != compiler && compiler_info[i].compiler != CC_UNKNOWN)
        ++i;
    return &(compiler_info[i]);
}

/*!
    Returns the qmakespec for the compiler detected on the system.
*/
QString Environment::detectQMakeSpec()
{
    QString spec;
    switch (detectCompiler()) {
    case CC_NET2013:
        spec = "win32-msvc2013";
        break;
    case CC_NET2012:
        spec = "win32-msvc2012";
        break;
    case CC_NET2010:
        spec = "win32-msvc2010";
        break;
    case CC_NET2008:
        spec = "win32-msvc2008";
        break;
    case CC_NET2005:
        spec = "win32-msvc2005";
        break;
    case CC_NET2003:
        spec = "win32-msvc2003";
        break;
    case CC_INTEL:
        spec = "win32-icc";
        break;
    case CC_MINGW:
        spec = "win32-g++";
        break;
    case CC_BORLAND:
        spec = "win32-borland";
        break;
    default:
        break;
    }

    return spec;
}

Compiler Environment::compilerFromQMakeSpec(const QString &qmakeSpec)
{
    if (qmakeSpec == QLatin1String("win32-msvc2013"))
        return CC_NET2013;
    if (qmakeSpec == QLatin1String("win32-msvc2012"))
        return CC_NET2012;
    if (qmakeSpec == QLatin1String("win32-msvc2010"))
        return CC_NET2010;
    if (qmakeSpec == QLatin1String("win32-msvc2008"))
        return CC_NET2008;
    if (qmakeSpec == QLatin1String("win32-msvc2005"))
        return CC_NET2005;
    if (qmakeSpec == QLatin1String("win32-msvc2003"))
        return CC_NET2003;
    if (qmakeSpec == QLatin1String("win32-icc"))
        return CC_INTEL;
    if (qmakeSpec == QLatin1String("win32-g++"))
        return CC_MINGW;
    if (qmakeSpec == QLatin1String("win32-borland"))
        return CC_BORLAND;
    return CC_UNKNOWN;
}

/*!
    Returns the enum of the compiler which was detected on the system.
    The compilers are detected in the order as entered into the
    compiler_info list.

    If more than one compiler is found, CC_UNKNOWN is returned.
*/
Compiler Environment::detectCompiler()
{
#ifndef Q_OS_WIN32
    return CC_UNKNOWN; // Always generate CC_UNKNOWN on other platforms
#else
    if(detectedCompiler != CC_UNKNOWN)
        return detectedCompiler;

    int installed = 0;

    // Check for compilers in registry first, to see which version is in PATH
    QString paths = qgetenv("PATH");
    QStringList pathlist = paths.toLower().split(";");
    for(int i = 0; compiler_info[i].compiler; ++i) {
        QString productPath = qt_readRegistryKey(HKEY_LOCAL_MACHINE, compiler_info[i].regKey).toLower();
        if (productPath.length()) {
            QStringList::iterator it;
            for(it = pathlist.begin(); it != pathlist.end(); ++it) {
                if((*it).contains(productPath)) {
                    if (detectedCompiler != compiler_info[i].compiler) {
                        ++installed;
                        detectedCompiler = compiler_info[i].compiler;
                    }
                    /* else {

                        We detected the same compiler again, which happens when
                        configure is build with the 64-bit compiler. Skip the
                        duplicate so that we don't think it's installed twice.

                    }
                    */
                    break;
                }
            }
        }
    }

    // Now just go looking for the executables, and accept any executable as the lowest version
    if (!installed) {
        for(int i = 0; compiler_info[i].compiler; ++i) {
            QString executable = QString(compiler_info[i].executable).toLower();
            if (executable.length() && !QStandardPaths::findExecutable(executable).isEmpty()) {
                if (detectedCompiler != compiler_info[i].compiler) {
                    ++installed;
                    detectedCompiler = compiler_info[i].compiler;
                }
                /* else {

                    We detected the same compiler again, which happens when
                    configure is build with the 64-bit compiler. Skip the
                    duplicate so that we don't think it's installed twice.

                }
                */
                break;
            }
        }
    }

    if (installed > 1) {
        cout << "Found more than one known compiler! Using \"" << compilerInfo(detectedCompiler)->compilerStr << "\"" << endl;
        detectedCompiler = CC_UNKNOWN;
    }
    return detectedCompiler;
#endif
};

/*!
    Creates a commandling from \a program and it \a arguments,
    escaping characters that needs it.
*/
static QString qt_create_commandline(const QString &program, const QStringList &arguments)
{
    QString programName = program;
    if (!programName.startsWith("\"") && !programName.endsWith("\"") && programName.contains(" "))
        programName = "\"" + programName + "\"";
    programName.replace("/", "\\");

    QString args;
    // add the prgram as the first arrg ... it works better
    args = programName + " ";
    for (int i=0; i<arguments.size(); ++i) {
        QString tmp = arguments.at(i);
        // in the case of \" already being in the string the \ must also be escaped
        tmp.replace( "\\\"", "\\\\\"" );
        // escape a single " because the arguments will be parsed
        tmp.replace( "\"", "\\\"" );
        if (tmp.isEmpty() || tmp.contains(' ') || tmp.contains('\t')) {
            // The argument must not end with a \ since this would be interpreted
            // as escaping the quote -- rather put the \ behind the quote: e.g.
            // rather use "foo"\ than "foo\"
            QString endQuote("\"");
            int i = tmp.length();
            while (i>0 && tmp.at(i-1) == '\\') {
                --i;
                endQuote += "\\";
            }
            args += QString(" \"") + tmp.left(i) + endQuote;
        } else {
            args += ' ' + tmp;
        }
    }
    return args;
}

/*!
    Creates a QByteArray of the \a environment.
*/
static QByteArray qt_create_environment(const QStringList &environment)
{
    QByteArray envlist;
    if (environment.isEmpty())
        return envlist;

    int pos = 0;
    // add PATH if necessary (for DLL loading)
    QByteArray path = qgetenv("PATH");
    if (environment.filter(QRegExp("^PATH=",Qt::CaseInsensitive)).isEmpty() && !path.isNull()) {
            QString tmp = QString(QLatin1String("PATH=%1")).arg(QString::fromLocal8Bit(path));
            uint tmpSize = sizeof(wchar_t) * (tmp.length() + 1);
            envlist.resize(envlist.size() + tmpSize);
            memcpy(envlist.data() + pos, tmp.utf16(), tmpSize);
            pos += tmpSize;
    }
    // add the user environment
    foreach (const QString &tmp, environment) {
            uint tmpSize = sizeof(wchar_t) * (tmp.length() + 1);
            envlist.resize(envlist.size() + tmpSize);
            memcpy(envlist.data() + pos, tmp.utf16(), tmpSize);
            pos += tmpSize;
    }
    // add the 2 terminating 0 (actually 4, just to be on the safe side)
    envlist.resize(envlist.size() + 4);
    envlist[pos++] = 0;
    envlist[pos++] = 0;
    envlist[pos++] = 0;
    envlist[pos++] = 0;

    return envlist;
}

/*!
    Executes the command described in \a arguments, in the
    environment inherited from the parent process, with the
    \a additionalEnv settings applied.
    \a removeEnv removes the specified environment variables from
    the environment of the executed process.

    Returns the exit value of the process, or -1 if the command could
    not be executed.

    This function uses _(w)spawnvpe to spawn a process by searching
    through the PATH environment variable.
*/
int Environment::execute(QStringList arguments, const QStringList &additionalEnv, const QStringList &removeEnv)
{
#ifdef CONFIGURE_DEBUG_EXECUTE
    qDebug() << "About to Execute: " << arguments;
    qDebug() << "   " << QDir::currentPath();
    qDebug() << "   " << additionalEnv;
    qDebug() << "   " << removeEnv;
#endif
    // Create the full environment from the current environment and
    // the additionalEnv strings, then remove all variables defined
    // in removeEnv
    QMap<QString, QString> fullEnvMap;
    LPWSTR envStrings = GetEnvironmentStrings();
    if (envStrings) {
        int strLen = 0;
        for (LPWSTR envString = envStrings; *(envString); envString += strLen + 1) {
            strLen = int(wcslen(envString));
            QString str = QString((const QChar*)envString, strLen);
            if (!str.startsWith("=")) { // These are added by the system
                int sepIndex = str.indexOf('=');
                fullEnvMap.insert(str.left(sepIndex).toUpper(), str.mid(sepIndex +1));
            }
        }
    }
    FreeEnvironmentStrings(envStrings);

    // Add additionalEnv variables
    for (int i = 0; i < additionalEnv.count(); ++i) {
        const QString &str = additionalEnv.at(i);
        int sepIndex = str.indexOf('=');
        fullEnvMap.insert(str.left(sepIndex).toUpper(), str.mid(sepIndex +1));
    }

    // Remove removeEnv variables
    for (int j = 0; j < removeEnv.count(); ++j)
        fullEnvMap.remove(removeEnv.at(j).toUpper());

    // Add all variables to a QStringList
    QStringList fullEnv;
    QMapIterator<QString, QString> it(fullEnvMap);
    while (it.hasNext()) {
        it.next();
        fullEnv += QString(it.key() + "=" + it.value());
    }

    // ----------------------------
    QString program = arguments.takeAt(0);
    QString args = qt_create_commandline(program, arguments);
    QByteArray envlist = qt_create_environment(fullEnv);

    DWORD exitCode = DWORD(-1);
    PROCESS_INFORMATION procInfo;
    memset(&procInfo, 0, sizeof(procInfo));

    STARTUPINFO startInfo;
    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);

    bool couldExecute = CreateProcess(0, (wchar_t*)args.utf16(),
                                      0, 0, true, CREATE_UNICODE_ENVIRONMENT,
                                      envlist.isEmpty() ? 0 : envlist.data(),
                                      0, &startInfo, &procInfo);

    if (couldExecute) {
        WaitForSingleObject(procInfo.hProcess, INFINITE);
        GetExitCodeProcess(procInfo.hProcess, &exitCode);
        CloseHandle(procInfo.hThread);
        CloseHandle(procInfo.hProcess);
    }


    if (exitCode == DWORD(-1)) {
        switch(GetLastError()) {
        case E2BIG:
            cerr << "execute: Argument list exceeds 1024 bytes" << endl;
            foreach (const QString &arg, arguments)
                cerr << "   (" << arg.toLocal8Bit().constData() << ")" << endl;
            break;
        case ENOENT:
            cerr << "execute: File or path is not found (" << program.toLocal8Bit().constData() << ")" << endl;
            break;
        case ENOEXEC:
            cerr << "execute: Specified file is not executable or has invalid executable-file format (" << program.toLocal8Bit().constData() << ")" << endl;
            break;
        case ENOMEM:
            cerr << "execute: Not enough memory is available to execute new process." << endl;
            break;
        default:
            cerr << "execute: Unknown error" << endl;
            foreach (const QString &arg, arguments)
                cerr << "   (" << arg.toLocal8Bit().constData() << ")" << endl;
            break;
        }
    }
    return exitCode;
}

/*!
    Executes \a command with _popen() and returns the stdout of the command.

    Taken from qmake's system() command.
*/
QString Environment::execute(const QString &command, int *returnCode)
{
    QString output;
    FILE *proc = _popen(command.toLatin1().constData(), "r");
    char buff[256];
    while (proc && !feof(proc)) {
        int read_in = int(fread(buff, 1, 255, proc));
        if (!read_in)
            break;
        buff[read_in] = '\0';
        output += buff;
    }
    if (proc) {
        int r = _pclose(proc);
        if (returnCode)
            *returnCode = r;
    }
    return output;
}

/*!
    Copies the \a srcDir contents into \a destDir.

    Returns true if copying was successful.
*/
bool Environment::cpdir(const QString &srcDir, const QString &destDir)
{
    QString cleanSrcName = QDir::cleanPath(srcDir);
    QString cleanDstName = QDir::cleanPath(destDir);

#ifdef CONFIGURE_DEBUG_CP_DIR
    qDebug() << "Attempt to cpdir " << cleanSrcName << "->" << cleanDstName;
#endif
    if(!QFile::exists(cleanDstName) && !QDir().mkpath(cleanDstName)) {
        qDebug() << "cpdir: Failure to create " << cleanDstName;
        return false;
    }

    bool result = true;
    QDir dir = QDir(cleanSrcName);
    QFileInfoList allEntries = dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; result && (i < allEntries.count()); ++i) {
        QFileInfo entry = allEntries.at(i);
        bool intermediate = true;
        if (entry.isDir()) {
            intermediate = cpdir(QString("%1/%2").arg(cleanSrcName).arg(entry.fileName()),
                                 QString("%1/%2").arg(cleanDstName).arg(entry.fileName()));
        } else {
            QString destFile = QString("%1/%2").arg(cleanDstName).arg(entry.fileName());
#ifdef CONFIGURE_DEBUG_CP_DIR
            qDebug() << "About to cp (file)" << entry.absoluteFilePath() << "->" << destFile;
#endif
            QFile::remove(destFile);
            intermediate = QFile::copy(entry.absoluteFilePath(), destFile);
            SetFileAttributes((wchar_t*)destFile.utf16(), FILE_ATTRIBUTE_NORMAL);
        }
        if (!intermediate) {
            qDebug() << "cpdir: Failure for " << entry.fileName() << entry.isDir();
            result = false;
        }
    }
    return result;
}

bool Environment::rmdir(const QString &name)
{
    bool result = true;
    QString cleanName = QDir::cleanPath(name);

    QDir dir = QDir(cleanName);
    QFileInfoList allEntries = dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; result && (i < allEntries.count()); ++i) {
        QFileInfo entry = allEntries.at(i);
        if (entry.isDir()) {
            result &= rmdir(entry.absoluteFilePath());
        } else {
            result &= QFile::remove(entry.absoluteFilePath());
        }
    }
    result &= dir.rmdir(cleanName);
    return result;
}

static QStringList splitPathList(const QString &path)
{
#if defined(Q_OS_WIN)
    QRegExp splitReg(QStringLiteral("[;,]"));
#else
    QRegExp splitReg(QStringLiteral("[:]"));
#endif
    QStringList result = path.split(splitReg, QString::SkipEmptyParts);
    const QStringList::iterator end = result.end();
    for (QStringList::iterator it = result.begin(); it != end; ++it) {
        // Remove any leading or trailing ", this is commonly used in the environment
        // variables
        if (it->startsWith('"'))
            it->remove(0, 1);
        if (it->endsWith('"'))
            it->chop(1);
        *it = QDir::cleanPath(*it);
        if (it->endsWith(QLatin1Char('/')))
            it->chop(1);
    }
    return result;
}

QString Environment::findFileInPaths(const QString &fileName, const QStringList &paths)
{
    if (!paths.isEmpty()) {
        QDir d;
        const QChar separator = QDir::separator();
        foreach (const QString &path, paths)
            if (d.exists(path + separator + fileName))
                    return path;
    }
    return QString();
}

QStringList Environment::path()
{
    return splitPathList(QString::fromLocal8Bit(qgetenv("PATH")));
}

static QStringList mingwPaths(const QString &mingwPath, const QString &pathName)
{
    QStringList ret;
    QDir mingwDir(mingwPath);
    const QFileInfoList subdirs = mingwDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (int i = 0 ;i < subdirs.length(); ++i) {
        const QFileInfo &fi = subdirs.at(i);
        const QString name = fi.fileName();
        if (name == pathName)
            ret += fi.absoluteFilePath();
        else if (name.contains(QLatin1String("mingw"))) {
            ret += fi.absoluteFilePath() + QLatin1Char('/') + pathName;
        }
    }
    return ret;
}

// Return MinGW location from "c:\mingw\bin" -> "c:\mingw"
static inline QString detectMinGW()
{
    const QString gcc = QStandardPaths::findExecutable(QLatin1String("g++.exe"));
    return gcc.isEmpty() ?
           gcc : QFileInfo(QFileInfo(gcc).absolutePath()).absolutePath();
}

// Detect Direct X SDK up tp June 2010. Included in Windows Kit 8.
QString Environment::detectDirectXSdk()
{
    const QByteArray directXSdkEnv = qgetenv("DXSDK_DIR");
    if (directXSdkEnv.isEmpty())
        return QString();
    QString directXSdk = QDir::cleanPath(QString::fromLocal8Bit(directXSdkEnv));
    if (directXSdk.endsWith(QLatin1Char('/')))
        directXSdk.truncate(directXSdk.size() - 1);
    return directXSdk;
}

QStringList Environment::headerPaths(Compiler compiler)
{
    QStringList headerPaths;
    if (compiler == CC_MINGW) {
        const QString mingwPath = detectMinGW();
        headerPaths = mingwPaths(mingwPath, QLatin1String("include"));
        // Additional compiler paths
        const QFileInfoList mingwConfigs = QDir(mingwPath + QLatin1String("/lib/gcc")).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (int i = 0; i < mingwConfigs.length(); ++i) {
            const QDir mingwLibDir = mingwConfigs.at(i).absoluteFilePath();
            foreach (const QFileInfo &version, mingwLibDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
                headerPaths += version.absoluteFilePath() + QLatin1String("/include");
        }
    }
    // MinGW: Although gcc doesn't care about INCLUDE, qmake automatically adds it via -I
    headerPaths += splitPathList(QString::fromLocal8Bit(getenv("INCLUDE")));

    // Add Direct X SDK for ANGLE
    const QString directXSdk = detectDirectXSdk();
    if (!directXSdk.isEmpty()) // Add Direct X SDK for ANGLE
        headerPaths += directXSdk + QLatin1String("/include");
    return headerPaths;
}

QStringList Environment::libraryPaths(Compiler compiler)
{
    QStringList libraryPaths;
    if (compiler == CC_MINGW) {
        libraryPaths = mingwPaths(detectMinGW(), "lib");
    }
    // MinGW: Although gcc doesn't care about LIB, qmake automatically adds it via -L
    libraryPaths += splitPathList(QString::fromLocal8Bit(qgetenv("LIB")));

    // Add Direct X SDK for ANGLE
    const QString directXSdk = detectDirectXSdk();
    if (!directXSdk.isEmpty()) {
#ifdef Q_OS_WIN64
        libraryPaths += directXSdk + QLatin1String("/lib/x64");
#else
        libraryPaths += directXSdk + QLatin1String("/lib/x86");
#endif
    }
    return libraryPaths;
}

QT_END_NAMESPACE
