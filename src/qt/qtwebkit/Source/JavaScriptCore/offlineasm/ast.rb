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

#
# Base utility types for the AST.
#

# Valid methods for Node:
#
# node.children -> Returns an array of immediate children.
#
# node.descendents -> Returns an array of all strict descendants (children
#     and children of children, transitively).
#
# node.flatten -> Returns an array containing the strict descendants and
#     the node itself.
#
# node.filter(type) -> Returns an array containing those elements in
#     node.flatten that are of the given type (is_a? type returns true).
#
# node.mapChildren{|v| ...} -> Returns a new node with all children
#     replaced according to the given block.
#
# Examples:
#
# node.filter(Setting).uniq -> Returns all of the settings that the AST's
#     IfThenElse blocks depend on.
#
# node.filter(StructOffset).uniq -> Returns all of the structure offsets
#     that the AST depends on.

class Node
    attr_reader :codeOrigin
    
    def initialize(codeOrigin)
        @codeOrigin = codeOrigin
    end
    
    def codeOriginString
        @codeOrigin.to_s
    end
    
    def descendants
        children.collect{|v| v.flatten}.flatten
    end
    
    def flatten
        [self] + descendants
    end
    
    def filter(type)
        flatten.select{|v| v.is_a? type}
    end
end

class NoChildren < Node
    def initialize(codeOrigin)
        super(codeOrigin)
    end
    
    def children
        []
    end
    
    def mapChildren
        self
    end
end

class StructOffsetKey
    attr_reader :struct, :field
    
    def initialize(struct, field)
        @struct = struct
        @field = field
    end
    
    def hash
        @struct.hash + @field.hash * 3
    end
    
    def eql?(other)
        @struct == other.struct and @field == other.field
    end
end

#
# AST nodes.
#

class StructOffset < NoChildren
    attr_reader :struct, :field
    
    def initialize(codeOrigin, struct, field)
        super(codeOrigin)
        @struct = struct
        @field = field
    end
    
    @@mapping = {}
    
    def self.forField(codeOrigin, struct, field)
        key = StructOffsetKey.new(struct, field)
        
        unless @@mapping[key]
            @@mapping[key] = StructOffset.new(codeOrigin, struct, field)
        end
        @@mapping[key]
    end
    
    def dump
        "#{struct}::#{field}"
    end
    
    def <=>(other)
        if @struct != other.struct
            return @struct <=> other.struct
        end
        @field <=> other.field
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class Sizeof < NoChildren
    attr_reader :struct
    
    def initialize(codeOrigin, struct)
        super(codeOrigin)
        @struct = struct
    end
    
    @@mapping = {}
    
    def self.forName(codeOrigin, struct)
        unless @@mapping[struct]
            @@mapping[struct] = Sizeof.new(codeOrigin, struct)
        end
        @@mapping[struct]
    end
    
    def dump
        "sizeof #{@struct}"
    end
    
    def <=>(other)
        @struct <=> other.struct
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class Immediate < NoChildren
    attr_reader :value
    
    def initialize(codeOrigin, value)
        super(codeOrigin)
        @value = value
        raise "Bad immediate value #{value.inspect} at #{codeOriginString}" unless value.is_a? Integer
    end
    
    def dump
        "#{value}"
    end
    
    def ==(other)
        other.is_a? Immediate and other.value == @value
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class AddImmediates < Node
    attr_reader :left, :right
    
    def initialize(codeOrigin, left, right)
        super(codeOrigin)
        @left = left
        @right = right
    end
    
    def children
        [@left, @right]
    end
    
    def mapChildren
        AddImmediates.new(codeOrigin, (yield @left), (yield @right))
    end
    
    def dump
        "(#{left.dump} + #{right.dump})"
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class SubImmediates < Node
    attr_reader :left, :right
    
    def initialize(codeOrigin, left, right)
        super(codeOrigin)
        @left = left
        @right = right
    end
    
    def children
        [@left, @right]
    end
    
    def mapChildren
        SubImmediates.new(codeOrigin, (yield @left), (yield @right))
    end
    
    def dump
        "(#{left.dump} - #{right.dump})"
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class MulImmediates < Node
    attr_reader :left, :right
    
    def initialize(codeOrigin, left, right)
        super(codeOrigin)
        @left = left
        @right = right
    end
    
    def children
        [@left, @right]
    end
    
    def mapChildren
        MulImmediates.new(codeOrigin, (yield @left), (yield @right))
    end
    
    def dump
        "(#{left.dump} * #{right.dump})"
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class NegImmediate < Node
    attr_reader :child
    
    def initialize(codeOrigin, child)
        super(codeOrigin)
        @child = child
    end
    
    def children
        [@child]
    end
    
    def mapChildren
        NegImmediate.new(codeOrigin, (yield @child))
    end
    
    def dump
        "(-#{@child.dump})"
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class OrImmediates < Node
    attr_reader :left, :right
    
    def initialize(codeOrigin, left, right)
        super(codeOrigin)
        @left = left
        @right = right
    end
    
    def children
        [@left, @right]
    end
    
    def mapChildren
        OrImmediates.new(codeOrigin, (yield @left), (yield @right))
    end
    
    def dump
        "(#{left.dump} | #{right.dump})"
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class AndImmediates < Node
    attr_reader :left, :right
    
    def initialize(codeOrigin, left, right)
        super(codeOrigin)
        @left = left
        @right = right
    end
    
    def children
        [@left, @right]
    end
    
    def mapChildren
        AndImmediates.new(codeOrigin, (yield @left), (yield @right))
    end
    
    def dump
        "(#{left.dump} & #{right.dump})"
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class XorImmediates < Node
    attr_reader :left, :right
    
    def initialize(codeOrigin, left, right)
        super(codeOrigin)
        @left = left
        @right = right
    end
    
    def children
        [@left, @right]
    end
    
    def mapChildren
        XorImmediates.new(codeOrigin, (yield @left), (yield @right))
    end
    
    def dump
        "(#{left.dump} ^ #{right.dump})"
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class BitnotImmediate < Node
    attr_reader :child
    
    def initialize(codeOrigin, child)
        super(codeOrigin)
        @child = child
    end
    
    def children
        [@child]
    end
    
    def mapChildren
        BitnotImmediate.new(codeOrigin, (yield @child))
    end
    
    def dump
        "(~#{@child.dump})"
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        true
    end
    
    def register?
        false
    end
end

class RegisterID < NoChildren
    attr_reader :name
    
    def initialize(codeOrigin, name)
        super(codeOrigin)
        @name = name
    end
    
    @@mapping = {}
    
    def self.forName(codeOrigin, name)
        unless @@mapping[name]
            @@mapping[name] = RegisterID.new(codeOrigin, name)
        end
        @@mapping[name]
    end
    
    def dump
        name
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        false
    end
    
    def register?
        true
    end
end

class FPRegisterID < NoChildren
    attr_reader :name
    
    def initialize(codeOrigin, name)
        super(codeOrigin)
        @name = name
    end
    
    @@mapping = {}
    
    def self.forName(codeOrigin, name)
        unless @@mapping[name]
            @@mapping[name] = FPRegisterID.new(codeOrigin, name)
        end
        @@mapping[name]
    end
    
    def dump
        name
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        false
    end
    
    def register?
        true
    end
end

class SpecialRegister < NoChildren
    def initialize(name)
        @name = name
    end
    
    def address?
        false
    end
    
    def label?
        false
    end
    
    def immediate?
        false
    end
    
    def register?
        true
    end
end

class Variable < NoChildren
    attr_reader :name
    
    def initialize(codeOrigin, name)
        super(codeOrigin)
        @name = name
    end
    
    @@mapping = {}
    
    def self.forName(codeOrigin, name)
        unless @@mapping[name]
            @@mapping[name] = Variable.new(codeOrigin, name)
        end
        @@mapping[name]
    end
    
    def dump
        name
    end
    
    def inspect
        "<variable #{name} at #{codeOriginString}>"
    end
end

class Address < Node
    attr_reader :base, :offset
    
    def initialize(codeOrigin, base, offset)
        super(codeOrigin)
        @base = base
        @offset = offset
        raise "Bad base for address #{base.inspect} at #{codeOriginString}" unless base.is_a? Variable or base.register?
        raise "Bad offset for address #{offset.inspect} at #{codeOriginString}" unless offset.is_a? Variable or offset.immediate?
    end
    
    def withOffset(extraOffset)
        Address.new(codeOrigin, @base, Immediate.new(codeOrigin, @offset.value + extraOffset))
    end
    
    def children
        [@base, @offset]
    end
    
    def mapChildren
        Address.new(codeOrigin, (yield @base), (yield @offset))
    end
    
    def dump
        "#{offset.dump}[#{base.dump}]"
    end
    
    def address?
        true
    end
    
    def label?
        false
    end
    
    def immediate?
        false
    end
    
    def register?
        false
    end
end

class BaseIndex < Node
    attr_reader :base, :index, :scale, :offset
    
    def initialize(codeOrigin, base, index, scale, offset)
        super(codeOrigin)
        @base = base
        @index = index
        @scale = scale
        raise unless [1, 2, 4, 8].member? @scale
        @offset = offset
    end
    
    def scaleShift
        case scale
        when 1
            0
        when 2
            1
        when 4
            2
        when 8
            3
        else
            raise "Bad scale at #{codeOriginString}"
        end
    end
    
    def withOffset(extraOffset)
        BaseIndex.new(codeOrigin, @base, @index, @scale, Immediate.new(codeOrigin, @offset.value + extraOffset))
    end
    
    def children
        [@base, @index, @offset]
    end
    
    def mapChildren
        BaseIndex.new(codeOrigin, (yield @base), (yield @index), @scale, (yield @offset))
    end
    
    def dump
        "#{offset.dump}[#{base.dump}, #{index.dump}, #{scale}]"
    end
    
    def address?
        true
    end
    
    def label?
        false
    end
    
    def immediate?
        false
    end
    
    def register?
        false
    end
end

class AbsoluteAddress < NoChildren
    attr_reader :address
    
    def initialize(codeOrigin, address)
        super(codeOrigin)
        @address = address
    end
    
    def withOffset(extraOffset)
        AbsoluteAddress.new(codeOrigin, Immediate.new(codeOrigin, @address.value + extraOffset))
    end
    
    def dump
        "#{address.dump}[]"
    end
    
    def address?
        true
    end
    
    def label?
        false
    end
    
    def immediate?
        false
    end
    
    def register?
        false
    end
end

class Instruction < Node
    attr_reader :opcode, :operands, :annotation
    
    def initialize(codeOrigin, opcode, operands, annotation=nil)
        super(codeOrigin)
        @opcode = opcode
        @operands = operands
        @annotation = annotation
    end
    
    def children
        operands
    end
    
    def mapChildren(&proc)
        Instruction.new(codeOrigin, @opcode, @operands.map(&proc), @annotation)
    end
    
    def dump
        "\t" + opcode.to_s + " " + operands.collect{|v| v.dump}.join(", ")
    end

    def lowerDefault
        case opcode
        when "localAnnotation"
            $asm.putLocalAnnotation
        when "globalAnnotation"
            $asm.putGlobalAnnotation
        else
            raise "Unhandled opcode #{opcode} at #{codeOriginString}"
        end
    end
end

class Error < NoChildren
    def initialize(codeOrigin)
        super(codeOrigin)
    end
    
    def dump
        "\terror"
    end
end

class ConstDecl < Node
    attr_reader :variable, :value
    
    def initialize(codeOrigin, variable, value)
        super(codeOrigin)
        @variable = variable
        @value = value
    end
    
    def children
        [@variable, @value]
    end
    
    def mapChildren
        ConstDecl.new(codeOrigin, (yield @variable), (yield @value))
    end
    
    def dump
        "const #{@variable.dump} = #{@value.dump}"
    end
end

$labelMapping = {}

class Label < NoChildren
    attr_reader :name
    
    def initialize(codeOrigin, name)
        super(codeOrigin)
        @name = name
    end
    
    def self.forName(codeOrigin, name)
        if $labelMapping[name]
            raise "Label name collision: #{name}" unless $labelMapping[name].is_a? Label
        else
            $labelMapping[name] = Label.new(codeOrigin, name)
        end
        $labelMapping[name]
    end
    
    def dump
        "#{name}:"
    end
end

class LocalLabel < NoChildren
    attr_reader :name
    
    def initialize(codeOrigin, name)
        super(codeOrigin)
        @name = name
    end

    @@uniqueNameCounter = 0
    
    def self.forName(codeOrigin, name)
        if $labelMapping[name]
            raise "Label name collision: #{name}" unless $labelMapping[name].is_a? LocalLabel
        else
            $labelMapping[name] = LocalLabel.new(codeOrigin, name)
        end
        $labelMapping[name]
    end
    
    def self.unique(comment)
        newName = "_#{comment}"
        if $labelMapping[newName]
            while $labelMapping[newName = "_#{@@uniqueNameCounter}_#{comment}"]
                @@uniqueNameCounter += 1
            end
        end
        forName(nil, newName)
    end
    
    def cleanName
        if name =~ /^\./
            "_" + name[1..-1]
        else
            name
        end
    end
    
    def dump
        "#{name}:"
    end
end

class LabelReference < Node
    attr_reader :label
    
    def initialize(codeOrigin, label)
        super(codeOrigin)
        @label = label
    end
    
    def children
        [@label]
    end
    
    def mapChildren
        LabelReference.new(codeOrigin, (yield @label))
    end
    
    def name
        label.name
    end
    
    def dump
        label.name
    end
    
    def address?
        false
    end
    
    def label?
        true
    end
    
    def immediate?
        false
    end
end

class LocalLabelReference < NoChildren
    attr_reader :label
    
    def initialize(codeOrigin, label)
        super(codeOrigin)
        @label = label
    end
    
    def children
        [@label]
    end
    
    def mapChildren
        LocalLabelReference.new(codeOrigin, (yield @label))
    end
    
    def name
        label.name
    end
    
    def dump
        label.name
    end
    
    def address?
        false
    end
    
    def label?
        true
    end
    
    def immediate?
        false
    end
end

class Sequence < Node
    attr_reader :list
    
    def initialize(codeOrigin, list)
        super(codeOrigin)
        @list = list
    end
    
    def children
        list
    end
    
    def mapChildren(&proc)
        Sequence.new(codeOrigin, @list.map(&proc))
    end
    
    def dump
        list.collect{|v| v.dump}.join("\n")
    end
end

class True < NoChildren
    def initialize
        super(nil)
    end
    
    @@instance = True.new
    
    def self.instance
        @@instance
    end
    
    def value
        true
    end
    
    def dump
        "true"
    end
end

class False < NoChildren
    def initialize
        super(nil)
    end
    
    @@instance = False.new
    
    def self.instance
        @@instance
    end
    
    def value
        false
    end
    
    def dump
        "false"
    end
end

class TrueClass
    def asNode
        True.instance
    end
end

class FalseClass
    def asNode
        False.instance
    end
end

class Setting < NoChildren
    attr_reader :name
    
    def initialize(codeOrigin, name)
        super(codeOrigin)
        @name = name
    end
    
    @@mapping = {}
    
    def self.forName(codeOrigin, name)
        unless @@mapping[name]
            @@mapping[name] = Setting.new(codeOrigin, name)
        end
        @@mapping[name]
    end
    
    def dump
        name
    end
end

class And < Node
    attr_reader :left, :right
    
    def initialize(codeOrigin, left, right)
        super(codeOrigin)
        @left = left
        @right = right
    end
    
    def children
        [@left, @right]
    end
    
    def mapChildren
        And.new(codeOrigin, (yield @left), (yield @right))
    end
    
    def dump
        "(#{left.dump} and #{right.dump})"
    end
end

class Or < Node
    attr_reader :left, :right
    
    def initialize(codeOrigin, left, right)
        super(codeOrigin)
        @left = left
        @right = right
    end
    
    def children
        [@left, @right]
    end
    
    def mapChildren
        Or.new(codeOrigin, (yield @left), (yield @right))
    end
    
    def dump
        "(#{left.dump} or #{right.dump})"
    end
end

class Not < Node
    attr_reader :child
    
    def initialize(codeOrigin, child)
        super(codeOrigin)
        @child = child
    end
    
    def children
        [@child]
    end
    
    def mapChildren
        Not.new(codeOrigin, (yield @child))
    end
    
    def dump
        "(not #{child.dump})"
    end
end

class Skip < NoChildren
    def initialize(codeOrigin)
        super(codeOrigin)
    end
    
    def dump
        "\tskip"
    end
end

class IfThenElse < Node
    attr_reader :predicate, :thenCase
    attr_accessor :elseCase
    
    def initialize(codeOrigin, predicate, thenCase)
        super(codeOrigin)
        @predicate = predicate
        @thenCase = thenCase
        @elseCase = Skip.new(codeOrigin)
    end
    
    def children
        if @elseCase
            [@predicate, @thenCase, @elseCase]
        else
            [@predicate, @thenCase]
        end
    end
    
    def mapChildren
        IfThenElse.new(codeOrigin, (yield @predicate), (yield @thenCase), (yield @elseCase))
    end
    
    def dump
        "if #{predicate.dump}\n" + thenCase.dump + "\nelse\n" + elseCase.dump + "\nend"
    end
end

class Macro < Node
    attr_reader :name, :variables, :body

    def initialize(codeOrigin, name, variables, body)
        super(codeOrigin)
        @name = name
        @variables = variables
        @body = body
    end
    
    def children
        @variables + [@body]
    end
    
    def mapChildren
        Macro.new(codeOrigin, @name, @variables.map{|v| yield v}, (yield @body))
    end
    
    def dump
        "macro #{name}(" + variables.collect{|v| v.dump}.join(", ") + ")\n" + body.dump + "\nend"
    end
end

class MacroCall < Node
    attr_reader :name, :operands, :annotation
    
    def initialize(codeOrigin, name, operands, annotation)
        super(codeOrigin)
        @name = name
        @operands = operands
        raise unless @operands
        @operands.each{|v| raise unless v}
        @annotation = annotation
    end
    
    def children
        @operands
    end
    
    def mapChildren(&proc)
        MacroCall.new(codeOrigin, @name, @operands.map(&proc), @annotation)
    end
    
    def dump
        "\t#{name}(" + operands.collect{|v| v.dump}.join(", ") + ")"
    end
end

