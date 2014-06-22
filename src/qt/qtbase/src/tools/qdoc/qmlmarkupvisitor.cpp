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

#include <qstringlist.h>
#include <qglobal.h>
#include "qqmljsast_p.h"
#include "qqmljsastfwd_p.h"
#include "qqmljsengine_p.h"

#include "qmlmarkupvisitor.h"

QT_BEGIN_NAMESPACE

QmlMarkupVisitor::QmlMarkupVisitor(const QString &source,
                                   const QList<QQmlJS::AST::SourceLocation> &pragmas,
                                   QQmlJS::Engine *engine)
{
    this->source = source;
    this->engine = engine;

    cursor = 0;
    extraIndex = 0;

    // Merge the lists of locations of pragmas and comments in the source code.
    int i = 0;
    int j = 0;
    while (i < engine->comments().length() && j < pragmas.length()) {
        if (engine->comments()[i].offset < pragmas[j].offset) {
            extraTypes.append(Comment);
            extraLocations.append(engine->comments()[i]);
            ++i;
        } else {
            extraTypes.append(Pragma);
            extraLocations.append(engine->comments()[j]);
            ++j;
        }
    }

    while (i < engine->comments().length()) {
        extraTypes.append(Comment);
        extraLocations.append(engine->comments()[i]);
        ++i;
    }

    while (j < pragmas.length()) {
        extraTypes.append(Pragma);
        extraLocations.append(pragmas[j]);
        ++j;
    }
}

QmlMarkupVisitor::~QmlMarkupVisitor()
{
}

// The protect() function is a copy of the one from CppCodeMarker.

static const QString samp  = QLatin1String("&amp;");
static const QString slt   = QLatin1String("&lt;");
static const QString sgt   = QLatin1String("&gt;");
static const QString squot = QLatin1String("&quot;");

QString QmlMarkupVisitor::protect(const QString& str)
{
    int n = str.length();
    QString marked;
    marked.reserve(n * 2 + 30);
    const QChar *data = str.constData();
    for (int i = 0; i != n; ++i) {
        switch (data[i].unicode()) {
        case '&': marked += samp;  break;
        case '<': marked += slt;   break;
        case '>': marked += sgt;   break;
        case '"': marked += squot; break;
        default : marked += data[i];
        }
    }
    return marked;
}

QString QmlMarkupVisitor::markedUpCode()
{
    if (int(cursor) < source.length())
        addExtra(cursor, source.length());

    return output;
}

void QmlMarkupVisitor::addExtra(quint32 start, quint32 finish)
{
    if (extraIndex >= extraLocations.length()) {
        QString extra = source.mid(start, finish - start);
        if (extra.trimmed().isEmpty())
            output += extra;
        else
            output += protect(extra); // text that should probably have been caught by the parser

        cursor = finish;
        return;
    }

    while (extraIndex < extraLocations.length()) {
        if (extraTypes[extraIndex] == Comment) {
            if (extraLocations[extraIndex].offset - 2 >= start)
                break;
        } else {
            if (extraLocations[extraIndex].offset >= start)
                break;
        }
        extraIndex++;
    }

    quint32 i = start;
    while (i < finish && extraIndex < extraLocations.length()) {
        quint32 j = extraLocations[extraIndex].offset - 2;
        if (i <= j && j < finish) {
            if (i < j)
                output += protect(source.mid(i, j - i));

            quint32 l = extraLocations[extraIndex].length;
            if (extraTypes[extraIndex] == Comment) {
                if (source.mid(j, 2) == QLatin1String("/*"))
                    l += 4;
                else
                    l += 2;
                output += QLatin1String("<@comment>");
                output += protect(source.mid(j, l));
                output += QLatin1String("</@comment>");
            } else
                output += protect(source.mid(j, l));

            extraIndex++;
            i = j + l;
        } else
            break;
    }

    QString extra = source.mid(i, finish - i);
    if (extra.trimmed().isEmpty())
        output += extra;
    else
        output += protect(extra); // text that should probably have been caught by the parser

    cursor = finish;
}

void QmlMarkupVisitor::addMarkedUpToken(
        QQmlJS::AST::SourceLocation &location, const QString &tagName,
        const QHash<QString, QString> &attributes)
{
    if (!location.isValid())
        return;

    if (cursor < location.offset)
        addExtra(cursor, location.offset);
    else if (cursor > location.offset)
        return;

    output += QString(QLatin1String("<@%1")).arg(tagName);
    foreach (const QString &key, attributes)
        output += QString(QLatin1String(" %1=\"%2\"")).arg(key).arg(attributes[key]);
    output += QString(QLatin1String(">%2</@%3>")).arg(protect(sourceText(location)), tagName);
    cursor += location.length;
}

QString QmlMarkupVisitor::sourceText(QQmlJS::AST::SourceLocation &location)
{
    return source.mid(location.offset, location.length);
}

void QmlMarkupVisitor::addVerbatim(QQmlJS::AST::SourceLocation first,
                                   QQmlJS::AST::SourceLocation last)
{
    if (!first.isValid())
        return;

    quint32 start = first.begin();
    quint32 finish;
    if (last.isValid())
        finish = last.end();
    else
        finish = first.end();

    if (cursor < start)
        addExtra(cursor, start);
    else if (cursor > start)
        return;

    QString text = source.mid(start, finish - start);
    output += protect(text);
    cursor = finish;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiImport *uiimport)
{
    addVerbatim(uiimport->importToken);
    if (!uiimport->importUri)
        addMarkedUpToken(uiimport->fileNameToken, QLatin1String("headerfile"));
    return false;
}

void QmlMarkupVisitor::endVisit(QQmlJS::AST::UiImport *uiimport)
{
    addVerbatim(uiimport->versionToken);
    addVerbatim(uiimport->asToken);
    addMarkedUpToken(uiimport->importIdToken, QLatin1String("headerfile"));
    addVerbatim(uiimport->semicolonToken);
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiPublicMember *member)
{
    if (member->type == QQmlJS::AST::UiPublicMember::Property) {
        addVerbatim(member->defaultToken);
        addVerbatim(member->readonlyToken);
        addVerbatim(member->propertyToken);
        addVerbatim(member->typeModifierToken);
        addMarkedUpToken(member->typeToken, QLatin1String("type"));
        addMarkedUpToken(member->identifierToken, QLatin1String("name"));
        addVerbatim(member->colonToken);
        if (member->binding)
            QQmlJS::AST::Node::accept(member->binding, this);
        else if (member->statement)
            QQmlJS::AST::Node::accept(member->statement, this);
    } else {
        addVerbatim(member->propertyToken);
        addVerbatim(member->typeModifierToken);
        addMarkedUpToken(member->typeToken, QLatin1String("type"));
        //addVerbatim(member->identifierToken);
        QQmlJS::AST::Node::accept(member->parameters, this);
    }
    addVerbatim(member->semicolonToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiObjectInitializer *initializer)
{
    addVerbatim(initializer->lbraceToken, initializer->lbraceToken);
    return true;
}

void QmlMarkupVisitor::endVisit(QQmlJS::AST::UiObjectInitializer *initializer)
{
    addVerbatim(initializer->rbraceToken, initializer->rbraceToken);
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiObjectBinding *binding)
{
    QQmlJS::AST::Node::accept(binding->qualifiedId, this);
    addVerbatim(binding->colonToken);
    QQmlJS::AST::Node::accept(binding->qualifiedTypeNameId, this);
    QQmlJS::AST::Node::accept(binding->initializer, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiScriptBinding *binding)
{
    QQmlJS::AST::Node::accept(binding->qualifiedId, this);
    addVerbatim(binding->colonToken);
    QQmlJS::AST::Node::accept(binding->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiArrayBinding *binding)
{
    QQmlJS::AST::Node::accept(binding->qualifiedId, this);
    addVerbatim(binding->colonToken);
    addVerbatim(binding->lbracketToken);
    QQmlJS::AST::Node::accept(binding->members, this);
    addVerbatim(binding->rbracketToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiArrayMemberList *list)
{
    for (QQmlJS::AST::UiArrayMemberList *it = list; it; it = it->next) {
        QQmlJS::AST::Node::accept(it->member, this);
        //addVerbatim(it->commaToken);
    }
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiQualifiedId *id)
{
    addMarkedUpToken(id->identifierToken, QLatin1String("name"));
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ThisExpression *expression)
{
    addVerbatim(expression->thisToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::IdentifierExpression *identifier)
{
    addMarkedUpToken(identifier->identifierToken, QLatin1String("name"));
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::NullExpression *null)
{
    addMarkedUpToken(null->nullToken, QLatin1String("number"));
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::TrueLiteral *literal)
{
    addMarkedUpToken(literal->trueToken, QLatin1String("number"));
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::FalseLiteral *literal)
{
    addMarkedUpToken(literal->falseToken, QLatin1String("number"));
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::NumericLiteral *literal)
{
    addMarkedUpToken(literal->literalToken, QLatin1String("number"));
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::StringLiteral *literal)
{
    addMarkedUpToken(literal->literalToken, QLatin1String("string"));
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::RegExpLiteral *literal)
{
    addVerbatim(literal->literalToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ArrayLiteral *literal)
{
    addVerbatim(literal->lbracketToken);
    QQmlJS::AST::Node::accept(literal->elements, this);
    addVerbatim(literal->rbracketToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ObjectLiteral *literal)
{
    addVerbatim(literal->lbraceToken);
    return true;
}

void QmlMarkupVisitor::endVisit(QQmlJS::AST::ObjectLiteral *literal)
{
    addVerbatim(literal->rbraceToken);
}


bool QmlMarkupVisitor::visit(QQmlJS::AST::ElementList *list)
{
    for (QQmlJS::AST::ElementList *it = list; it; it = it->next) {
        QQmlJS::AST::Node::accept(it->expression, this);
        //addVerbatim(it->commaToken);
    }
    QQmlJS::AST::Node::accept(list->elision, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::Elision *elision)
{
    addVerbatim(elision->commaToken, elision->commaToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::PropertyNameAndValue *list)
{
    QQmlJS::AST::Node::accept(list->name, this);
    addVerbatim(list->colonToken, list->colonToken);
    QQmlJS::AST::Node::accept(list->value, this);
    addVerbatim(list->commaToken, list->commaToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ArrayMemberExpression *expression)
{
    QQmlJS::AST::Node::accept(expression->base, this);
    addVerbatim(expression->lbracketToken);
    QQmlJS::AST::Node::accept(expression->expression, this);
    addVerbatim(expression->rbracketToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::FieldMemberExpression *expression)
{
    QQmlJS::AST::Node::accept(expression->base, this);
    addVerbatim(expression->dotToken);
    addMarkedUpToken(expression->identifierToken, QLatin1String("name"));
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::NewMemberExpression *expression)
{
    addVerbatim(expression->newToken);
    QQmlJS::AST::Node::accept(expression->base, this);
    addVerbatim(expression->lparenToken);
    QQmlJS::AST::Node::accept(expression->arguments, this);
    addVerbatim(expression->rparenToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::NewExpression *expression)
{
    addVerbatim(expression->newToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ArgumentList *list)
{
    addVerbatim(list->commaToken, list->commaToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::PostIncrementExpression *expression)
{
    addVerbatim(expression->incrementToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::PostDecrementExpression *expression)
{
    addVerbatim(expression->decrementToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::DeleteExpression *expression)
{
    addVerbatim(expression->deleteToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::VoidExpression *expression)
{
    addVerbatim(expression->voidToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::TypeOfExpression *expression)
{
    addVerbatim(expression->typeofToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::PreIncrementExpression *expression)
{
    addVerbatim(expression->incrementToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::PreDecrementExpression *expression)
{
    addVerbatim(expression->decrementToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UnaryPlusExpression *expression)
{
    addVerbatim(expression->plusToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::UnaryMinusExpression *expression)
{
    addVerbatim(expression->minusToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::TildeExpression *expression)
{
    addVerbatim(expression->tildeToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::NotExpression *expression)
{
    addVerbatim(expression->notToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::BinaryExpression *expression)
{
    QQmlJS::AST::Node::accept(expression->left, this);
    addMarkedUpToken(expression->operatorToken, QLatin1String("op"));
    QQmlJS::AST::Node::accept(expression->right, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ConditionalExpression *expression)
{
    QQmlJS::AST::Node::accept(expression->expression, this);
    addVerbatim(expression->questionToken);
    QQmlJS::AST::Node::accept(expression->ok, this);
    addVerbatim(expression->colonToken);
    QQmlJS::AST::Node::accept(expression->ko, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::Expression *expression)
{
    QQmlJS::AST::Node::accept(expression->left, this);
    addVerbatim(expression->commaToken);
    QQmlJS::AST::Node::accept(expression->right, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::Block *block)
{
    addVerbatim(block->lbraceToken);
    return true;
}

void QmlMarkupVisitor::endVisit(QQmlJS::AST::Block *block)
{
    addVerbatim(block->rbraceToken);
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::VariableStatement *statement)
{
    addVerbatim(statement->declarationKindToken);
    QQmlJS::AST::Node::accept(statement->declarations, this);
    addVerbatim(statement->semicolonToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::VariableDeclarationList *list)
{
    for (QQmlJS::AST::VariableDeclarationList *it = list; it; it = it->next) {
        QQmlJS::AST::Node::accept(it->declaration, this);
        addVerbatim(it->commaToken);
    }
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::VariableDeclaration *declaration)
{
    addMarkedUpToken(declaration->identifierToken, QLatin1String("name"));
    QQmlJS::AST::Node::accept(declaration->expression, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::EmptyStatement *statement)
{
    addVerbatim(statement->semicolonToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ExpressionStatement *statement)
{
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->semicolonToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::IfStatement *statement)
{
    addMarkedUpToken(statement->ifToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    QQmlJS::AST::Node::accept(statement->ok, this);
    if (statement->ko) {
        addMarkedUpToken(statement->elseToken, QLatin1String("keyword"));
        QQmlJS::AST::Node::accept(statement->ko, this);
    }
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::DoWhileStatement *statement)
{
    addMarkedUpToken(statement->doToken, QLatin1String("keyword"));
    QQmlJS::AST::Node::accept(statement->statement, this);
    addMarkedUpToken(statement->whileToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    addVerbatim(statement->semicolonToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::WhileStatement *statement)
{
    addMarkedUpToken(statement->whileToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    QQmlJS::AST::Node::accept(statement->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ForStatement *statement)
{
    addMarkedUpToken(statement->forToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    QQmlJS::AST::Node::accept(statement->initialiser, this);
    addVerbatim(statement->firstSemicolonToken);
    QQmlJS::AST::Node::accept(statement->condition, this);
    addVerbatim(statement->secondSemicolonToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    QQmlJS::AST::Node::accept(statement->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::LocalForStatement *statement)
{
    addMarkedUpToken(statement->forToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    addMarkedUpToken(statement->varToken, QLatin1String("keyword"));
    QQmlJS::AST::Node::accept(statement->declarations, this);
    addVerbatim(statement->firstSemicolonToken);
    QQmlJS::AST::Node::accept(statement->condition, this);
    addVerbatim(statement->secondSemicolonToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    QQmlJS::AST::Node::accept(statement->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ForEachStatement *statement)
{
    addMarkedUpToken(statement->forToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    QQmlJS::AST::Node::accept(statement->initialiser, this);
    addVerbatim(statement->inToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    QQmlJS::AST::Node::accept(statement->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::LocalForEachStatement *statement)
{
    addMarkedUpToken(statement->forToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    addMarkedUpToken(statement->varToken, QLatin1String("keyword"));
    QQmlJS::AST::Node::accept(statement->declaration, this);
    addVerbatim(statement->inToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    QQmlJS::AST::Node::accept(statement->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ContinueStatement *statement)
{
    addMarkedUpToken(statement->continueToken, QLatin1String("keyword"));
    addMarkedUpToken(statement->identifierToken, QLatin1String("name"));
    addVerbatim(statement->semicolonToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::BreakStatement *statement)
{
    addMarkedUpToken(statement->breakToken, QLatin1String("keyword"));
    addMarkedUpToken(statement->identifierToken, QLatin1String("name"));
    addVerbatim(statement->semicolonToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ReturnStatement *statement)
{
    addMarkedUpToken(statement->returnToken, QLatin1String("keyword"));
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->semicolonToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::WithStatement *statement)
{
    addMarkedUpToken(statement->withToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    QQmlJS::AST::Node::accept(statement->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::CaseBlock *block)
{
    addVerbatim(block->lbraceToken);
    return true;
}

void QmlMarkupVisitor::endVisit(QQmlJS::AST::CaseBlock *block)
{
    addVerbatim(block->rbraceToken, block->rbraceToken);
}


bool QmlMarkupVisitor::visit(QQmlJS::AST::SwitchStatement *statement)
{
    addMarkedUpToken(statement->switchToken, QLatin1String("keyword"));
    addVerbatim(statement->lparenToken);
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->rparenToken);
    QQmlJS::AST::Node::accept(statement->block, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::CaseClause *clause)
{
    addMarkedUpToken(clause->caseToken, QLatin1String("keyword"));
    QQmlJS::AST::Node::accept(clause->expression, this);
    addVerbatim(clause->colonToken);
    QQmlJS::AST::Node::accept(clause->statements, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::DefaultClause *clause)
{
    addMarkedUpToken(clause->defaultToken, QLatin1String("keyword"));
    addVerbatim(clause->colonToken, clause->colonToken);
    return true;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::LabelledStatement *statement)
{
    addMarkedUpToken(statement->identifierToken, QLatin1String("name"));
    addVerbatim(statement->colonToken);
    QQmlJS::AST::Node::accept(statement->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::ThrowStatement *statement)
{
    addMarkedUpToken(statement->throwToken, QLatin1String("keyword"));
    QQmlJS::AST::Node::accept(statement->expression, this);
    addVerbatim(statement->semicolonToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::Catch *c)
{
    addMarkedUpToken(c->catchToken, QLatin1String("keyword"));
    addVerbatim(c->lparenToken);
    addMarkedUpToken(c->identifierToken, QLatin1String("name"));
    addVerbatim(c->rparenToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::Finally *f)
{
    addMarkedUpToken(f->finallyToken, QLatin1String("keyword"));
    QQmlJS::AST::Node::accept(f->statement, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::TryStatement *statement)
{
    addMarkedUpToken(statement->tryToken, QLatin1String("keyword"));
    QQmlJS::AST::Node::accept(statement->statement, this);
    QQmlJS::AST::Node::accept(statement->catchExpression, this);
    QQmlJS::AST::Node::accept(statement->finallyExpression, this);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::FunctionExpression *expression)
{
    addMarkedUpToken(expression->functionToken, QLatin1String("keyword"));
    addMarkedUpToken(expression->identifierToken, QLatin1String("name"));
    addVerbatim(expression->lparenToken);
    QQmlJS::AST::Node::accept(expression->formals, this);
    addVerbatim(expression->rparenToken);
    addVerbatim(expression->lbraceToken);
    QQmlJS::AST::Node::accept(expression->body, this);
    addVerbatim(expression->rbraceToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::FunctionDeclaration *declaration)
{
    addMarkedUpToken(declaration->functionToken, QLatin1String("keyword"));
    addMarkedUpToken(declaration->identifierToken, QLatin1String("name"));
    addVerbatim(declaration->lparenToken);
    QQmlJS::AST::Node::accept(declaration->formals, this);
    addVerbatim(declaration->rparenToken);
    addVerbatim(declaration->lbraceToken);
    QQmlJS::AST::Node::accept(declaration->body, this);
    addVerbatim(declaration->rbraceToken);
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::FormalParameterList *list)
{
    addVerbatim(list->commaToken);
    addMarkedUpToken(list->identifierToken, QLatin1String("name"));
    return false;
}

bool QmlMarkupVisitor::visit(QQmlJS::AST::DebuggerStatement *statement)
{
    addVerbatim(statement->debuggerToken);
    addVerbatim(statement->semicolonToken);
    return true;
}

// Elements and items are represented by UiObjectDefinition nodes.

bool QmlMarkupVisitor::visit(QQmlJS::AST::UiObjectDefinition *definition)
{
    QHash<QString, QString> attributes;
    addMarkedUpToken(definition->qualifiedTypeNameId->identifierToken, QLatin1String("type"));
    QQmlJS::AST::Node::accept(definition->initializer, this);
    return false;
}

QT_END_NAMESPACE
