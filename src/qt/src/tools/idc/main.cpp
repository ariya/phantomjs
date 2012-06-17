/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include <QFile>
#include <QProcess>
#include <QLibraryInfo>
#include <qt_windows.h>
#include <io.h>

QT_BEGIN_NAMESPACE

static QString quotePath(const QString &s)
{
    if (!s.startsWith(QLatin1Char('\"')) && s.contains(QLatin1Char(' ')))
        return QLatin1Char('\"') + s + QLatin1Char('\"');
    return s;
}


static bool runWithQtInEnvironment(const QString &cmd)
{
    QProcess proc;
    
    // prepend the qt binary directory to the path
    QStringList env = QProcess::systemEnvironment();
    for (int i=0; i<env.count(); ++i) {
        QString var = env.at(i);
        int setidx = var.indexOf(QLatin1Char('='));
        if (setidx != -1) {
            QString varname = var.left(setidx).trimmed().toUpper();
            if (varname == QLatin1String("PATH")) {
                var = var.mid(setidx + 1);
                var = QLatin1String("PATH=") + 
                    QLibraryInfo::location(QLibraryInfo::BinariesPath) +
                    QLatin1Char(';') + var;
                env[i] = var;
                break;
            }
        }
    }

    proc.setEnvironment(env);
    proc.start(cmd);
    proc.waitForFinished(-1);
    
    return (proc.exitCode() == 0);
}

static bool attachTypeLibrary(const QString &applicationName, int resource, const QByteArray &data, QString *errorMessage)
{
    HANDLE hExe = BeginUpdateResource((const wchar_t *)applicationName.utf16(), false);
    if (hExe == 0) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach type library to binary %1 - could not open file.").arg(applicationName);
        return false;
    }
    if (!UpdateResource(hExe, L"TYPELIB", MAKEINTRESOURCE(resource), 0, (void*)data.data(), data.count())) {
        EndUpdateResource(hExe, true);
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach type library to binary %1 - could not update file.").arg(applicationName);
        return false;
    }

    if (!EndUpdateResource(hExe,false)) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach type library to binary %1 - could not write file.").arg(applicationName);
        return false;
    }
    
    if (errorMessage)
        *errorMessage = QString::fromLatin1("Type library attached to %1.").arg(applicationName);
    return true;
}

static bool registerServer(const QString &input)
{
    bool ok = false;    
    if (input.endsWith(QLatin1String(".exe"))) {
        ok = runWithQtInEnvironment(quotePath(input) + QLatin1String(" -regserver"));
    } else {
        HMODULE hdll = LoadLibrary((const wchar_t *)input.utf16());
        if (!hdll) {
            fprintf(stderr, "Couldn't load library file %s\n", (const char*)input.toLocal8Bit().data());
            return false;
        }
        typedef HRESULT(__stdcall* RegServerProc)();
        RegServerProc DllRegisterServer = (RegServerProc)GetProcAddress(hdll, "DllRegisterServer");
        if (!DllRegisterServer) {
            fprintf(stderr, "Library file %s doesn't appear to be a COM library\n", (const char*)input.toLocal8Bit().data());
            return false;
        }
        ok = DllRegisterServer() == S_OK;
    }
    return ok;
}

static bool unregisterServer(const QString &input)
{
    bool ok = false;
    if (input.endsWith(QLatin1String(".exe"))) {        
        ok = runWithQtInEnvironment(quotePath(input) + QLatin1String(" -unregserver"));
    } else {
        HMODULE hdll = LoadLibrary((const wchar_t *)input.utf16());
        if (!hdll) {
            fprintf(stderr, "Couldn't load library file %s\n", (const char*)input.toLocal8Bit().data());
            return false;
        }
        typedef HRESULT(__stdcall* RegServerProc)();
        RegServerProc DllUnregisterServer = (RegServerProc)GetProcAddress(hdll, "DllUnregisterServer");
        if (!DllUnregisterServer) {
            fprintf(stderr, "Library file %s doesn't appear to be a COM library\n", (const char*)input.toLocal8Bit().data());
            return false;
        }
        ok = DllUnregisterServer() == S_OK;
    }
    return ok;
}

static HRESULT dumpIdl(const QString &input, const QString &idlfile, const QString &version)
{
    HRESULT res = E_FAIL;
    
    if (input.endsWith(QLatin1String(".exe"))) {
        if (runWithQtInEnvironment(quotePath(input) + QLatin1String(" -dumpidl ") + idlfile + QLatin1String(" -version ") + version))
            res = S_OK;
    } else {
        HMODULE hdll = LoadLibrary((const wchar_t *)input.utf16());
        if (!hdll) {
            fprintf(stderr, "Couldn't load library file %s\n", (const char*)input.toLocal8Bit().data());
            return 3;
        }
        typedef HRESULT(__stdcall* DumpIDLProc)(const QString&, const QString&);
        DumpIDLProc DumpIDL = (DumpIDLProc)GetProcAddress(hdll, "DumpIDL");
        if (!DumpIDL) {
            fprintf(stderr, "Couldn't resolve 'DumpIDL' symbol in %s\n", (const char*)input.toLocal8Bit().data());
            return 3;
        }
        res = DumpIDL(idlfile, version);
        FreeLibrary(hdll);
    }
    
    return res;
}

static void slashify(QString &s)
{
    if (!s.contains(QLatin1Char('/')))
        return;
    
    int i = 0;
    while (i < (int)s.length()) {
        if (s[i] == QLatin1Char('/'))
            s[i] = QLatin1Char('\\');
        ++i;
    }
}

int runIdc(int argc, char **argv)
{
    QString error;
    QString tlbfile;
    QString idlfile;
    QString input;
    QString version = QLatin1String("1.0");
    
    int i = 1;
    while (i < argc) {
        QString p = QString::fromLocal8Bit(argv[i]).toLower();
        
        if (p == QLatin1String("/idl") || p == QLatin1String("-idl")) {
            ++i;
            if (i > argc) {
                error = QLatin1String("Missing name for interface definition file!");
                break;
            }
            idlfile = QLatin1String(argv[i]);
            idlfile = idlfile.trimmed().toLower();            
        } else if (p == QLatin1String("/version") || p == QLatin1String("-version")) {
            ++i;
            if (i > argc)
                version = QLatin1String("1.0");
            else
                version = QLatin1String(argv[i]);
        } else if (p == QLatin1String("/tlb") || p == QLatin1String("-tlb")) {
            ++i;
            if (i > argc) {
                error = QLatin1String("Missing name for type library file!");
                break;
            }
            tlbfile = QLatin1String(argv[i]);
            tlbfile = tlbfile.trimmed().toLower();            
        } else if (p == QLatin1String("/v") || p == QLatin1String("-v")) {
            fprintf(stdout, "Qt Interface Definition Compiler version 1.0\n");
            return 0;
        } else if (p == QLatin1String("/regserver") || p == QLatin1String("-regserver")) {
            if (!registerServer(input)) {
                fprintf(stderr, "Failed to register server!\n");
                return 1;
            }
            fprintf(stderr, "Server registered successfully!\n");
            return 0;
        } else if (p == QLatin1String("/unregserver") || p == QLatin1String("-unregserver")) {
            if (!unregisterServer(input)) {
                fprintf(stderr, "Failed to unregister server!\n");
                return 1;
            }
            fprintf(stderr, "Server unregistered successfully!\n");
            return 0;
        } else if (p[0] == QLatin1Char('/') || p[0] == QLatin1Char('-')) {
            error = QLatin1String("Unknown option \"") + p + QLatin1Char('\"');
            break;
        } else {
            input = QLatin1String(argv[i]);
            input = input.trimmed().toLower();            
        }
        i++;
    }
    if (!error.isEmpty()) {
        fprintf(stderr, "%s", error.toLatin1().data());
        fprintf(stderr, "\n");
        return 5;
    }
    if (input.isEmpty()) {
        fprintf(stderr, "No input file specified!\n");
        return 1;
    }
    if (input.endsWith(QLatin1String(".exe")) && tlbfile.isEmpty() && idlfile.isEmpty()) {
        fprintf(stderr, "No type output file specified!\n");
        return 2;
    }
    if (input.endsWith(QLatin1String(".dll")) && idlfile.isEmpty() && tlbfile.isEmpty()) {
        fprintf(stderr, "No interface definition file and no type library file specified!\n");
        return 3;
    }
    slashify(input);
    if (!tlbfile.isEmpty()) {
        slashify(tlbfile);
        QFile file(tlbfile);
        if (!file.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Couldn't open %s for read\n", (const char*)tlbfile.toLocal8Bit().data());
            return 4;
        }
        QByteArray data = file.readAll();
        QString error;
        bool ok = attachTypeLibrary(input, 1, data, &error);
        fprintf(stderr, "%s", error.toLatin1().data());
        fprintf(stderr, "\n");
        return ok ? 0 : 4;
    } else if (!idlfile.isEmpty()) {
        slashify(idlfile);
        idlfile = quotePath(idlfile);
        fprintf(stderr, "\n\n%s\n\n", (const char*)idlfile.toLocal8Bit().data());
        quotePath(input);
        HRESULT res = dumpIdl(input, idlfile, version);
        
        switch(res) {
        case S_OK:
            break;
        case E_FAIL:
            fprintf(stderr, "IDL generation failed trying to run program %s!\n", (const char*)input.toLocal8Bit().data());
            return res;
        case -1:
            fprintf(stderr, "Couldn't open %s for writing!\n", (const char*)idlfile.toLocal8Bit().data());
            return res;
        case 1:
            fprintf(stderr, "Malformed appID value in %s!\n", (const char*)input.toLocal8Bit().data());
            return res;
        case 2:
            fprintf(stderr, "Malformed typeLibID value in %s!\n", (const char*)input.toLocal8Bit().data());
            return res;
        case 3:
            fprintf(stderr, "Class has no metaobject information (error in %s)!\n", (const char*)input.toLocal8Bit().data());
            return res;
        case 4:
            fprintf(stderr, "Malformed classID value in %s!\n", (const char*)input.toLocal8Bit().data());
            return res;
        case 5:
            fprintf(stderr, "Malformed interfaceID value in %s!\n", (const char*)input.toLocal8Bit().data());
            return res;
        case 6:
            fprintf(stderr, "Malformed eventsID value in %s!\n", (const char*)input.toLocal8Bit().data());
            return res;
            
        default:
            fprintf(stderr, "Unknown error writing IDL from %s\n", (const char*)input.toLocal8Bit().data());
            return 7;
        }
    }
    return 0;
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    return QT_PREPEND_NAMESPACE(runIdc)(argc, argv);
}
