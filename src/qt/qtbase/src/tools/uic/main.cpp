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

#include "uic.h"
#include "option.h"
#include "driver.h"

#include <qfile.h>
#include <qdir.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <qcoreapplication.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

QT_BEGIN_NAMESPACE

int runUic(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QString::fromLatin1(QT_VERSION_STR));

    Driver driver;

    // Note that uic isn't translated.
    // If you use this code as an example for a translated app, make sure to translate the strings.
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.setApplicationDescription(QStringLiteral("Qt User Interface Compiler version %1").arg(QString::fromLatin1(QT_VERSION_STR)));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dependenciesOption(QStringList() << QStringLiteral("d") << QStringLiteral("dependencies"));
    dependenciesOption.setDescription(QStringLiteral("Display the dependencies."));
    parser.addOption(dependenciesOption);

    QCommandLineOption outputOption(QStringList() << QStringLiteral("o") << QStringLiteral("output"));
    outputOption.setDescription(QStringLiteral("Place the output into <file>"));
    outputOption.setValueName(QStringLiteral("file"));
    parser.addOption(outputOption);

    QCommandLineOption noProtOption(QStringList() << QStringLiteral("p") << QStringLiteral("no-protection"));
    noProtOption.setDescription(QStringLiteral("Disable header protection."));
    parser.addOption(noProtOption);

    QCommandLineOption noImplicitIncludesOption(QStringList() << QStringLiteral("n") << QStringLiteral("no-implicit-includes"));
    noImplicitIncludesOption.setDescription(QStringLiteral("Disable generation of #include-directives."));
    parser.addOption(noImplicitIncludesOption);

    QCommandLineOption postfixOption(QStringLiteral("postfix"));
    postfixOption.setDescription(QStringLiteral("Postfix to add to all generated classnames."));
    postfixOption.setValueName(QStringLiteral("postfix"));
    parser.addOption(postfixOption);

    QCommandLineOption translateOption(QStringList() << QStringLiteral("tr") << QStringLiteral("translate"));
    translateOption.setDescription(QStringLiteral("Use <function> for i18n."));
    translateOption.setValueName(QStringLiteral("function"));
    parser.addOption(translateOption);

    QCommandLineOption includeOption(QStringList() << QStringLiteral("include"));
    includeOption.setDescription(QStringLiteral("Add #include <include-file> to <file>."));
    includeOption.setValueName(QStringLiteral("include-file"));
    parser.addOption(includeOption);

    QCommandLineOption generatorOption(QStringList() << QStringLiteral("g") << QStringLiteral("generator"));
    generatorOption.setDescription(QStringLiteral("Select generator."));
    generatorOption.setValueName(QStringLiteral("java|cpp"));
    parser.addOption(generatorOption);

    parser.addPositionalArgument(QStringLiteral("[uifile]"), QStringLiteral("Input file (*.ui), otherwise stdin."));

    parser.process(app);

    driver.option().dependencies = parser.isSet(dependenciesOption);
    driver.option().outputFile = parser.value(outputOption);
    driver.option().headerProtection = !parser.isSet(noProtOption);
    driver.option().implicitIncludes = !parser.isSet(noImplicitIncludesOption);
    driver.option().postfix = parser.value(postfixOption);
    driver.option().translateFunction = parser.value(translateOption);
    driver.option().includeFile = parser.value(includeOption);
    driver.option().generator = (parser.value(generatorOption).toLower() == QLatin1String("java")) ? Option::JavaGenerator : Option::CppGenerator;

    QString inputFile;
    if (!parser.positionalArguments().isEmpty())
        inputFile = parser.positionalArguments().first();
    else // reading from stdin
        driver.option().headerProtection = false;

    if (driver.option().dependencies) {
        return !driver.printDependencies(inputFile);
    }

    QTextStream *out = 0;
    QFile f;
    if (driver.option().outputFile.size()) {
        f.setFileName(driver.option().outputFile);
        if (!f.open(QIODevice::WriteOnly | QFile::Text)) {
            fprintf(stderr, "Could not create output file\n");
            return 1;
        }
        out = new QTextStream(&f);
        out->setCodec(QTextCodec::codecForName("UTF-8"));
    }

    bool rtn = driver.uic(inputFile, out);
    delete out;

    if (!rtn) {
        if (driver.option().outputFile.size()) {
            f.close();
            f.remove();
        }
        fprintf(stderr, "File '%s' is not valid\n", inputFile.isEmpty() ? "<stdin>" : inputFile.toLocal8Bit().constData());
    }

    return !rtn;
}

QT_END_NAMESPACE

int main(int argc, char *argv[])
{
    return QT_PREPEND_NAMESPACE(runUic)(argc, argv);
}
