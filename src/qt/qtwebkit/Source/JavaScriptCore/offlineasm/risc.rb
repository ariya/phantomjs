# Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

require 'config'
require 'ast'
require 'opt'

# This file contains utilities that are useful for implementing a backend
# for RISC-like processors (ARM, MIPS, etc).

#
# Lowering of simple branch ops. For example:
#
# baddiz foo, bar, baz
#
# will become:
#
# addi foo, bar
# bz baz
#

def riscLowerSimpleBranchOps(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            annotation = node.annotation
            case node.opcode
            when /^b(addi|subi|ori|addp)/
                op = $1
                branch = "b" + $~.post_match
                
                case op
                when "addi"
                    op = "addis"
                when "addp"
                    op = "addps"
                when "subi"
                    op = "subis"
                when "ori"
                    op = "oris"
                end
                
                newList << Instruction.new(node.codeOrigin, op, node.operands[0..-2], annotation)
                newList << Instruction.new(node.codeOrigin, branch, [node.operands[-1]])
            when 'bmulis', 'bmulz', 'bmulnz'
                condition = $~.post_match
                newList << Instruction.new(node.codeOrigin, "muli", node.operands[0..-2], annotation)
                newList << Instruction.new(node.codeOrigin, "bti" + condition, [node.operands[-2], node.operands[-1]])
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

#
# Lowing of complex branch ops. For example:
#
# bmulio foo, bar, baz
#
# becomes:
#
# smulli foo, bar, bar, tmp1
# rshifti bar, 31, tmp2
# bineq tmp1, tmp2, baz
#

def riscLowerHardBranchOps(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction and node.opcode == "bmulio"
            tmp1 = Tmp.new(node.codeOrigin, :gpr)
            tmp2 = Tmp.new(node.codeOrigin, :gpr)
            newList << Instruction.new(node.codeOrigin, "smulli", [node.operands[0], node.operands[1], node.operands[1], tmp1], node.annotation)
            newList << Instruction.new(node.codeOrigin, "rshifti", [node.operands[-2], Immediate.new(node.codeOrigin, 31), tmp2])
            newList << Instruction.new(node.codeOrigin, "bineq", [tmp1, tmp2, node.operands[-1]])
        else
            newList << node
        end
    }
    newList
end

#
# Lowering of shift ops. For example:
#
# lshifti foo, bar
#
# will become:
#
# andi foo, 31, tmp
# lshifti tmp, bar
#

def riscSanitizeShift(operand, list)
    return operand if operand.immediate?
    
    tmp = Tmp.new(operand.codeOrigin, :gpr)
    list << Instruction.new(operand.codeOrigin, "andi", [operand, Immediate.new(operand.codeOrigin, 31), tmp])
    tmp
end

def riscLowerShiftOps(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "lshifti", "rshifti", "urshifti", "lshiftp", "rshiftp", "urshiftp"
                if node.operands.size == 2
                    newList << Instruction.new(node.codeOrigin, node.opcode, [riscSanitizeShift(node.operands[0], newList), node.operands[1]], node.annotation)
                else
                    newList << Instruction.new(node.codeOrigin, node.opcode, [node.operands[0], riscSanitizeShift(node.operands[1], newList), node.operands[2]], node.annotation)
                    raise "Wrong number of operands for shift at #{node.codeOriginString}" unless node.operands.size == 3
                end
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

#
# Lowering of malformed addresses. For example:
#
# loadp 10000[foo], bar
#
# will become:
#
# move 10000, tmp
# addp foo, tmp
# loadp 0[tmp], bar
#
# Note that you use this lowering phase by passing it a block that returns true
# if you don't want to lower the address, or false if you do. For example to get
# the effect of the example above, the block would have to say something like:
#
# riscLowerMalformedAddresses(thingy) {
#     | node, address |
#     if address.is_a? Address
#         address.offset.value > 1000
#     else
#         true # don't lower anything else, in this example
#     end
# }
#
# See arm.rb for a different example, in which we lower all BaseIndex addresses
# that have non-zero offset, all Address addresses that have large offsets, and
# all other addresses (like AbsoluteAddress).
#

class Node
    def riscLowerMalformedAddressesRecurse(list, topLevelNode, &block)
        mapChildren {
            | subNode |
            subNode.riscLowerMalformedAddressesRecurse(list, topLevelNode, &block)
        }
    end
end

class Address
    def riscLowerMalformedAddressesRecurse(list, node, &block)
        return self if yield node, self

        tmp = Tmp.new(codeOrigin, :gpr)
        list << Instruction.new(codeOrigin, "move", [offset, tmp])
        list << Instruction.new(codeOrigin, "addp", [base, tmp])
        Address.new(codeOrigin, tmp, Immediate.new(codeOrigin, 0))
    end
end

class BaseIndex
    def riscLowerMalformedAddressesRecurse(list, node, &block)
        return self if yield node, self
        
        tmp = Tmp.new(codeOrigin, :gpr)
        list << Instruction.new(codeOrigin, "leap", [BaseIndex.new(codeOrigin, base, index, scale, Immediate.new(codeOrigin, 0)), tmp])
        Address.new(codeOrigin, tmp, offset).riscLowerMalformedAddressesRecurse(list, node, &block)
    end
end

class AbsoluteAddress
    def riscLowerMalformedAddressesRecurse(list, node, &block)
        return self if yield node, self
        
        tmp = Tmp.new(codeOrigin, :gpr)
        list << Instruction.new(codeOrigin, "move", [address, tmp])
        Address.new(codeOrigin, tmp, Immediate.new(codeOrigin, 0))
    end
end

def riscLowerMalformedAddresses(list, &block)
    newList = []
    list.each {
        | node |
        newList << node.riscLowerMalformedAddressesRecurse(newList, node, &block)
    }
    newList
end

#
# Lowering of malformed addresses in double loads and stores. For example:
#
# loadd [foo, bar, 8], baz
#
# becomes:
#
# leap [foo, bar, 8], tmp
# loadd [tmp], baz
#

class Node
    def riscDoubleAddress(list)
        self
    end
end

class BaseIndex
    def riscDoubleAddress(list)
        tmp = Tmp.new(codeOrigin, :gpr)
        list << Instruction.new(codeOrigin, "leap", [self, tmp])
        Address.new(codeOrigin, tmp, Immediate.new(codeOrigin, 0))
    end
end

def riscLowerMalformedAddressesDouble(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "loadd"
                newList << Instruction.new(node.codeOrigin, "loadd", [node.operands[0].riscDoubleAddress(newList), node.operands[1]], node.annotation)
            when "stored"
                newList << Instruction.new(node.codeOrigin, "stored", [node.operands[0], node.operands[1].riscDoubleAddress(newList)], node.annotation)
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

#
# Lowering of misplaced immediates for opcodes in opcodeList. For example, if storei is in opcodeList:
#
# storei 0, [foo]
#
# will become:
#
# move 0, tmp
# storei tmp, [foo]
#

def riscLowerMisplacedImmediates(list, opcodeList)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            if opcodeList.include? node.opcode
                operands = node.operands
                newOperands = []
                operands.each {
                    | operand |
                    if operand.is_a? Immediate
                        tmp = Tmp.new(operand.codeOrigin, :gpr)
                        newList << Instruction.new(operand.codeOrigin, "move", [operand, tmp])
                        newOperands << tmp
                    else
                        newOperands << operand
                    end
                }
                newList << Instruction.new(node.codeOrigin, node.opcode, newOperands, node.annotation)
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

#
# Lowering of malformed immediates except when used in a "move" instruction.
# For example:
#
# addp 642641, foo
#
# will become:
#
# move 642641, tmp
# addp tmp, foo
#

class Node
    def riscLowerMalformedImmediatesRecurse(list, validImmediates)
        mapChildren {
            | node |
            node.riscLowerMalformedImmediatesRecurse(list, validImmediates)
        }
    end
end

class Address
    def riscLowerMalformedImmediatesRecurse(list, validImmediates)
        self
    end
end

class BaseIndex
    def riscLowerMalformedImmediatesRecurse(list, validImmediates)
        self
    end
end

class AbsoluteAddress
    def riscLowerMalformedImmediatesRecurse(list, validImmediates)
        self
    end
end

class Immediate
    def riscLowerMalformedImmediatesRecurse(list, validImmediates)
        unless validImmediates.include? value
            tmp = Tmp.new(codeOrigin, :gpr)
            list << Instruction.new(codeOrigin, "move", [self, tmp])
            tmp
        else
            self
        end
    end
end

def riscLowerMalformedImmediates(list, validImmediates)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            annotation = node.annotation
            case node.opcode
            when "move"
                newList << node
            when "addi", "addp", "addis", "subi", "subp", "subis"
                if node.operands[0].is_a? Immediate and
                        (not validImmediates.include? node.operands[0].value) and
                        validImmediates.include? -node.operands[0].value
                        node.operands.size == 2
                    if node.opcode =~ /add/
                        newOpcode = "sub" + $~.post_match
                    else
                        newOpcode = "add" + $~.post_match
                    end
                    newList << Instruction.new(node.codeOrigin, newOpcode,
                                               [Immediate.new(node.codeOrigin, -node.operands[0].value)] + node.operands[1..-1],
                                               annotation)
                else
                    newList << node.riscLowerMalformedImmediatesRecurse(newList, validImmediates)
                end
            when "muli", "mulp"
                if node.operands[0].is_a? Immediate
                    tmp = Tmp.new(codeOrigin, :gpr)
                    newList << Instruction.new(node.codeOrigin, "move", [node.operands[0], tmp], annotation)
                    newList << Instruction.new(node.codeOrigin, node.opcode, [tmp] + node.operands[1..-1])
                else
                    newList << node.riscLowerMalformedImmediatesRecurse(newList, validImmediates)
                end
            else
                newList << node.riscLowerMalformedImmediatesRecurse(newList, validImmediates)
            end
        else
            newList << node
        end
    }
    newList
end

#
# Lowering of misplaced addresses. For example:
#
# addi foo, [bar]
#
# will become:
#
# loadi [bar], tmp
# addi foo, tmp
# storei tmp, [bar]
#
# Another example:
#
# addi [foo], bar
#
# will become:
#
# loadi [foo], tmp
# addi tmp, bar
#

def riscAsRegister(preList, postList, operand, suffix, needStore)
    return operand unless operand.address?
    
    tmp = Tmp.new(operand.codeOrigin, if suffix == "d" then :fpr else :gpr end)
    preList << Instruction.new(operand.codeOrigin, "load" + suffix, [operand, tmp])
    if needStore
        postList << Instruction.new(operand.codeOrigin, "store" + suffix, [tmp, operand])
    end
    tmp
end

def riscAsRegisters(preList, postList, operands, suffix)
    newOperands = []
    operands.each_with_index {
        | operand, index |
        newOperands << riscAsRegister(preList, postList, operand, suffix, index == operands.size - 1)
    }
    newOperands
end

def riscLowerMisplacedAddresses(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            postInstructions = []
            annotation = node.annotation
            case node.opcode
            when "addi", "addis", "andi", "lshifti", "muli", "negi", "noti", "ori", "oris",
                "rshifti", "urshifti", "subi", "subis", "xori", /^bi/, /^bti/, /^ci/, /^ti/
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           riscAsRegisters(newList, postInstructions, node.operands, "i"),
                                           annotation)
            when "addp", "andp", "lshiftp", "mulp", "negp", "orp", "rshiftp", "urshiftp",
                "subp", "xorp", /^bp/, /^btp/, /^cp/
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           riscAsRegisters(newList, postInstructions, node.operands, "p"),
                                           annotation)
            when "bbeq", "bbneq", "bba", "bbaeq", "bbb", "bbbeq", "btbz", "btbnz", "tbz", "tbnz",
                "cbeq", "cbneq", "cba", "cbaeq", "cbb", "cbbeq"
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           riscAsRegisters(newList, postInstructions, node.operands, "b"),
                                           annotation)
            when "bbgt", "bbgteq", "bblt", "bblteq", "btbs", "tbs", "cbgt", "cbgteq", "cblt", "cblteq"
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           riscAsRegisters(newList, postInstructions, node.operands, "bs"),
                                           annotation)
            when "addd", "divd", "subd", "muld", "sqrtd", /^bd/
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           riscAsRegisters(newList, postInstructions, node.operands, "d"),
                                           annotation)
            when "jmp", "call"
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           [riscAsRegister(newList, postInstructions, node.operands[0], "p", false)],
                                           annotation)
            else
                newList << node
            end
            newList += postInstructions
        else
            newList << node
        end
    }
    newList
end

#
# Lowering of register reuse in compare instructions. For example:
#
# cieq t0, t1, t0
#
# will become:
#
# mov tmp, t0
# cieq tmp, t1, t0
#

def riscLowerRegisterReuse(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            annotation = node.annotation
            case node.opcode
            when "cieq", "cineq", "cia", "ciaeq", "cib", "cibeq", "cigt", "cigteq", "cilt", "cilteq",
                "cpeq", "cpneq", "cpa", "cpaeq", "cpb", "cpbeq", "cpgt", "cpgteq", "cplt", "cplteq",
                "tis", "tiz", "tinz", "tbs", "tbz", "tbnz", "tps", "tpz", "tpnz", "cbeq", "cbneq",
                "cba", "cbaeq", "cbb", "cbbeq", "cbgt", "cbgteq", "cblt", "cblteq"
                if node.operands.size == 2
                    if node.operands[0] == node.operands[1]
                        tmp = Tmp.new(node.codeOrigin, :gpr)
                        newList << Instruction.new(node.codeOrigin, "move", [node.operands[0], tmp], annotation)
                        newList << Instruction.new(node.codeOrigin, node.opcode, [tmp, node.operands[1]])
                    else
                        newList << node
                    end
                else
                    raise "Wrong number of arguments at #{node.codeOriginString}" unless node.operands.size == 3
                    if node.operands[0] == node.operands[2]
                        tmp = Tmp.new(node.codeOrigin, :gpr)
                        newList << Instruction.new(node.codeOrigin, "move", [node.operands[0], tmp], annotation)
                        newList << Instruction.new(node.codeOrigin, node.opcode, [tmp, node.operands[1], node.operands[2]])
                    elsif node.operands[1] == node.operands[2]
                        tmp = Tmp.new(node.codeOrigin, :gpr)
                        newList << Instruction.new(node.codeOrigin, "move", [node.operands[1], tmp], annotation)
                        newList << Instruction.new(node.codeOrigin, node.opcode, [node.operands[0], tmp, node.operands[2]])
                    else
                        newList << node
                    end
                end
            else
                newList << node
            end
        else
            newList << node
        end
    }
    newList
end

