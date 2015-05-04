/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <rcc.h>

#include <qdebug.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qatomic.h>
#include <qglobal.h>
#include <qcoreapplication.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

#ifdef Q_OS_WIN
#  include <fcntl.h>
#  include <io.h>
#  include <stdio.h>
#endif // Q_OS_WIN

QT_BEGIN_NAMESPACE

void dumpRecursive(const QDir &dir, QTextStream &out)
{
    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot
                                              | QDir::NoSymLinks);
    foreach (const QFileInfo &entry, entries) {
        if (entry.isDir()) {
            dumpRecursive(entry.filePath(), out);
        } else {
            out << QLatin1String("<file>")
                << entry.filePath()
                << QLatin1String("</file>\n");
        }
    }
}

int createProject(const QString &outFileName)
{
    QDir currentDir = QDir::current();
    QString currentDirName = currentDir.dirName();
    if (currentDirName.isEmpty())
        currentDirName = QLatin1String("root");

    QFile file;
    bool isOk = false;
    if (outFileName.isEmpty()) {
        isOk = file.open(stdout, QFile::WriteOnly | QFile::Text);
    } else {
        file.setFileName(outFileName);
        isOk = file.open(QFile::WriteOnly | QFile::Text);
    }
    if (!isOk) {
        fprintf(stderr, "Unable to open %s: %s\n",
                outFileName.isEmpty() ? qPrintable(outFileName) : "standard output",
                qPrintable(file.errorString()));
        return 1;
    }

    QTextStream out(&file);
    out << QLatin1String("<!DOCTYPE RCC><RCC version=\"1.0\">\n"
                         "<qresource>\n");

    // use "." as dir to get relative file pathes
    dumpRecursive(QDir(QLatin1String(".")), out);

    out << QLatin1String("</qresource>\n"
                         "</RCC>\n");

    return 0;
}

int runRcc(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QStringLiteral(QT_VERSION_STR));

    // Note that rcc isn't translated.
    // If you use this code as an example for a translated app, make sure to translate the strings.
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(QLatin1String("Qt Resource Compiler version " QT_VERSION_STR));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption outputOption(QStringList() << QStringLiteral("o") << QStringLiteral("output"));
    outputOption.setDescription(QStringLiteral("Write output to <file> rather than stdout."));
    outputOption.setValueName(QStringLiteral("file"));
    parser.addOption(outputOption);

    QCommandLineOption tempOption(QStringList() << QStringLiteral("t") << QStringLiteral("temp"));
    tempOption.setDescription(QStringLiteral("Use temporary <file> for big resources."));
    tempOption.setValueName(QStringLiteral("file"));
    parser.addOption(tempOption);

    QCommandLineOption nameOption(QStringLiteral("name"), QStringLiteral("Create an external initialization function with <name>."), QStringLiteral("name"));
    parser.addOption(nameOption);

    QCommandLineOption rootOption(QStringLiteral("root"), QStringLiteral("Prefix resource access path with root path."), QStringLiteral("path"));
    parser.addOption(rootOption);

    QCommandLineOption compressOption(QStringLiteral("compress"), QStringLiteral("Compress input files by <level>."), QStringLiteral("level"));
    parser.addOption(compressOption);

    QCommandLineOption nocompressOption(QStringLiteral("no-compress"), QStringLiteral("Disable all compression."));
    parser.addOption(nocompressOption);

    QCommandLineOption thresholdOption(QStringLiteral("threshold"), QStringLiteral("Threshold to consider compressing files."), QStringLiteral("level"));
    parser.addOption(thresholdOption);

    QCommandLineOption binaryOption(QStringLiteral("binary"), QStringLiteral("Output a binary file for use as a dynamic resource."));
    parser.addOption(binaryOption);

    QCommandLineOption passOption(QStringLiteral("pass"), QStringLiteral("Pass number for big resources"), QStringLiteral("number"));
    parser.addOption(passOption);

    QCommandLineOption namespaceOption(QStringLiteral("namespace"), QStringLiteral("Turn off namespace macros."));
    parser.addOption(namespaceOption);

    QCommandLineOption verboseOption(QStringLiteral("verbose"), QStringLiteral("Enable verbose mode."));
    parser.addOption(verboseOption);

    QCommandLineOption listOption(QStringLiteral("list"), QStringLiteral("Only list .qrc file entries, do not generate code."));
    parser.addOption(listOption);

    QCommandLineOption projectOption(QStringLiteral("project"), QStringLiteral("Output a resource file containing all files from the current directory."));
    parser.addOption(projectOption);

    parser.addPositionalArgument(QStringLiteral("inputs"), QStringLiteral("Input files (*.qrc)."));


    //parse options
    parser.process(app);

    QString errorMsg;
    RCCResourceLibrary library;
    if (parser.isSet(nameOption))
        library.setInitName(parser.value(nameOption));
    if (parser.isSet(rootOption)) {
        library.setResourceRoot(QDir::cleanPath(parser.value(rootOption)));
        if (library.resourceRoot().isEmpty()
                || library.resourceRoot().at(0) != QLatin1Char('/'))
            errorMsg = QLatin1String("Root must start with a /");
    }
    if (parser.isSet(compressOption))
        library.setCompressLevel(parser.value(compressOption).toInt());
    if (parser.isSet(nocompressOption))
        library.setCompressLevel(-2);
    if (parser.isSet(thresholdOption))
        library.setCompressThreshold(parser.value(thresholdOption).toInt());
    if (parser.isSet(binaryOption))
        library.setFormat(RCCResourceLibrary::Binary);
    if (parser.isSet(passOption)) {
        if (parser.value(passOption) == QLatin1String("1"))
            library.setFormat(RCCResourceLibrary::Pass1);
        else if (parser.value(passOption) == QLatin1String("2"))
            library.setFormat(RCCResourceLibrary::Pass2);
        else
            errorMsg = QLatin1String("Pass number must be 1 or 2");
    }
    if (parser.isSet(namespaceOption))
        library.setUseNameSpace(!library.useNameSpace());
    if (parser.isSet(verboseOption))
        library.setVerbose(true);

    const bool list = parser.isSet(listOption);
    const bool projectRequested = parser.isSet(projectOption);
    const QStringList filenamesIn = parser.positionalArguments();

    foreach (const QString &file, filenamesIn) {
        if (!QFile::exists(file)) {
            qWarning("%s: File does not exist '%s'", argv[0], qPrintable(file));
            return 1;
        }
    }

    QString outFilename = parser.value(outputOption);
    QString tempFilename = parser.value(tempOption);

    if (projectRequested) {
        return createProject(outFilename);
    }

    if (filenamesIn.isEmpty())
        errorMsg = QStringLiteral("No input files specified.");

    if (!errorMsg.isEmpty()) {
        fprintf(stderr, "%s: %s\n", argv[0], qPrintable(errorMsg));
        parser.showHelp(1);
        return 1;
    }
    QFile errorDevice;
    errorDevice.open(stderr, QIODevice::WriteOnly|QIODevice::Text);

    if (library.verbose())
        errorDevice.write("Qt resource compiler\n");

    library.setInputFiles(filenamesIn);

    if (!library.readFiles(list, errorDevice))
        return 1;

    QFile out;

    // open output
    QIODevice::OpenMode mode = QIODevice::NotOpen;
    switch (library.format()) {
        case RCCResourceLibrary::C_Code:
        case RCCResourceLibrary::Pass1:
            mode = QIODevice::WriteOnly | QIODevice::Text;
            break;
        case RCCResourceLibrary::Pass2:
        case RCCResourceLibrary::Binary:
            mode = QIODevice::WriteOnly;
            break;
    }


    if (outFilename.isEmpty() || outFilename == QLatin1String("-")) {
#ifdef Q_OS_WIN
        // Make sure fwrite to stdout doesn't do LF->CRLF
        if (library.format() == RCCResourceLibrary::Binary)
            _setmode(_fileno(stdout), _O_BINARY);
#endif // Q_OS_WIN
        // using this overload close() only flushes.
        out.open(stdout, mode);
    } else {
        out.setFileName(outFilename);
        if (!out.open(mode)) {
            const QString msg = QString::fromLatin1("Unable to open %1 for writing: %2\n").arg(outFilename).arg(out.errorString());
            errorDevice.write(msg.toUtf8());
            return 1;
        }
    }

    // do the task
    if (list) {
        const QStringList data = library.dataFiles();
        for (int i = 0; i < data.size(); ++i) {
            out.write(qPrintable(QDir::cleanPath(data.at(i))));
            out.write("\n");
        }
        return 0;
    }

    QFile temp;
    if (!tempFilename.isEmpty()) {
        temp.setFileName(tempFilename);
        if (!temp.open(QIODevice::ReadOnly)) {
            const QString msg = QString::fromUtf8("Unable to open temporary file %1 for reading: %2\n")
                    .arg(outFilename).arg(out.errorString());
            errorDevice.write(msg.toUtf8());
            return 1;
        }
    }
    bool success = library.output(out, temp, errorDevice);
    if (!success) {
        // erase the output file if we failed
        out.remove();
        return 1;
    }
    return 0;
}

Q_CORE_EXPORT extern QBasicAtomicInt qt_qhash_seed; // from qhash.cpp

QT_END_NAMESPACE

int main(int argc, char *argv[])
{
    // rcc uses a QHash to store files in the resource system.
    // we must force a certain hash order when testing or tst_rcc will fail, see QTBUG-25078
    if (!qEnvironmentVariableIsEmpty("QT_RCC_TEST") && !qt_qhash_seed.testAndSetRelaxed(-1, 0))
        qFatal("Cannot force QHash seed for testing as requested");

    return QT_PREPEND_NAMESPACE(runRcc)(argc, argv);
}
