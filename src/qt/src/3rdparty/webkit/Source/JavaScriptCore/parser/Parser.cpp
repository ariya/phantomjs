/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Parser.h"

#include "Debugger.h"
#include "JSParser.h"
#include "Lexer.h"

namespace JSC {

void Parser::parse(JSGlobalData* globalData, FunctionParameters* parameters, JSParserStrictness strictness, JSParserMode mode, int* errLine, UString* errMsg)
{
    ASSERT(globalData);
    m_sourceElements = 0;

    int defaultErrLine;
    UString defaultErrMsg;

    if (!errLine)
        errLine = &defaultErrLine;
    if (!errMsg)
        errMsg = &defaultErrMsg;

    *errLine = -1;
    *errMsg = UString();

    Lexer& lexer = *globalData->lexer;
    lexer.setCode(*m_source, m_arena);

    const char* parseError = jsParse(globalData, parameters, strictness, mode, m_source);
    int lineNumber = lexer.lineNumber();
    bool lexError = lexer.sawError();
    lexer.clear();

    if (parseError || lexError) {
        *errLine = lineNumber;
        *errMsg = parseError ? parseError : "Parse error";
        m_sourceElements = 0;
    }
}

void Parser::didFinishParsing(SourceElements* sourceElements, ParserArenaData<DeclarationStacks::VarStack>* varStack, 
                              ParserArenaData<DeclarationStacks::FunctionStack>* funcStack, CodeFeatures features, int lastLine, int numConstants, IdentifierSet& capturedVars)
{
    m_sourceElements = sourceElements;
    m_varDeclarations = varStack;
    m_funcDeclarations = funcStack;
    m_capturedVariables.swap(capturedVars);
    m_features = features;
    m_lastLine = lastLine;
    m_numConstants = numConstants;
}

} // namespace JSC
