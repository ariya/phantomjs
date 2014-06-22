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

#include "generator.h"

#include <QFile>

void Function::printDeclaration(CodeBlock &block, const QString &funcNamePrefix) const
{
    block << (iline ? "inline " : "") << signature(funcNamePrefix) << (iline ? QLatin1String(" {") : QLatin1String(";"));
    if (!iline)
        return;

    block.indent();
    QString tmp = body;
    if (tmp.endsWith(QLatin1Char('\n')))
        tmp.chop(1);
    foreach (QString line, tmp.split(QLatin1Char('\n')))
        block << line;
    block.outdent();
    block << "}";
}

QString Function::signature(const QString &funcNamePrefix) const
{
   QString sig;
   if (!rtype.isEmpty()) {
       sig += rtype;
       sig += QLatin1Char(' ');
   }
   sig += funcNamePrefix;
   sig += fname;
   if (cnst)
       sig += " const";
   return sig;
}

QString Function::definition() const
{
    if (iline)
        return QString();

    QString result;
    result += signature();
    result += QLatin1String("\n{\n");

    QString tmp = body;

    if (tmp.endsWith(QLatin1Char('\n')))
        tmp.chop(1);
    if (!tmp.startsWith(QLatin1Char('\n')))
        tmp.prepend("    ");

    tmp.replace(QLatin1Char('\n'), QLatin1String("\n    "));

    result += tmp;

    result += QLatin1String("\n}\n");

    return result;
}

void Class::Section::printDeclaration(const Class *klass, CodeBlock &block) const
{
    foreach (Function ctor, constructors)
        ctor.printDeclaration(block, klass->name());

    if (!constructors.isEmpty())
        block.addNewLine();

    foreach (Function func, functions)
        func.printDeclaration(block);

    if (!functions.isEmpty())
        block.addNewLine();

    foreach (QString var, variables)
        block << var << ';';
}

void Class::addConstructor(Access access, const QString &body, const QString &_args)
{
    Function ctor;
    QString args = _args;
    if (!args.startsWith(QLatin1Char('('))
        && !args.endsWith(QLatin1Char(')'))) {
        args.prepend('(');
        args.append(')');
    }
    ctor.setName(args);
    ctor.addBody(body);
    sections[access].constructors.append(ctor);
}

QString Class::Section::definition(const Class *klass) const
{
    QString result;

    foreach (Function ctor, constructors) {
        ctor.setName(klass->name() + "::" + klass->name() + ctor.name());
        result += ctor.definition();
        result += QLatin1Char('\n');
    }

    foreach (Function func, functions) {
        if (!func.hasBody()) continue;
        func.setName(klass->name() + "::" + func.name());
        result += func.definition();
        result += QLatin1Char('\n');
    }

    return result;
}

QString Class::declaration() const
{
    CodeBlock block;

    block << QLatin1String("class ") << cname;
    block << "{";

    if (!sections[PublicMember].isEmpty()) {
        block << "public:";
        block.indent();
        sections[PublicMember].printDeclaration(this, block);
        block.outdent();
    }

    if (!sections[ProtectedMember].isEmpty()) {
        block << "protected:";
        block.indent();
        sections[ProtectedMember].printDeclaration(this, block);
        block.outdent();
    }

    if (!sections[PrivateMember].isEmpty()) {
        block << "private:";
        block.indent();
        sections[PrivateMember].printDeclaration(this, block);
        block.outdent();
    }

    block << "};";
    block.addNewLine();

    return block.toString();
}

QString Class::definition() const
{
    return sections[PrivateMember].definition(this)
           + sections[ProtectedMember].definition(this)
           + sections[PublicMember].definition(this);
}

Generator::Generator(const DFA &_dfa, const Config &config)
     : dfa(_dfa), cfg(config)
{
    QList<InputType> lst = cfg.maxInputSet.toList();
    qSort(lst);
    minInput = lst.first();
    maxInput = lst.last();

    ConfigFile::Section section = config.configSections.value("Code Generator Options");

    foreach (ConfigFile::Entry entry, section) {
        if (!entry.key.startsWith(QLatin1String("MapToCode["))
            || !entry.key.endsWith(QLatin1Char(']')))
            continue;
        QString range = entry.key;
        range.remove(0, qstrlen("MapToCode["));
        range.chop(1);
        if (range.length() != 3
            || range.at(1) != QLatin1Char('-')) {
            qWarning("Invalid range for char mapping function: %s", qPrintable(range));
            continue;
        }
        TransitionSequence seq;
        seq.first = range.at(0).unicode();
        seq.last = range.at(2).unicode();
        seq.testFunction = entry.value;
        charFunctionRanges.append(seq);
    }

    QString tokenPrefix = section.value("TokenPrefix");
    if (!tokenPrefix.isEmpty()) {
        for (int i = 0; i < dfa.count(); ++i)
            if (!dfa.at(i).symbol.isEmpty()
                && !dfa.at(i).symbol.endsWith(QLatin1String("()")))
                dfa[i].symbol.prepend(tokenPrefix);
    }

    headerFileName = section.value("FileHeader");
}

static inline bool adjacentKeys(int left, int right) { return left + 1 == right; }
//static inline bool adjacentKeys(const InputType &left, const InputType &right)
//{ return left.val + 1 == right.val; }

static QVector<Generator::TransitionSequence> convertToSequences(const TransitionMap &transitions)
{
    QVector<Generator::TransitionSequence> sequences;
    if (transitions.isEmpty())
        return sequences;

    QList<InputType> keys = transitions.keys();
    qSort(keys);
    int i = 0;
    Generator::TransitionSequence sequence;
    sequence.first = keys.at(0);
    ++i;
    for (; i < keys.count(); ++i) {
        if (adjacentKeys(keys.at(i - 1), keys.at(i))
            && transitions.value(keys.at(i)) == transitions.value(keys.at(i - 1))) {
            continue;
        }
        sequence.last = keys.at(i - 1);
        sequence.transition = transitions.value(sequence.last);
        sequences.append(sequence);

        sequence.first = keys.at(i);
    }
    sequence.last = keys.at(i - 1);
    sequence.transition = transitions.value(sequence.last);
    sequences.append(sequence);

    return sequences;
}

QDebug &operator<<(QDebug &debug, const Generator::TransitionSequence &seq)
{
    return debug << "[first:" << seq.first << "; last:" << seq.last << "; transition:" << seq.transition
                 << (seq.testFunction.isEmpty() ? QString() : QString(QString("; testfunction:" + seq.testFunction)))
                 << "]";
}

bool Generator::isSingleReferencedFinalState(int i) const
{
    return backReferenceMap.value(i) == 1
           && dfa.at(i).transitions.isEmpty()
           && !dfa.at(i).symbol.isEmpty();
}

void Generator::generateTransitions(CodeBlock &body, const TransitionMap &transitions)
{
    if (transitions.isEmpty())
        return;

    QVector<TransitionSequence> sequences = convertToSequences(transitions);

    bool needsCharFunction = false;
    if (!charFunctionRanges.isEmpty()) {
        int i = 0;
        while (i < sequences.count()) {
            const TransitionSequence &seq = sequences.at(i);
            if (!seq.testFunction.isEmpty()) {
                ++i;
                continue;
            }

            foreach (TransitionSequence range, charFunctionRanges)
                if (range.first >= seq.first && range.last <= seq.last) {
                    needsCharFunction = true;

                    TransitionSequence left, middle, right;

                    left.first = seq.first;
                    left.last = range.first - 1;
                    left.transition = seq.transition;

                    middle = range;
                    middle.transition = seq.transition;

                    right.first = range.last + 1;
                    right.last = seq.last;
                    right.transition = seq.transition;

                    sequences.remove(i);
                    if (left.last >= left.first) {
                        sequences.insert(i, left);
                        ++i;
                    }
                    sequences.insert(i, middle);
                    ++i;
                    if (right.last >= right.first) {
                        sequences.insert(i, right);
                        ++i;
                    }

                    i = -1;
                    break;
                }

            ++i;
        }
    }

    //qDebug() << "sequence count" << sequences.count();
    //qDebug() << sequences;

    if (sequences.count() < 10
        || sequences.last().last == maxInput
        || needsCharFunction) {
        foreach (TransitionSequence seq, sequences) {
            const bool embedFinalState = isSingleReferencedFinalState(seq.transition);

            QString brace;
            if (embedFinalState)
                brace = " {";

            if (!seq.testFunction.isEmpty()) {
                body << "if (" << seq.testFunction << ")" << brace;
            } else if (seq.first == seq.last) {
                body << "if (ch.unicode() == " << seq.first << ")" << brace;
            } else {
                if (seq.last < maxInput)
                    body << "if (ch.unicode() >= " << seq.first
                         << " && ch.unicode() <= " << seq.last << ")" << brace;
                else
                    body << "if (ch.unicode() >= " << seq.first << ")" << brace;
            }
            body.indent();
            if (embedFinalState) {
                body << "token = " << dfa.at(seq.transition).symbol << ";";
                body << "goto found;";

                body.outdent();
                body << "}";
            } else {
                body << "goto state_" << seq.transition << ";";
                body.outdent();
            }
        }
    } else {
        QList<InputType> keys = transitions.keys();
        qSort(keys);

        body << "switch (ch.unicode()) {";
        body.indent();
        for (int k = 0; k < keys.count(); ++k) {
            const InputType key = keys.at(k);
            const int trans = transitions.value(key);

            QString keyStr;
            if (key == '\\')
                keyStr = QString("\'\\\\\'");
            else if (key >= 48 && key < 127)
                keyStr = QString('\'') + QChar::fromLatin1(char(key)) + QChar('\'');
            else
                keyStr = QString::number(key);

            if (k < keys.count() - 1
                && transitions.value(keys.at(k + 1)) == trans) {
                body << "case " << keyStr << ":";
            } else {
                if (isSingleReferencedFinalState(trans)) {
                    body << "case " << keyStr << ": token = " << dfa.at(trans).symbol << "; goto found;";
                } else {
                    body << "case " << keyStr << ": goto state_" << trans << ";";
                }
            }
        }
        body.outdent();
        body << "}";
    }
}

QString Generator::generate()
{
    Class klass(cfg.className);

    klass.addMember(Class::PublicMember, "QString input");
    klass.addMember(Class::PublicMember, "int pos");
    klass.addMember(Class::PublicMember, "int lexemStart");
    klass.addMember(Class::PublicMember, "int lexemLength");

    {
        CodeBlock body;
        body << "input = inp;";
        body << "pos = 0;";
        body << "lexemStart = 0;";
        body << "lexemLength = 0;";
        klass.addConstructor(Class::PublicMember, body, "const QString &inp");
    }

    {
        Function next("QChar", "next()");
        next.setInline(true);
        if (cfg.caseSensitivity == Qt::CaseSensitive)
            next.addBody("return (pos < input.length()) ? input.at(pos++) : QChar();");
        else
            next.addBody("return (pos < input.length()) ? input.at(pos++).toLower() : QChar();");
        klass.addMember(Class::PublicMember, next);
    }

    /*
    {
        Function lexem("QString", "lexem()");
        lexem.setConst(true);
        lexem.setInline(true);
        lexem.addBody("return input.mid(lexemStart, lexemLength);");
        klass.addMember(Class::PublicMember, lexem);
    }
    */

    for (int i = 0; i < dfa.count(); ++i)
        if (dfa.at(i).symbol.endsWith(QLatin1String("()"))) {
            Function handlerFunc("int", dfa.at(i).symbol);
            klass.addMember(Class::PublicMember, handlerFunc);
        }

    Function lexFunc;
    lexFunc.setReturnType("int");
    lexFunc.setName("lex()");

    CodeBlock body;
    body << "lexemStart = pos;";
    body << "lexemLength = 0;";
    body << "int lastAcceptingPos = -1;";
    body << "int token = -1;";
    body << "QChar ch;";
    body.addNewLine();

    backReferenceMap.clear();
    foreach (State s, dfa)
        foreach (int state, s.transitions)
            backReferenceMap[state]++;

    bool haveSingleReferencedFinalState = false;

    for (int i = 0; i < dfa.count(); ++i) {
        if (isSingleReferencedFinalState(i)) {
            haveSingleReferencedFinalState = true;
            continue;
        }

        if (i > 0)
            body << "state_" << i << ":";
        else
            body << "// initial state";

        body.indent();

        if (!dfa.at(i).symbol.isEmpty()) {
            body << "lastAcceptingPos = pos;";
            body << "token = " << dfa.at(i).symbol << ";";
        }

        body.outdent();

        body.indent();

        if (!dfa.at(i).transitions.isEmpty()) {
            body << "ch = next();";
            generateTransitions(body, dfa.at(i).transitions);
        }

        body << "goto out;";

        body.outdent();
    }

    if (haveSingleReferencedFinalState) {
        body << "found:";
        body << "lastAcceptingPos = pos;";
        body.addNewLine();
    }

    body << "out:";
    body << "if (lastAcceptingPos != -1) {";
    body.indent();
    body << "lexemLength = lastAcceptingPos - lexemStart;";
    body << "pos = lastAcceptingPos;";
    body.outdent();
    body << "}";
    body << "return token;";

    lexFunc.addBody(body);

    klass.addMember(Class::PublicMember, lexFunc);

    QString header;
    QFile headerFile(headerFileName);
    if (!headerFileName.isEmpty()
        && headerFile.exists()
        && headerFile.open(QIODevice::ReadOnly)) {
        header = QString::fromUtf8(headerFile.readAll());
    }

    header += QLatin1String("// auto generated. DO NOT EDIT.\n");

    return header + klass.declaration() + klass.definition();
}

