/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

function FormattedContentBuilder(content, mapping, originalOffset, formattedOffset, indentString)
{
    this._originalContent = content;
    this._originalOffset = originalOffset;
    this._lastOriginalPosition = 0;

    this._formattedContent = [];
    this._formattedContentLength = 0;
    this._formattedOffset = formattedOffset;
    this._lastFormattedPosition = 0;

    this._mapping = mapping;

    this._lineNumber = 0;
    this._nestingLevel = 0;
    this._indentString = indentString;
    this._cachedIndents = {};
}

FormattedContentBuilder.prototype = {
    addToken: function(token)
    {
        for (var i = 0; i < token.comments_before.length; ++i)
            this._addComment(token.comments_before[i]);

        while (this._lineNumber < token.line) {
            this._addText("\n");
            this._addIndent();
            this._needNewLine = false;
            this._lineNumber += 1;
        }

        if (this._needNewLine) {
            this._addText("\n");
            this._addIndent();
            this._needNewLine = false;
        }

        this._addMappingIfNeeded(token.pos);
        this._addText(this._originalContent.substring(token.pos, token.endPos));
        this._lineNumber = token.endLine;
    },

    addSpace: function()
    {
        this._addText(" ");
    },

    addNewLine: function()
    {
        this._needNewLine = true;
    },

    increaseNestingLevel: function()
    {
        this._nestingLevel += 1;
    },

    decreaseNestingLevel: function()
    {
        this._nestingLevel -= 1;
    },

    content: function()
    {
        return this._formattedContent.join("");
    },

    mapping: function()
    {
        return { original: this._originalPositions, formatted: this._formattedPositions };
    },

    _addIndent: function()
    {
        if (this._cachedIndents[this._nestingLevel]) {
            this._addText(this._cachedIndents[this._nestingLevel]);
            return;
        }

        var fullIndent = "";
        for (var i = 0; i < this._nestingLevel; ++i)
            fullIndent += this._indentString;
        this._addText(fullIndent);

        // Cache a maximum of 20 nesting level indents.
        if (this._nestingLevel <= 20)
            this._cachedIndents[this._nestingLevel] = fullIndent;
    },

    _addComment: function(comment)
    {
        if (this._lineNumber < comment.line) {
            for (var j = this._lineNumber; j < comment.line; ++j)
                this._addText("\n");
            this._lineNumber = comment.line;
            this._needNewLine = false;
            this._addIndent();
        } else
            this.addSpace();

        this._addMappingIfNeeded(comment.pos);
        if (comment.type === "comment1")
            this._addText("//");
        else
            this._addText("/*");

        this._addText(comment.value);

        if (comment.type !== "comment1") {
            this._addText("*/");
            var position;
            while ((position = comment.value.indexOf("\n", position + 1)) !== -1)
                this._lineNumber += 1;
        }
    },

    _addText: function(text)
    {
        this._formattedContent.push(text);
        this._formattedContentLength += text.length;
    },

    _addMappingIfNeeded: function(originalPosition)
    {
        if (originalPosition - this._lastOriginalPosition === this._formattedContentLength - this._lastFormattedPosition)
            return;
        this._mapping.original.push(this._originalOffset + originalPosition);
        this._lastOriginalPosition = originalPosition;
        this._mapping.formatted.push(this._formattedOffset + this._formattedContentLength);
        this._lastFormattedPosition = this._formattedContentLength;
    }
}

var tokens = [
    ["EOS"],
    ["LPAREN", "("], ["RPAREN", ")"], ["LBRACK", "["], ["RBRACK", "]"], ["LBRACE", "{"], ["RBRACE", "}"], ["COLON", ":"], ["SEMICOLON", ";"], ["PERIOD", "."], ["CONDITIONAL", "?"],
    ["INC", "++"], ["DEC", "--"],
    ["ASSIGN", "="], ["ASSIGN_BIT_OR", "|="], ["ASSIGN_BIT_XOR", "^="], ["ASSIGN_BIT_AND", "&="], ["ASSIGN_SHL", "<<="], ["ASSIGN_SAR", ">>="], ["ASSIGN_SHR", ">>>="],
    ["ASSIGN_ADD", "+="], ["ASSIGN_SUB", "-="], ["ASSIGN_MUL", "*="], ["ASSIGN_DIV", "/="], ["ASSIGN_MOD", "%="],
    ["COMMA", ","], ["OR", "||"], ["AND", "&&"], ["BIT_OR", "|"], ["BIT_XOR", "^"], ["BIT_AND", "&"], ["SHL", "<<"], ["SAR", ">>"], ["SHR", ">>>"],
    ["ADD", "+"], ["SUB", "-"], ["MUL", "*"], ["DIV", "/"], ["MOD", "%"],
    ["EQ", "=="], ["NE", "!="], ["EQ_STRICT", "==="], ["NE_STRICT", "!=="], ["LT", "<"], ["GT", ">"], ["LTE", "<="], ["GTE", ">="],
    ["INSTANCEOF", "instanceof"], ["IN", "in"], ["NOT", "!"], ["BIT_NOT", "~"], ["DELETE", "delete"], ["TYPEOF", "typeof"], ["VOID", "void"],
    ["BREAK", "break"], ["CASE", "case"], ["CATCH", "catch"], ["CONTINUE", "continue"], ["DEBUGGER", "debugger"], ["DEFAULT", "default"], ["DO", "do"], ["ELSE", "else"], ["FINALLY", "finally"],
    ["FOR", "for"], ["FUNCTION", "function"], ["IF", "if"], ["NEW", "new"], ["RETURN", "return"], ["SWITCH", "switch"], ["THIS", "this"], ["THROW", "throw"], ["TRY", "try"], ["VAR", "var"],
    ["WHILE", "while"], ["WITH", "with"], ["NULL_LITERAL", "null"], ["TRUE_LITERAL", "true"], ["FALSE_LITERAL", "false"], ["NUMBER"], ["STRING"], ["IDENTIFIER"], ["CONST", "const"]
];

var Tokens = {};
for (var i = 0; i < tokens.length; ++i)
    Tokens[tokens[i][0]] = i;

var TokensByValue = {};
for (var i = 0; i < tokens.length; ++i) {
    if (tokens[i][1])
        TokensByValue[tokens[i][1]] = i;
}

var TokensByType = {
    "eof": Tokens.EOS,
    "name": Tokens.IDENTIFIER,
    "num": Tokens.NUMBER,
    "regexp": Tokens.DIV,
    "string": Tokens.STRING
};

function Tokenizer(content)
{
    this._readNextToken = parse.tokenizer(content);
    this._state = this._readNextToken.context();
}

Tokenizer.prototype = {
    content: function()
    {
        return this._state.text;
    },

    next: function(forceRegexp)
    {
        var uglifyToken = this._readNextToken(forceRegexp);
        uglifyToken.endPos = this._state.pos;
        uglifyToken.endLine = this._state.line;
        uglifyToken.token = this._convertUglifyToken(uglifyToken);
        return uglifyToken;
    },

    _convertUglifyToken: function(uglifyToken)
    {
        var token = TokensByType[uglifyToken.type];
        if (typeof token === "number")
            return token;
        token = TokensByValue[uglifyToken.value];
        if (typeof token === "number")
            return token;
        throw "Unknown token type " + uglifyToken.type;
    }
}

function JavaScriptFormatter(tokenizer, builder)
{
    this._tokenizer = tokenizer;
    this._builder = builder;
    this._token = null;
    this._nextToken = this._tokenizer.next();
}

JavaScriptFormatter.prototype = {
    format: function()
    {
        this._parseSourceElements(Tokens.EOS);
        this._consume(Tokens.EOS);
    },

    _peek: function()
    {
        return this._nextToken.token;
    },

    _next: function()
    {
        if (this._token && this._token.token === Tokens.EOS)
            throw "Unexpected EOS token";

        this._builder.addToken(this._nextToken);
        this._token = this._nextToken;
        this._nextToken = this._tokenizer.next(this._forceRegexp);
        this._forceRegexp = false;
        return this._token.token;
    },

    _consume: function(token)
    {
        var next = this._next();
        if (next !== token)
            throw "Unexpected token in consume: expected " + token + ", actual " + next;
    },

    _expect: function(token)
    {
        var next = this._next();
        if (next !== token)
            throw "Unexpected token: expected " + token + ", actual " + next;
    },

    _expectSemicolon: function()
    {
        if (this._peek() === Tokens.SEMICOLON)
            this._consume(Tokens.SEMICOLON);
    },

    _hasLineTerminatorBeforeNext: function()
    {
        return this._nextToken.nlb;
    },

    _parseSourceElements: function(endToken)
    {
        while (this._peek() !== endToken) {
            this._parseStatement();
            this._builder.addNewLine();
        }
    },

    _parseStatementOrBlock: function()
    {
        if (this._peek() === Tokens.LBRACE) {
            this._builder.addSpace();
            this._parseBlock();
            return true;
        }

        this._builder.addNewLine();
        this._builder.increaseNestingLevel();
        this._parseStatement();
        this._builder.decreaseNestingLevel();
    },

    _parseStatement: function()
    {
        switch (this._peek()) {
        case Tokens.LBRACE:
            return this._parseBlock();
        case Tokens.CONST:
        case Tokens.VAR:
            return this._parseVariableStatement();
        case Tokens.SEMICOLON:
            return this._next();
        case Tokens.IF:
            return this._parseIfStatement();
        case Tokens.DO:
            return this._parseDoWhileStatement();
        case Tokens.WHILE:
            return this._parseWhileStatement();
        case Tokens.FOR:
            return this._parseForStatement();
        case Tokens.CONTINUE:
            return this._parseContinueStatement();
        case Tokens.BREAK:
            return this._parseBreakStatement();
        case Tokens.RETURN:
            return this._parseReturnStatement();
        case Tokens.WITH:
            return this._parseWithStatement();
        case Tokens.SWITCH:
            return this._parseSwitchStatement();
        case Tokens.THROW:
            return this._parseThrowStatement();
        case Tokens.TRY:
            return this._parseTryStatement();
        case Tokens.FUNCTION:
            return this._parseFunctionDeclaration();
        case Tokens.DEBUGGER:
            return this._parseDebuggerStatement();
        default:
            return this._parseExpressionOrLabelledStatement();
        }
    },

    _parseFunctionDeclaration: function()
    {
        this._expect(Tokens.FUNCTION);
        this._builder.addSpace();
        this._expect(Tokens.IDENTIFIER);
        this._parseFunctionLiteral()
    },

    _parseBlock: function()
    {
        this._expect(Tokens.LBRACE);
        this._builder.addNewLine();
        this._builder.increaseNestingLevel();
        while (this._peek() !== Tokens.RBRACE) {
            this._parseStatement();
            this._builder.addNewLine();
        }
        this._builder.decreaseNestingLevel();
        this._expect(Tokens.RBRACE);
    },

    _parseVariableStatement: function()
    {
        this._parseVariableDeclarations();
        this._expectSemicolon();
    },

    _parseVariableDeclarations: function()
    {
        if (this._peek() === Tokens.VAR)
            this._consume(Tokens.VAR);
        else
            this._consume(Tokens.CONST)
        this._builder.addSpace();

        var isFirstVariable = true;
        do {
            if (!isFirstVariable) {
                this._consume(Tokens.COMMA);
                this._builder.addSpace();
            }
            isFirstVariable = false;
            this._expect(Tokens.IDENTIFIER);
            if (this._peek() === Tokens.ASSIGN) {
                this._builder.addSpace();
                this._consume(Tokens.ASSIGN);
                this._builder.addSpace();
                this._parseAssignmentExpression();
            }
        } while (this._peek() === Tokens.COMMA);
    },

    _parseExpressionOrLabelledStatement: function()
    {
        this._parseExpression();
        if (this._peek() === Tokens.COLON) {
            this._expect(Tokens.COLON);
            this._builder.addSpace();
            this._parseStatement();
        }
        this._expectSemicolon();
    },

    _parseIfStatement: function()
    {
        this._expect(Tokens.IF);
        this._builder.addSpace();
        this._expect(Tokens.LPAREN);
        this._parseExpression();
        this._expect(Tokens.RPAREN);

        var isBlock = this._parseStatementOrBlock();
        if (this._peek() === Tokens.ELSE) {
            if (isBlock)
                this._builder.addSpace();
            else
                this._builder.addNewLine();
            this._next();

            if (this._peek() === Tokens.IF) {
                this._builder.addSpace();
                this._parseStatement();
            } else
                this._parseStatementOrBlock();
        }
    },

    _parseContinueStatement: function()
    {
        this._expect(Tokens.CONTINUE);
        var token = this._peek();
        if (!this._hasLineTerminatorBeforeNext() && token !== Tokens.SEMICOLON && token !== Tokens.RBRACE && token !== Tokens.EOS) {
            this._builder.addSpace();
            this._expect(Tokens.IDENTIFIER);
        }
        this._expectSemicolon();
    },

    _parseBreakStatement: function()
    {
        this._expect(Tokens.BREAK);
        var token = this._peek();
        if (!this._hasLineTerminatorBeforeNext() && token !== Tokens.SEMICOLON && token !== Tokens.RBRACE && token !== Tokens.EOS) {
            this._builder.addSpace();
            this._expect(Tokens.IDENTIFIER);
        }
        this._expectSemicolon();
    },

    _parseReturnStatement: function()
    {
        this._expect(Tokens.RETURN);
        var token = this._peek();
        if (!this._hasLineTerminatorBeforeNext() && token !== Tokens.SEMICOLON && token !== Tokens.RBRACE && token !== Tokens.EOS) {
            this._builder.addSpace();
            this._parseExpression();
        }
        this._expectSemicolon();
    },

    _parseWithStatement: function()
    {
        this._expect(Tokens.WITH);
        this._builder.addSpace();
        this._expect(Tokens.LPAREN);
        this._parseExpression();
        this._expect(Tokens.RPAREN);
        this._parseStatementOrBlock();
    },

    _parseCaseClause: function()
    {
        if (this._peek() === Tokens.CASE) {
            this._expect(Tokens.CASE);
            this._builder.addSpace();
            this._parseExpression();
        } else
            this._expect(Tokens.DEFAULT);
        this._expect(Tokens.COLON);
        this._builder.addNewLine();

        this._builder.increaseNestingLevel();
        while (this._peek() !== Tokens.CASE && this._peek() !== Tokens.DEFAULT && this._peek() !== Tokens.RBRACE) {
            this._parseStatement();
            this._builder.addNewLine();
        }
        this._builder.decreaseNestingLevel();
    },

    _parseSwitchStatement: function()
    {
        this._expect(Tokens.SWITCH);
        this._builder.addSpace();
        this._expect(Tokens.LPAREN);
        this._parseExpression();
        this._expect(Tokens.RPAREN);
        this._builder.addSpace();

        this._expect(Tokens.LBRACE);
        this._builder.addNewLine();
        this._builder.increaseNestingLevel();
        while (this._peek() !== Tokens.RBRACE)
            this._parseCaseClause();
        this._builder.decreaseNestingLevel();
        this._expect(Tokens.RBRACE);
    },

    _parseThrowStatement: function()
    {
        this._expect(Tokens.THROW);
        this._builder.addSpace();
        this._parseExpression();
        this._expectSemicolon();
    },

    _parseTryStatement: function()
    {
        this._expect(Tokens.TRY);
        this._builder.addSpace();
        this._parseBlock();

        var token = this._peek();
        if (token === Tokens.CATCH) {
            this._builder.addSpace();
            this._consume(Tokens.CATCH);
            this._builder.addSpace();
            this._expect(Tokens.LPAREN);
            this._expect(Tokens.IDENTIFIER);
            this._expect(Tokens.RPAREN);
            this._builder.addSpace();
            this._parseBlock();
            token = this._peek();
        }

        if (token === Tokens.FINALLY) {
            this._consume(Tokens.FINALLY);
            this._builder.addSpace();
            this._parseBlock();
        }
    },

    _parseDoWhileStatement: function()
    {
        this._expect(Tokens.DO);
        var isBlock = this._parseStatementOrBlock();
        if (isBlock)
            this._builder.addSpace();
        else
            this._builder.addNewLine();
        this._expect(Tokens.WHILE);
        this._builder.addSpace();
        this._expect(Tokens.LPAREN);
        this._parseExpression();
        this._expect(Tokens.RPAREN);
        this._expectSemicolon();
    },

    _parseWhileStatement: function()
    {
        this._expect(Tokens.WHILE);
        this._builder.addSpace();
        this._expect(Tokens.LPAREN);
        this._parseExpression();
        this._expect(Tokens.RPAREN);
        this._parseStatementOrBlock();
    },

    _parseForStatement: function()
    {
        this._expect(Tokens.FOR);
        this._builder.addSpace();
        this._expect(Tokens.LPAREN);
        if (this._peek() !== Tokens.SEMICOLON) {
            if (this._peek() === Tokens.VAR || this._peek() === Tokens.CONST) {
                this._parseVariableDeclarations();
                if (this._peek() === Tokens.IN) {
                    this._builder.addSpace();
                    this._consume(Tokens.IN);
                    this._builder.addSpace();
                    this._parseExpression();
                }
            } else
                this._parseExpression();
        }

        if (this._peek() !== Tokens.RPAREN) {
            this._expect(Tokens.SEMICOLON);
            this._builder.addSpace();
            if (this._peek() !== Tokens.SEMICOLON)
                this._parseExpression();
            this._expect(Tokens.SEMICOLON);
            this._builder.addSpace();
            if (this._peek() !== Tokens.RPAREN)
                this._parseExpression();
        }
        this._expect(Tokens.RPAREN);

        this._parseStatementOrBlock();
    },

    _parseExpression: function()
    {
        this._parseAssignmentExpression();
        while (this._peek() === Tokens.COMMA) {
            this._expect(Tokens.COMMA);
            this._builder.addSpace();
            this._parseAssignmentExpression();
        }
    },

    _parseAssignmentExpression: function()
    {
        this._parseConditionalExpression();
        var token = this._peek();
        if (Tokens.ASSIGN <= token && token <= Tokens.ASSIGN_MOD) {
            this._builder.addSpace();
            this._next();
            this._builder.addSpace();
            this._parseAssignmentExpression();
        }
    },

    _parseConditionalExpression: function()
    {
        this._parseBinaryExpression();
        if (this._peek() === Tokens.CONDITIONAL) {
            this._builder.addSpace();
            this._consume(Tokens.CONDITIONAL);
            this._builder.addSpace();
            this._parseAssignmentExpression();
            this._builder.addSpace();
            this._expect(Tokens.COLON);
            this._builder.addSpace();
            this._parseAssignmentExpression();
        }
    },

    _parseBinaryExpression: function()
    {
        this._parseUnaryExpression();
        var token = this._peek();
        while (Tokens.OR <= token && token <= Tokens.IN) {
            this._builder.addSpace();
            this._next();
            this._builder.addSpace();
            this._parseBinaryExpression();
            token = this._peek();
        }
    },

    _parseUnaryExpression: function()
    {
        var token = this._peek();
        if ((Tokens.NOT <= token && token <= Tokens.VOID) || token === Tokens.ADD || token === Tokens.SUB || token ===  Tokens.INC || token === Tokens.DEC) {
            this._next();
            if (token === Tokens.DELETE || token === Tokens.TYPEOF || token === Tokens.VOID)
                this._builder.addSpace();
            this._parseUnaryExpression();
        } else
            return this._parsePostfixExpression();
    },

    _parsePostfixExpression: function()
    {
        this._parseLeftHandSideExpression();
        var token = this._peek();
        if (!this._hasLineTerminatorBeforeNext() && (token === Tokens.INC || token === Tokens.DEC))
            this._next();
    },

    _parseLeftHandSideExpression: function()
    {
        if (this._peek() === Tokens.NEW)
            this._parseNewExpression();
        else
            this._parseMemberExpression();

        while (true) {
            switch (this._peek()) {
            case Tokens.LBRACK:
                this._consume(Tokens.LBRACK);
                this._parseExpression();
                this._expect(Tokens.RBRACK);
                break;

            case Tokens.LPAREN:
                this._parseArguments();
                break;

            case Tokens.PERIOD:
                this._consume(Tokens.PERIOD);
                this._expect(Tokens.IDENTIFIER);
                break;

            default:
                return;
            }
        }
    },

    _parseNewExpression: function()
    {
        this._expect(Tokens.NEW);
        this._builder.addSpace();
        if (this._peek() === Tokens.NEW)
            this._parseNewExpression();
        else
            this._parseMemberExpression();
    },

    _parseMemberExpression: function()
    {
        if (this._peek() === Tokens.FUNCTION) {
            this._expect(Tokens.FUNCTION);
            if (this._peek() === Tokens.IDENTIFIER) {
                this._builder.addSpace();
                this._expect(Tokens.IDENTIFIER);
            }
            this._parseFunctionLiteral();
        } else
            this._parsePrimaryExpression();

        while (true) {
            switch (this._peek()) {
            case Tokens.LBRACK:
                this._consume(Tokens.LBRACK);
                this._parseExpression();
                this._expect(Tokens.RBRACK);
                break;

            case Tokens.PERIOD:
                this._consume(Tokens.PERIOD);
                this._expect(Tokens.IDENTIFIER);
                break;

            case Tokens.LPAREN:
                this._parseArguments();
                break;

            default:
                return;
            }
        }
    },

    _parseDebuggerStatement: function()
    {
        this._expect(Tokens.DEBUGGER);
        this._expectSemicolon();
    },

    _parsePrimaryExpression: function()
    {
        switch (this._peek()) {
        case Tokens.THIS:
            return this._consume(Tokens.THIS);
        case Tokens.NULL_LITERAL:
            return this._consume(Tokens.NULL_LITERAL);
        case Tokens.TRUE_LITERAL:
            return this._consume(Tokens.TRUE_LITERAL);
        case Tokens.FALSE_LITERAL:
            return this._consume(Tokens.FALSE_LITERAL);
        case Tokens.IDENTIFIER:
            return this._consume(Tokens.IDENTIFIER);
        case Tokens.NUMBER:
            return this._consume(Tokens.NUMBER);
        case Tokens.STRING:
            return this._consume(Tokens.STRING);
        case Tokens.ASSIGN_DIV:
            return this._parseRegExpLiteral();
        case Tokens.DIV:
            return this._parseRegExpLiteral();
        case Tokens.LBRACK:
            return this._parseArrayLiteral();
        case Tokens.LBRACE:
            return this._parseObjectLiteral();
        case Tokens.LPAREN:
            this._consume(Tokens.LPAREN);
            this._parseExpression();
            this._expect(Tokens.RPAREN);
            return;
        default:
            return this._next();
        }
    },

    _parseArrayLiteral: function()
    {
        this._expect(Tokens.LBRACK);
        this._builder.increaseNestingLevel();
        while (this._peek() !== Tokens.RBRACK) {
            if (this._peek() !== Tokens.COMMA)
                this._parseAssignmentExpression();
            if (this._peek() !== Tokens.RBRACK) {
                this._expect(Tokens.COMMA);
                this._builder.addSpace();
            }
        }
        this._builder.decreaseNestingLevel();
        this._expect(Tokens.RBRACK);
    },

    _parseObjectLiteralGetSet: function()
    {
        var token = this._peek();
        if (token === Tokens.IDENTIFIER || token === Tokens.NUMBER || token === Tokens.STRING ||
            Tokens.DELETE <= token && token <= Tokens.FALSE_LITERAL ||
            token === Tokens.INSTANCEOF || token === Tokens.IN || token === Tokens.CONST) {
            this._next();
            this._parseFunctionLiteral();
        }
    },

    _parseObjectLiteral: function()
    {
        this._expect(Tokens.LBRACE);
        this._builder.increaseNestingLevel();
        while (this._peek() !== Tokens.RBRACE) {
            var token = this._peek();
            switch (token) {
            case Tokens.IDENTIFIER:
                this._consume(Tokens.IDENTIFIER);
                var name = this._token.value;
                if ((name === "get" || name === "set") && this._peek() !== Tokens.COLON) {
                    this._builder.addSpace();
                    this._parseObjectLiteralGetSet();
                    if (this._peek() !== Tokens.RBRACE) {
                        this._expect(Tokens.COMMA);
                    }
                    continue;
                }
                break;

            case Tokens.STRING:
                this._consume(Tokens.STRING);
                break;

            case Tokens.NUMBER:
                this._consume(Tokens.NUMBER);
                break;

            default:
                this._next();
            }

            this._expect(Tokens.COLON);
            this._builder.addSpace();
            this._parseAssignmentExpression();
            if (this._peek() !== Tokens.RBRACE) {
                this._expect(Tokens.COMMA);
            }
        }
        this._builder.decreaseNestingLevel();

        this._expect(Tokens.RBRACE);
    },

    _parseRegExpLiteral: function()
    {
        if (this._nextToken.type === "regexp")
            this._next();
        else {
            this._forceRegexp = true;
            this._next();
        }
    },

    _parseArguments: function()
    {
        this._expect(Tokens.LPAREN);
        var done = (this._peek() === Tokens.RPAREN);
        while (!done) {
            this._parseAssignmentExpression();
            done = (this._peek() === Tokens.RPAREN);
            if (!done) {
                this._expect(Tokens.COMMA);
                this._builder.addSpace();
            }
        }
        this._expect(Tokens.RPAREN);
    },

    _parseFunctionLiteral: function()
    {
        this._expect(Tokens.LPAREN);
        var done = (this._peek() === Tokens.RPAREN);
        while (!done) {
            this._expect(Tokens.IDENTIFIER);
            done = (this._peek() === Tokens.RPAREN);
            if (!done) {
                this._expect(Tokens.COMMA);
                this._builder.addSpace();
            }
        }
        this._expect(Tokens.RPAREN);
        this._builder.addSpace();

        this._expect(Tokens.LBRACE);
        this._builder.addNewLine();
        this._builder.increaseNestingLevel();
        this._parseSourceElements(Tokens.RBRACE);
        this._builder.decreaseNestingLevel();
        this._expect(Tokens.RBRACE);
    }
}
