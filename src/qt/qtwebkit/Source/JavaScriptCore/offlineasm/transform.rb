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

#
# node.resolveSettings(settings)
#
# Construct a new AST that does not have any IfThenElse nodes by
# substituting concrete boolean values for each Setting.
#

class Node
    def resolveSettings(settings)
        mapChildren {
            | child |
            child.resolveSettings(settings)
        }
    end
end

class True
    def resolveSettings(settings)
        self
    end
end

class False
    def resolveSettings(settings)
        self
    end
end

class Setting
    def resolveSettings(settings)
        settings[@name].asNode
    end
end

class And
    def resolveSettings(settings)
        (@left.resolveSettings(settings).value and @right.resolveSettings(settings).value).asNode
    end
end

class Or
    def resolveSettings(settings)
        (@left.resolveSettings(settings).value or @right.resolveSettings(settings).value).asNode
    end
end

class Not
    def resolveSettings(settings)
        (not @child.resolveSettings(settings).value).asNode
    end
end

class IfThenElse
    def resolveSettings(settings)
        if @predicate.resolveSettings(settings).value
            @thenCase.resolveSettings(settings)
        else
            @elseCase.resolveSettings(settings)
        end
    end
end

class Sequence
    def resolveSettings(settings)
        newList = []
        @list.each {
            | item |
            item = item.resolveSettings(settings)
            if item.is_a? Sequence
                newList += item.list
            else
                newList << item
            end
        }
        Sequence.new(codeOrigin, newList)
    end
end

#
# node.demacroify(macros)
# node.substitute(mapping)
#
# demacroify() constructs a new AST that does not have any Macro
# nodes, while substitute() replaces Variable nodes with the given
# nodes in the mapping.
#

class Node
    def demacroify(macros)
        mapChildren {
            | child |
            child.demacroify(macros)
        }
    end
    
    def substitute(mapping)
        mapChildren {
            | child |
            child.substitute(mapping)
        }
    end
    
    def substituteLabels(mapping)
        mapChildren {
            | child |
            child.substituteLabels(mapping)
        }
    end
end

class Macro
    def substitute(mapping)
        myMapping = {}
        mapping.each_pair {
            | key, value |
            unless @variables.include? key
                myMapping[key] = value
            end
        }
        mapChildren {
            | child |
            child.substitute(myMapping)
        }
    end
end

class Variable
    def substitute(mapping)
        if mapping[self]
            mapping[self]
        else
            self
        end
    end
end

class LocalLabel
    def substituteLabels(mapping)
        if mapping[self]
            mapping[self]
        else
            self
        end
    end
end

class Sequence
    def substitute(constants)
        newList = []
        myConstants = constants.dup
        @list.each {
            | item |
            if item.is_a? ConstDecl
                myConstants[item.variable] = item.value.substitute(myConstants)
            else
                newList << item.substitute(myConstants)
            end
        }
        Sequence.new(codeOrigin, newList)
    end
    
    def renameLabels(comment)
        mapping = {}
        
        @list.each {
            | item |
            if item.is_a? LocalLabel
                mapping[item] = LocalLabel.unique(if comment then comment + "_" else "" end + item.cleanName)
            end
        }
        
        substituteLabels(mapping)
    end
    
    def demacroify(macros)
        myMacros = macros.dup
        @list.each {
            | item |
            if item.is_a? Macro
                myMacros[item.name] = item
            end
        }
        newList = []
        @list.each {
            | item |
            if item.is_a? Macro
                # Ignore.
            elsif item.is_a? MacroCall
                mapping = {}
                myMyMacros = myMacros.dup
                raise "Could not find macro #{item.name} at #{item.codeOriginString}" unless myMacros[item.name]
                raise "Argument count mismatch for call to #{item.name} at #{item.codeOriginString}" unless item.operands.size == myMacros[item.name].variables.size
                item.operands.size.times {
                    | idx |
                    if item.operands[idx].is_a? Variable and myMacros[item.operands[idx].name]
                        myMyMacros[myMacros[item.name].variables[idx].name] = myMacros[item.operands[idx].name]
                        mapping[myMacros[item.name].variables[idx].name] = nil
                    elsif item.operands[idx].is_a? Macro
                        myMyMacros[myMacros[item.name].variables[idx].name] = item.operands[idx]
                        mapping[myMacros[item.name].variables[idx].name] = nil
                    else
                        myMyMacros[myMacros[item.name].variables[idx]] = nil
                        mapping[myMacros[item.name].variables[idx]] = item.operands[idx]
                    end
                }
                if item.annotation
                    newList << Instruction.new(item.codeOrigin, "localAnnotation", [], item.annotation)
                end
                newList += myMacros[item.name].body.substitute(mapping).demacroify(myMyMacros).renameLabels(item.name).list
            else
                newList << item.demacroify(myMacros)
            end
        }
        Sequence.new(codeOrigin, newList).substitute({})
    end
end

#
# node.resolveOffsets(offsets, sizes)
#
# Construct a new AST that has offset values instead of symbolic
# offsets.
#

class Node
    def resolveOffsets(offsets, sizes)
        mapChildren {
            | child |
            child.resolveOffsets(offsets, sizes)
        }
    end
end

class StructOffset
    def resolveOffsets(offsets, sizes)
        if offsets[self]
            Immediate.new(codeOrigin, offsets[self])
        else
            self
        end
    end
end

class Sizeof
    def resolveOffsets(offsets, sizes)
        if sizes[self]
            Immediate.new(codeOrigin, sizes[self])
        else
            puts "Could not find #{self.inspect} in #{sizes.keys.inspect}"
            puts "sizes = #{sizes.inspect}"
            self
        end
    end
end

#
# node.fold
#
# Resolve constant references and compute arithmetic expressions.
#

class Node
    def fold
        mapChildren {
            | child |
            child.fold
        }
    end
end

class AddImmediates
    def fold
        @left = @left.fold
        @right = @right.fold
        return self unless @left.is_a? Immediate
        return self unless @right.is_a? Immediate
        Immediate.new(codeOrigin, @left.value + @right.value)
    end
end

class SubImmediates
    def fold
        @left = @left.fold
        @right = @right.fold
        return self unless @left.is_a? Immediate
        return self unless @right.is_a? Immediate
        Immediate.new(codeOrigin, @left.value - @right.value)
    end
end

class MulImmediates
    def fold
        @left = @left.fold
        @right = @right.fold
        return self unless @left.is_a? Immediate
        return self unless @right.is_a? Immediate
        Immediate.new(codeOrigin, @left.value * @right.value)
    end
end

class NegImmediate
    def fold
        @child = @child.fold
        return self unless @child.is_a? Immediate
        Immediate.new(codeOrigin, -@child.value)
    end
end

class OrImmediates
    def fold
        @left = @left.fold
        @right = @right.fold
        return self unless @left.is_a? Immediate
        return self unless @right.is_a? Immediate
        Immediate.new(codeOrigin, @left.value | @right.value)
    end
end

class AndImmediates
    def fold
        @left = @left.fold
        @right = @right.fold
        return self unless @left.is_a? Immediate
        return self unless @right.is_a? Immediate
        Immediate.new(codeOrigin, @left.value & @right.value)
    end
end

class XorImmediates
    def fold
        @left = @left.fold
        @right = @right.fold
        return self unless @left.is_a? Immediate
        return self unless @right.is_a? Immediate
        Immediate.new(codeOrigin, @left.value ^ @right.value)
    end
end

class BitnotImmediate
    def fold
        @child = @child.fold
        return self unless @child.is_a? Immediate
        Immediate.new(codeOrigin, ~@child.value)
    end
end

#
# node.resolveAfterSettings(offsets, sizes)
#
# Compile assembly against a set of offsets.
#

class Node
    def resolve(offsets, sizes)
        demacroify({}).resolveOffsets(offsets, sizes).fold
    end
end

#
# node.validate
#
# Checks that the node is ready for backend compilation.
#

class Node
    def validate
        raise "Unresolved #{dump} at #{codeOriginString}"
    end
    
    def validateChildren
        children.each {
            | node |
            node.validate
        }
    end
end

class Sequence
    def validate
        validateChildren
    end
end

class Immediate
    def validate
    end
end

class RegisterID
    def validate
    end
end

class FPRegisterID
    def validate
    end
end

class Address
    def validate
        validateChildren
    end
end

class BaseIndex
    def validate
        validateChildren
    end
end

class AbsoluteAddress
    def validate
        validateChildren
    end
end

class Instruction
    def validate
        validateChildren
    end
end

class Error
    def validate
    end
end

class Label
    def validate
    end
end

class LocalLabel
    def validate
    end
end

class LabelReference
    def validate
    end
end

class LocalLabelReference
    def validate
    end
end

class Skip
    def validate
    end
end

