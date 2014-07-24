# Copyright (C) 2012 Apple Inc. All rights reserved.
# Copyright (C) 2012 MIPS Technologies, Inc. All rights reserved.
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
# THIS SOFTWARE IS PROVIDED BY MIPS TECHNOLOGIES, INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL MIPS TECHNOLOGIES, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

require 'risc'

class Assembler
    def putStr(str)
        @outp.puts str
    end
end

class Node
    def mipsSingleHi
        doubleOperand = mipsOperand
        raise "Bogus register name #{doubleOperand}" unless doubleOperand =~ /^\$f/
        "$f" + ($~.post_match.to_i + 1).to_s
    end
    def mipsSingleLo
        doubleOperand = mipsOperand
        raise "Bogus register name #{doubleOperand}" unless doubleOperand =~ /^\$f/
        doubleOperand
    end
end

class SpecialRegister < NoChildren
    def mipsOperand
        @name
    end

    def dump
        @name
    end

    def register?
        true
    end
end

MIPS_TEMP_GPRS = [SpecialRegister.new("$t5"), SpecialRegister.new("$t6"), SpecialRegister.new("$t7"),
                    SpecialRegister.new("$t8")]
MIPS_ZERO_REG = SpecialRegister.new("$zero")
MIPS_GP_REG = SpecialRegister.new("$gp")
MIPS_GPSAVE_REG = SpecialRegister.new("$s4")
MIPS_CALL_REG = SpecialRegister.new("$t9")
MIPS_TEMP_FPRS = [SpecialRegister.new("$f16")]
MIPS_SCRATCH_FPR = SpecialRegister.new("$f18")

def mipsMoveImmediate(value, register)
    if value == 0
        $asm.puts "add #{register.mipsOperand}, $zero, $zero"
    else
        $asm.puts "li #{register.mipsOperand}, #{value}"
    end
end

class RegisterID
    def mipsOperand
        case name
        when "a0"
            "$a0"
        when "a1"
            "$a1"
        when "r0", "t0"
            "$v0"
        when "r1", "t1"
            "$v1"
        when "t2"
            "$t2"
        when "t3"
            "$s3"
        when "t4"   # PC reg in llint
            "$s2"
        when "t5"
            "$t5"
        when "t6"
            "$t6"
        when "t7"
            "$t7"
        when "t8"
            "$t8"
        when "cfr"
            "$s0"
        when "lr"
            "$ra"
        when "sp"
            "$sp"
        else
            raise "Bad register #{name} for MIPS at #{codeOriginString}"
        end
    end
end

class FPRegisterID
    def mipsOperand
        case name
        when "ft0", "fr"
            "$f0"
        when "ft1"
            "$f2"
        when "ft2"
            "$f4"
        when "ft3"
            "$f6"
        when "ft4"
            "$f8"
        when "ft5"
            "$f10"
        when "fa0"
            "$f12"
        when "fa1"
            "$f14"
        else
            raise "Bad register #{name} for MIPS at #{codeOriginString}"
        end
    end
end

class Immediate
    def mipsOperand
        raise "Invalid immediate #{value} at #{codeOriginString}" if value < -0x7fff or value > 0x7fff
        "#{value}"
    end
end

class Address
    def mipsOperand
        raise "Bad offset at #{codeOriginString}" if offset.value < -0x7fff or offset.value > 0x7fff
        "#{offset.value}(#{base.mipsOperand})"
    end
end

class AbsoluteAddress
    def mipsOperand
        raise "Unconverted absolute address at #{codeOriginString}"
    end
end

#
# Negate condition of branches to labels.
#

class Instruction
    def mipsNegateCondition(list)
        /^(b(add|sub|or|mul|t)?)([ipb])/.match(opcode)
        case $~.post_match
        when "eq"
            op = "neq"
        when "neq"
            op = "eq"
        when "z"
            op = "nz"
        when "nz"
            op = "z"
        when "gt"
            op = "lteq"
        when "gteq"
            op = "lt"
        when "lt"
            op = "gteq"
        when "lteq"
            op = "gt"
        when "a"
            op = "beq"
        when "b"
            op = "aeq"
        when "aeq"
            op = "b"
        when "beq"
            op = "a"
        else
            raise "Can't negate #{opcode} branch."
        end
        noBranch = LocalLabel.unique("nobranch")
        noBranchRef = LocalLabelReference.new(codeOrigin, noBranch)
        toRef = operands[-1]
        list << Instruction.new(codeOrigin, "#{$1}#{$3}#{op}", operands[0..-2].push(noBranchRef), annotation)
        list << Instruction.new(codeOrigin, "la", [toRef, MIPS_CALL_REG])
        list << Instruction.new(codeOrigin, "jmp", [MIPS_CALL_REG])
        list << noBranch
    end
end

def mipsLowerFarBranchOps(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            annotation = node.annotation
            case node.opcode
            when /^b(add|sub|or|mul|t)?([ipb])/
                if node.operands[-1].is_a? LabelReference
                    node.mipsNegateCondition(newList)
                    next
                end
            end
        end
        newList << node
    }
    newList
end

#
# Lower 'and' masked branches
#

def lowerMIPSCondBranch(list, condOp, node)
    if node.operands.size == 2
        list << Instruction.new(node.codeOrigin,
                                condOp,
                                [node.operands[0], MIPS_ZERO_REG, node.operands[-1]],
                                node.annotation)
    elsif node.operands.size == 3
        tmp = Tmp.new(node.codeOrigin, :gpr)
        list << Instruction.new(node.codeOrigin,
                                "andi",
                                [node.operands[0], node.operands[1], tmp],
                                node.annotation)
        list << Instruction.new(node.codeOrigin,
                                condOp,
                                [tmp, MIPS_ZERO_REG, node.operands[-1]])
    else
        raise "Expected 2 or 3 operands but got #{node.operands.size} at #{node.codeOriginString}"
    end
end

#
# Lowering of branch ops. For example:
#
# baddiz foo, bar, baz
#
# will become:
#
# addi foo, bar
# bz baz
#

def mipsLowerSimpleBranchOps(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            annotation = node.annotation
            case node.opcode
            when /^b(addi|subi|ori|addp)/
                op = $1
                bc = $~.post_match
                branch = "b" + bc

                case op
                when "addi", "addp"
                    op = "addi"
                when "subi"
                    op = "subi"
                when "ori"
                    op = "ori"
                end

                if bc == "o"
                    case op
                    when "addi"
                        #  addu $s0, $s1, $s2
                        #  xor $t0, $s1, $s2
                        #  blt $t0, $zero, no overflow
                        #  xor $t0, $s0, $s1
                        #  blt $t0, $zero, overflow
                        # no overflow:
                        #
                        tr = Tmp.new(node.codeOrigin, :gpr)
                        tmp = Tmp.new(node.codeOrigin, :gpr)
                        noFlow = LocalLabel.unique("noflow")
                        noFlowRef = LocalLabelReference.new(node.codeOrigin, noFlow)
                        newList << Instruction.new(node.codeOrigin, op, [node.operands[0], node.operands[1], tr], annotation)
                        newList << Instruction.new(node.codeOrigin, "xori", [node.operands[0], node.operands[1], tmp])
                        newList << Instruction.new(node.codeOrigin, "bilt", [tmp, MIPS_ZERO_REG, noFlowRef])
                        newList << Instruction.new(node.codeOrigin, "xori", [tr, node.operands[0], tmp])
                        newList << Instruction.new(node.codeOrigin, "bilt", [tmp, MIPS_ZERO_REG, node.operands[2]])
                        newList << noFlow
                        newList << Instruction.new(node.codeOrigin, "move", [tr, node.operands[1]])
                    when "subi"
                        #  subu $s0, $s1, $s2
                        #  xor $t0, $s1, $s2
                        #  bge $t0, $zero, no overflow
                        #  xor $t0, $s0, $s1
                        #  blt $t0, $zero, overflow
                        # no overflow:
                        #
                        tr = Tmp.new(node.codeOrigin, :gpr)
                        tmp = Tmp.new(node.codeOrigin, :gpr)
                        noFlow = LocalLabel.unique("noflow")
                        noFlowRef = LocalLabelReference.new(node.codeOrigin, noFlow)
                        newList << Instruction.new(node.codeOrigin, op, [node.operands[1], node.operands[0], tr], annotation)
                        newList << Instruction.new(node.codeOrigin, "xori", [node.operands[1], node.operands[0], tmp])
                        newList << Instruction.new(node.codeOrigin, "bigteq", [tmp, MIPS_ZERO_REG, noFlowRef])
                        newList << Instruction.new(node.codeOrigin, "xori", [tr, node.operands[1], tmp])
                        newList << Instruction.new(node.codeOrigin, "bilt", [tmp, MIPS_ZERO_REG, node.operands[2]])
                        newList << noFlow
                        newList << Instruction.new(node.codeOrigin, "move", [tr, node.operands[1]])
                    when "ori"
                        # no ovwerflow at ori
                        newList << Instruction.new(node.codeOrigin, op, node.operands[0..1], annotation)
                    end
                else
                    if node.operands[1].is_a? Address
                        addr = node.operands[1]
                        tr = Tmp.new(node.codeOrigin, :gpr)
                        newList << Instruction.new(node.codeOrigin, "loadp", [addr, tr], annotation)
                        newList << Instruction.new(node.codeOrigin, op, [node.operands[0], tr])
                        newList << Instruction.new(node.codeOrigin, "storep", [tr, addr])
                    else
                        tr = node.operands[1]
                        newList << Instruction.new(node.codeOrigin, op, node.operands[0..-2], annotation)
                    end
                    newList << Instruction.new(node.codeOrigin, branch, [tr, MIPS_ZERO_REG, node.operands[-1]])
                end
            when "bia", "bpa", "bba"
                tmp = Tmp.new(node.codeOrigin, :gpr)
                comp = node.opcode[1] == ?b ? "sltub" : "sltu"
                newList << Instruction.new(node.codeOrigin, comp, [tmp, node.operands[1], node.operands[0]], annotation)
                newList << Instruction.new(node.codeOrigin, "bnz", [tmp, MIPS_ZERO_REG, node.operands[2]])
            when "biaeq", "bpaeq", "bbaeq"
                tmp = Tmp.new(node.codeOrigin, :gpr)
                comp = node.opcode[1] == ?b ? "sltub" : "sltu"
                newList << Instruction.new(node.codeOrigin, comp, [tmp, node.operands[0], node.operands[1]], annotation)
                newList << Instruction.new(node.codeOrigin, "bz", [tmp, MIPS_ZERO_REG, node.operands[2]])
            when "bib", "bpb", "bbb"
                tmp = Tmp.new(node.codeOrigin, :gpr)
                comp = node.opcode[1] == ?b ? "sltub" : "sltu"
                newList << Instruction.new(node.codeOrigin, comp, [tmp, node.operands[0], node.operands[1]], annotation)
                newList << Instruction.new(node.codeOrigin, "bnz", [tmp, MIPS_ZERO_REG, node.operands[2]])
            when "bibeq", "bpbeq", "bbbeq"
                tmp = Tmp.new(node.codeOrigin, :gpr)
                comp = node.opcode[1] == ?b ? "sltub" : "sltu"
                newList << Instruction.new(node.codeOrigin, comp, [tmp, node.operands[1], node.operands[0]], annotation)
                newList << Instruction.new(node.codeOrigin, "bz", [tmp, MIPS_ZERO_REG, node.operands[2]])
            when /^bt(i|p|b)/
                lowerMIPSCondBranch(newList, "b" + $~.post_match + $1, node)
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
# Specialization of lowering of malformed BaseIndex addresses.
#

class Node
    def mipsLowerMalformedAddressesRecurse(list, topLevelNode, &block)
        mapChildren {
            | subNode |
            subNode.mipsLowerMalformedAddressesRecurse(list, topLevelNode, &block)
        }
    end
end

class Address
    def mipsLowerMalformedAddressesRecurse(list, node, &block)
        riscLowerMalformedAddressesRecurse(list, node, &block)
    end
end

class BaseIndex
    def mipsLowerMalformedAddressesRecurse(list, node, &block)
        if scaleShift == 0
            tmp0 = Tmp.new(codeOrigin, :gpr)
            list << Instruction.new(codeOrigin, "addp", [base, index, tmp0])
            Address.new(codeOrigin, tmp0, Immediate.new(codeOrigin, offset.value));
        else
            tmp0 = Tmp.new(codeOrigin, :gpr)
            list << Instruction.new(codeOrigin, "lshifti", [index, Immediate.new(codeOrigin, scaleShift), tmp0]);
            list << Instruction.new(codeOrigin, "addp", [base, tmp0])
            Address.new(codeOrigin, tmp0, Immediate.new(codeOrigin, offset.value));
        end
    end
end

class AbsoluteAddress
    def mipsLowerMalformedAddressesRecurse(list, node, &block)
        riscLowerMalformedAddressesRecurse(list, node, &block)
    end
end

def mipsLowerMalformedAddresses(list, &block)
    newList = []
    list.each {
        | node |
        newList << node.mipsLowerMalformedAddressesRecurse(newList, node, &block)
    }
    newList
end

#
# Lowering of misplaced immediates of MIPS specific instructions. For example:
#
# sltu reg, 4, 2
#
# will become:
#
# move 4, tmp
# sltu reg, tmp, 2
#

def mipsLowerMisplacedImmediates(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "slt", "sltu", "sltb", "sltub"
                if node.operands[1].is_a? Immediate
                    tmp = Tmp.new(node.codeOrigin, :gpr)
                    newList << Instruction.new(node.codeOrigin, "move", [node.operands[1], tmp], node.annotation)
                    newList << Instruction.new(node.codeOrigin, node.opcode,
                                               [node.operands[0], tmp, node.operands[2]],
                                               node.annotation)
                else
                    newList << node
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
# Specialization of lowering of misplaced addresses.
#

class LocalLabelReference
    def register?
        false
    end
end

def mipsAsRegister(preList, postList, operand, needRestore)
    tmp = MIPS_CALL_REG
    if operand.address?
        preList << Instruction.new(operand.codeOrigin, "loadp", [operand, MIPS_CALL_REG])
    elsif operand.is_a? LabelReference
        preList << Instruction.new(operand.codeOrigin, "la", [operand, MIPS_CALL_REG])
    elsif operand.register? and operand != MIPS_CALL_REG
        preList << Instruction.new(operand.codeOrigin, "move", [operand, MIPS_CALL_REG])
    else
        needRestore = false
        tmp = operand
    end
    if needRestore
        postList << Instruction.new(operand.codeOrigin, "move", [MIPS_GPSAVE_REG, MIPS_GP_REG])
    end
    tmp
end

def mipsLowerMisplacedAddresses(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            postInstructions = []
            annotation = node.annotation
            case node.opcode
            when "jmp"
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           [mipsAsRegister(newList, [], node.operands[0], false)])
            when "call"
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           [mipsAsRegister(newList, postInstructions, node.operands[0], true)])
            when "slt", "sltu"
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           riscAsRegisters(newList, [], node.operands, "i"))
            when "sltub", "sltb"
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           riscAsRegisters(newList, [], node.operands, "b"))
            when /^(bz|bnz|bs|bo)/
                tl = $~.post_match == "" ? "i" : $~.post_match
                newList << Instruction.new(node.codeOrigin,
                                           node.opcode,
                                           riscAsRegisters(newList, [], node.operands, tl))
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
# Lowering compares and tests.
#

def mipsLowerCompareTemplate(list, node, opCmp, opMov)
    tmp0 = Tmp.new(node.codeOrigin, :gpr)
    tmp1 = Tmp.new(node.codeOrigin, :gpr)
    list << Instruction.new(node.codeOrigin, "move", [Immediate.new(nil, 0), node.operands[2]])
    list << Instruction.new(node.codeOrigin, opCmp, [node.operands[1], node.operands[0], tmp0])
    list << Instruction.new(node.codeOrigin, "move", [Immediate.new(nil, 1), tmp1])
    list << Instruction.new(node.codeOrigin, opMov, [node.operands[2], tmp1, tmp0])
end

def mipsLowerCompares(list)
    newList = []
    list.each {
        | node |
        if node.is_a? Instruction
            case node.opcode
            when "cieq", "cpeq", "cbeq"
                mipsLowerCompareTemplate(newList, node, "subp", "movz")
            when "cineq", "cpneq", "cbneq"
                mipsLowerCompareTemplate(newList, node, "subp", "movn")
            when "tiz", "tbz", "tpz"
                mipsLowerCompareTemplate(newList, node, "andp", "movz")
            when "tinz", "tbnz", "tpnz"
                mipsLowerCompareTemplate(newList, node, "andp", "movn")
            when "tio", "tbo", "tpo"
                tmp = Tmp.new(node.codeOrigin, :gpr)
                list << Instruction.new(node.codeOrigin, "andp", [node.operands[1], node.operands[0], tmp])
                list << Instruction.new(node.codeOrigin, "slt", [node.operands[2], MIPS_ZERO_REG, tmp])
            when "tis", "tbs", "tps"
                tmp = Tmp.new(node.codeOrigin, :gpr)
                list << Instruction.new(node.codeOrigin, "andp", [node.operands[1], node.operands[0], tmp])
                list << Instruction.new(node.codeOrigin, "slt", [node.operands[2], tmp, MIPS_ZERO_REG])
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
# Lea support.
#

class Address
    def mipsEmitLea(destination)
        if destination == base
            $asm.puts "addiu #{destination.mipsOperand}, #{offset.value}"
        else
            $asm.puts "addiu #{destination.mipsOperand}, #{base.mipsOperand}, #{offset.value}"
        end
    end
end

#
# Add PIC compatible header code to all the LLInt rutins.
#

def mipsAddPICCode(list)
    myList = []
    list.each {
        | node |
        myList << node
        if node.is_a? Label
            myList << Instruction.new(node.codeOrigin, "pichdr", [])
        end
    }
    myList
end

#
# Actual lowering code follows.
#

class Sequence
    def getModifiedListMIPS
        result = @list

        # Verify that we will only see instructions and labels.
        result.each {
            | node |
            unless node.is_a? Instruction or
                    node.is_a? Label or
                    node.is_a? LocalLabel or
                    node.is_a? Skip
                raise "Unexpected #{node.inspect} at #{node.codeOrigin}"
            end
        }

        result = mipsAddPICCode(result)
        result = mipsLowerFarBranchOps(result)
        result = mipsLowerSimpleBranchOps(result)
        result = riscLowerSimpleBranchOps(result)
        result = riscLowerHardBranchOps(result)
        result = riscLowerShiftOps(result)
        result = mipsLowerMalformedAddresses(result) {
            | node, address |
            if address.is_a? Address
                (-0xffff..0xffff).include? address.offset.value
            else
                false
            end
        }
        result = riscLowerMalformedAddressesDouble(result)
        result = riscLowerMisplacedImmediates(result, ["storeb", "storei", "storep"])
        result = mipsLowerMisplacedImmediates(result)
        result = riscLowerMalformedImmediates(result, -0xffff..0xffff)
        result = mipsLowerMisplacedAddresses(result)
        result = riscLowerMisplacedAddresses(result)
        result = riscLowerRegisterReuse(result)
        result = mipsLowerCompares(result)
        result = assignRegistersToTemporaries(result, :gpr, MIPS_TEMP_GPRS)
        result = assignRegistersToTemporaries(result, :fpr, MIPS_TEMP_FPRS)

        return result
    end
end

def mipsOperands(operands)
    operands.map{|v| v.mipsOperand}.join(", ")
end

def mipsFlippedOperands(operands)
    mipsOperands([operands[-1]] + operands[0..-2])
end

def getMIPSOpcode(opcode, suffix)

end

def emitMIPSCompact(opcode, opcodei, operands)
    postfix = ""
    if opcode == "sub"
        if operands[0].is_a? Immediate
            opcode = "add"
            operands[0] = Immediate.new(operands[0].codeOrigin, -1 * operands[0].value)
        elsif operands[1].is_a? Immediate
            opcode = "add"
            operands[1] = Immediate.new(operands[1].codeOrigin, -1 * operands[1].value)
        end
        postfix = "u"
    elsif opcode == "add"
        postfix = "u"
    end
    if operands.size == 3
        if operands[0].is_a? Immediate
            $asm.puts "#{opcode}i#{postfix} #{operands[2].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].value}"
        elsif operands[1].is_a? Immediate
            $asm.puts "#{opcode}i#{postfix} #{operands[2].mipsOperand}, #{operands[0].mipsOperand}, #{operands[1].value}"
        else
            $asm.puts "#{opcode}#{postfix} #{mipsFlippedOperands(operands)}"
        end
    else
        raise unless operands.size == 2
        raise unless operands[1].register?
        if operands[0].is_a? Immediate
            $asm.puts "#{opcode}i#{postfix} #{operands[1].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
        else
            $asm.puts "#{opcode}#{postfix} #{operands[1].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
        end
    end
end

def emitMIPSShiftCompact(opcode, operands)
    if operands.size == 3
        if (operands[1].is_a? Immediate)
            $asm.puts "#{opcode} #{operands[2].mipsOperand}, #{operands[0].mipsOperand}, #{operands[1].value}"
        else
            $asm.puts "#{opcode}v #{mipsFlippedOperands(operands)}"
        end
    else
        raise unless operands.size == 2
        if operands[0].register?
            $asm.puts "#{opcode}v #{operands[1].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
        else
            $asm.puts "#{opcode} #{operands[1].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].value}"
        end
    end
end

def emitMIPS(opcode, operands)
    if operands.size == 3
        $asm.puts "#{opcode} #{mipsFlippedOperands(operands)}"
    else
        raise unless operands.size == 2
        $asm.puts "#{opcode} #{operands[1].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
    end
end

def emitMIPSDoubleBranch(branchOpcode, neg, operands)
    $asm.puts "c.#{branchOpcode}.d #{mipsOperands(operands[0..1])}"
    if (!neg)
        $asm.puts "bc1t #{operands[2].asmLabel}"
    else
        $asm.puts "bc1f #{operands[2].asmLabel}"
    end
end

def emitMIPSJumpOrCall(opcode, operand)
    if operand.label?
        raise "Direct call/jump to a not local label." unless operand.is_a? LocalLabelReference
        $asm.puts "#{opcode} #{operand.asmLabel}"
    else
        raise "Invalid call/jump register." unless operand == MIPS_CALL_REG
        $asm.puts "#{opcode}r #{MIPS_CALL_REG.mipsOperand}"
    end
end

class Instruction
    def lowerMIPS
        $asm.comment codeOriginString
        case opcode
        when "addi", "addp", "addis"
            if operands.size == 3 and operands[0].is_a? Immediate
                raise unless operands[1].register?
                raise unless operands[2].register?
                if operands[0].value == 0 #and suffix.empty?
                    unless operands[1] == operands[2]
                        $asm.puts "move #{operands[2].mipsOperand}, #{operands[1].mipsOperand}"
                    end
                else
                    $asm.puts "addiu #{operands[2].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
                end
            elsif operands.size == 3 and operands[0].register?
                raise unless operands[1].register?
                raise unless operands[2].register?
                $asm.puts "addu #{mipsFlippedOperands(operands)}"
            else
                if operands[0].is_a? Immediate
                    unless Immediate.new(nil, 0) == operands[0]
                        $asm.puts "addiu #{operands[1].mipsOperand}, #{mipsFlippedOperands(operands)}"
                    end
                else
                    $asm.puts "addu #{operands[1].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
                end
            end
        when "andi", "andp"
            emitMIPSCompact("and", "and", operands)
        when "ori", "orp"
            emitMIPSCompact("or", "orr", operands)
        when "oris"
            emitMIPSCompact("or", "orrs", operands)
        when "xori", "xorp"
            emitMIPSCompact("xor", "eor", operands)
        when "lshifti", "lshiftp"
            emitMIPSShiftCompact("sll", operands)
        when "rshifti", "rshiftp"
            emitMIPSShiftCompact("sra", operands)
        when "urshifti", "urshiftp"
            emitMIPSShiftCompact("srl", operands)
        when "muli", "mulp"
            emitMIPS("mul", operands)
        when "subi", "subp", "subis"
            emitMIPSCompact("sub", "subs", operands)
        when "negi", "negp"
            $asm.puts "negu #{operands[0].mipsOperand}, #{operands[0].mipsOperand}"
        when "noti"
            $asm.puts "nor #{operands[0].mipsOperand}, #{operands[0].mipsOperand}, $zero"
        when "loadi", "loadis", "loadp"
            $asm.puts "lw #{mipsFlippedOperands(operands)}"
        when "storei", "storep"
            $asm.puts "sw #{mipsOperands(operands)}"
        when "loadb"
            $asm.puts "lbu #{mipsFlippedOperands(operands)}"
        when "loadbs"
            $asm.puts "lb #{mipsFlippedOperands(operands)}"
        when "storeb"
            $asm.puts "sb #{mipsOperands(operands)}"
        when "loadh"
            $asm.puts "lhu #{mipsFlippedOperands(operands)}"
        when "loadhs"
            $asm.puts "lh #{mipsFlippedOperands(operands)}"
        when "storeh"
            $asm.puts "shv #{mipsOperands(operands)}"
        when "loadd"
            $asm.puts "ldc1 #{mipsFlippedOperands(operands)}"
        when "stored"
            $asm.puts "sdc1 #{mipsOperands(operands)}"
        when "la"
            $asm.puts "la #{operands[1].mipsOperand}, #{operands[0].asmLabel}"
        when "addd"
            emitMIPS("add.d", operands)
        when "divd"
            emitMIPS("div.d", operands)
        when "subd"
            emitMIPS("sub.d", operands)
        when "muld"
            emitMIPS("mul.d", operands)
        when "sqrtd"
            $asm.puts "sqrt.d #{mipsFlippedOperands(operands)}"
        when "ci2d"
            raise "invalid ops of #{self.inspect} at #{codeOriginString}" unless operands[1].is_a? FPRegisterID and operands[0].register?
            $asm.puts "mtc1 #{operands[0].mipsOperand}, #{operands[1].mipsOperand}"
            $asm.puts "cvt.d.w #{operands[1].mipsOperand}, #{operands[1].mipsOperand}"
        when "bdeq"
            emitMIPSDoubleBranch("eq", false, operands)
        when "bdneq"
            emitMIPSDoubleBranch("ueq", true, operands)
        when "bdgt"
            emitMIPSDoubleBranch("ule", true, operands)
        when "bdgteq"
            emitMIPSDoubleBranch("ult", true, operands)
        when "bdlt"
            emitMIPSDoubleBranch("olt", false, operands)
        when "bdlteq"
            emitMIPSDoubleBranch("ole", false, operands)
        when "bdequn"
            emitMIPSDoubleBranch("ueq", false, operands)
        when "bdnequn"
            emitMIPSDoubleBranch("eq", true, operands)
        when "bdgtun"
            emitMIPSDoubleBranch("ole", true, operands)
        when "bdgtequn"
            emitMIPSDoubleBranch("olt", true, operands)
        when "bdltun"
            emitMIPSDoubleBranch("ult", false, operands)
        when "bdltequn"
            emitMIPSDoubleBranch("ule", false, operands)
        when "btd2i"
            # FIXME: may be a good idea to just get rid of this instruction, since the interpreter
            # currently does not use it.
            raise "MIPS does not support this opcode yet, #{codeOrigin}"
        when "td2i"
            $asm.puts "cvt.w.d #{MIPS_SCRATCH_FPR.mipsSingleLo}, #{operands[0].mipsOperand}"
            $asm.puts "mfc1 #{operands[1].mipsOperand}, #{MIPS_SCRATCH_FPR.mipsSingleLo}"
        when "bcd2i"
            $asm.puts "cvt.w.d #{MIPS_SCRATCH_FPR.mipsSingleLo}, #{operands[0].mipsOperand}"
            $asm.puts "mfc1 #{operands[1].mipsOperand}, #{MIPS_SCRATCH_FPR.mipsSingleLo}"
            $asm.puts "cvt.d.w #{MIPS_SCRATCH_FPR.mipsOperand}, #{MIPS_SCRATCH_FPR.mipsSingleLo}"
            emitMIPSDoubleBranch("eq", true, [MIPS_SCRATCH_FPR, operands[0], operands[2]])
            $asm.puts "beq #{operands[1].mipsOperand}, $zero, #{operands[2].asmLabel}"
        when "movdz"
            # FIXME: either support this or remove it.
            raise "MIPS does not support this opcode yet, #{codeOrigin}"
        when "pop"
            $asm.puts "lw #{operands[0].mipsOperand}, 0($sp)"
            $asm.puts "addiu $sp, $sp, 4"
        when "push"
            $asm.puts "addiu $sp, $sp, -4"
            $asm.puts "sw #{operands[0].mipsOperand}, 0($sp)"
        when "move", "sxi2p", "zxi2p"
            if operands[0].is_a? Immediate
                mipsMoveImmediate(operands[0].value, operands[1])
            else
                $asm.puts "move #{mipsFlippedOperands(operands)}"
            end
        when "nop"
            $asm.puts "nop"
        when "bieq", "bpeq", "bbeq"
            $asm.puts "beq #{mipsOperands(operands[0..1])}, #{operands[2].asmLabel}"
        when "bineq", "bpneq", "bbneq"
            $asm.puts "bne #{mipsOperands(operands[0..1])}, #{operands[2].asmLabel}"
        when "bigt", "bpgt", "bbgt"
            $asm.puts "bgt #{mipsOperands(operands[0..1])}, #{operands[2].asmLabel}"
        when "bigteq", "bpgteq", "bbgteq"
            $asm.puts "bge #{mipsOperands(operands[0..1])}, #{operands[2].asmLabel}"
        when "bilt", "bplt", "bblt"
            $asm.puts "blt #{mipsOperands(operands[0..1])}, #{operands[2].asmLabel}"
        when "bilteq", "bplteq", "bblteq"
            $asm.puts "ble #{mipsOperands(operands[0..1])}, #{operands[2].asmLabel}"
        when "jmp"
            emitMIPSJumpOrCall("j", operands[0])
        when "call"
            emitMIPSJumpOrCall("jal", operands[0])
        when "break"
            $asm.puts "break"
        when "ret"
            $asm.puts "jr $ra"
        when "cia", "cpa", "cba"
            $asm.puts "sltu #{operands[2].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
        when "ciaeq", "cpaeq", "cbaeq"
            $asm.puts "sltu #{operands[2].mipsOperand}, #{operands[0].mipsOperand}, #{operands[1].mipsOperand}"
            $asm.puts "xori #{operands[2].mipsOperand}, 1"
        when "cib", "cpb", "cbb"
            $asm.puts "sltu #{operands[2].mipsOperand}, #{operands[0].mipsOperand}, #{operands[1].mipsOperand}"
        when "cibeq", "cpbeq", "cbbeq"
            $asm.puts "sltu #{operands[2].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
            $asm.puts "xori #{operands[2].mipsOperand}, 1"
        when "cigt", "cpgt", "cbgt"
            $asm.puts "slt #{operands[2].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
        when "cigteq", "cpgteq", "cbgteq"
            $asm.puts "slt #{operands[2].mipsOperand}, #{operands[0].mipsOperand}, #{operands[1].mipsOperand}"
            $asm.puts "xori #{operands[2].mipsOperand}, 1"
        when "cilt", "cplt", "cblt"
            $asm.puts "slt #{operands[2].mipsOperand}, #{operands[0].mipsOperand}, #{operands[1].mipsOperand}"
        when "cilteq", "cplteq", "cblteq"
            $asm.puts "slt #{operands[2].mipsOperand}, #{operands[1].mipsOperand}, #{operands[0].mipsOperand}"
            $asm.puts "xori #{operands[2].mipsOperand}, 1"
        when "peek"
            $asm.puts "lw #{operands[1].mipsOperand}, #{operands[0].value * 4}($sp)"
        when "poke"
            $asm.puts "sw #{operands[1].mipsOperand}, #{operands[0].value * 4}($sp)"
        when "fii2d"
            $asm.puts "mtc1 #{operands[0].mipsOperand}, #{operands[2].mipsSingleLo}"
            $asm.puts "mtc1 #{operands[1].mipsOperand}, #{operands[2].mipsSingleHi}"
        when "fd2ii"
            $asm.puts "mfc1 #{operands[1].mipsOperand}, #{operands[0].mipsSingleLo}"
            $asm.puts "mfc1 #{operands[2].mipsOperand}, #{operands[0].mipsSingleHi}"
        when /^bo/
            $asm.puts "bgt #{operands[0].mipsOperand}, #{operands[1].mipsOperand}, #{operands[2].asmLabel}"
        when /^bs/
            $asm.puts "blt #{operands[0].mipsOperand}, #{operands[1].mipsOperand}, #{operands[2].asmLabel}"
        when /^bz/
            $asm.puts "beq #{operands[0].mipsOperand}, #{operands[1].mipsOperand}, #{operands[2].asmLabel}"
        when /^bnz/
            $asm.puts "bne #{operands[0].mipsOperand}, #{operands[1].mipsOperand}, #{operands[2].asmLabel}"
        when "leai", "leap"
            operands[0].mipsEmitLea(operands[1])
        when "smulli"
            raise "Wrong number of arguments to smull in #{self.inspect} at #{codeOriginString}" unless operands.length == 4
            $asm.puts "mult #{operands[0].mipsOperand}, #{operands[1].mipsOperand}"
            $asm.puts "mflo #{operands[2].mipsOperand}"
            $asm.puts "mfhi #{operands[3].mipsOperand}"
        when "movz"
            $asm.puts "movz #{operands[0].mipsOperand}, #{operands[1].mipsOperand}, #{operands[2].mipsOperand}"
        when "movn"
            $asm.puts "movn #{operands[0].mipsOperand}, #{operands[1].mipsOperand}, #{operands[2].mipsOperand}"
        when "slt", "sltb"
            $asm.puts "slt #{operands[0].mipsOperand}, #{operands[1].mipsOperand}, #{operands[2].mipsOperand}"
        when "sltu", "sltub"
            $asm.puts "sltu #{operands[0].mipsOperand}, #{operands[1].mipsOperand}, #{operands[2].mipsOperand}"
        when "pichdr"
            $asm.putStr("OFFLINE_ASM_CPLOAD(#{MIPS_CALL_REG.mipsOperand})")
            $asm.puts "move #{MIPS_GPSAVE_REG.mipsOperand}, #{MIPS_GP_REG.mipsOperand}"
        else
            raise "Unhandled opcode #{opcode} at #{codeOriginString}"
        end
    end
end
