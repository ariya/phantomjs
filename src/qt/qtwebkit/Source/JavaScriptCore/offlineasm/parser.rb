# Copyright (C) 2011 Apple Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

require "config"
require "ast"
require "instructions"
require "pathname"
require "registers"
require "self_hash"

class CodeOrigin
    attr_reader :fileName, :lineNumber
    
    def initialize(fileName, lineNumber)
        @fileName = fileName
        @lineNumber = lineNumber
    end
    
    def to_s
        "#{fileName}:#{lineNumber}"
    end
end

class Token
    attr_reader :codeOrigin, :string
    
    def initialize(codeOrigin, string)
        @codeOrigin = codeOrigin
        @string = string
    end
    
    def ==(other)
        if other.is_a? Token
            @string == other.string
        else
            @string == other
        end
    end
    
    def =~(other)
        @string =~ other
    end
    
    def to_s
        "#{@string.inspect} at #{codeOrigin}"
    end
    
    def parseError(*comment)
        if comment.empty?
            raise "Parse error: #{to_s}"
        else
            raise "Parse error: #{to_s}: #{comment[0]}"
        end
    end
end

class Annotation
    attr_reader :codeOrigin, :type, :string
    def initialize(codeOrigin, type, string)
        @codeOrigin = codeOrigin
        @type = type
        @string = string
    end
end

#
# The lexer. Takes a string and returns an array of tokens.
#

def lex(str, fileName)
    fileName = Pathname.new(fileName)
    result = []
    lineNumber = 1
    annotation = nil
    whitespaceFound = false
    while not str.empty?
        case str
        when /\A\#([^\n]*)/
            # comment, ignore
        when /\A\/\/\ ?([^\n]*)/
            # annotation
            annotation = $1
            annotationType = whitespaceFound ? :local : :global
        when /\A\n/
            # We've found a '\n'.  Emit the last comment recorded if appropriate:
            # We need to parse annotations regardless of whether the backend does
            # anything with them or not. This is because the C++ backend may make
            # use of this for its cloopDo debugging utility even if
            # enableInstrAnnotations is not enabled.
            if annotation
                result << Annotation.new(CodeOrigin.new(fileName, lineNumber),
                                         annotationType, annotation)
                annotation = nil
            end
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&)
            lineNumber += 1
        when /\A[a-zA-Z]([a-zA-Z0-9_.]*)/
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&)
        when /\A\.([a-zA-Z0-9_]*)/
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&)
        when /\A_([a-zA-Z0-9_]*)/
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&)
        when /\A([ \t]+)/
            # whitespace, ignore
            whitespaceFound = true
            str = $~.post_match
            next
        when /\A0x([0-9a-fA-F]+)/
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&.hex.to_s)
        when /\A0([0-7]+)/
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&.oct.to_s)
        when /\A([0-9]+)/
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&)
        when /\A::/
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&)
        when /\A[:,\(\)\[\]=\+\-~\|&^*]/
            result << Token.new(CodeOrigin.new(fileName, lineNumber), $&)
        else
            raise "Lexer error at #{CodeOrigin.new(fileName, lineNumber).to_s}, unexpected sequence #{str[0..20].inspect}"
        end
        whitespaceFound = false
        str = $~.post_match
    end
    result
end

#
# Token identification.
#

def isRegister(token)
    token =~ REGISTER_PATTERN
end

def isInstruction(token)
    token =~ INSTRUCTION_PATTERN
end

def isKeyword(token)
    token =~ /\A((true)|(false)|(if)|(then)|(else)|(elsif)|(end)|(and)|(or)|(not)|(macro)|(const)|(sizeof)|(error)|(include))\Z/ or
        token =~ REGISTER_PATTERN or
        token =~ INSTRUCTION_PATTERN
end

def isIdentifier(token)
    token =~ /\A[a-zA-Z]([a-zA-Z0-9_.]*)\Z/ and not isKeyword(token)
end

def isLabel(token)
    token =~ /\A_([a-zA-Z0-9_]*)\Z/
end

def isLocalLabel(token)
    token =~ /\A\.([a-zA-Z0-9_]*)\Z/
end

def isVariable(token)
    isIdentifier(token) or isRegister(token)
end

def isInteger(token)
    token =~ /\A[0-9]/
end

#
# The parser. Takes an array of tokens and returns an AST. Methods
# other than parse(tokens) are not for public consumption.
#

class Parser
    def initialize(data, fileName)
        @tokens = lex(data, fileName)
        @idx = 0
        @annotation = nil
    end
    
    def parseError(*comment)
        if @tokens[@idx]
            @tokens[@idx].parseError(*comment)
        else
            if comment.empty?
                raise "Parse error at end of file"
            else
                raise "Parse error at end of file: #{comment[0]}"
            end
        end
    end
    
    def consume(regexp)
        if regexp
            parseError unless @tokens[@idx] =~ regexp
        else
            parseError unless @idx == @tokens.length
        end
        @idx += 1
    end
    
    def skipNewLine
        while @tokens[@idx] == "\n"
            @idx += 1
        end
    end
    
    def parsePredicateAtom
        if @tokens[@idx] == "not"
            codeOrigin = @tokens[@idx].codeOrigin
            @idx += 1
            Not.new(codeOrigin, parsePredicateAtom)
        elsif @tokens[@idx] == "("
            @idx += 1
            skipNewLine
            result = parsePredicate
            parseError unless @tokens[@idx] == ")"
            @idx += 1
            result
        elsif @tokens[@idx] == "true"
            result = True.instance
            @idx += 1
            result
        elsif @tokens[@idx] == "false"
            result = False.instance
            @idx += 1
            result
        elsif isIdentifier @tokens[@idx]
            result = Setting.forName(@tokens[@idx].codeOrigin, @tokens[@idx].string)
            @idx += 1
            result
        else
            parseError
        end
    end
    
    def parsePredicateAnd
        result = parsePredicateAtom
        while @tokens[@idx] == "and"
            codeOrigin = @tokens[@idx].codeOrigin
            @idx += 1
            skipNewLine
            right = parsePredicateAtom
            result = And.new(codeOrigin, result, right)
        end
        result
    end
    
    def parsePredicate
        # some examples of precedence:
        # not a and b -> (not a) and b
        # a and b or c -> (a and b) or c
        # a or b and c -> a or (b and c)
        
        result = parsePredicateAnd
        while @tokens[@idx] == "or"
            codeOrigin = @tokens[@idx].codeOrigin
            @idx += 1
            skipNewLine
            right = parsePredicateAnd
            result = Or.new(codeOrigin, result, right)
        end
        result
    end
    
    def parseVariable
        if isRegister(@tokens[@idx])
            if @tokens[@idx] =~ FPR_PATTERN
                result = FPRegisterID.forName(@tokens[@idx].codeOrigin, @tokens[@idx].string)
            else
                result = RegisterID.forName(@tokens[@idx].codeOrigin, @tokens[@idx].string)
            end
        elsif isIdentifier(@tokens[@idx])
            result = Variable.forName(@tokens[@idx].codeOrigin, @tokens[@idx].string)
        else
            parseError
        end
        @idx += 1
        result
    end
    
    def parseAddress(offset)
        parseError unless @tokens[@idx] == "["
        codeOrigin = @tokens[@idx].codeOrigin
        
        # Three possibilities:
        # []       -> AbsoluteAddress
        # [a]      -> Address
        # [a,b]    -> BaseIndex with scale = 1
        # [a,b,c]  -> BaseIndex
        
        @idx += 1
        if @tokens[@idx] == "]"
            @idx += 1
            return AbsoluteAddress.new(codeOrigin, offset)
        end
        a = parseVariable
        if @tokens[@idx] == "]"
            result = Address.new(codeOrigin, a, offset)
        else
            parseError unless @tokens[@idx] == ","
            @idx += 1
            b = parseVariable
            if @tokens[@idx] == "]"
                result = BaseIndex.new(codeOrigin, a, b, 1, offset)
            else
                parseError unless @tokens[@idx] == ","
                @idx += 1
                parseError unless ["1", "2", "4", "8"].member? @tokens[@idx].string
                c = @tokens[@idx].string.to_i
                @idx += 1
                parseError unless @tokens[@idx] == "]"
                result = BaseIndex.new(codeOrigin, a, b, c, offset)
            end
        end
        @idx += 1
        result
    end
    
    def parseColonColon
        skipNewLine
        codeOrigin = @tokens[@idx].codeOrigin
        parseError unless isIdentifier @tokens[@idx]
        names = [@tokens[@idx].string]
        @idx += 1
        while @tokens[@idx] == "::"
            @idx += 1
            parseError unless isIdentifier @tokens[@idx]
            names << @tokens[@idx].string
            @idx += 1
        end
        raise if names.empty?
        [codeOrigin, names]
    end
    
    def parseExpressionAtom
        skipNewLine
        if @tokens[@idx] == "-"
            @idx += 1
            NegImmediate.new(@tokens[@idx - 1].codeOrigin, parseExpressionAtom)
        elsif @tokens[@idx] == "~"
            @idx += 1
            BitnotImmediate.new(@tokens[@idx - 1].codeOrigin, parseExpressionAtom)
        elsif @tokens[@idx] == "("
            @idx += 1
            result = parseExpression
            parseError unless @tokens[@idx] == ")"
            @idx += 1
            result
        elsif isInteger @tokens[@idx]
            result = Immediate.new(@tokens[@idx].codeOrigin, @tokens[@idx].string.to_i)
            @idx += 1
            result
        elsif isIdentifier @tokens[@idx]
            codeOrigin, names = parseColonColon
            if names.size > 1
                StructOffset.forField(codeOrigin, names[0..-2].join('::'), names[-1])
            else
                Variable.forName(codeOrigin, names[0])
            end
        elsif isRegister @tokens[@idx]
            parseVariable
        elsif @tokens[@idx] == "sizeof"
            @idx += 1
            codeOrigin, names = parseColonColon
            Sizeof.forName(codeOrigin, names.join('::'))
        else
            parseError
        end
    end
    
    def parseExpressionMul
        skipNewLine
        result = parseExpressionAtom
        while @tokens[@idx] == "*"
            if @tokens[@idx] == "*"
                @idx += 1
                result = MulImmediates.new(@tokens[@idx - 1].codeOrigin, result, parseExpressionAtom)
            else
                raise
            end
        end
        result
    end
    
    def couldBeExpression
        @tokens[@idx] == "-" or @tokens[@idx] == "~" or @tokens[@idx] == "sizeof" or isInteger(@tokens[@idx]) or isVariable(@tokens[@idx]) or @tokens[@idx] == "("
    end
    
    def parseExpressionAdd
        skipNewLine
        result = parseExpressionMul
        while @tokens[@idx] == "+" or @tokens[@idx] == "-"
            if @tokens[@idx] == "+"
                @idx += 1
                result = AddImmediates.new(@tokens[@idx - 1].codeOrigin, result, parseExpressionMul)
            elsif @tokens[@idx] == "-"
                @idx += 1
                result = SubImmediates.new(@tokens[@idx - 1].codeOrigin, result, parseExpressionMul)
            else
                raise
            end
        end
        result
    end
    
    def parseExpressionAnd
        skipNewLine
        result = parseExpressionAdd
        while @tokens[@idx] == "&"
            @idx += 1
            result = AndImmediates.new(@tokens[@idx - 1].codeOrigin, result, parseExpressionAdd)
        end
        result
    end
    
    def parseExpression
        skipNewLine
        result = parseExpressionAnd
        while @tokens[@idx] == "|" or @tokens[@idx] == "^"
            if @tokens[@idx] == "|"
                @idx += 1
                result = OrImmediates.new(@tokens[@idx - 1].codeOrigin, result, parseExpressionAnd)
            elsif @tokens[@idx] == "^"
                @idx += 1
                result = XorImmediates.new(@tokens[@idx - 1].codeOrigin, result, parseExpressionAnd)
            else
                raise
            end
        end
        result
    end
    
    def parseOperand(comment)
        skipNewLine
        if couldBeExpression
            expr = parseExpression
            if @tokens[@idx] == "["
                parseAddress(expr)
            else
                expr
            end
        elsif @tokens[@idx] == "["
            parseAddress(Immediate.new(@tokens[@idx].codeOrigin, 0))
        elsif isLabel @tokens[@idx]
            result = LabelReference.new(@tokens[@idx].codeOrigin, Label.forName(@tokens[@idx].codeOrigin, @tokens[@idx].string))
            @idx += 1
            result
        elsif isLocalLabel @tokens[@idx]
            result = LocalLabelReference.new(@tokens[@idx].codeOrigin, LocalLabel.forName(@tokens[@idx].codeOrigin, @tokens[@idx].string))
            @idx += 1
            result
        else
            parseError(comment)
        end
    end
    
    def parseMacroVariables
        skipNewLine
        consume(/\A\(\Z/)
        variables = []
        loop {
            skipNewLine
            if @tokens[@idx] == ")"
                @idx += 1
                break
            elsif isIdentifier(@tokens[@idx])
                variables << Variable.forName(@tokens[@idx].codeOrigin, @tokens[@idx].string)
                @idx += 1
                skipNewLine
                if @tokens[@idx] == ")"
                    @idx += 1
                    break
                elsif @tokens[@idx] == ","
                    @idx += 1
                else
                    parseError
                end
            else
                parseError
            end
        }
        variables
    end
    
    def parseSequence(final, comment)
        firstCodeOrigin = @tokens[@idx].codeOrigin
        list = []
        loop {
            if (@idx == @tokens.length and not final) or (final and @tokens[@idx] =~ final)
                break
            elsif @tokens[@idx].is_a? Annotation
                # This is the only place where we can encounter a global
                # annotation, and hence need to be able to distinguish between
                # them.
                # globalAnnotations are the ones that start from column 0. All
                # others are considered localAnnotations.  The only reason to
                # distinguish between them is so that we can format the output
                # nicely as one would expect.

                codeOrigin = @tokens[@idx].codeOrigin
                annotationOpcode = (@tokens[@idx].type == :global) ? "globalAnnotation" : "localAnnotation"
                list << Instruction.new(codeOrigin, annotationOpcode, [], @tokens[@idx].string)
                @annotation = nil
                @idx += 2 # Consume the newline as well.
            elsif @tokens[@idx] == "\n"
                # ignore
                @idx += 1
            elsif @tokens[@idx] == "const"
                @idx += 1
                parseError unless isVariable @tokens[@idx]
                variable = Variable.forName(@tokens[@idx].codeOrigin, @tokens[@idx].string)
                @idx += 1
                parseError unless @tokens[@idx] == "="
                @idx += 1
                value = parseOperand("while inside of const #{variable.name}")
                list << ConstDecl.new(@tokens[@idx].codeOrigin, variable, value)
            elsif @tokens[@idx] == "error"
                list << Error.new(@tokens[@idx].codeOrigin)
                @idx += 1
            elsif @tokens[@idx] == "if"
                codeOrigin = @tokens[@idx].codeOrigin
                @idx += 1
                skipNewLine
                predicate = parsePredicate
                consume(/\A((then)|(\n))\Z/)
                skipNewLine
                ifThenElse = IfThenElse.new(codeOrigin, predicate, parseSequence(/\A((else)|(end)|(elsif))\Z/, "while inside of \"if #{predicate.dump}\""))
                list << ifThenElse
                while @tokens[@idx] == "elsif"
                    codeOrigin = @tokens[@idx].codeOrigin
                    @idx += 1
                    skipNewLine
                    predicate = parsePredicate
                    consume(/\A((then)|(\n))\Z/)
                    skipNewLine
                    elseCase = IfThenElse.new(codeOrigin, predicate, parseSequence(/\A((else)|(end)|(elsif))\Z/, "while inside of \"if #{predicate.dump}\""))
                    ifThenElse.elseCase = elseCase
                    ifThenElse = elseCase
                end
                if @tokens[@idx] == "else"
                    @idx += 1
                    ifThenElse.elseCase = parseSequence(/\Aend\Z/, "while inside of else case for \"if #{predicate.dump}\"")
                    @idx += 1
                else
                    parseError unless @tokens[@idx] == "end"
                    @idx += 1
                end
            elsif @tokens[@idx] == "macro"
                codeOrigin = @tokens[@idx].codeOrigin
                @idx += 1
                skipNewLine
                parseError unless isIdentifier(@tokens[@idx])
                name = @tokens[@idx].string
                @idx += 1
                variables = parseMacroVariables
                body = parseSequence(/\Aend\Z/, "while inside of macro #{name}")
                @idx += 1
                list << Macro.new(codeOrigin, name, variables, body)
            elsif isInstruction @tokens[@idx]
                codeOrigin = @tokens[@idx].codeOrigin
                name = @tokens[@idx].string
                @idx += 1
                if (not final and @idx == @tokens.size) or (final and @tokens[@idx] =~ final)
                    # Zero operand instruction, and it's the last one.
                    list << Instruction.new(codeOrigin, name, [], @annotation)
                    @annotation = nil
                    break
                elsif @tokens[@idx].is_a? Annotation
                    list << Instruction.new(codeOrigin, name, [], @tokens[@idx].string)
                    @annotation = nil
                    @idx += 2 # Consume the newline as well.
                elsif @tokens[@idx] == "\n"
                    # Zero operand instruction.
                    list << Instruction.new(codeOrigin, name, [], @annotation)
                    @annotation = nil
                    @idx += 1
                else
                    # It's definitely an instruction, and it has at least one operand.
                    operands = []
                    endOfSequence = false
                    loop {
                        operands << parseOperand("while inside of instruction #{name}")
                        if (not final and @idx == @tokens.size) or (final and @tokens[@idx] =~ final)
                            # The end of the instruction and of the sequence.
                            endOfSequence = true
                            break
                        elsif @tokens[@idx] == ","
                            # Has another operand.
                            @idx += 1
                        elsif @tokens[@idx].is_a? Annotation
                            @annotation = @tokens[@idx].string
                            @idx += 2 # Consume the newline as well.
                            break
                        elsif @tokens[@idx] == "\n"
                            # The end of the instruction.
                            @idx += 1
                            break
                        else
                            parseError("Expected a comma, newline, or #{final} after #{operands.last.dump}")
                        end
                    }
                    list << Instruction.new(codeOrigin, name, operands, @annotation)
                    @annotation = nil
                    if endOfSequence
                        break
                    end
                end

            # Check for potential macro invocation:
            elsif isIdentifier @tokens[@idx]
                codeOrigin = @tokens[@idx].codeOrigin
                name = @tokens[@idx].string
                @idx += 1
                if @tokens[@idx] == "("
                    # Macro invocation.
                    @idx += 1
                    operands = []
                    skipNewLine
                    if @tokens[@idx] == ")"
                        @idx += 1
                    else
                        loop {
                            skipNewLine
                            if @tokens[@idx] == "macro"
                                # It's a macro lambda!
                                codeOriginInner = @tokens[@idx].codeOrigin
                                @idx += 1
                                variables = parseMacroVariables
                                body = parseSequence(/\Aend\Z/, "while inside of anonymous macro passed as argument to #{name}")
                                @idx += 1
                                operands << Macro.new(codeOriginInner, nil, variables, body)
                            else
                                operands << parseOperand("while inside of macro call to #{name}")
                            end
                            skipNewLine
                            if @tokens[@idx] == ")"
                                @idx += 1
                                break
                            elsif @tokens[@idx] == ","
                                @idx += 1
                            else
                                parseError "Unexpected #{@tokens[@idx].string.inspect} while parsing invocation of macro #{name}"
                            end
                        }
                    end
                    # Check if there's a trailing annotation after the macro invoke:
                    if @tokens[@idx].is_a? Annotation
                        @annotation = @tokens[@idx].string
                        @idx += 2 # Consume the newline as well.
                    end
                    list << MacroCall.new(codeOrigin, name, operands, @annotation)
                    @annotation = nil
                else
                    parseError "Expected \"(\" after #{name}"
                end
            elsif isLabel @tokens[@idx] or isLocalLabel @tokens[@idx]
                codeOrigin = @tokens[@idx].codeOrigin
                name = @tokens[@idx].string
                @idx += 1
                parseError unless @tokens[@idx] == ":"
                # It's a label.
                if isLabel name
                    list << Label.forName(codeOrigin, name)
                else
                    list << LocalLabel.forName(codeOrigin, name)
                end
                @idx += 1
            elsif @tokens[@idx] == "include"
                @idx += 1
                parseError unless isIdentifier(@tokens[@idx])
                moduleName = @tokens[@idx].string
                fileName = @tokens[@idx].codeOrigin.fileName.dirname + (moduleName + ".asm")
                @idx += 1
                $stderr.puts "offlineasm: Including file #{fileName}"
                list << parse(fileName)
            else
                parseError "Expecting terminal #{final} #{comment}"
            end
        }
        Sequence.new(firstCodeOrigin, list)
    end
end

def parseData(data, fileName)
    parser = Parser.new(data, fileName)
    parser.parseSequence(nil, "")
end

def parse(fileName)
    parseData(IO::read(fileName), fileName)
end

def parseHash(fileName)
    dirHash(Pathname.new(fileName).dirname, /\.asm$/)
end

