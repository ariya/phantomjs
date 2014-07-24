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

#include "preprocessor.h"
#include "moc.h"
#include "outputrevision.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <qcoreapplication.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

QT_BEGIN_NAMESPACE

/*
    This function looks at two file names and returns the name of the
    infile with a path relative to outfile.

    Examples:

        /tmp/abc, /tmp/bcd -> abc
        xyz/a/bc, xyz/b/ac -> ../a/bc
        /tmp/abc, xyz/klm -> /tmp/abc
 */

static QByteArray combinePath(const QString &infile, const QString &outfile)
{
    QFileInfo inFileInfo(QDir::current(), infile);
    QFileInfo outFileInfo(QDir::current(), outfile);
    const QByteArray relativePath = QFile::encodeName(outFileInfo.dir().relativeFilePath(inFileInfo.filePath()));
#ifdef Q_OS_WIN
    // It's a system limitation.
    // It depends on the Win API function which is used by the program to open files.
    // cl apparently uses the functions that have the MAX_PATH limitation.
    if (outFileInfo.dir().absolutePath().length() + relativePath.length() + 1 >= 260)
        return QFile::encodeName(inFileInfo.absoluteFilePath());
#endif
    return relativePath;
}


void error(const char *msg = "Invalid argument")
{
    if (msg)
        fprintf(stderr, "moc: %s\n", msg);
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
            memset(output.data() + output.size() - padding, '\n', padding);
            lineNum = sym.lineNum;
        }

        output += sym.lexem();
    }

    return output;
}

static QStringList argumentsFromCommandLineAndFile(const QStringList &arguments)
{
    QStringList allArguments;
    allArguments.reserve(arguments.size());
    foreach (const QString &argument, arguments) {
        // "@file" doesn't start with a '-' so we can't use QCommandLineParser for it
        if (argument.startsWith(QLatin1Char('@'))) {
            QString optionsFile = argument;
            optionsFile.remove(0, 1);
            if (optionsFile.isEmpty()) {
                error("The @ option requires an input file");
                return QStringList();
            }
            QFile f(optionsFile);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                error("Cannot open options file specified with @");
                return QStringList();
            }
            while (!f.atEnd()) {
                QString line = QString::fromLocal8Bit(f.readLine().trimmed());
                if (!line.isEmpty())
                    allArguments << line;
            }
        } else {
            allArguments << argument;
        }
    }
    return allArguments;
}


int runMoc(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QString::fromLatin1(QT_VERSION_STR));

    bool autoInclude = true;
    bool defaultInclude = true;
    Preprocessor pp;
    Moc moc;
    pp.macros["Q_MOC_RUN"];
    pp.macros["__cplusplus"];

    // Don't stumble over GCC extensions
    Macro dummyVariadicFunctionMacro;
    dummyVariadicFunctionMacro.isFunction = true;
    dummyVariadicFunctionMacro.isVariadic = true;
    dummyVariadicFunctionMacro.arguments += Symbol(0, PP_IDENTIFIER, "__VA_ARGS__");
    pp.macros["__attribute__"] = dummyVariadicFunctionMacro;
    pp.macros["__declspec"] = dummyVariadicFunctionMacro;

    QString filename;
    QString output;
    QFile in;
    FILE *out = 0;

    // Note that moc isn't translated.
    // If you use this code as an example for a translated app, make sure to translate the strings.
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt Meta Object Compiler version %1 (Qt %2)")
                                     .arg(mocOutputRevision).arg(QString::fromLatin1(QT_VERSION_STR)));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption outputOption(QStringLiteral("o"));
    outputOption.setDescription(QStringLiteral("Write output to file rather than stdout."));
    outputOption.setValueName(QStringLiteral("file"));
    parser.addOption(outputOption);

    QCommandLineOption includePathOption(QStringLiteral("I"));
    includePathOption.setDescription(QStringLiteral("Add dir to the include path for header files."));
    includePathOption.setValueName(QStringLiteral("dir"));
    parser.addOption(includePathOption);

    QCommandLineOption macFrameworkOption(QStringLiteral("F"));
    macFrameworkOption.setDescription(QStringLiteral("Add Mac framework to the include path for header files."));
    macFrameworkOption.setValueName(QStringLiteral("framework"));
    parser.addOption(macFrameworkOption);

    QCommandLineOption preprocessOption(QStringLiteral("E"));
    preprocessOption.setDescription(QStringLiteral("Preprocess only; do not generate meta object code."));
    parser.addOption(preprocessOption);

    QCommandLineOption defineOption(QStringLiteral("D"));
    defineOption.setDescription(QStringLiteral("Define macro, with optional definition."));
    defineOption.setValueName(QStringLiteral("macro[=def]"));
    parser.addOption(defineOption);

    QCommandLineOption undefineOption(QStringLiteral("U"));
    undefineOption.setDescription(QStringLiteral("Undefine macro."));
    undefineOption.setValueName(QStringLiteral("macro"));
    parser.addOption(undefineOption);

    QCommandLineOption metadataOption(QStringLiteral("M"));
    metadataOption.setDescription(QStringLiteral("Add key/value pair to plugin meta data"));
    metadataOption.setValueName(QStringLiteral("key=value"));
    parser.addOption(metadataOption);

    QCommandLineOption noIncludeOption(QStringLiteral("i"));
    noIncludeOption.setDescription(QStringLiteral("Do not generate an #include statement."));
    parser.addOption(noIncludeOption);

    QCommandLineOption pathPrefixOption(QStringLiteral("p"));
    pathPrefixOption.setDescription(QStringLiteral("Path prefix for included file."));
    pathPrefixOption.setValueName(QStringLiteral("path"));
    parser.addOption(pathPrefixOption);

    QCommandLineOption forceIncludeOption(QStringLiteral("f"));
    forceIncludeOption.setDescription(QStringLiteral("Force #include <file> (overwrite default)."));
    forceIncludeOption.setValueName(QStringLiteral("file"));
    parser.addOption(forceIncludeOption);

    QCommandLineOption prependIncludeOption(QStringLiteral("b"));
    prependIncludeOption.setDescription(QStringLiteral("Prepend #include <file> (preserve default include)."));
    prependIncludeOption.setValueName(QStringLiteral("file"));
    parser.addOption(prependIncludeOption);

    QCommandLineOption noNotesWarningsCompatOption(QStringLiteral("n"));
    noNotesWarningsCompatOption.setDescription(QStringLiteral("Do not display notes (-nn) or warnings (-nw). Compatibility option."));
    noNotesWarningsCompatOption.setValueName(QStringLiteral("which"));
    parser.addOption(noNotesWarningsCompatOption);

    QCommandLineOption noNotesOption(QStringLiteral("no-notes"));
    noNotesOption.setDescription(QStringLiteral("Do not display notes."));
    parser.addOption(noNotesOption);

    QCommandLineOption noWarningsOption(QStringLiteral("no-warnings"));
    noWarningsOption.setDescription(QStringLiteral("Do not display warnings (implies --no-notes)."));
    parser.addOption(noWarningsOption);

    QCommandLineOption ignoreConflictsOption(QStringLiteral("ignore-option-clashes"));
    ignoreConflictsOption.setDescription(QStringLiteral("Ignore all options that conflict with compilers, like -pthread conflicting with moc's -p option."));
    parser.addOption(ignoreConflictsOption);

    parser.addPositionalArgument(QStringLiteral("[header-file]"),
            QStringLiteral("Header file to read from, otherwise stdin."));
    parser.addPositionalArgument(QStringLiteral("[@option-file]"),
            QStringLiteral("Read additional options from option-file."));

    const QStringList arguments = argumentsFromCommandLineAndFile(app.arguments());

    parser.process(arguments);

    const QStringList files = parser.positionalArguments();
    if (files.count() > 1) {
        error(qPrintable(QStringLiteral("Too many input files specified: '") + files.join(QStringLiteral("' '")) + QLatin1Char('\'')));
        parser.showHelp(1);
    } else if (!files.isEmpty()) {
        filename = files.first();
    }

    const bool ignoreConflictingOptions = parser.isSet(ignoreConflictsOption);
    output = parser.value(outputOption);
    pp.preprocessOnly = parser.isSet(preprocessOption);
    if (parser.isSet(noIncludeOption)) {
        moc.noInclude = true;
        autoInclude = false;
    }
    if (!ignoreConflictingOptions) {
        if (parser.isSet(forceIncludeOption)) {
            moc.noInclude = false;
            autoInclude = false;
            foreach (const QString &include, parser.values(forceIncludeOption)) {
                moc.includeFiles.append(QFile::encodeName(include));
                defaultInclude = false;
             }
        }
        foreach (const QString &include, parser.values(prependIncludeOption))
            moc.includeFiles.prepend(QFile::encodeName(include));
        if (parser.isSet(pathPrefixOption))
            moc.includePath = QFile::encodeName(parser.value(pathPrefixOption));
    }
    foreach (const QString &path, parser.values(includePathOption))
        pp.includes += Preprocessor::IncludePath(QFile::encodeName(path));
    foreach (const QString &path, parser.values(macFrameworkOption)) {
        // minimalistic framework support for the mac
        Preprocessor::IncludePath p(QFile::encodeName(path));
        p.isFrameworkPath = true;
        pp.includes += p;
    }
    foreach (const QString &arg, parser.values(defineOption)) {
        QByteArray name = arg.toLocal8Bit();
        QByteArray value("1");
        int eq = name.indexOf('=');
        if (eq >= 0) {
            value = name.mid(eq + 1);
            name = name.left(eq);
        }
        if (name.isEmpty()) {
            error("Missing macro name");
            parser.showHelp(1);
        }
        Macro macro;
        macro.symbols = Preprocessor::tokenize(value, 1, Preprocessor::TokenizeDefine);
        macro.symbols.removeLast(); // remove the EOF symbol
        pp.macros.insert(name, macro);
    }
    foreach (const QString &arg, parser.values(undefineOption)) {
        QByteArray macro = arg.toLocal8Bit();
        if (macro.isEmpty()) {
            error("Missing macro name");
            parser.showHelp(1);
        }
        pp.macros.remove(macro);
    }
    const QStringList noNotesCompatValues = parser.values(noNotesWarningsCompatOption);
    if (parser.isSet(noNotesOption) || noNotesCompatValues.contains(QStringLiteral("n")))
        moc.displayNotes = false;
    if (parser.isSet(noWarningsOption) || noNotesCompatValues.contains(QStringLiteral("w")))
        moc.displayWarnings = moc.displayNotes = false;

    if (autoInclude) {
        int spos = filename.lastIndexOf(QDir::separator());
        int ppos = filename.lastIndexOf(QLatin1Char('.'));
        // spos >= -1 && ppos > spos => ppos >= 0
        moc.noInclude = (ppos > spos && filename[ppos + 1].toLower() != QLatin1Char('h'));
    }
    if (defaultInclude) {
        if (moc.includePath.isEmpty()) {
            if (filename.size()) {
                if (output.size())
                    moc.includeFiles.append(combinePath(filename, output));
                else
                    moc.includeFiles.append(QFile::encodeName(filename));
            }
        } else {
            moc.includeFiles.append(combinePath(filename, filename));
        }
    }

    if (filename.isEmpty()) {
        filename = QStringLiteral("standard input");
        in.open(stdin, QIODevice::ReadOnly);
    } else {
        in.setFileName(filename);
        if (!in.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "moc: %s: No such file\n", qPrintable(filename));
            return 1;
        }
        moc.filename = filename.toLocal8Bit();
    }

    foreach (const QString &md, parser.values(metadataOption)) {
        int split = md.indexOf(QLatin1Char('='));
        QString key = md.left(split);
        QString value = md.mid(split + 1);

        if (split == -1 || key.isEmpty() || value.isEmpty()) {
            error("missing key or value for option '-M'");
        } else if (key.indexOf(QLatin1Char('.')) != -1) {
            // Don't allow keys with '.' for now, since we might need this
            // format later for more advanced meta data API
            error("A key cannot contain the letter '.' for option '-M'");
        } else {
            QJsonArray array = moc.metaArgs.value(key);
            array.append(value);
            moc.metaArgs.insert(key, array);
        }
    }

    moc.currentFilenames.push(filename.toLocal8Bit());
    moc.includes = pp.includes;

    // 1. preprocess
    moc.symbols = pp.preprocessed(moc.filename, &in);

    if (!pp.preprocessOnly) {
        // 2. parse
        moc.parse();
    }

    // 3. and output meta object code

    if (output.size()) { // output file specified
#if defined(_MSC_VER) && _MSC_VER >= 1400
        if (fopen_s(&out, QFile::encodeName(output).constData(), "w"))
#else
        out = fopen(QFile::encodeName(output).constData(), "w"); // create output file
        if (!out)
#endif
        {
            fprintf(stderr, "moc: Cannot create %s\n", QFile::encodeName(output).constData());
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
