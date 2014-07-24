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

#include <qglobal.h>
#include <stdlib.h>
#include "codemarker.h"
#include "codeparser.h"
#include "config.h"
#include "cppcodemarker.h"
#include "cppcodeparser.h"
#include "ditaxmlgenerator.h"
#include "doc.h"
#include "htmlgenerator.h"
#include "plaincodemarker.h"
#include "puredocparser.h"
#include "tokenizer.h"
#include "tree.h"
#include "qdocdatabase.h"
#include "jscodemarker.h"
#include "qmlcodemarker.h"
#include "qmlcodeparser.h"
#include <qdatetime.h>
#include <qdebug.h>
#include "qtranslator.h"
#ifndef QT_BOOTSTRAPPED
#  include "qcoreapplication.h"
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE


bool creationTimeBefore(const QFileInfo &fi1, const QFileInfo &fi2)
{
    return fi1.lastModified() < fi2.lastModified();
}

static bool highlighting = false;
static bool showInternal = false;
static bool redirectDocumentationToDevNull = false;
static bool noLinkErrors = false;
static bool obsoleteLinks = false;
static QStringList defines;
static QStringList dependModules;
static QStringList indexDirs;
static QString currentDir;
static QString prevCurrentDir;
static QString documentationPath;

/*!
  Print the help message to \c stdout.
 */
static void printHelp()
{
    Location::information(QCoreApplication::translate("QDoc", "Usage: qdoc [options] file1.qdocconf ...\n"
                             "Options:\n"
                             "    -D<name>       "
                             "Define <name> as a macro while parsing sources\n"
                             "    -depends       "
                             "Specify dependent modules\n"
                             "    -help          "
                             "Display this information and exit\n"
                             "    -highlighting  "
                             "Turn on syntax highlighting (makes qdoc run slower)\n"
                             "    -indexdir      "
                             "Specify a directory where QDoc should search for index files to load\n"
                             "    -installdir    "
                             "Specify the directory where the output will be after running \"make install\"\n"
                             "    -no-examples   "
                             "Do not generate documentation for examples\n"
                             "    -no-link-errors   "
                             "Do not print link errors (i.e. missing targets)\n"
                             "    -obsoletelinks "
                             "Report links from obsolete items to non-obsolete items\n"
                             "    -outputdir     "
                             "Specify output directory, overrides setting in qdocconf file\n"
                             "    -outputformat  "
                             "Specify output format, overrides setting in qdocconf file\n"
                             "    -prepare        "
                             "Run qdoc only to generate an index file, not the docs\n"
                             "    -generate        "
                             "Run qdoc to read the index files and generate the docs\n"
                             "    -showinternal  "
                             "Include content marked internal\n"
                             "    -redirect-documentation-to-dev-null "
                             "Save all documentation content to /dev/null. Useful if someone is interested in qdoc errors only.\n"
                             "    -version       "
                             "Display version of qdoc and exit\n") );
}

/*!
  Prints the qdoc version number to stdout.
 */
static void printVersion()
{
    QString s = QCoreApplication::translate("QDoc", "qdoc version %1").arg(QT_VERSION_STR);
    Location::information(s);
}

static void loadIndexFiles(Config& config)
{
    QDocDatabase* qdb = QDocDatabase::qdocDB();
    /*
      Read some XML indexes containing definitions from other documentation sets.
     */
    QStringList indexFiles = config.getStringList(CONFIG_INDEXES);

    dependModules += config.getStringList(CONFIG_DEPENDS);

    bool noOutputSubdirs = false;
    QString singleOutputSubdir;
    if (config.getBool(QString("HTML.nosubdirs"))) {
        noOutputSubdirs = true;
        singleOutputSubdir = config.getString("HTML.outputsubdir");
        if (singleOutputSubdir.isEmpty())
            singleOutputSubdir = "html";
    }

    // Allow modules and third-party application/libraries to link
    // to the Qt docs without having to explicitly pass --indexdir.
    if (!indexDirs.contains(documentationPath))
        indexDirs.append(documentationPath);

    if (dependModules.size() > 0) {
        if (indexDirs.size() > 0) {
            for (int i = 0; i < indexDirs.size(); i++) {
                if (indexDirs[i].startsWith("..")) {
                    const QString prefix(QDir(currentDir).relativeFilePath(prevCurrentDir));
                    if (!prefix.isEmpty())
                        indexDirs[i].prepend(prefix + QLatin1Char('/'));
                }
            }
            /*
              Add all subdirectories of the indexdirs as dependModules,
              when an asterisk is used in the 'depends' list.
            */
            if (dependModules.contains("*")) {
                dependModules.removeOne("*");
                for (int i = 0; i < indexDirs.size(); i++) {
                    QDir scanDir = QDir(indexDirs[i]);
                    scanDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
                    QFileInfoList dirList = scanDir.entryInfoList();
                    for (int j = 0; j < dirList.size(); j++) {
                        if (dirList[j].fileName().toLower() != config.getString(CONFIG_PROJECT).toLower())
                            dependModules.append(dirList[j].fileName());
                    }
                }
            }
            for (int i = 0; i < dependModules.size(); i++) {
                QString indexToAdd;
                QList<QFileInfo> foundIndices;
                for (int j = 0; j < indexDirs.size(); j++) {
                    QString fileToLookFor = indexDirs[j] + QLatin1Char('/');
                    if (noOutputSubdirs)
                        fileToLookFor += singleOutputSubdir + QLatin1Char('/');
                    else
                        fileToLookFor += dependModules[i] + QLatin1Char('/');
                    fileToLookFor += dependModules[i] + QLatin1String(".index");
                    if (QFile::exists(fileToLookFor)) {
                        QFileInfo tempFileInfo(fileToLookFor);
                        if (!foundIndices.contains(tempFileInfo))
                            foundIndices.append(tempFileInfo);
                    }
                }
                std::sort(foundIndices.begin(), foundIndices.end(), creationTimeBefore);
                if (foundIndices.size() > 1) {
                    /*
                        QDoc should always use the last entry in the multimap when there are
                        multiple index files for a module, since the last modified file has the
                        highest UNIX timestamp.
                    */
                    qDebug() << "Multiple indices found for dependency:" << dependModules[i] << "\nFound:";
                    for (int k = 0; k < foundIndices.size(); k++)
                        qDebug() << foundIndices[k].absoluteFilePath();
                    qDebug() << "Using" << foundIndices[foundIndices.size() - 1].absoluteFilePath()
                            << "as index for" << dependModules[i];
                    indexToAdd = foundIndices[foundIndices.size() - 1].absoluteFilePath();
                }
                else if (foundIndices.size() == 1) {
                    indexToAdd = foundIndices[0].absoluteFilePath();
                }
                if (!indexToAdd.isEmpty()) {
                    if (!indexFiles.contains(indexToAdd))
                        indexFiles << indexToAdd;
                }
                else if (Generator::runGenerateOnly()) {
                    qDebug() << "warning:" << config.getString(CONFIG_PROJECT)
                             << "Cannot locate index file for dependency"
                             << dependModules[i];
                }
            }
        }
        else {
            qDebug() << "Dependant modules specified, but no index directories or "
                     << "install directory were set."
                     << "There will probably be errors for missing links.";
        }
    }
    qdb->readIndexes(indexFiles);
}

/*!
  Processes the qdoc config file \a fileName. This is the
  controller for all of qdoc.
 */
static void processQdocconfFile(const QString &fileName)
{
#ifndef QT_NO_TRANSLATION
    QList<QTranslator *> translators;
#endif

    /*
      The Config instance represents the configuration data for qdoc.
      All the other classes are initialized with the config. Here we
      initialize the configuration with some default values.
     */
    Config config(QCoreApplication::translate("QDoc", "qdoc"));

    /*
      The default indent for code is 4.
      The default value for false is 0.
      The default supported file extensions are cpp, h, qdoc and qml.
      The default language is c++.
      The default output format is html.
      The default tab size is 8.
      And those are all the default values for configuration variables.
     */
    static QHash<QString,QString> defaults;
    if (defaults.isEmpty()) {
        defaults.insert(CONFIG_CODEINDENT, QLatin1String("4"));
        defaults.insert(CONFIG_FALSEHOODS, QLatin1String("0"));
        defaults.insert(CONFIG_FILEEXTENSIONS, QLatin1String("*.cpp *.h *.qdoc *.qml"));
        defaults.insert(CONFIG_LANGUAGE, QLatin1String("Cpp"));
        defaults.insert(CONFIG_OUTPUTFORMATS, QLatin1String("HTML"));
        defaults.insert(CONFIG_TABSIZE, QLatin1String("8"));
    }

    QHash<QString,QString>::iterator iter;
    for (iter = defaults.begin(); iter != defaults.end(); ++iter)
        config.setStringList(iter.key(), QStringList() << iter.value());

    config.setStringList(CONFIG_SYNTAXHIGHLIGHTING, QStringList(highlighting ? "true" : "false"));
    config.setStringList(CONFIG_SHOWINTERNAL, QStringList(showInternal ? "true" : "false"));
    config.setStringList(CONFIG_REDIRECTDOCUMENTATIONTODEVNULL, QStringList(redirectDocumentationToDevNull ? "true" : "false"));
    config.setStringList(CONFIG_NOLINKERRORS, QStringList(noLinkErrors ? "true" : "false"));
    config.setStringList(CONFIG_OBSOLETELINKS, QStringList(obsoleteLinks ? "true" : "false"));

    /*
      With the default configuration values in place, load
      the qdoc configuration file. Note that the configuration
      file may include other configuration files.

      The Location class keeps track of the current location
      in the file being processed, mainly for error reporting
      purposes.
     */
    currentDir = QFileInfo(fileName).path();
    Location::initialize(config);
    config.load(fileName);

    /*
      Add the defines to the configuration variables.
     */
    QStringList defs = defines + config.getStringList(CONFIG_DEFINES);
    config.setStringList(CONFIG_DEFINES,defs);
    Location::terminate();

    prevCurrentDir = QDir::currentPath();
    currentDir = QFileInfo(fileName).path();
    if (!currentDir.isEmpty())
        QDir::setCurrent(currentDir);

    QString phase;
    if (Generator::runPrepareOnly())
        phase = " in -prepare mode ";
    else if (Generator::runGenerateOnly())
        phase = " in -generate mode ";
    QString msg = "Running qdoc for " + config.getString(CONFIG_PROJECT) + phase;
    Location::logToStdErr(msg);

    /*
      Initialize all the classes and data structures with the
      qdoc configuration.
     */
    Location::initialize(config);
    Tokenizer::initialize(config);
    Doc::initialize(config);
    CodeMarker::initialize(config);
    CodeParser::initialize(config);
    Generator::initialize(config);

#ifndef QT_NO_TRANSLATION
    /*
      Load the language translators, if the configuration specifies any.
     */
    QStringList fileNames = config.getStringList(CONFIG_TRANSLATORS);
    QStringList::ConstIterator fn = fileNames.constBegin();
    while (fn != fileNames.constEnd()) {
        QTranslator *translator = new QTranslator(0);
        if (!translator->load(*fn))
            config.lastLocation().error(QCoreApplication::translate("QDoc", "Cannot load translator '%1'").arg(*fn));
        QCoreApplication::instance()->installTranslator(translator);
        translators.append(translator);
        ++fn;
    }
#endif

    //QSet<QString> outputLanguages = config.getStringSet(CONFIG_OUTPUTLANGUAGES);

    /*
      Get the source language (Cpp) from the configuration
      and the location in the configuration file where the
      source language was set.
     */
    QString lang = config.getString(CONFIG_LANGUAGE);
    Location langLocation = config.lastLocation();

    /*
      Initialize the qdoc database, where all the parsed source files
      will be stored. The database includes a tree of nodes, which gets
      built as the source files are parsed. The documentation output is
      generated by traversing that tree.
     */
    QDocDatabase* qdb = QDocDatabase::qdocDB();
    qdb->setVersion(config.getString(CONFIG_VERSION));
    qdb->setShowInternal(config.getBool(CONFIG_SHOWINTERNAL));
    /*
      By default, the only output format is HTML.
     */
    QSet<QString> outputFormats = config.getOutputFormats();
    Location outputFormatsLocation = config.lastLocation();

    //if (!Generator::runPrepareOnly())
    loadIndexFiles(config);

    QSet<QString> excludedDirs;
    QSet<QString> excludedFiles;
    QStringList headerList;
    QStringList sourceList;
    QStringList excludedDirsList;
    QStringList excludedFilesList;

    Generator::debugSegfault("Reading excludedirs");
    excludedDirsList = config.getCanonicalPathList(CONFIG_EXCLUDEDIRS);
    foreach (const QString &excludeDir, excludedDirsList) {
        QString p = QDir::fromNativeSeparators(excludeDir);
        QDir tmp(p);
        if (tmp.exists())
            excludedDirs.insert(p);
    }

    Generator::debugSegfault("Reading excludefiles");
    excludedFilesList = config.getCleanPathList(CONFIG_EXCLUDEFILES);
    foreach (const QString& excludeFile, excludedFilesList) {
        QString p = QDir::fromNativeSeparators(excludeFile);
        excludedFiles.insert(p);
    }

    Generator::debugSegfault("Reading headerdirs");
    headerList = config.getAllFiles(CONFIG_HEADERS,CONFIG_HEADERDIRS,excludedDirs,excludedFiles);
    QMap<QString,QString> headers;
    QMultiMap<QString,QString> headerFileNames;
    for (int i=0; i<headerList.size(); ++i) {
        if (headerList[i].contains(QString("doc/snippets")))
            continue;
        if (headers.contains(headerList[i]))
            continue;
        headers.insert(headerList[i],headerList[i]);
        QString t = headerList[i].mid(headerList[i].lastIndexOf('/')+1);
        headerFileNames.insert(t,t);
    }

    Generator::debugSegfault("Reading sourcedirs");
    sourceList = config.getAllFiles(CONFIG_SOURCES,CONFIG_SOURCEDIRS,excludedDirs,excludedFiles);
    QMap<QString,QString> sources;
    QMultiMap<QString,QString> sourceFileNames;
    for (int i=0; i<sourceList.size(); ++i) {
        if (sourceList[i].contains(QString("doc/snippets")))
            continue;
        if (sources.contains(sourceList[i]))
            continue;
        sources.insert(sourceList[i],sourceList[i]);
        QString t = sourceList[i].mid(sourceList[i].lastIndexOf('/')+1);
        sourceFileNames.insert(t,t);
    }
    /*
      Find all the qdoc files in the example dirs, and add
      them to the source files to be parsed.
     */
    Generator::debugSegfault("Reading exampledirs");
    QStringList exampleQdocList = config.getExampleQdocFiles(excludedDirs, excludedFiles);
    for (int i=0; i<exampleQdocList.size(); ++i) {
        if (!sources.contains(exampleQdocList[i])) {
            sources.insert(exampleQdocList[i],exampleQdocList[i]);
            QString t = exampleQdocList[i].mid(exampleQdocList[i].lastIndexOf('/')+1);
            sourceFileNames.insert(t,t);
        }
    }

    Generator::debugSegfault("Adding doc/image dirs found in exampledirs to imagedirs");
    QSet<QString> exampleImageDirs;
    QStringList exampleImageList = config.getExampleImageFiles(excludedDirs, excludedFiles);
    for (int i=0; i<exampleImageList.size(); ++i) {
        if (exampleImageList[i].contains("doc/images")) {
            QString t = exampleImageList[i].left(exampleImageList[i].lastIndexOf("doc/images")+10);
            if (!exampleImageDirs.contains(t)) {
                exampleImageDirs.insert(t);
            }
        }
    }
    Generator::augmentImageDirs(exampleImageDirs);

    /*
      Parse each header file in the set using the appropriate parser and add it
      to the big tree.
     */
    QSet<CodeParser *> usedParsers;

    Generator::debugSegfault("Parsing header files");
    int parsed = 0;
    QMap<QString,QString>::ConstIterator h = headers.constBegin();
    while (h != headers.constEnd()) {
        CodeParser *codeParser = CodeParser::parserForHeaderFile(h.key());
        if (codeParser) {
            ++parsed;
            codeParser->parseHeaderFile(config.location(), h.key());
            usedParsers.insert(codeParser);
        }
        ++h;
    }

    foreach (CodeParser *codeParser, usedParsers)
        codeParser->doneParsingHeaderFiles();

    usedParsers.clear();
    /*
      Parse each source text file in the set using the appropriate parser and
      add it to the big tree.
     */
    parsed = 0;
    Generator::debugSegfault("Parsing source files");
    QMap<QString,QString>::ConstIterator s = sources.constBegin();
    while (s != sources.constEnd()) {
        CodeParser *codeParser = CodeParser::parserForSourceFile(s.key());
        if (codeParser) {
            ++parsed;
            codeParser->parseSourceFile(config.location(), s.key());
            usedParsers.insert(codeParser);
        }
        ++s;
    }

    foreach (CodeParser *codeParser, usedParsers)
        codeParser->doneParsingSourceFiles();

    /*
      Now the big tree has been built from all the header and
      source files. Resolve all the class names, function names,
      targets, URLs, links, and other stuff that needs resolving.
     */
    Generator::debugSegfault("Resolving stuff prior to generating docs");
    qdb->resolveIssues();

    /*
      The tree is built and all the stuff that needed resolving
      has been resolved. Now traverse the tree and generate the
      documentation output. More than one output format can be
      requested. The tree is traversed for each one.
     */
    Generator::debugSegfault("Generating docs");
    QSet<QString>::ConstIterator of = outputFormats.constBegin();
    while (of != outputFormats.constEnd()) {
        Generator* generator = Generator::generatorForFormat(*of);
        if (generator == 0)
            outputFormatsLocation.fatal(QCoreApplication::translate("QDoc", "Unknown output format '%1'").arg(*of));
        generator->generateTree();
        ++of;
    }


    //Generator::writeOutFileNames();
    Generator::debugSegfault("Shutting down qdoc");

    QDocDatabase::qdocDB()->setVersion(QString());
    Generator::terminate();
    CodeParser::terminate();
    CodeMarker::terminate();
    Doc::terminate();
    Tokenizer::terminate();
    Location::terminate();
    QDir::setCurrent(prevCurrentDir);

#ifndef QT_NO_TRANSLATION
    qDeleteAll(translators);
#endif
#ifdef DEBUG_SHUTDOWN_CRASH
    qDebug() << "main(): Delete qdoc database";
#endif
    QDocDatabase::destroyQdocDB();
#ifdef DEBUG_SHUTDOWN_CRASH
    qDebug() << "main(): qdoc database deleted";
#endif
    Generator::debugSegfault("qdoc finished!");
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    QT_USE_NAMESPACE

#ifndef QT_BOOTSTRAPPED
    QCoreApplication app(argc, argv);
#endif

    /*
      Create code parsers for the languages to be parsed,
      and create a tree for C++.
     */
    CppCodeParser cppParser;
    QmlCodeParser qmlParser;
    PureDocParser docParser;

    /*
      Create code markers for plain text, C++,
      javascript, and QML.
     */
    PlainCodeMarker plainMarker;
    CppCodeMarker cppMarker;
    JsCodeMarker jsMarker;
    QmlCodeMarker qmlMarker;

    HtmlGenerator htmlGenerator;
    DitaXmlGenerator ditaxmlGenerator;

    QStringList qdocFiles;
    QString opt;
    int i = 1;

    while (i < argc) {
        opt = argv[i++];

        if (opt == "-help") {
            printHelp();
            return EXIT_SUCCESS;
        }
        else if (opt == "-version") {
            printVersion();
            return EXIT_SUCCESS;
        }
        else if (opt == "--") {
            while (i < argc)
                qdocFiles.append(argv[i++]);
        }
        else if (opt.startsWith("-D")) {
            QString define = opt.mid(2);
            defines += define;
        }
        else if (opt == "-depends") {
            dependModules += argv[i];
            i++;
        }
        else if (opt == "-highlighting") {
            highlighting = true;
        }
        else if (opt == "-showinternal") {
            showInternal = true;
        }
        else if (opt == "-redirect-documentation-to-dev-null") {
            redirectDocumentationToDevNull = true;
        }
        else if (opt == "-no-examples") {
            Config::generateExamples = false;
        }
        else if (opt == "-indexdir") {
            if (QFile::exists(argv[i])) {
                indexDirs += argv[i];
            }
            else {
                qDebug() << "Cannot find index directory" << argv[i];
            }
            i++;
        }
        else if (opt == "-installdir") {
            Config::installDir = argv[i];
            indexDirs += argv[i];
            i++;
        }
        else if (opt == "-obsoletelinks") {
            obsoleteLinks = true;
        }
        else if (opt == "-outputdir") {
            Config::overrideOutputDir = argv[i];
            i++;
        }
        else if (opt == "-outputformat") {
            Config::overrideOutputFormats.insert(argv[i]);
            i++;
        }
        else if (opt == "-no-link-errors") {
            noLinkErrors = true;
        }
        else if (opt == "-debug") {
            Generator::setDebugSegfaultFlag(true);
        }
        else if (opt == "-prepare") {
            Generator::setQDocPass(Generator::Prepare);
        }
        else if (opt == "-generate") {
            Generator::setQDocPass(Generator::Generate);
        }
        else if (opt == "-log-progress") {
            Location::startLoggingProgress();
        }
        else {
            qdocFiles.append(opt);
        }
    }

    if (qdocFiles.isEmpty()) {
        printHelp();
        return EXIT_FAILURE;
    }

    /*
      Main loop.
     */
    foreach (const QString &qf, qdocFiles) {
        //qDebug() << "PROCESSING:" << qf;
        processQdocconfFile(qf);
    }

    return EXIT_SUCCESS;
}

