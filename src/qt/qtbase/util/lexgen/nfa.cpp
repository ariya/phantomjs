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
#include <QSet>
#include <limits.h>

NFA NFA::createSingleInputNFA(InputType input)
{
    NFA result;
    result.initialize(2);
    result.addTransition(result.initialState, input, result.finalState);
    return result;
}

NFA NFA::createSymbolNFA(const QString &symbol)
{
    NFA result = NFA::createSingleInputNFA(Epsilon);
    result.states[result.finalState].symbol = symbol;
    return result;
}

void NFA::initialize(int size)
{
    states.resize(size);
    states.fill(State());
    initialState = 0;
    finalState = size - 1;
}

void NFA::addTransition(int from, InputType input, int to)
{
    assertValidState(from);
    assertValidState(to);

    states[from].transitions.insertMulti(input, to);
}

void NFA::copyFrom(const NFA &other, int baseState)
{
    assertValidState(baseState);
    assertValidState(baseState + other.states.count() - 1);

    for (int i = 0; i < other.states.count(); ++i) {
        State s = other.states.at(i);

        for (TransitionMap::Iterator it = s.transitions.begin(),
             end = s.transitions.end(); it != end; ++it)
            *it += baseState;

        states[baseState + i] = s;
    }
}

void NFA::initializeFromPair(const NFA &a, const NFA &b,
                             int *initialA, int *finalA,
                             int *initialB, int *finalB)
{
    initialize(a.states.count() + b.states.count() + 2);

    int baseIdxA = 1;
    int baseIdxB = 1 + a.states.count();

    *initialA = a.initialState + baseIdxA;
    *finalA = a.finalState + baseIdxA;

    *initialB = b.initialState + baseIdxB;
    *finalB = b.finalState + baseIdxB;

    copyFrom(a, baseIdxA);
    copyFrom(b, baseIdxB);
}

NFA NFA::createAlternatingNFA(const NFA &a, const NFA &b)
{
    NFA result;

    int newInitialA, newFinalA,
        newInitialB, newFinalB;

    result.initializeFromPair(a, b, &newInitialA, &newFinalA,
                              &newInitialB, &newFinalB);

    result.addTransition(result.initialState, Epsilon, newInitialA);
    result.addTransition(result.initialState, Epsilon, newInitialB);

    result.addTransition(newFinalA, Epsilon, result.finalState);
    result.addTransition(newFinalB, Epsilon, result.finalState);

    return result;
}

NFA NFA::createConcatenatingNFA(const NFA &a, const NFA &b)
{
    NFA result;

    int initialA, finalA,
        initialB, finalB;

    result.initializeFromPair(a, b, &initialA, &finalA, &initialB, &finalB);

    result.addTransition(result.initialState, Epsilon, initialA);
    result.addTransition(finalA, Epsilon, initialB);
    result.addTransition(finalB, Epsilon, result.finalState);
    return result;
}

NFA NFA::createOptionalNFA(const NFA &a)
{
    NFA result;

    result.initialize(a.states.count() + 2);

    int baseIdxA = 1;
    int initialA = a.initialState + baseIdxA;
    int finalA = a.finalState + baseIdxA;

    result.copyFrom(a, baseIdxA);

    result.addTransition(result.initialState, Epsilon, initialA);
    result.addTransition(result.initialState, Epsilon, result.finalState);

    result.addTransition(finalA, Epsilon, initialA);
    result.addTransition(finalA, Epsilon, result.finalState);

    return result;
}

NFA NFA::createStringNFA(const QByteArray &str)
{
    NFA result;
    foreach (char c, str) {
        NFA ch = NFA::createSingleInputNFA(c);
        if (result.isEmpty())
            result = ch;
        else
            result = NFA::createConcatenatingNFA(result, ch);
    }
    return result;
}

NFA NFA::createSetNFA(const QSet<InputType> &set)
{
    NFA result;
    result.initialize(set.count() + 2);

    int state = 1;
    for (QSet<InputType>::ConstIterator it = set.constBegin(), end = set.constEnd();
         it != end; ++it, ++state) {
        result.addTransition(result.initialState, Epsilon, state);
        result.addTransition(state, *it, result.finalState);
    }

    /*
    foreach (InputType input, set) {
        NFA ch = NFA::createSingleInputNFA(input);
        if (result.isEmpty())
            result = ch;
        else
            result = NFA::createAlternatingNFA(result, ch);
    }
    */
    return result;
}

NFA NFA::createZeroOrOneNFA(const NFA &a)
{
    NFA epsilonNFA = createSingleInputNFA(Epsilon);
    return NFA::createAlternatingNFA(a, epsilonNFA);
}

NFA NFA::applyQuantity(const NFA &a, int minOccurrences, int maxOccurrences)
{
    NFA result = a;
    NFA epsilonNFA = createSingleInputNFA(Epsilon);

    if (minOccurrences == 0) {
        result = NFA::createAlternatingNFA(result, epsilonNFA);
    } else {
        minOccurrences--;
    }
    maxOccurrences--;

    for (int i = 0; i < minOccurrences; ++i)
        result = NFA::createConcatenatingNFA(result, a);

    for (int i = minOccurrences; i < maxOccurrences; ++i)
        result = NFA::createConcatenatingNFA(result, NFA::createAlternatingNFA(a, epsilonNFA));

    return result;
}

void NFA::debug()
{
    qDebug() << "NFA has" << states.count() << "states";
    qDebug() << "initial state is" << initialState;
    qDebug() << "final state is" << finalState;

    for (int i = 0; i < states.count(); ++i) {
        const State &s = states.at(i);
        for (TransitionMap::ConstIterator it = s.transitions.constBegin(),
             end = s.transitions.constEnd(); it != end; ++it)
            qDebug() << "transition from state" << i << "to" << it.value() << "through"
                     << (it.key() == Epsilon ? QString("Epsilon") : QString(char(it.key())));
        if (!s.symbol.isEmpty())
            qDebug() << "State" << i << "leads to symbol" << s.symbol;
    }
}

// helper
typedef QSet<int> DFAState;

// that's a bad hash, but it's good enough for us
// and it allows us to use the nice QHash API :)
inline uint qHash(const DFAState &state)
{
    uint val = 0;
    foreach (int s, state)
        val |= qHash(s);
    return val;
}

DFA NFA::toDFA() const
{
    DFA result;
    result.reserve(states.count());

    QHash<QString, int> symbolReferenceCounts;
    {
        QSet<int> symbolStates;
        for (int i = 0; i < states.count(); ++i)
            if (!states.at(i).symbol.isEmpty())
                symbolStates.insert(i);

        QHash<int, QString> epsilonStates;
        for (int i = 0; i < states.count(); ++i) {
            const State &s = states.at(i);
            for (TransitionMap::ConstIterator transition = s.transitions.constBegin(), end = s.transitions.constEnd();
                 transition != end; ++transition)
                if (transition.key() == Epsilon && symbolStates.contains(transition.value()))
                    epsilonStates.insert(i, states.at(transition.value()).symbol);
        }

        int lastCount;
        do {
            lastCount = epsilonStates.count();
            for (int i = 0; i < states.count(); ++i) {
                const State &s = states.at(i);
                for (TransitionMap::ConstIterator transition = s.transitions.constBegin(), end = s.transitions.constEnd();
                     transition != end; ++transition)
                    if (transition.key() == Epsilon && epsilonStates.contains(transition.value()))
                        epsilonStates.insert(i, epsilonStates.value(transition.value()));
            }
        } while (lastCount != epsilonStates.count());

        for (int i = 0; i < states.count(); ++i) {
            const State &s = states.at(i);
            for (TransitionMap::ConstIterator transition = s.transitions.constBegin(), end = s.transitions.constEnd();
                 transition != end; ++transition) {
                if (transition.key() == Epsilon)
                    continue;
                if (symbolStates.contains(transition.value())) {
                    const QString symbol = states.at(transition.value()).symbol;
                    symbolReferenceCounts[symbol]++;
                } else if (epsilonStates.contains(transition.value())) {
                    const QString symbol = epsilonStates.value(transition.value());
                    symbolReferenceCounts[symbol]++;
                }
            }
        }
        /*
        for (QHash<QString, int>::ConstIterator symIt = symbolReferenceCounts.constBegin(), symEnd = symbolReferenceCounts.constEnd();
             symIt != symEnd; ++symIt)
            qDebug() << "symbol" << symIt.key() << "is reached" << symIt.value() << "times";
            */
    }

    QSet<InputType> validInput;
    foreach (const State &s, states)
        for (TransitionMap::ConstIterator it = s.transitions.constBegin(),
             end = s.transitions.constEnd(); it != end; ++it)
            if (it.key() != Epsilon)
                validInput.insert(it.key());

    // A DFA state can consist of multiple NFA states.
    // the dfaStateMap maps from these to the actual
    // state index within the resulting DFA vector
    QHash<DFAState, int> dfaStateMap;
    QStack<DFAState> pendingDFAStates;

    DFAState startState = epsilonClosure(QSet<int>() << initialState);

    result.resize(1);
    dfaStateMap.insert(startState, 0);

    pendingDFAStates.push(startState);

    while (!pendingDFAStates.isEmpty()) {
        DFAState state = pendingDFAStates.pop();
//        qDebug() << "processing" << state << "from the stack of pending states";

        foreach (InputType input, validInput) {

            QSet<int> reachableStates;

            foreach (int nfaState, state) {
                const TransitionMap &transitions = states.at(nfaState).transitions;
                TransitionMap::ConstIterator it = transitions.find(input);
                while (it != transitions.constEnd() && it.key() == input) {
                    reachableStates.insert(it.value());
                    ++it;
                }
            }

            if (reachableStates.isEmpty())
                continue;

//            qDebug() << "can reach" << reachableStates << "from input" << char(input);

            QSet<int> closure = epsilonClosure(reachableStates);

//            qDebug() << "closure is" << closure;

            if (!dfaStateMap.contains(closure)) {
                int dfaState = result.count();
                result.append(State());

                QString symbol;
                int refCount = INT_MAX;
                foreach (int nfaState, closure)
                    if (!states.at(nfaState).symbol.isEmpty()) {
//                        qDebug() << "closure also contains symbol" << states.at(nfaState).symbol;
                        QString candidate = states.at(nfaState).symbol;
                        int candidateRefCount =symbolReferenceCounts.value(candidate, INT_MAX);
                        if (candidateRefCount < refCount) {
                            refCount = candidateRefCount;
                            symbol = candidate;
                        }
                    }
                if (!symbol.isEmpty())
                    result.last().symbol = symbol;

                dfaStateMap.insert(closure, dfaState);

                Q_ASSERT(!pendingDFAStates.contains(closure));
                pendingDFAStates.prepend(closure);
            }

            result[dfaStateMap.value(state)].transitions.insert(input, dfaStateMap.value(closure));
        }
    }

    return result;
}

QSet<int> NFA::epsilonClosure(const QSet<int> &initialClosure) const
{
    QSet<int> closure = initialClosure;
    closure.reserve(closure.count() * 4);

    QStack<int> stateStack;
    stateStack.resize(closure.count());
    qCopy(closure.constBegin(), closure.constEnd(), stateStack.begin());

    while (!stateStack.isEmpty()) {
        int t = stateStack.pop();
        const TransitionMap &transitions = states.at(t).transitions;
        TransitionMap::ConstIterator it = transitions.find(Epsilon);
        while (it != transitions.constEnd() && it.key() == Epsilon) {
            const int u = it.value();
            if (!closure.contains(u)) {
                closure.insert(u);
                stateStack.push(u);
            }
            ++it;
        }
    }

    return closure;
}

void NFA::setTerminationSymbol(const QString &symbol)
{
    states[finalState].symbol = symbol;
}

void DFA::debug() const
{
    qDebug() << "DFA has" << count() << "states";

    for (int i = 0; i < count(); ++i) {
        const State &s = at(i);
        if (s.transitions.isEmpty()) {
            qDebug() << "State" << i << "has no transitions";
        } else {
            for (TransitionMap::ConstIterator it = s.transitions.constBegin(),
                 end = s.transitions.constEnd(); it != end; ++it)
                qDebug() << "transition from state" << i << "to" << it.value() << "through"
                         << (it.key() == Epsilon ? QString("Epsilon") : QString(char(it.key())));
        }
        if (!s.symbol.isEmpty())
            qDebug() << "State" << i << "leads to symbol" << s.symbol;
    }

}

DFA DFA::minimize() const
{
    QVector<bool> inequivalentStates(count() * count());
    inequivalentStates.fill(false);

    for (int i = 0; i < count(); ++i)
        for (int j = 0; j < i; ++j) {
            if (i != j && at(i).symbol != at(j).symbol)
                inequivalentStates[i * count() + j] = true;
        }

    bool done;
    do {
        done = true;
        for (int i = 0; i < count(); ++i)
            for (int j = 0; j < count(); ++j) {
                if (i == j)
                    continue;

                if (inequivalentStates[i * count() + j])
                    continue;

                if (at(i).transitions.keys() != at(j).transitions.keys()) {
                    inequivalentStates[i * count() + j] = true;
                    done = false;
                    continue;
                }

                foreach (InputType a, at(i).transitions.keys()) {
                    int r = at(i).transitions.value(a, -1);
                    if (r == -1)
                        continue;
                    int s = at(j).transitions.value(a, -1);
                    if (s == -1)
                        continue;

                    if (inequivalentStates[r * count() + s]
                        || r == s) {
                        inequivalentStates[i * count() + j] = true;
                        done = false;
                        break;
                    }
                }
            }
    } while (!done);

    QHash<int, int> statesToEliminate;
    for (int i = 0; i < count(); ++i)
        for (int j = 0; j < i; ++j)
            if (!inequivalentStates[i * count() + j]) {
                statesToEliminate.insertMulti(i, j);
            }

    /*
    qDebug() << "states to eliminiate:" << statesToEliminate.count();;
    qDebug() << "merging" << statesToEliminate;
    debug();
    */

    return *this;
}
