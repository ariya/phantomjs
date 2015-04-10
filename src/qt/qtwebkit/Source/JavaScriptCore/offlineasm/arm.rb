# Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
# Copyright (C) 2013 University of Szeged. All rights reserved.
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
require "opt"
require "risc"

def isARMv7
    case $activeBackend
    when "ARMv7"
        true
    when "ARMv7_TRADITIONAL", "ARM"
        false
    else
        raise "bad value for $activeBackend: #{$activeBackend}"
    end
end

def isARMv7Traditional
    case $activeBackend
    when "ARMv7_TRADITIONAL"
        true
    when "ARMv7", "ARM"
        false
    else
        raise "bad value for $activeBackend: #{$activeBackend}"
    end
end

class Node
    def armSingle
        doubleOperand = armOperand
        raise "Bogus register name #{doubleOperand}" unless doubleOperand =~ /^d/
        "s" + ($~.post_match.to_i * 2).to_s
    end
end

class SpecialRegister
    def armOperand
        @name
    end
end

ARM_EXTRA_GPRS = [SpecialRegister.new("r9"), SpecialRegister.new("r8"), SpecialRegister.new("r3")]
ARM_EXTRA_FPRS = [SpecialRegister.new("d7")]
ARM_SCRATCH_FPR = SpecialRegister.new("d6")

def armMoveImmediate(value, register)
    # Currently we only handle the simple cases, and fall back to mov/movt for the complex ones.
    if value >= 0 && value < 256
        $asm.puts "mov #{register.armOperand}, \##{value}"
    elsif (~value) >= 0 && (~value) < 256
        $asm.puts "mvn #{register.armOperand}, \##{~value}"
    elsif isARMv7 or isARMv7Traditional
        $asm.puts "movw #{register.armOperand}, \##{value & 0xffff}"
        if (value & 0xffff0000) != 0
            $asm.puts "movt #{register.armOperand}, \##{(value >> 16) & 0xffff}"
        end
    else
        $asm.puts "ldr #{register.armOperand}, =#{value}"
    end
end

class RegisterID
    def armOperand
        case name
        when "t0", "a0", "r0"
            "r0"
        when "t1", "a1", "r1"
            "r1"
        when "t2", "a2"
            "r2"
        when "a3"
            "r3"
        when "t3"
            "r4"
        when "t4"
            "r10"
        when "cfr"
            "r5"
        when "lr"
            "lr"
        when "sp"
            "sp"
        else
            raise "Bad register #{name} for ARM at #{codeOriginString}"
        end
    end
end

class FPRegisterID
    def armOperand
        case name
        when "ft0", "fr"
            "d0"
        when "ft1"
            "d1"
        when "ft2"
            "d2"
        when "ft3"
            "d3"
        when "ft4"
            "d4"
        when "ft5"
            "d5"
        else
            raise "Bad register #{name} for ARM at #{codeOriginString}"
        end
    end
end

class Immediate
    def armOperand
        raise "Invalid immediate #{value} at #{codeOriginString}" if value < 0 or value > 255
        "\##{value}"
    end
end

class Address
    def armOperand
        raise "Bad offset at #{codeOriginString}" if offset.value < -0xff or offset.value > 0xfff
        "[#{base.armOperand}, \##{offset.value}]"
    end
end

class BaseIndex
    def armOperand
        raise "Bad offset at #{codeOriginString}" if offset.value != 0
        "[#{base.armOperand}, #{index.armOperand}, lsl \##{scaleShift}]"
    end
end

class AbsoluteAddress
    def armOperand
        raise "Unconverted absolute address at #{codeOriginString}"
    end
end

#
# Lea support.
#

class Address
    def armEmitLea(destination)
        if destination == base
            $asm.puts "adds #{destination.armOperand}, \##{offset.value}"
        else
            $asm.puts "adds #{destination.armOperand}, #{base.armOperand}, \##{offset.value}"
        end
    end
end

class BaseIndex
    def armEmitLea(destination)
        raise "Malformed BaseIndex, offset should be zero at #{codeOriginString}" unless offset.value == 0
        $asm.puts "add #{destination.armOperand}, #{base.armOperand}, #{index.armOperand}, lsl \##{scaleShift}"
    end
end

# FIXME: we could support AbsoluteAddress for lea, but we don't.

#
# Actual lowering code follows.
#

class Sequence
    def getModifiedListARM
        raise unless $activeBackend == "ARM"
        getModifiedListARMCommon
    end

    def getModifiedListARMv7
        raise unless $activeBackend == "ARMv7"
        getModifiedListARMCommon
    end

    def getModifiedListARMv7_TRADITIONAL
        raise unless $activeBackend == "ARMv7_TRADITIONAL"
        getModifiedListARMCommon
    end

    def getModifiedListARMCommon
        result = @list
        result = riscLowerSimpleBranchOps(result)
        result = riscLowerHardBranchOps(result)
        result = riscLowerShiftOps(result)
        result = riscLowerMalformedAddresses(result) {
            | node, address |
            if address.is_a? BaseIndex
                address.offset.value == 0
            elsif address.is_a? Address
                (-0xff..0xfff).include? address.offset.value
            else
                false
            end
        }
        result = riscLowerMalformedAddressesDouble(result)
        result = riscLowerMisplacedImmediates(result, ["storeb", "storei", "storep"])
        result = riscLowerMalformedImmediates(result, 0..0xff)
        result = riscLowerMisplacedAddresses(result)
        result = riscLowerRegisterReuse(result)
        result = assignRegistersToTemporaries(result, :gpr, ARM_EXTRA_GPRS)
        result = assignRegistersToTemporaries(result, :fpr, ARM_EXTRA_FPRS)
        return result
    end
end

def armOperands(operands)
    operands.map{|v| v.armOperand}.join(", ")
end

def armFlippedOperands(operands)
    armOperands([operands[-1]] + operands[0..-2])
end

def emitArmCompact(opcode2, opcode3, operands)
    if operands.size == 3
        $asm.puts "#{opcode3} #{armFlippedOperands(operands)}"
    else
        raise unless operands.size == 2
        raise unless operands[1].register?
        if operands[0].immediate?
            $asm.puts "#{opcode3} #{operands[1].armOperand}, #{operands[1].armOperand}, #{operands[0].armOperand}"
        else
            $asm.puts "#{opcode2} #{armFlippedOperands(operands)}"
        end
    end
end

def emitArm(opcode, operands)
    if operands.size == 3
        $asm.puts "#{opcode} #{armFlippedOperands(operands)}"
    else
        raise unless operands.size == 2
        $asm.puts "#{opcode} #{operands[1].armOperand}, #{operands[1].armOperand}, #{operands[0].armOperand}"
    end
end

def emitArmDoubleBranch(branchOpcode, operands)
    $asm.puts "vcmpe.f64 #{armOperands(operands[0..1])}"
    $asm.puts "vmrs apsr_nzcv, fpscr"
    $asm.puts "#{branchOpcode} #{operands[2].asmLabel}"
end

def emitArmTest(operands)
    value = operands[0]
    case operands.size
    when 2
        mask = Immediate.new(codeOrigin, -1)
    when 3
        mask = operands[1]
    else
        raise "Expected 2 or 3 operands but got #{operands.size} at #{codeOriginString}"
    end
    
    if mask.immediate? and mask.value == -1
        $asm.puts "tst #{value.armOperand}, #{value.armOperand}"
    else
        $asm.puts "tst #{value.armOperand}, #{mask.armOperand}"
    end
end

def emitArmCompare(operands, code)
    $asm.puts "movs #{operands[2].armOperand}, \#0"
    $asm.puts "cmp #{operands[0].armOperand}, #{operands[1].armOperand}"
    $asm.puts "it #{code}"
    $asm.puts "mov#{code} #{operands[2].armOperand}, \#1"
end

def emitArmTestSet(operands, code)
    $asm.puts "movs #{operands[-1].armOperand}, \#0"
    emitArmTest(operands)
    $asm.puts "it #{code}"
    $asm.puts "mov#{code} #{operands[-1].armOperand}, \#1"
end

class Instruction
    def lowerARM
        raise unless $activeBackend == "ARM"
        lowerARMCommon
    end

    def lowerARMv7
        raise unless $activeBackend == "ARMv7"
        lowerARMCommon
    end

    def lowerARMv7_TRADITIONAL
        raise unless $activeBackend == "ARMv7_TRADITIONAL"
        lowerARMCommon
    end

    def lowerARMCommon
        $asm.codeOrigin codeOriginString if $enableCodeOriginComments
        $asm.annotation annotation if $enableInstrAnnotations

        case opcode
        when "addi", "addp", "addis", "addps"
            if opcode == "addis" or opcode == "addps"
                suffix = "s"
            else
                suffix = ""
            end
            if operands.size == 3 and operands[0].immediate?
                raise unless operands[1].register?
                raise unless operands[2].register?
                if operands[0].value == 0 and suffix.empty?
                    unless operands[1] == operands[2]
                        $asm.puts "mov #{operands[2].armOperand}, #{operands[1].armOperand}"
                    end
                else
                    $asm.puts "adds #{operands[2].armOperand}, #{operands[1].armOperand}, #{operands[0].armOperand}"
                end
            elsif operands.size == 3 and operands[0].immediate?
                raise unless operands[1].register?
                raise unless operands[2].register?
                $asm.puts "adds #{armFlippedOperands(operands)}"
            else
                if operands[0].immediate?
                    unless Immediate.new(nil, 0) == operands[0]
                        $asm.puts "adds #{armFlippedOperands(operands)}"
                    end
                else
                    $asm.puts "add#{suffix} #{armFlippedOperands(operands)}"
                end
            end
        when "andi", "andp"
            emitArmCompact("ands", "and", operands)
        when "ori", "orp"
            emitArmCompact("orrs", "orr", operands)
        when "oris"
            emitArmCompact("orrs", "orrs", operands)
        when "xori", "xorp"
            emitArmCompact("eors", "eor", operands)
        when "lshifti", "lshiftp"
            emitArmCompact("lsls", "lsls", operands)
        when "rshifti", "rshiftp"
            emitArmCompact("asrs", "asrs", operands)
        when "urshifti", "urshiftp"
            emitArmCompact("lsrs", "lsrs", operands)
        when "muli", "mulp"
            emitArm("mul", operands)
        when "subi", "subp", "subis"
            emitArmCompact("subs", "subs", operands)
        when "negi", "negp"
            $asm.puts "rsbs #{operands[0].armOperand}, #{operands[0].armOperand}, \#0"
        when "noti"
            $asm.puts "mvns #{operands[0].armOperand}, #{operands[0].armOperand}"
        when "loadi", "loadis", "loadp"
            $asm.puts "ldr #{armFlippedOperands(operands)}"
        when "storei", "storep"
            $asm.puts "str #{armOperands(operands)}"
        when "loadb"
            $asm.puts "ldrb #{armFlippedOperands(operands)}"
        when "loadbs"
            $asm.puts "ldrsb.w #{armFlippedOperands(operands)}"
        when "storeb"
            $asm.puts "strb #{armOperands(operands)}"
        when "loadh"
            $asm.puts "ldrh #{armFlippedOperands(operands)}"
        when "loadhs"
            $asm.puts "ldrsh.w #{armFlippedOperands(operands)}"
        when "storeh"
            $asm.puts "strh #{armOperands(operands)}"
        when "loadd"
            $asm.puts "vldr.64 #{armFlippedOperands(operands)}"
        when "stored"
            $asm.puts "vstr.64 #{armOperands(operands)}"
        when "addd"
            emitArm("vadd.f64", operands)
        when "divd"
            emitArm("vdiv.f64", operands)
        when "subd"
            emitArm("vsub.f64", operands)
        when "muld"
            emitArm("vmul.f64", operands)
        when "sqrtd"
            $asm.puts "vsqrt.f64 #{armFlippedOperands(operands)}"
        when "ci2d"
            $asm.puts "vmov #{operands[1].armSingle}, #{operands[0].armOperand}"
            $asm.puts "vcvt.f64.s32 #{operands[1].armOperand}, #{operands[1].armSingle}"
        when "bdeq"
            emitArmDoubleBranch("beq", operands)
        when "bdneq"
            $asm.puts "vcmpe.f64 #{armOperands(operands[0..1])}"
            $asm.puts "vmrs apsr_nzcv, fpscr"
            isUnordered = LocalLabel.unique("bdneq")
            $asm.puts "bvs #{LocalLabelReference.new(codeOrigin, isUnordered).asmLabel}"
            $asm.puts "bne #{operands[2].asmLabel}"
            isUnordered.lower("ARM")
        when "bdgt"
            emitArmDoubleBranch("bgt", operands)
        when "bdgteq"
            emitArmDoubleBranch("bge", operands)
        when "bdlt"
            emitArmDoubleBranch("bmi", operands)
        when "bdlteq"
            emitArmDoubleBranch("bls", operands)
        when "bdequn"
            $asm.puts "vcmpe.f64 #{armOperands(operands[0..1])}"
            $asm.puts "vmrs apsr_nzcv, fpscr"
            $asm.puts "bvs #{operands[2].asmLabel}"
            $asm.puts "beq #{operands[2].asmLabel}"
        when "bdnequn"
            emitArmDoubleBranch("bne", operands)
        when "bdgtun"
            emitArmDoubleBranch("bhi", operands)
        when "bdgtequn"
            emitArmDoubleBranch("bpl", operands)
        when "bdltun"
            emitArmDoubleBranch("blt", operands)
        when "bdltequn"
            emitArmDoubleBranch("ble", operands)
        when "btd2i"
            # FIXME: may be a good idea to just get rid of this instruction, since the interpreter
            # currently does not use it.
            raise "ARM does not support this opcode yet, #{codeOrigin}"
        when "td2i"
            $asm.puts "vcvt.s32.f64 #{ARM_SCRATCH_FPR.armSingle}, #{operands[0].armOperand}"
            $asm.puts "vmov #{operands[1].armOperand}, #{ARM_SCRATCH_FPR.armSingle}"
        when "bcd2i"
            $asm.puts "vcvt.s32.f64 #{ARM_SCRATCH_FPR.armSingle}, #{operands[0].armOperand}"
            $asm.puts "vmov #{operands[1].armOperand}, #{ARM_SCRATCH_FPR.armSingle}"
            $asm.puts "vcvt.f64.s32 #{ARM_SCRATCH_FPR.armOperand}, #{ARM_SCRATCH_FPR.armSingle}"
            emitArmDoubleBranch("bne", [ARM_SCRATCH_FPR, operands[0], operands[2]])
            $asm.puts "tst #{operands[1].armOperand}, #{operands[1].armOperand}"
            $asm.puts "beq #{operands[2].asmLabel}"
        when "movdz"
            # FIXME: either support this or remove it.
            raise "ARM does not support this opcode yet, #{codeOrigin}"
        when "pop"
            $asm.puts "pop #{operands[0].armOperand}"
        when "push"
            $asm.puts "push #{operands[0].armOperand}"
        when "move"
            if operands[0].immediate?
                armMoveImmediate(operands[0].value, operands[1])
            else
                $asm.puts "mov #{armFlippedOperands(operands)}"
            end
        when "nop"
            $asm.puts "nop"
        when "bieq", "bpeq", "bbeq"
            if Immediate.new(nil, 0) == operands[0]
                $asm.puts "tst #{operands[1].armOperand}, #{operands[1].armOperand}"
            elsif Immediate.new(nil, 0) == operands[1]
                $asm.puts "tst #{operands[0].armOperand}, #{operands[0].armOperand}"
            else
                $asm.puts "cmp #{armOperands(operands[0..1])}"
            end
            $asm.puts "beq #{operands[2].asmLabel}"
        when "bineq", "bpneq", "bbneq"
            if Immediate.new(nil, 0) == operands[0]
                $asm.puts "tst #{operands[1].armOperand}, #{operands[1].armOperand}"
            elsif Immediate.new(nil, 0) == operands[1]
                $asm.puts "tst #{operands[0].armOperand}, #{operands[0].armOperand}"
            else
                $asm.puts "cmp #{armOperands(operands[0..1])}"
            end
            $asm.puts "bne #{operands[2].asmLabel}"
        when "bia", "bpa", "bba"
            $asm.puts "cmp #{armOperands(operands[0..1])}"
            $asm.puts "bhi #{operands[2].asmLabel}"
        when "biaeq", "bpaeq", "bbaeq"
            $asm.puts "cmp #{armOperands(operands[0..1])}"
            $asm.puts "bhs #{operands[2].asmLabel}"
        when "bib", "bpb", "bbb"
            $asm.puts "cmp #{armOperands(operands[0..1])}"
            $asm.puts "blo #{operands[2].asmLabel}"
        when "bibeq", "bpbeq", "bbbeq"
            $asm.puts "cmp #{armOperands(operands[0..1])}"
            $asm.puts "bls #{operands[2].asmLabel}"
        when "bigt", "bpgt", "bbgt"
            $asm.puts "cmp #{armOperands(operands[0..1])}"
            $asm.puts "bgt #{operands[2].asmLabel}"
        when "bigteq", "bpgteq", "bbgteq"
            $asm.puts "cmp #{armOperands(operands[0..1])}"
            $asm.puts "bge #{operands[2].asmLabel}"
        when "bilt", "bplt", "bblt"
            $asm.puts "cmp #{armOperands(operands[0..1])}"
            $asm.puts "blt #{operands[2].asmLabel}"
        when "bilteq", "bplteq", "bblteq"
            $asm.puts "cmp #{armOperands(operands[0..1])}"
            $asm.puts "ble #{operands[2].asmLabel}"
        when "btiz", "btpz", "btbz"
            emitArmTest(operands)
            $asm.puts "beq #{operands[-1].asmLabel}"
        when "btinz", "btpnz", "btbnz"
            emitArmTest(operands)
            $asm.puts "bne #{operands[-1].asmLabel}"
        when "btis", "btps", "btbs"
            emitArmTest(operands)
            $asm.puts "bmi #{operands[-1].asmLabel}"
        when "jmp"
            if operands[0].label?
                $asm.puts "b #{operands[0].asmLabel}"
            else
                $asm.puts "mov pc, #{operands[0].armOperand}"
            end
            if not isARMv7 and not isARMv7Traditional
                $asm.puts ".ltorg"
            end
        when "call"
            if operands[0].label?
                $asm.puts "blx #{operands[0].asmLabel}"
            else
                $asm.puts "blx #{operands[0].armOperand}"
            end
        when "break"
            $asm.puts "bkpt #0"
        when "ret"
            $asm.puts "bx lr"
        when "cieq", "cpeq", "cbeq"
            emitArmCompare(operands, "eq")
        when "cineq", "cpneq", "cbneq"
            emitArmCompare(operands, "ne")
        when "cia", "cpa", "cba"
            emitArmCompare(operands, "hi")
        when "ciaeq", "cpaeq", "cbaeq"
            emitArmCompare(operands, "hs")
        when "cib", "cpb", "cbb"
            emitArmCompare(operands, "lo")
        when "cibeq", "cpbeq", "cbbeq"
            emitArmCompare(operands, "ls")
        when "cigt", "cpgt", "cbgt"
            emitArmCompare(operands, "gt")
        when "cigteq", "cpgteq", "cbgteq"
            emitArmCompare(operands, "ge")
        when "cilt", "cplt", "cblt"
            emitArmCompare(operands, "lt")
        when "cilteq", "cplteq", "cblteq"
            emitArmCompare(operands, "le")
        when "tis", "tbs", "tps"
            emitArmTestSet(operands, "mi")
        when "tiz", "tbz", "tpz"
            emitArmTestSet(operands, "eq")
        when "tinz", "tbnz", "tpnz"
            emitArmTestSet(operands, "ne")
        when "peek"
            $asm.puts "ldr #{operands[1].armOperand}, [sp, \##{operands[0].value * 4}]"
        when "poke"
            $asm.puts "str #{operands[1].armOperand}, [sp, \##{operands[0].value * 4}]"
        when "fii2d"
            $asm.puts "vmov #{operands[2].armOperand}, #{operands[0].armOperand}, #{operands[1].armOperand}"
        when "fd2ii"
            $asm.puts "vmov #{operands[1].armOperand}, #{operands[2].armOperand}, #{operands[0].armOperand}"
        when "bo"
            $asm.puts "bvs #{operands[0].asmLabel}"
        when "bs"
            $asm.puts "bmi #{operands[0].asmLabel}"
        when "bz"
            $asm.puts "beq #{operands[0].asmLabel}"
        when "bnz"
            $asm.puts "bne #{operands[0].asmLabel}"
        when "leai", "leap"
            operands[0].armEmitLea(operands[1])
        when "smulli"
            raise "Wrong number of arguments to smull in #{self.inspect} at #{codeOriginString}" unless operands.length == 4
            $asm.puts "smull #{operands[2].armOperand}, #{operands[3].armOperand}, #{operands[0].armOperand}, #{operands[1].armOperand}"
        else
            lowerDefault
        end
    end
end

