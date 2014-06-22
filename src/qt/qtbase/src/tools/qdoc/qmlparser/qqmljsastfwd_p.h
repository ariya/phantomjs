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

#ifndef QQMLJSAST_FWD_P_H
#define QQMLJSAST_FWD_P_H

#include "qqmljsglobal_p.h"

#include <QtCore/qglobal.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_QML_BEGIN_NAMESPACE

namespace QQmlJS { namespace AST {

class SourceLocation
{
public:
    SourceLocation(quint32 offset = 0, quint32 length = 0, quint32 line = 0, quint32 column = 0)
        : offset(offset), length(length),
          startLine(line), startColumn(column)
    { }

    bool isValid() const { return length != 0; }

    quint32 begin() const { return offset; }
    quint32 end() const { return offset + length; }

// attributes
    // ### encode
    quint32 offset;
    quint32 length;
    quint32 startLine;
    quint32 startColumn;
};

class Visitor;
class Node;
class ExpressionNode;
class Statement;
class ThisExpression;
class IdentifierExpression;
class NullExpression;
class TrueLiteral;
class FalseLiteral;
class NumericLiteral;
class StringLiteral;
class RegExpLiteral;
class ArrayLiteral;
class ObjectLiteral;
class ElementList;
class Elision;
class PropertyAssignmentList;
class PropertyGetterSetter;
class PropertyNameAndValue;
class PropertyName;
class IdentifierPropertyName;
class StringLiteralPropertyName;
class NumericLiteralPropertyName;
class ArrayMemberExpression;
class FieldMemberExpression;
class NewMemberExpression;
class NewExpression;
class CallExpression;
class ArgumentList;
class PostIncrementExpression;
class PostDecrementExpression;
class DeleteExpression;
class VoidExpression;
class TypeOfExpression;
class PreIncrementExpression;
class PreDecrementExpression;
class UnaryPlusExpression;
class UnaryMinusExpression;
class TildeExpression;
class NotExpression;
class BinaryExpression;
class ConditionalExpression;
class Expression; // ### rename
class Block;
class StatementList;
class VariableStatement;
class VariableDeclarationList;
class VariableDeclaration;
class EmptyStatement;
class ExpressionStatement;
class IfStatement;
class DoWhileStatement;
class WhileStatement;
class ForStatement;
class LocalForStatement;
class ForEachStatement;
class LocalForEachStatement;
class ContinueStatement;
class BreakStatement;
class ReturnStatement;
class WithStatement;
class SwitchStatement;
class CaseBlock;
class CaseClauses;
class CaseClause;
class DefaultClause;
class LabelledStatement;
class ThrowStatement;
class TryStatement;
class Catch;
class Finally;
class FunctionDeclaration;
class FunctionExpression;
class FormalParameterList;
class FunctionBody;
class Program;
class SourceElements;
class SourceElement;
class FunctionSourceElement;
class StatementSourceElement;
class DebuggerStatement;
class NestedExpression;

// ui elements
class UiProgram;
class UiPragma;
class UiImport;
class UiPublicMember;
class UiParameterList;
class UiObjectDefinition;
class UiObjectInitializer;
class UiObjectBinding;
class UiScriptBinding;
class UiSourceElement;
class UiArrayBinding;
class UiObjectMember;
class UiObjectMemberList;
class UiArrayMemberList;
class UiQualifiedId;
class UiQualifiedPragmaId;
class UiHeaderItemList;

} } // namespace AST

QT_QML_END_NAMESPACE

#endif
