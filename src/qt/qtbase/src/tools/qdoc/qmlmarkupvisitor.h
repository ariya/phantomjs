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

#ifndef QMLMARKUPVISITOR_H
#define QMLMARKUPVISITOR_H

#include <qstring.h>
#include "qqmljsastvisitor_p.h"
#include "node.h"
#include "tree.h"

QT_BEGIN_NAMESPACE

class QmlMarkupVisitor : public QQmlJS::AST::Visitor
{
public:
    enum ExtraType{
        Comment,
        Pragma
    };

    QmlMarkupVisitor(const QString &code,
                     const QList<QQmlJS::AST::SourceLocation> &pragmas,
                     QQmlJS::Engine *engine);
    virtual ~QmlMarkupVisitor();

    QString markedUpCode();

    virtual bool visit(QQmlJS::AST::UiImport *);
    virtual void endVisit(QQmlJS::AST::UiImport *);

    virtual bool visit(QQmlJS::AST::UiPublicMember *);
    virtual bool visit(QQmlJS::AST::UiObjectDefinition *);

    virtual bool visit(QQmlJS::AST::UiObjectInitializer *);
    virtual void endVisit(QQmlJS::AST::UiObjectInitializer *);

    virtual bool visit(QQmlJS::AST::UiObjectBinding *);
    virtual bool visit(QQmlJS::AST::UiScriptBinding *);
    virtual bool visit(QQmlJS::AST::UiArrayBinding *);
    virtual bool visit(QQmlJS::AST::UiArrayMemberList *);
    virtual bool visit(QQmlJS::AST::UiQualifiedId *);

    virtual bool visit(QQmlJS::AST::ThisExpression *);
    virtual bool visit(QQmlJS::AST::IdentifierExpression *);
    virtual bool visit(QQmlJS::AST::NullExpression *);
    virtual bool visit(QQmlJS::AST::TrueLiteral *);
    virtual bool visit(QQmlJS::AST::FalseLiteral *);
    virtual bool visit(QQmlJS::AST::NumericLiteral *);
    virtual bool visit(QQmlJS::AST::StringLiteral *);
    virtual bool visit(QQmlJS::AST::RegExpLiteral *);
    virtual bool visit(QQmlJS::AST::ArrayLiteral *);

    virtual bool visit(QQmlJS::AST::ObjectLiteral *);
    virtual void endVisit(QQmlJS::AST::ObjectLiteral *);

    virtual bool visit(QQmlJS::AST::ElementList *);
    virtual bool visit(QQmlJS::AST::Elision *);
    virtual bool visit(QQmlJS::AST::PropertyNameAndValue *);
    virtual bool visit(QQmlJS::AST::ArrayMemberExpression *);
    virtual bool visit(QQmlJS::AST::FieldMemberExpression *);
    virtual bool visit(QQmlJS::AST::NewMemberExpression *);
    virtual bool visit(QQmlJS::AST::NewExpression *);
    virtual bool visit(QQmlJS::AST::ArgumentList *);
    virtual bool visit(QQmlJS::AST::PostIncrementExpression *);
    virtual bool visit(QQmlJS::AST::PostDecrementExpression *);
    virtual bool visit(QQmlJS::AST::DeleteExpression *);
    virtual bool visit(QQmlJS::AST::VoidExpression *);
    virtual bool visit(QQmlJS::AST::TypeOfExpression *);
    virtual bool visit(QQmlJS::AST::PreIncrementExpression *);
    virtual bool visit(QQmlJS::AST::PreDecrementExpression *);
    virtual bool visit(QQmlJS::AST::UnaryPlusExpression *);
    virtual bool visit(QQmlJS::AST::UnaryMinusExpression *);
    virtual bool visit(QQmlJS::AST::TildeExpression *);
    virtual bool visit(QQmlJS::AST::NotExpression *);
    virtual bool visit(QQmlJS::AST::BinaryExpression *);
    virtual bool visit(QQmlJS::AST::ConditionalExpression *);
    virtual bool visit(QQmlJS::AST::Expression *);

    virtual bool visit(QQmlJS::AST::Block *);
    virtual void endVisit(QQmlJS::AST::Block *);

    virtual bool visit(QQmlJS::AST::VariableStatement *);
    virtual bool visit(QQmlJS::AST::VariableDeclarationList *);
    virtual bool visit(QQmlJS::AST::VariableDeclaration *);
    virtual bool visit(QQmlJS::AST::EmptyStatement *);
    virtual bool visit(QQmlJS::AST::ExpressionStatement *);
    virtual bool visit(QQmlJS::AST::IfStatement *);
    virtual bool visit(QQmlJS::AST::DoWhileStatement *);
    virtual bool visit(QQmlJS::AST::WhileStatement *);
    virtual bool visit(QQmlJS::AST::ForStatement *);
    virtual bool visit(QQmlJS::AST::LocalForStatement *);
    virtual bool visit(QQmlJS::AST::ForEachStatement *);
    virtual bool visit(QQmlJS::AST::LocalForEachStatement *);
    virtual bool visit(QQmlJS::AST::ContinueStatement *);
    virtual bool visit(QQmlJS::AST::BreakStatement *);
    virtual bool visit(QQmlJS::AST::ReturnStatement *);
    virtual bool visit(QQmlJS::AST::WithStatement *);

    virtual bool visit(QQmlJS::AST::CaseBlock *);
    virtual void endVisit(QQmlJS::AST::CaseBlock *);

    virtual bool visit(QQmlJS::AST::SwitchStatement *);
    virtual bool visit(QQmlJS::AST::CaseClause *);
    virtual bool visit(QQmlJS::AST::DefaultClause *);
    virtual bool visit(QQmlJS::AST::LabelledStatement *);
    virtual bool visit(QQmlJS::AST::ThrowStatement *);
    virtual bool visit(QQmlJS::AST::TryStatement *);
    virtual bool visit(QQmlJS::AST::Catch *);
    virtual bool visit(QQmlJS::AST::Finally *);
    virtual bool visit(QQmlJS::AST::FunctionDeclaration *);
    virtual bool visit(QQmlJS::AST::FunctionExpression *);
    virtual bool visit(QQmlJS::AST::FormalParameterList *);
    virtual bool visit(QQmlJS::AST::DebuggerStatement *);

protected:
    QString protect(const QString &string);

private:
    typedef QHash<QString, QString> StringHash;
    void addExtra(quint32 start, quint32 finish);
    void addMarkedUpToken(QQmlJS::AST::SourceLocation &location,
                          const QString &text,
                          const StringHash &attributes = StringHash());
    void addVerbatim(QQmlJS::AST::SourceLocation first,
                     QQmlJS::AST::SourceLocation last = QQmlJS::AST::SourceLocation());
    QString sourceText(QQmlJS::AST::SourceLocation &location);

    QQmlJS::Engine *engine;
    QList<ExtraType> extraTypes;
    QList<QQmlJS::AST::SourceLocation> extraLocations;
    QString source;
    QString output;
    quint32 cursor;
    int extraIndex;
};

QT_END_NAMESPACE

#endif
