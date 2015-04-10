/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qqmljsast_p.h"

#include "qqmljsastvisitor_p.h"

QT_QML_BEGIN_NAMESPACE

namespace QQmlJS { namespace AST {

void Node::accept(Visitor *visitor)
{
    if (visitor->preVisit(this)) {
        accept0(visitor);
    }
    visitor->postVisit(this);
}

void Node::accept(Node *node, Visitor *visitor)
{
    if (node)
        node->accept(visitor);
}

ExpressionNode *Node::expressionCast()
{
    return 0;
}

BinaryExpression *Node::binaryExpressionCast()
{
    return 0;
}

Statement *Node::statementCast()
{
    return 0;
}

UiObjectMember *Node::uiObjectMemberCast()
{
    return 0;
}

ExpressionNode *ExpressionNode::expressionCast()
{
    return this;
}

BinaryExpression *BinaryExpression::binaryExpressionCast()
{
    return this;
}

Statement *Statement::statementCast()
{
    return this;
}

UiObjectMember *UiObjectMember::uiObjectMemberCast()
{
    return this;
}

void NestedExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }
    visitor->endVisit(this);
}

void ThisExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void IdentifierExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NullExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void TrueLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void FalseLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void StringLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NumericLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void RegExpLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ArrayLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(elements, visitor);
        accept(elision, visitor);
    }

    visitor->endVisit(this);
}

void ObjectLiteral::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(properties, visitor);
    }

    visitor->endVisit(this);
}

void ElementList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (ElementList *it = this; it; it = it->next) {
            accept(it->elision, visitor);
            accept(it->expression, visitor);
        }
    }

    visitor->endVisit(this);
}

void Elision::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        // ###
    }

    visitor->endVisit(this);
}

void PropertyNameAndValue::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
        accept(value, visitor);
    }

    visitor->endVisit(this);
}

void PropertyGetterSetter::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(name, visitor);
        accept(formals, visitor);
        accept(functionBody, visitor);
    }

    visitor->endVisit(this);
}

void PropertyAssignmentList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (PropertyAssignmentList *it = this; it; it = it->next) {
            accept(it->assignment, visitor);
        }
    }

    visitor->endVisit(this);
}

void IdentifierPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void StringLiteralPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void NumericLiteralPropertyName::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ArrayMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void FieldMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
    }

    visitor->endVisit(this);
}

void NewMemberExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
        accept(arguments, visitor);
    }

    visitor->endVisit(this);
}

void NewExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void CallExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
        accept(arguments, visitor);
    }

    visitor->endVisit(this);
}

void ArgumentList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (ArgumentList *it = this; it; it = it->next) {
            accept(it->expression, visitor);
        }
    }

    visitor->endVisit(this);
}

void PostIncrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
    }

    visitor->endVisit(this);
}

void PostDecrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(base, visitor);
    }

    visitor->endVisit(this);
}

void DeleteExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void VoidExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void TypeOfExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void PreIncrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void PreDecrementExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void UnaryPlusExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void UnaryMinusExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void TildeExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void NotExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void BinaryExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left, visitor);
        accept(right, visitor);
    }

    visitor->endVisit(this);
}

void ConditionalExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(ok, visitor);
        accept(ko, visitor);
    }

    visitor->endVisit(this);
}

void Expression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(left, visitor);
        accept(right, visitor);
    }

    visitor->endVisit(this);
}

void Block::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statements, visitor);
    }

    visitor->endVisit(this);
}

void StatementList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (StatementList *it = this; it; it = it->next) {
            accept(it->statement, visitor);
        }
    }

    visitor->endVisit(this);
}

void VariableStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarations, visitor);
    }

    visitor->endVisit(this);
}

void VariableDeclarationList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (VariableDeclarationList *it = this; it; it = it->next) {
            accept(it->declaration, visitor);
        }
    }

    visitor->endVisit(this);
}

void VariableDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void EmptyStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ExpressionStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void IfStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(ok, visitor);
        accept(ko, visitor);
    }

    visitor->endVisit(this);
}

void DoWhileStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void WhileStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void ForStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(initialiser, visitor);
        accept(condition, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void LocalForStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declarations, visitor);
        accept(condition, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void ForEachStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(initialiser, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void LocalForEachStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declaration, visitor);
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void ContinueStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void BreakStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void ReturnStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void WithStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void SwitchStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(block, visitor);
    }

    visitor->endVisit(this);
}

void CaseBlock::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(clauses, visitor);
        accept(defaultClause, visitor);
        accept(moreClauses, visitor);
    }

    visitor->endVisit(this);
}

void CaseClauses::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (CaseClauses *it = this; it; it = it->next) {
            accept(it->clause, visitor);
        }
    }

    visitor->endVisit(this);
}

void CaseClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
        accept(statements, visitor);
    }

    visitor->endVisit(this);
}

void DefaultClause::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statements, visitor);
    }

    visitor->endVisit(this);
}

void LabelledStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void ThrowStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(expression, visitor);
    }

    visitor->endVisit(this);
}

void TryStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(catchExpression, visitor);
        accept(finallyExpression, visitor);
    }

    visitor->endVisit(this);
}

void Catch::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void Finally::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void FunctionDeclaration::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(formals, visitor);
        accept(body, visitor);
    }

    visitor->endVisit(this);
}

void FunctionExpression::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(formals, visitor);
        accept(body, visitor);
    }

    visitor->endVisit(this);
}

void FormalParameterList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        // ###
    }

    visitor->endVisit(this);
}

void FunctionBody::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(elements, visitor);
    }

    visitor->endVisit(this);
}

void Program::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(elements, visitor);
    }

    visitor->endVisit(this);
}

void SourceElements::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (SourceElements *it = this; it; it = it->next) {
            accept(it->element, visitor);
        }
    }

    visitor->endVisit(this);
}

void FunctionSourceElement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(declaration, visitor);
    }

    visitor->endVisit(this);
}

void StatementSourceElement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void DebuggerStatement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void UiProgram::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(headers, visitor);
        accept(members, visitor);
    }

    visitor->endVisit(this);
}

void UiPublicMember::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(statement, visitor);
        accept(binding, visitor);
    }

    visitor->endVisit(this);
}

void UiObjectDefinition::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(qualifiedTypeNameId, visitor);
        accept(initializer, visitor);
    }

    visitor->endVisit(this);
}

void UiObjectInitializer::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(members, visitor);
    }

    visitor->endVisit(this);
}

void UiParameterList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }
    visitor->endVisit(this);
}

void UiObjectBinding::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(qualifiedId, visitor);
        accept(qualifiedTypeNameId, visitor);
        accept(initializer, visitor);
    }

    visitor->endVisit(this);
}

void UiScriptBinding::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(qualifiedId, visitor);
        accept(statement, visitor);
    }

    visitor->endVisit(this);
}

void UiArrayBinding::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(qualifiedId, visitor);
        accept(members, visitor);
    }

    visitor->endVisit(this);
}

void UiObjectMemberList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (UiObjectMemberList *it = this; it; it = it->next)
            accept(it->member, visitor);
    }

    visitor->endVisit(this);
}

void UiArrayMemberList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        for (UiArrayMemberList *it = this; it; it = it->next)
            accept(it->member, visitor);
    }

    visitor->endVisit(this);
}

void UiQualifiedId::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void UiImport::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(importUri, visitor);
    }

    visitor->endVisit(this);
}

void UiQualifiedPragmaId::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
    }

    visitor->endVisit(this);
}

void UiPragma::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(pragmaType, visitor);
    }

    visitor->endVisit(this);
}

void UiHeaderItemList::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(headerItem, visitor);
        accept(next, visitor);
    }

    visitor->endVisit(this);
}


void UiSourceElement::accept0(Visitor *visitor)
{
    if (visitor->visit(this)) {
        accept(sourceElement, visitor);
    }

    visitor->endVisit(this);
}

} } // namespace QQmlJS::AST

QT_QML_END_NAMESPACE


