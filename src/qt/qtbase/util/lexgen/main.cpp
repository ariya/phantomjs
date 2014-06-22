/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the utils of the Qt Toolkit.
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

#include "nfa.h"
#include "re2nfa.h"
#include "configfile.h"
#include "generator.h"

#include <QFile>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDateTime>

struct Symbol
{
    QString token;
    QString lexem;
};

static QList<Symbol> tokenize(const DFA &dfa, const QString &input, Config *cfg, bool *ok = 0)
{
    QList<Symbol> symbols;
    Symbol lastSymbol;
    int state = 0;
    int lastAcceptingState = -1;
    QString lastAcceptingLexem;
    int lastAcceptingPos = -1;
    for (int i = 0; i < input.length(); ++i) {
        QChar ch = input.at(i);
        QChar chForInput = ch;
        if (cfg->caseSensitivity == Qt::CaseInsensitive)
            chForInput = chForInput.toLower();
        int next = dfa.at(state).transitions.value(chForInput.unicode());
        if (cfg->debug)
            qDebug() << "input" << input.at(i) << "leads to state" << next;
        if (next) {
            lastSymbol.lexem.append(input.at(i));
            lastSymbol.token = dfa.at(next).symbol;
            if (!lastSymbol.token.isEmpty()) {
                lastAcceptingState = next;
                lastAcceptingLexem = lastSymbol.lexem;
                lastAcceptingPos = i;
            }
            state = next;
        } else {
            if (lastAcceptingState != -1) {
                if (cfg->debug)
                    qDebug() << "adding" << dfa.at(lastAcceptingState).symbol << "and backtracking to" << lastAcceptingPos;
                Symbol s;
                s.token = dfa.at(lastAcceptingState).symbol;
                s.lexem = lastAcceptingLexem;
                symbols << s;
                lastSymbol = Symbol();
                state = 0;
                i = lastAcceptingPos;
                lastAcceptingPos = -1;
                lastAcceptingState = -1;
                continue;
            }
            if (state == 0 || lastSymbol.token.isEmpty()) {
                if (cfg->debug)
                    qDebug() << "invalid input";
                if (ok)
                    *ok = false;
                return symbols;
            }
            if (cfg->debug)
                qDebug() << "appending symbol with token" << lastSymbol.token;
            symbols << lastSymbol;
            lastSymbol = Symbol();
            state = 0;
            lastAcceptingState = -1;
            --i;
        }
    }
    if (!lastSymbol.token.isEmpty()) {
        if (cfg->debug)
            qDebug() << "appending (last) symbol with token" << lastSymbol.token;
        symbols << lastSymbol;
    } else if (lastAcceptingState != -1) {
        if (cfg->debug)
            qDebug() << "appending last accepting state with token" << dfa.at(lastAcceptingState).symbol;
        Symbol s;
        s.lexem = lastAcceptingLexem;
        s.token = dfa.at(lastAcceptingState).symbol;
        symbols << s;
    }
    if (ok)
        *ok = true;
    return symbols;
}

static QSet<InputType> determineMaxInputSet(const ConfigFile::Section &section)
{
    QSet<InputType> set;

    QString inputTypeName;

    foreach (const ConfigFile::Entry &entry, section)
        if (entry.key == QLatin1String("InputType")) {
            if (!inputTypeName.isEmpty()) {
                qWarning("Error: InputType field specified multiple times in config file");
                return QSet<InputType>();
            }
            inputTypeName = entry.value;
        }

    if (inputTypeName.isEmpty())
        inputTypeName = "quint8";

    if (inputTypeName == "quint8") {
        for (int i = 1; i < 256; ++i)
            set.insert(i);
    } /* else if ### */
    else {
        qWarning("Error: Unknown input type '%s'", qPrintable(inputTypeName));
        return QSet<InputType>();
    }

    return set;
}

static bool loadConfig(const QString &ruleFile, Config *cfg)
{
    ConfigFile::SectionMap sections = ConfigFile::parse(ruleFile);
    if (sections.isEmpty()) {
        qWarning("Error parsing %s", qPrintable(ruleFile));
        return false;
    }

    QSet<InputType> maxInputSet = determineMaxInputSet(sections.value("Options"));
    if (maxInputSet.isEmpty())
        return false;

    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    if (sections.value("Options").contains("case-sensitive"))
        cs = Qt::CaseSensitive;

    cfg->configSections = sections;
    cfg->caseSensitivity = cs;
    cfg->className = sections.value("Options").value("classname", "Scanner");
    cfg->maxInputSet = maxInputSet;
    cfg->ruleFile = ruleFile;
    return true;
}

static DFA generateMachine(const Config &cfg)
{
    if (cfg.cache) {
        QFileInfo ruleInfo(cfg.ruleFile);
        QFileInfo cacheInfo(ruleInfo.baseName() + ".dfa");
        if (cacheInfo.exists()
            && cacheInfo.lastModified() > ruleInfo.lastModified()) {
            QFile f(cacheInfo.absoluteFilePath());
            f.open(QIODevice::ReadOnly);
            QDataStream stream(&f);
            DFA machine;
            stream >> machine;
            return machine;
        }
    }

    QMap<QString, NFA> macros;
    foreach (ConfigFile::Entry e, cfg.configSections.value("Macros")) {
        int errCol = 0;
        if (cfg.debug)
            qDebug() << "parsing" << e.value;
        NFA nfa = RE2NFA(macros, cfg.maxInputSet, cfg.caseSensitivity).parse(e.value, &errCol);
        if (nfa.isEmpty()) {
            qWarning("Parse error in line %d column %d", e.lineNumber, errCol);
            return DFA();
        }
        macros.insert(e.key, nfa);
    }

    if (!cfg.configSections.contains("Tokens")) {
        qWarning("Rule file does not contain a [Tokens] section!");
        return DFA();
    }

    QVector<NFA> tokens;

    foreach (ConfigFile::Entry e, cfg.configSections.value("Tokens")) {
        int errCol = 0;
        if (cfg.debug)
            qDebug() << "parsing" << e.value;
        NFA tok = RE2NFA(macros, cfg.maxInputSet, cfg.caseSensitivity).parse(e.value, &errCol);
        if (tok.isEmpty()) {
            qWarning("Parse error in line %d column %d while parsing token %s", e.lineNumber, errCol, e.key.toLocal8Bit().constData());
            return DFA();
        }
        tok.setTerminationSymbol(e.key);
        tokens.append(tok);
    }

    NFA giganticStateMachine;
    foreach (NFA nfa, tokens)
        if (giganticStateMachine.isEmpty())
            giganticStateMachine = nfa;
        else
            giganticStateMachine = NFA::createAlternatingNFA(giganticStateMachine, nfa);

    DFA result = giganticStateMachine.toDFA().minimize();
    if (cfg.cache) {
        QFileInfo ruleInfo(cfg.ruleFile);
        QFileInfo cacheInfo(ruleInfo.baseName() + ".dfa");
        QFile f(cacheInfo.absoluteFilePath());
        f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        QDataStream stream(&f);
        stream << result;
    }
    return result;
}

#if !defined(AUTOTEST)
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QString ruleFile;
    Config cfg;

    const QStringList arguments = app.arguments().mid(1);
    cfg.debug = arguments.contains("-debug");
    const bool testRules = arguments.contains("-test");
    cfg.cache = arguments.contains("-cache");

    foreach (const QString &arg, arguments)
        if (!arg.startsWith(QLatin1Char('-'))) {
            ruleFile = arg;
            break;
        }

    if (ruleFile.isEmpty()) {
        qWarning("usage: lexgen [-test rulefile");
        qWarning(" ");
        qWarning("    the -test option will cause lexgen to interpret standard input");
        qWarning("    according to the specified rules and print out pairs of token and");
        qWarning("    lexical element");
        return 1;
    }

    if (!loadConfig(ruleFile, &cfg))
        return 1;

    DFA machine = generateMachine(cfg);
    if (machine.isEmpty())
        return 1;

    if (testRules) {
        qWarning("Testing:");
        QString input = QTextStream(stdin).readAll();
        /*
        qDebug() << "NFA has" << machine.stateCount() << "states";
        qDebug() << "Converting to DFA... (this may take a while)";
        DFA dfa = machine.toDFA();
        qDebug() << "DFA has" << dfa.count() << "states";
        qDebug() << "Minimizing...";
        dfa = dfa.minimize();
        qDebug() << "Minimized DFA has" << dfa.count() << "states";
        */
        DFA dfa = machine;
        if (cfg.debug)
            qDebug() << "tokenizing" << input;
        bool ok = false;
        QList<Symbol> symbols = tokenize(dfa, input, &cfg, &ok);
        if (symbols.isEmpty()) {
            qWarning("No tokens produced!");
        } else {
            foreach (Symbol s, symbols)
                    qDebug() << s.token << ":" << s.lexem;
        }
        if (ok)
            qDebug() << symbols.count() << "tokens produced.";
        else
            qDebug() << "Error while tokenizing!";
    } else {
        Generator gen(machine, cfg);
        QTextStream(stdout)
            << gen.generate();
    }

    return 0;
}
#endif

