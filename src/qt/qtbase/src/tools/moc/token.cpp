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

#include "token.h"

QT_BEGIN_NAMESPACE

#if defined(DEBUG_MOC)
const char *tokenTypeName(Token t)
{
    switch (t) {
        case NOTOKEN: return "NOTOKEN";
        case IDENTIFIER: return "IDENTIFIER";
        case INTEGER_LITERAL: return "INTEGER_LITERAL";
        case CHARACTER_LITERAL: return "CHARACTER_LITERAL";
        case STRING_LITERAL: return "STRING_LITERAL";
        case BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
        case HEADER_NAME: return "HEADER_NAME";
        case LANGLE: return "LANGLE";
        case RANGLE: return "RANGLE";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case ELIPSIS: return "ELIPSIS";
        case LBRACK: return "LBRACK";
        case RBRACK: return "RBRACK";
        case LBRACE: return "LBRACE";
        case RBRACE: return "RBRACE";
        case EQ: return "EQ";
        case SCOPE: return "SCOPE";
        case SEMIC: return "SEMIC";
        case COLON: return "COLON";
        case DOTSTAR: return "DOTSTAR";
        case QUESTION: return "QUESTION";
        case DOT: return "DOT";
        case DYNAMIC_CAST: return "DYNAMIC_CAST";
        case STATIC_CAST: return "STATIC_CAST";
        case REINTERPRET_CAST: return "REINTERPRET_CAST";
        case CONST_CAST: return "CONST_CAST";
        case TYPEID: return "TYPEID";
        case THIS: return "THIS";
        case TEMPLATE: return "TEMPLATE";
        case THROW: return "THROW";
        case TRY: return "TRY";
        case CATCH: return "CATCH";
        case TYPEDEF: return "TYPEDEF";
        case FRIEND: return "FRIEND";
        case CLASS: return "CLASS";
        case NAMESPACE: return "NAMESPACE";
        case ENUM: return "ENUM";
        case STRUCT: return "STRUCT";
        case UNION: return "UNION";
        case VIRTUAL: return "VIRTUAL";
        case PRIVATE: return "PRIVATE";
        case PROTECTED: return "PROTECTED";
        case PUBLIC: return "PUBLIC";
        case EXPORT: return "EXPORT";
        case AUTO: return "AUTO";
        case REGISTER: return "REGISTER";
        case EXTERN: return "EXTERN";
        case MUTABLE: return "MUTABLE";
        case ASM: return "ASM";
        case USING: return "USING";
        case INLINE: return "INLINE";
        case EXPLICIT: return "EXPLICIT";
        case STATIC: return "STATIC";
        case CONST: return "CONST";
        case VOLATILE: return "VOLATILE";
        case OPERATOR: return "OPERATOR";
        case SIZEOF: return "SIZEOF";
        case NEW: return "NEW";
        case DELETE: return "DELETE";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case STAR: return "STAR";
        case SLASH: return "SLASH";
        case PERCENT: return "PERCENT";
        case HAT: return "HAT";
        case AND: return "AND";
        case OR: return "OR";
        case TILDE: return "TILDE";
        case NOT: return "NOT";
        case PLUS_EQ: return "PLUS_EQ";
        case MINUS_EQ: return "MINUS_EQ";
        case STAR_EQ: return "STAR_EQ";
        case SLASH_EQ: return "SLASH_EQ";
        case PERCENT_EQ: return "PERCENT_EQ";
        case HAT_EQ: return "HAT_EQ";
        case AND_EQ: return "AND_EQ";
        case OR_EQ: return "OR_EQ";
        case LTLT: return "LTLT";
        case GTGT: return "GTGT";
        case GTGT_EQ: return "GTGT_EQ";
        case LTLT_EQ: return "LTLT_EQ";
        case EQEQ: return "EQEQ";
        case NE: return "NE";
        case LE: return "LE";
        case GE: return "GE";
        case ANDAND: return "ANDAND";
        case OROR: return "OROR";
        case INCR: return "INCR";
        case DECR: return "DECR";
        case COMMA: return "COMMA";
        case ARROW_STAR: return "ARROW_STAR";
        case ARROW: return "ARROW";
        case CHAR: return "CHAR";
        case WCHAR: return "WCHAR";
        case BOOL: return "BOOL";
        case SHORT: return "SHORT";
        case INT: return "INT";
        case LONG: return "LONG";
        case SIGNED: return "SIGNED";
        case UNSIGNED: return "UNSIGNED";
        case FLOAT: return "FLOAT";
        case DOUBLE: return "DOUBLE";
        case VOID: return "VOID";
        case CASE: return "CASE";
        case DEFAULT: return "DEFAULT";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case SWITCH: return "SWITCH";
        case WHILE: return "WHILE";
        case DO: return "DO";
        case FOR: return "FOR";
        case BREAK: return "BREAK";
        case CONTINUE: return "CONTINUE";
        case GOTO: return "GOTO";
        case SIGNALS: return "SIGNALS";
        case SLOTS: return "SLOTS";
        case RETURN: return "RETURN";
        case Q_OBJECT_TOKEN: return "Q_OBJECT_TOKEN";
        case Q_GADGET_TOKEN: return "Q_GADGET_TOKEN";
        case Q_PROPERTY_TOKEN: return "Q_PROPERTY_TOKEN";
        case Q_ENUMS_TOKEN: return "Q_ENUMS_TOKEN";
        case Q_FLAGS_TOKEN: return "Q_FLAGS_TOKEN";
        case Q_DECLARE_FLAGS_TOKEN: return "Q_DECLARE_FLAGS_TOKEN";
        case Q_DECLARE_INTERFACE_TOKEN: return "Q_DECLARE_INTERFACE_TOKEN";
        case Q_CLASSINFO_TOKEN: return "Q_CLASSINFO_TOKEN";
        case Q_INTERFACES_TOKEN: return "Q_INTERFACES_TOKEN";
        case Q_SIGNALS_TOKEN: return "Q_SIGNALS_TOKEN";
        case Q_SLOTS_TOKEN: return "Q_SLOTS_TOKEN";
        case Q_SIGNAL_TOKEN: return "Q_SIGNAL_TOKEN";
        case Q_SLOT_TOKEN: return "Q_SLOT_TOKEN";
        case Q_PRIVATE_SLOT_TOKEN: return "Q_PRIVATE_SLOT_TOKEN";
        case Q_PRIVATE_PROPERTY_TOKEN: return "Q_PRIVATE_PROPERTY_TOKEN";
        case Q_REVISION_TOKEN: return "Q_REVISION_TOKEN";
        case SPECIAL_TREATMENT_MARK: return "SPECIAL_TREATMENT_MARK";
        case MOC_INCLUDE_BEGIN: return "MOC_INCLUDE_BEGIN";
        case MOC_INCLUDE_END: return "MOC_INCLUDE_END";
        case CPP_COMMENT: return "CPP_COMMENT";
        case C_COMMENT: return "C_COMMENT";
        case FLOATING_LITERAL: return "FLOATING_LITERAL";
        case HASH: return "HASH";
        case QUOTE: return "QUOTE";
        case SINGLEQUOTE: return "SINGLEQUOTE";
        case DIGIT: return "DIGIT";
        case CHARACTER: return "CHARACTER";
        case NEWLINE: return "NEWLINE";
        case WHITESPACE: return "WHITESPACE";
        case BACKSLASH: return "BACKSLASH";
        case INCOMPLETE: return "INCOMPLETE";
        case PP_DEFINE: return "PP_DEFINE";
        case PP_UNDEF: return "PP_UNDEF";
        case PP_IF: return "PP_IF";
        case PP_IFDEF: return "PP_IFDEF";
        case PP_IFNDEF: return "PP_IFNDEF";
        case PP_ELIF: return "PP_ELIF";
        case PP_ELSE: return "PP_ELSE";
        case PP_ENDIF: return "PP_ENDIF";
        case PP_INCLUDE: return "PP_INCLUDE";
        case PP_HASHHASH: return "PP_HASHHASH";
        case PP_HASH: return "PP_HASH";
        case PP_DEFINED: return "PP_DEFINED";
        case PP_INCOMPLETE: return "PP_INCOMPLETE";
        case PP_MOC_TRUE: return "PP_MOC_TRUE";
        case PP_MOC_FALSE: return "PP_MOC_FALSE";
        case Q_DECLARE_METATYPE_TOKEN: return "Q_DECLARE_METATYPE_TOKEN";
        case Q_MOC_COMPAT_TOKEN: return "Q_MOC_COMPAT_TOKEN";
        case Q_INVOKABLE_TOKEN: return "Q_INVOKABLE_TOKEN";
        case Q_SCRIPTABLE_TOKEN: return "Q_SCRIPTABLE_TOKEN";
    }
    return "";
}
#endif

QT_END_NAMESPACE
