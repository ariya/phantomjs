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

#include <QtTest/QtTest>
#define AUTOTEST
#include "../main.cpp"

class tst_LexGen : public QObject
{
    Q_OBJECT
private slots:
    void nfa_singleInput();
    void nfa_alternating();
    void nfa_concatenating();
    void nfa_optional();
    void nfa_toDFA_data();
    void nfa_toDFA();
    void lexgen_data();
    void lexgen();
};

void tst_LexGen::nfa_singleInput()
{
    NFA nfa = NFA::createSingleInputNFA('a');

    QCOMPARE(nfa.initialState, 0);
    QCOMPARE(nfa.finalState, 1);

    QCOMPARE(nfa.states.count(), 2);

    QCOMPARE(nfa.states.at(0).transitions.count(), 1);
    QVERIFY(nfa.states.at(0).transitions.contains('a'));
    QCOMPARE(nfa.states.at(0).transitions.values('a').count(), 1);
    QCOMPARE(nfa.states.at(0).transitions.value('a'), nfa.finalState);

    QVERIFY(nfa.states.at(1).transitions.isEmpty());
}

void tst_LexGen::nfa_alternating()
{
    NFA a = NFA::createSingleInputNFA('a');
    NFA b = NFA::createSingleInputNFA('b');
    NFA nfa = NFA::createAlternatingNFA(a, b);

    const int initialA = 1;
    const int finalA = 2;

    const int initialB = 3;
    const int finalB = 4;

    QCOMPARE(nfa.states.count(), 6);

    QCOMPARE(nfa.initialState, 0);
    QCOMPARE(nfa.finalState, 5);

    QList<int> initialTransitions = nfa.states.at(0).transitions.values(Epsilon);
    QCOMPARE(initialTransitions.count(), 2);
    QVERIFY(initialTransitions.contains(initialA));
    QVERIFY(initialTransitions.contains(initialB));

    // no need to test the individual a and b NFAs, the other
    // autotest already takes care of that. Just check whether
    // the epsilon transitions to the final state exist.

    QCOMPARE(nfa.states.at(finalA).transitions.count(), 1);
    QCOMPARE(nfa.states.at(finalA).transitions.values(Epsilon).count(), 1);
    QCOMPARE(nfa.states.at(finalA).transitions.value(Epsilon), nfa.finalState);

    QCOMPARE(nfa.states.at(finalB).transitions.count(), 1);
    QCOMPARE(nfa.states.at(finalB).transitions.values(Epsilon).count(), 1);
    QCOMPARE(nfa.states.at(finalB).transitions.value(Epsilon), nfa.finalState);
}

void tst_LexGen::nfa_concatenating()
{
    NFA a = NFA::createSingleInputNFA('a');
    NFA b = NFA::createSingleInputNFA('b');
    NFA nfa = NFA::createConcatenatingNFA(a, b);

    const int initialA = 1;
    const int finalA = 2;

    const int initialB = 3;
    const int finalB = 4;

    QCOMPARE(nfa.states.count(), 6);

    QCOMPARE(nfa.initialState, 0);
    QCOMPARE(nfa.finalState, 5);

    QCOMPARE(nfa.states.at(0).transitions.count(), 1);
    QCOMPARE(nfa.states.at(0).transitions.values(Epsilon).count(), 1);
    QCOMPARE(nfa.states.at(0).transitions.value(Epsilon), initialA);

    QCOMPARE(nfa.states.at(finalA).transitions.values(Epsilon).count(), 1);
    QCOMPARE(nfa.states.at(finalA).transitions.value(Epsilon), initialB);

    QCOMPARE(nfa.states.at(finalB).transitions.values(Epsilon).count(), 1);
    QCOMPARE(nfa.states.at(finalB).transitions.value(Epsilon), nfa.finalState);
}

void tst_LexGen::nfa_optional()
{
    NFA a = NFA::createSingleInputNFA('a');
    NFA nfa = NFA::createOptionalNFA(a);

    const int initialA = 1;
    const int finalA = 2;

    QCOMPARE(nfa.states.count(), 4);

    QCOMPARE(nfa.initialState, 0);
    QCOMPARE(nfa.finalState, 3);

    QCOMPARE(nfa.states.at(0).transitions.count(), 2);
    QList<int> initialTransitions = nfa.states.at(0).transitions.values(Epsilon);
    QVERIFY(initialTransitions.contains(nfa.finalState));
    QVERIFY(initialTransitions.contains(initialA));

    QList<int> finalEpsilonATransitions = nfa.states.at(finalA).transitions.values(Epsilon);
    QVERIFY(finalEpsilonATransitions.contains(initialA));
    QVERIFY(finalEpsilonATransitions.contains(nfa.finalState));
}

Q_DECLARE_METATYPE(NFA);
Q_DECLARE_METATYPE(DFA);

void tst_LexGen::nfa_toDFA_data()
{
    QTest::addColumn<NFA>("nfa");
    QTest::addColumn<DFA>("expectedDFA");

    NFA a = NFA::createSingleInputNFA('a');
    NFA b = NFA::createSingleInputNFA('b');
    NFA c = NFA::createSingleInputNFA('c');

    NFA nfa;
    DFA dfa;

    dfa.clear();
    dfa.resize(3);
    dfa[0].transitions.insert('a', 1);
    dfa[1].transitions.insert('b', 2);

    nfa = NFA::createConcatenatingNFA(a, b);

    QTest::newRow("simple concat") << nfa << dfa;

    dfa.clear();
    dfa.resize(3);
    dfa[0].transitions.insert('a', 1);
    dfa[0].transitions.insert('b', 2);

    nfa = NFA::createAlternatingNFA(a, b);

    QTest::newRow("simple alternate") << nfa << dfa;

}

void tst_LexGen::nfa_toDFA()
{
    QFETCH(NFA, nfa);
    QFETCH(DFA, expectedDFA);

    DFA dfa = nfa.toDFA();

    QCOMPARE(dfa.count(), expectedDFA.count());
    for (int i = 0; i < dfa.count(); ++i) {
        if (dfa.at(i).transitions != expectedDFA.at(i).transitions) {
            qDebug() << "DFAs differ in state" << i;
            qDebug() << "NFA:";
            nfa.debug();
            qDebug() << "Actual DFA:";
            dfa.debug();
            qDebug() << "Expected DFA:";
            expectedDFA.debug();
            QVERIFY(false);
        }
    }
}

void tst_LexGen::lexgen_data()
{
    QTest::addColumn<QString>("ruleFile");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    QDir d(QString(SRCDIR));
    d.cd("testdata");
    foreach (QString test, d.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QString dir = d.absoluteFilePath(test) + '/';
        QTest::newRow(qPrintable(test))
            << dir + "rules.lexgen"
            << dir + "input"
            << dir + "output"
            ;
    }
}

void tst_LexGen::lexgen()
{
    QFETCH(QString, ruleFile);
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);

    Config conf;
    QVERIFY(loadConfig(ruleFile, &conf));
    DFA dfa = generateMachine(conf);
    QVERIFY(!dfa.isEmpty());
    conf.debug = true;

    QFile f(input);
    QVERIFY(f.open(QIODevice::ReadOnly));
    input = QString::fromUtf8(f.readAll());
    f.close();
    if (input.endsWith(QLatin1Char('\n')))
        input.chop(1);
//    machine.debug();
    bool ok = false;
    QList<Symbol> symbols = tokenize(dfa, input, &conf, &ok);
    QVERIFY(ok);
    f.setFileName(expectedOutput);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QStringList lines;
    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine());
        if (line.endsWith(QLatin1Char('\n')))
            line.chop(1);
        lines << line;
    }
    f.close();

//    dfa.debug();
    QCOMPARE(lines.count(), symbols.count());

    for (int i = 0; i < lines.count(); ++i) {
        QStringList l = lines.at(i).split(QChar::fromLatin1('|'));
        QCOMPARE(l.count(), 2);
        QString expectedToken = l.at(0);
        QString expectedLexem = l.at(1);
        QCOMPARE(symbols.at(i).token, expectedToken);
        QCOMPARE(symbols.at(i).lexem, expectedLexem);
    }
}

QTEST_MAIN(tst_LexGen)
#include "tst_lexgen.moc"
