/*
 *  Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef NodeInfo_h
#define NodeInfo_h

#include "Nodes.h"

namespace JSC {

    template <typename T> struct NodeInfo {
        T m_node;
        CodeFeatures m_features;
        int m_numConstants;
    };

    typedef NodeInfo<FuncDeclNode*> FuncDeclNodeInfo;    
    typedef NodeInfo<FuncExprNode*> FuncExprNodeInfo;
    typedef NodeInfo<ExpressionNode*> ExpressionNodeInfo;
    typedef NodeInfo<ArgumentsNode*> ArgumentsNodeInfo;
    typedef NodeInfo<ConstDeclNode*> ConstDeclNodeInfo;
    typedef NodeInfo<PropertyNode*> PropertyNodeInfo;
    typedef NodeInfo<PropertyList> PropertyListInfo;
    typedef NodeInfo<ElementList> ElementListInfo;
    typedef NodeInfo<ArgumentList> ArgumentListInfo;
    
    template <typename T> struct NodeDeclarationInfo {
        T m_node;
        ParserArenaData<DeclarationStacks::VarStack>* m_varDeclarations;
        ParserArenaData<DeclarationStacks::FunctionStack>* m_funcDeclarations;
        CodeFeatures m_features;
        int m_numConstants;
    };
    
    typedef NodeDeclarationInfo<StatementNode*> StatementNodeInfo;
    typedef NodeDeclarationInfo<CaseBlockNode*> CaseBlockNodeInfo;
    typedef NodeDeclarationInfo<CaseClauseNode*> CaseClauseNodeInfo;
    typedef NodeDeclarationInfo<SourceElements*> SourceElementsInfo;
    typedef NodeDeclarationInfo<ClauseList> ClauseListInfo;
    typedef NodeDeclarationInfo<ExpressionNode*> VarDeclListInfo;
    typedef NodeDeclarationInfo<ConstDeclList> ConstDeclListInfo;
    typedef NodeDeclarationInfo<ParameterList> ParameterListInfo;

} // namespace JSC

#endif // NodeInfo_h
