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

#include "preprocessor.h"
#include "moc.h"
#include "outputrevision.h"
#include "../../corelib/global/qconfig.cpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

QT_BEGIN_NAMESPACE

/*
    This function looks at two file names and returns the name of the
    infile with a path relative to outfile.

    Examples:

        /tmp/abc, /tmp/bcd -> abc
        xyz/a/bc, xyz/b/ac -> ../a/bc
        /tmp/abc, xyz/klm -> /tmp/abc
 */

static QByteArray combinePath(const char *infile, const char *outfile)
{
    QFileInfo inFileInfo(QDir::current(), QFile::decodeName(infile));
    QFileInfo outFileInfo(QDir::current(), QFile::decodeName(outfile));
    int numCommonComponents = 0;

    QStringList inSplitted = inFileInfo.dir().canonicalPath().split(QLatin1Char('/'));
    QStringList outSplitted = outFileInfo.dir().canonicalPath().split(QLatin1Char('/'));

    while (!inSplitted.isEmpty() && !outSplitted.isEmpty() &&
            inSplitted.first() == outSplitted.first()) {
        inSplitted.removeFirst();
        outSplitted.removeFirst();
        numCommonComponents++;
    }

    if (numCommonComponents < 2)
        /*
          The paths don't have the same drive, or they don't have the
          same root directory. Use an absolute path.
        */
        return QFile::encodeName(inFileInfo.absoluteFilePath());
    /*
       The paths have something in common. Use a path relative to
       the output file.
     */
    while (!outSplitted.isEmpty()) {
        outSplitted.removeFirst();
        inSplitted.prepend(QLatin1String(".."));
    }
    inSplitted.append(inFileInfo.fileName());
    return QFile::encodeName(inSplitted.join(QLatin1String("/")));
}


void error(const char *msg = "Invalid argument")
{
    if (msg)
        fprintf(stderr, "moc: %s\n", msg);
    fprintf(stderr, "Usage: moc [options] <header-file>\n"
            "  -o<file>           write output to file rather than stdout\n"
            "  -I<dir>            add dir to the include path for header files\n"
            "  -E                 preprocess only; do not generate meta object code\n"
            "  -D<macro>[=<def>]  define macro, with optional definition\n"
            "  -U<macro>          undefine macro\n"
            "  -i                 do not generate an #include statement\n"
            "  -p<path>           path prefix for included file\n"
            "  -f[<file>]         force #include, optional file name\n"
            "  -nn                do not display notes\n"
            "  -nw                do not display warnings\n"
            "  @<file>            read additional options from file\n"
            "  -v                 display version of moc\n");
    exit(1);
}


static inline bool hasNext(const Symbols &symbols, int i)
{ return (i < symbols.size()); }

static inline const Symbol &next(const Symbols &symbols, int &i)
{ return symbols.at(i++); }


QByteArray composePreprocessorOutput(const Symbols &symbols) {
    QByteArray output;
    int lineNum = 1;
    Token last = PP_NOTOKEN;
    Token secondlast = last;
    int i = 0;
    while (hasNext(symbols, i)) {
        Symbol sym = next(symbols, i);
        switch (sym.token) {
        case PP_NEWLINE:
        case PP_WHITESPACE:
            if (last != PP_WHITESPACE) {
                secondlast = last;
                last = PP_WHITESPACE;
                output += ' ';
            }
            continue;
        case PP_STRING_LITERAL:
            if (last == PP_STRING_LITERAL)
                output.chop(1);
            else if (secondlast == PP_STRING_LITERAL && last == PP_WHITESPACE)
                output.chop(2);
            else
                break;
            output += sym.lexem().mid(1);
            secondlast = last;
            last = PP_STRING_LITERAL;
            continue;
        case MOC_INCLUDE_BEGIN:
            lineNum = 0;
            continue;
        case MOC_INCLUDE_END:
            lineNum = sym.lineNum;
            continue;
        default:
            break;
        }
        secondlast = last;
        last = sym.token;

        const int padding = sym.lineNum - lineNum;
        if (padding > 0) {
            output.resize(output.size() + padding);
            qMemSet(output.data() + output.size() - padding, '\n', padding);
            lineNum = sym.lineNum;
        }

        output += sym.lexem();
    }

    return output;
}


int runMoc(int _argc, char **_argv)
{
    bool autoInclude = true;
    Preprocessor pp;
    Moc moc;
    pp.macros["Q_MOC_RUN"];
    pp.macros["__cplusplus"];
    QByteArray filename;
    QByteArray output;
    FILE *in = 0;
    FILE *out = 0;
    bool ignoreConflictingOptions = false;

    QVector<QByteArray> argv;
    argv.resize(_argc - 1);
    for (int n = 1; n < _argc; ++n)
        argv[n - 1] = _argv[n];
    int argc = argv.count();

    for (int n = 0; n < argv.count(); ++n) {
        if (argv.at(n).startsWith('@')) {
            QByteArray optionsFile = argv.at(n);
            optionsFile.remove(0, 1);
            if (optionsFile.isEmpty())
                error("The @ option requires an input file");
            QFile f(QString::fromLatin1(optionsFile.constData()));
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
                error("Cannot open options file specified with @");
            argv.remove(n);
            while (!f.atEnd()) {
                QByteArray line = f.readLine().trimmed();
                if (!line.isEmpty())
                    argv.insert(n++, line);
            }
        }
    }

    argc = argv.count();

    for (int n = 0; n < argc; ++n) {
        QByteArray arg(argv[n]);
        if (arg[0] != '-') {
            if (filename.isEmpty()) {
                filename = arg;
                continue;
            }
            error("Too many input files specified");
        }
        QByteArray opt = arg.mid(1);
        bool more = (opt.size() > 1);
        switch (opt[0]) {
        case 'o': // output redirection
            if (!more) {
                if (!(n < argc-1))
                    error("Missing output file name");
                output = argv[++n];
            } else
                output = opt.mid(1);
            break;
        case 'E': // only preprocessor
            pp.preprocessOnly = true;
            break;
        case 'i': // no #include statement
            if (more)
                error();
            moc.noInclude        = true;
            autoInclude = false;
            break;
        case 'f': // produce #include statement
            if (ignoreConflictingOptions)
                break;
            moc.noInclude        = false;
            autoInclude = false;
            if (opt[1])                        // -fsomething.h
                moc.includeFiles.append(opt.mid(1));
            break;
        case 'p': // include file path
            if (ignoreConflictingOptions)
                break;
            if (!more) {
                if (!(n < argc-1))
                    error("Missing path name for the -p option.");
                moc.includePath = argv[++n];
            } else {
                moc.includePath = opt.mid(1);
            }
            break;
        case 'I': // produce #include statement
            if (!more) {
                if (!(n < argc-1))
                    error("Missing path name for the -I option.");
                pp.includes += Preprocessor::IncludePath(argv[++n]);
            } else {
                pp.includes += Preprocessor::IncludePath(opt.mid(1));
            }
            break;
        case 'F': // minimalistic framework support for the mac
            if (!more) {
                if (!(n < argc-1))
                    error("Missing path name for the -F option.");
                Preprocessor::IncludePath p(argv[++n]);
                p.isFrameworkPath = true;
                pp.includes += p;
            } else {
                Preprocessor::IncludePath p(opt.mid(1));
                p.isFrameworkPath = true;
                pp.includes += p;
            }
            break;
        case 'D': // define macro
            {
                QByteArray name;
                QByteArray value("1");
                if (!more) {
                    if (n < argc-1)
                        name = argv[++n];
                } else
                    name = opt.mid(1);
                int eq = name.indexOf('=');
                if (eq >= 0) {
                    value = name.mid(eq + 1);
                    name = name.left(eq);
                }
                if (name.isEmpty())
                    error("Missing macro name");
                Macro macro;
                macro.symbols += Symbol(0, PP_IDENTIFIER, value);
                pp.macros.insert(name, macro);

            }
            break;
        case 'U':
            {
                QByteArray macro;
                if (!more) {
                    if (n < argc-1)
                        macro = argv[++n];
                } else
                    macro = opt.mid(1);
                if (macro.isEmpty())
                    error("Missing macro name");
                pp.macros.remove(macro);

            }
            break;
        case 'v':  // version number
            if (more && opt != "version")
                error();
            fprintf(stderr, "Qt Meta Object Compiler version %d (Qt %s)\n",
                    mocOutputRevision, QT_VERSION_STR);
            return 1;
        case 'n': // don't display warnings
            if (ignoreConflictingOptions)
                break;
            if (opt == "nw")
                moc.displayWarnings = moc.displayNotes = false;
            else if (opt == "nn")
                moc.displayNotes = false;
            else
                error();
            break;
        case 'h': // help
            if (more && opt != "help")
                error();
            else
                error(0); // 0 means usage only
            break;
        case '-':
            if (more && arg == "--ignore-option-clashes") {
                // -- ignore all following moc specific options that conflict
                // with for example gcc, like -pthread conflicting with moc's
                // -p option.
                ignoreConflictingOptions = true;
                break;
            }
            // fall through
        default:
            error();
        }
    }


    if (autoInclude) {
        int spos = filename.lastIndexOf(QDir::separator().toLatin1());
        int ppos = filename.lastIndexOf('.');
        // spos >= -1 && ppos > spos => ppos >= 0
        moc.noInclude = (ppos > spos && tolower(filename[ppos + 1]) != 'h');
    }
    if (moc.includeFiles.isEmpty()) {
        if (moc.includePath.isEmpty()) {
            if (filename.size()) {
                if (output.size())
                    moc.includeFiles.append(combinePath(filename, output));
                else
                    moc.includeFiles.append(filename);
            }
        } else {
            moc.includeFiles.append(combinePath(filename, filename));
        }
    }

    if (filename.isEmpty()) {
        filename = "standard input";
        in = stdin;
    } else {
#if defined(_MSC_VER) && _MSC_VER >= 1400
		if (fopen_s(&in, filename.data(), "rb")) {
#else
        in = fopen(filename.data(), "rb");
		if (!in) {
#endif
            fprintf(stderr, "moc: %s: No such file\n", (const char*)filename);
            return 1;
        }
        moc.filename = filename;
    }

    moc.currentFilenames.push(filename);

    // 1. preprocess
    moc.symbols = pp.preprocessed(moc.filename, in);
    fclose(in);

    if (!pp.preprocessOnly) {
        // 2. parse
        moc.parse();
    }

    // 3. and output meta object code

    if (output.size()) { // output file specified
#if defined(_MSC_VER) && _MSC_VER >= 1400
        if (fopen_s(&out, output.data(), "w"))
#else
        out = fopen(output.data(), "w"); // create output file
        if (!out)
#endif
        {
            fprintf(stderr, "moc: Cannot create %s\n", (const char*)output);
            return 1;
        }
    } else { // use stdout
        out = stdout;
    }

    if (pp.preprocessOnly) {
        fprintf(out, "%s\n", composePreprocessorOutput(moc.symbols).constData());
    } else {
        if (moc.classList.isEmpty())
            moc.note("No relevant classes found. No output generated.");
        else
            moc.generate(out);
    }

    if (output.size())
        fclose(out);

    return 0;
}

QT_END_NAMESPACE

int main(int _argc, char **_argv)
{
    return QT_PREPEND_NAMESPACE(runMoc)(_argc, _argv);
}
