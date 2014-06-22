# Copyright (C) 2012 Apple Inc. All rights reserved.
# Copyright (C) 2013 Digia Plc. and/or its subsidiary(-ies)
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

def isX64
    case $activeBackend
    when "X86"
        false
    when "X86_64"
        true
    else
        raise "bad value for $activeBackend: #{$activeBackend}"
    end
end

def useX87
    case $activeBackend
    when "X86"
        true
    when "X86_64"
        false
    else
        raise "bad value for $activeBackend: #{$activeBackend}"
    end
end

class SpecialRegister < NoChildren
    def x86Operand(kind)
        raise unless @name =~ /^r/
        raise unless isX64
        case kind
        when :half
            "%" + @name + "w"
        when :int
            "%" + @name + "d"
        when :ptr
            "%" + @name
        when :quad
            "%" + @name
        else
            raise
        end
    end
    def x86CallOperand(kind)
        # Call operands are not allowed to be partial registers.
        "*#{x86Operand(:quad)}"
    end
end

X64_SCRATCH_REGISTER = SpecialRegister.new("r11")

class RegisterID
    def supports8BitOnX86
        case name
        when "t0", "a0", "r0", "t1", "a1", "r1", "t2", "t3"
            true
        when "cfr", "ttnr", "tmr"
            false
        when "t4", "t5"
            isX64
        else
            raise
        end
    end
    
    def x86Operand(kind)
        case name
        when "t0", "a0", "r0"
            case kind
            when :byte
                "%al"
            when :half
                "%ax"
            when :int
                "%eax"
            when :ptr
                isX64 ? "%rax" : "%eax"
            when :quad
                isX64 ? "%rax" : raise
            else
                raise
            end
        when "t1", "a1", "r1"
            case kind
            when :byte
                "%dl"
            when :half
                "%dx"
            when :int
                "%edx"
            when :ptr
                isX64 ? "%rdx" : "%edx"
            when :quad
                isX64 ? "%rdx" : raise
            else
                raise
            end
        when "t2"
            case kind
            when :byte
                "%cl"
            when :half
                "%cx"
            when :int
                "%ecx"
            when :ptr
                isX64 ? "%rcx" : "%ecx"
            when :quad
                isX64 ? "%rcx" : raise
            else
                raise
            end
        when "t3"
            case kind
            when :byte
                "%bl"
            when :half
                "%bx"
            when :int
                "%ebx"
            when :ptr
                isX64 ? "%rbx" : "%ebx"
            when :quad
                isX64 ? "%rbx" : raise
            else
                raise
            end
        when "t4"
            case kind
            when :byte
                "%sil"
            when :half
                "%si"
            when :int
                "%esi"
            when :ptr
                isX64 ? "%rsi" : "%esi"
            when :quad
                isX64 ? "%rsi" : raise
            else
                raise
            end
        when "cfr"
            if isX64
                case kind
                when :half
                    "%r13w"
                when :int
                    "%r13d"
                when :ptr
                    "%r13"
                when :quad
                    "%r13"
                else
                    raise
                end
            else
                case kind
                when :byte
                    "%dil"
                when :half
                    "%di"
                when :int
                    "%edi"
                when :ptr
                    "%edi"
                else
                    raise
                end
            end
        when "sp"
            case kind
            when :byte
                "%spl"
            when :half
                "%sp"
            when :int
                "%esp"
            when :ptr
                isX64 ? "%rsp" : "%esp"
            when :quad
                isX64 ? "%rsp" : raise
            else
                raise
            end
        when "t5"
            raise "Cannot use #{name} in 32-bit X86 at #{codeOriginString}" unless isX64
            case kind
            when :byte
                "%dil"
            when :half
                "%di"
            when :int
                "%edi"
            when :ptr
                "%rdi"
            when :quad
                "%rdi"
            end
        when "t6"
            raise "Cannot use #{name} in 32-bit X86 at #{codeOriginString}" unless isX64
            case kind
            when :half
                "%r10w"
            when :int
                "%r10d"
            when :ptr
                "%r10"
            when :quad
                "%r10"
            end
        when "csr1"
            raise "Cannot use #{name} in 32-bit X86 at #{codeOriginString}" unless isX64
            case kind
            when :half
                "%r14w"
            when :int
                "%r14d"
            when :ptr
                "%r14"
            when :quad
                "%r14"
            end
        when "csr2"
            raise "Cannot use #{name} in 32-bit X86 at #{codeOriginString}" unless isX64
            case kind
            when :half
                "%r15w"
            when :int
                "%r15d"
            when :ptr
                "%r15"
            when :quad
                "%r15"
            end
        else
            raise "Bad register #{name} for X86 at #{codeOriginString}"
        end
    end
    def x86CallOperand(kind)
        isX64 ? "*#{x86Operand(:quad)}" : "*#{x86Operand(:ptr)}"
    end
end

class FPRegisterID
    def x86Operand(kind)
        raise unless kind == :double
        raise if useX87
        case name
        when "ft0", "fa0", "fr"
            "%xmm0"
        when "ft1", "fa1"
            "%xmm1"
        when "ft2", "fa2"
            "%xmm2"
        when "ft3", "fa3"
            "%xmm3"
        when "ft4"
            "%xmm4"
        when "ft5"
            "%xmm5"
        else
            raise "Bad register #{name} for X86 at #{codeOriginString}"
        end
    end
    def x87DefaultStackPosition
        case name
        when "ft0", "fr"
            0
        when "ft1"
            1
        when "ft2", "ft3", "ft4", "ft5"
            raise "Unimplemented register #{name} for X86 at #{codeOriginString}"
        else
            raise "Bad register #{name} for X86 at #{codeOriginString}"
        end
    end
    def x87Operand(offset)
        raise unless useX87
        raise unless offset == 0 or offset == 1
        "%st(#{x87DefaultStackPosition + offset})"
    end
    def x86CallOperand(kind)
        "*#{x86Operand(kind)}"
    end
end

class Immediate
    def validX86Immediate?
        if isX64
            value >= -0x80000000 and value <= 0x7fffffff
        else
            true
        end
    end
    def x86Operand(kind)
        "$#{value}"
    end
    def x86CallOperand(kind)
        "#{value}"
    end
end

class Address
    def supports8BitOnX86
        true
    end
    
    def x86AddressOperand(addressKind)
        "#{offset.value}(#{base.x86Operand(addressKind)})"
    end
    def x86Operand(kind)
        x86AddressOperand(:ptr)
    end
    def x86CallOperand(kind)
        "*#{x86Operand(kind)}"
    end
end

class BaseIndex
    def supports8BitOnX86
        true
    end
    
    def x86AddressOperand(addressKind)
        "#{offset.value}(#{base.x86Operand(addressKind)}, #{index.x86Operand(addressKind)}, #{scale})"
    end
    
    def x86Operand(kind)
        x86AddressOperand(:ptr)
    end

    def x86CallOperand(kind)
        "*#{x86Operand(kind)}"
    end
end

class AbsoluteAddress
    def supports8BitOnX86
        true
    end
    
    def x86AddressOperand(addressKind)
        "#{address.value}"
    end
    
    def x86Operand(kind)
        "#{address.value}"
    end

    def x86CallOperand(kind)
        "*#{address.value}"
    end
end

class LabelReference
    def x86CallOperand(kind)
        asmLabel
    end
end

class LocalLabelReference
    def x86CallOperand(kind)
        asmLabel
    end
end

class Sequence
    def getModifiedListX86_64
        newList = []
        
        @list.each {
            | node |
            newNode = node
            if node.is_a? Instruction
                unless node.opcode == "move"
                    usedScratch = false
                    newOperands = node.operands.map {
                        | operand |
                        if operand.immediate? and not operand.validX86Immediate?
                            if usedScratch
                                raise "Attempt to use scratch register twice at #{operand.codeOriginString}"
                            end
                            newList << Instruction.new(operand.codeOrigin, "move", [operand, X64_SCRATCH_REGISTER])
                            usedScratch = true
                            X64_SCRATCH_REGISTER
                        else
                            operand
                        end
                    }
                    newNode = Instruction.new(node.codeOrigin, node.opcode, newOperands, node.annotation)
                end
            else
                unless node.is_a? Label or
                        node.is_a? LocalLabel or
                        node.is_a? Skip
                    raise "Unexpected #{node.inspect} at #{node.codeOrigin}" 
                end
            end
            if newNode
                newList << newNode
            end
        }
        
        return newList
    end
end

class Instruction
    def x86Operands(*kinds)
        raise unless kinds.size == operands.size
        result = []
        kinds.size.times {
            | idx |
            result << operands[idx].x86Operand(kinds[idx])
        }
        result.join(", ")
    end

    def x86Suffix(kind)
        case kind
        when :byte
            "b"
        when :half
            "w"
        when :int
            "l"
        when :ptr
            isX64 ? "q" : "l"
        when :quad
            isX64 ? "q" : raise
        when :double
            not useX87 ? "sd" : raise
        else
            raise
        end
    end
    
    def x86Bytes(kind)
        case kind
        when :byte
            1
        when :half
            2
        when :int
            4
        when :ptr
            isX64 ? 8 : 4
        when :quad
            isX64 ? 8 : raise
        when :double
            8
        else
            raise
        end
    end
    
    def handleX86OpWithNumOperands(opcode, kind, numOperands)
        if numOperands == 3
            if operands[0] == operands[2]
                $asm.puts "#{opcode} #{operands[1].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
            elsif operands[1] == operands[2]
                $asm.puts "#{opcode} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
            else
                $asm.puts "mov#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
                $asm.puts "#{opcode} #{operands[1].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
            end
        else
            $asm.puts "#{opcode} #{operands[0].x86Operand(kind)}, #{operands[1].x86Operand(kind)}"
        end
    end
    
    def handleX86Op(opcode, kind)
        handleX86OpWithNumOperands(opcode, kind, operands.size)
    end
    
    def handleX86Shift(opcode, kind)
        if operands[0].is_a? Immediate or operands[0] == RegisterID.forName(nil, "t2")
            $asm.puts "#{opcode} #{operands[0].x86Operand(:byte)}, #{operands[1].x86Operand(kind)}"
        else
            cx = RegisterID.forName(nil, "t2")
            $asm.puts "xchg#{x86Suffix(:ptr)} #{operands[0].x86Operand(:ptr)}, #{cx.x86Operand(:ptr)}"
            $asm.puts "#{opcode} %cl, #{operands[1].x86Operand(kind)}"
            $asm.puts "xchg#{x86Suffix(:ptr)} #{operands[0].x86Operand(:ptr)}, #{cx.x86Operand(:ptr)}"
        end
    end
    
    def handleX86DoubleBranch(branchOpcode, mode)
        if useX87
            handleX87Compare(mode)
        else
            case mode
            when :normal
                $asm.puts "ucomisd #{operands[1].x86Operand(:double)}, #{operands[0].x86Operand(:double)}"
            when :reverse
                $asm.puts "ucomisd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:double)}"
            else
                raise mode.inspect
            end
        end
        $asm.puts "#{branchOpcode} #{operands[2].asmLabel}"
    end
    
    def handleX86IntCompare(opcodeSuffix, kind)
        if operands[0].is_a? Immediate and operands[0].value == 0 and operands[1].is_a? RegisterID and (opcodeSuffix == "e" or opcodeSuffix == "ne")
            $asm.puts "test#{x86Suffix(kind)} #{operands[1].x86Operand(kind)}"
        elsif operands[1].is_a? Immediate and operands[1].value == 0 and operands[0].is_a? RegisterID and (opcodeSuffix == "e" or opcodeSuffix == "ne")
            $asm.puts "test#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}"
        else
            $asm.puts "cmp#{x86Suffix(kind)} #{operands[1].x86Operand(kind)}, #{operands[0].x86Operand(kind)}"
        end
    end
    
    def handleX86IntBranch(branchOpcode, kind)
        handleX86IntCompare(branchOpcode[1..-1], kind)
        $asm.puts "#{branchOpcode} #{operands[2].asmLabel}"
    end
    
    def handleX86Set(setOpcode, operand)
        if operand.supports8BitOnX86
            $asm.puts "#{setOpcode} #{operand.x86Operand(:byte)}"
            $asm.puts "movzbl #{operand.x86Operand(:byte)}, #{operand.x86Operand(:int)}"
        else
            ax = RegisterID.new(nil, "t0")
            $asm.puts "xchg#{x86Suffix(:ptr)} #{operand.x86Operand(:ptr)}, #{ax.x86Operand(:ptr)}"
            $asm.puts "#{setOpcode} %al"
            $asm.puts "movzbl %al, %eax"
            $asm.puts "xchg#{x86Suffix(:ptr)} #{operand.x86Operand(:ptr)}, #{ax.x86Operand(:ptr)}"
        end
    end
    
    def handleX86IntCompareSet(setOpcode, kind)
        handleX86IntCompare(setOpcode[3..-1], kind)
        handleX86Set(setOpcode, operands[2])
    end
    
    def handleX86Test(kind)
        value = operands[0]
        case operands.size
        when 2
            mask = Immediate.new(codeOrigin, -1)
        when 3
            mask = operands[1]
        else
            raise "Expected 2 or 3 operands, but got #{operands.size} at #{codeOriginString}"
        end
        
        if mask.is_a? Immediate and mask.value == -1
            if value.is_a? RegisterID
                $asm.puts "test#{x86Suffix(kind)} #{value.x86Operand(kind)}, #{value.x86Operand(kind)}"
            else
                $asm.puts "cmp#{x86Suffix(kind)} $0, #{value.x86Operand(kind)}"
            end
        else
            $asm.puts "test#{x86Suffix(kind)} #{mask.x86Operand(kind)}, #{value.x86Operand(kind)}"
        end
    end
    
    def handleX86BranchTest(branchOpcode, kind)
        handleX86Test(kind)
        $asm.puts "#{branchOpcode} #{operands.last.asmLabel}"
    end
    
    def handleX86SetTest(setOpcode, kind)
        handleX86Test(kind)
        handleX86Set(setOpcode, operands.last)
    end
    
    def handleX86OpBranch(opcode, branchOpcode, kind)
        handleX86OpWithNumOperands(opcode, kind, operands.size - 1)
        case operands.size
        when 4
            jumpTarget = operands[3]
        when 3
            jumpTarget = operands[2]
        else
            raise self.inspect
        end
        $asm.puts "#{branchOpcode} #{jumpTarget.asmLabel}"
    end
    
    def handleX86SubBranch(branchOpcode, kind)
        if operands.size == 4 and operands[1] == operands[2]
            $asm.puts "neg#{x86Suffix(kind)} #{operands[2].x86Operand(kind)}"
            $asm.puts "add#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
        else
            handleX86OpWithNumOperands("sub#{x86Suffix(kind)}", kind, operands.size - 1)
        end
        case operands.size
        when 4
            jumpTarget = operands[3]
        when 3
            jumpTarget = operands[2]
        else
            raise self.inspect
        end
        $asm.puts "#{branchOpcode} #{jumpTarget.asmLabel}"
    end

    def handleX86Add(kind)
        if operands.size == 3 and operands[1] == operands[2]
            unless Immediate.new(nil, 0) == operands[0]
                $asm.puts "add#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
            end
        elsif operands.size == 3 and operands[0].is_a? Immediate
            raise unless operands[1].is_a? RegisterID
            raise unless operands[2].is_a? RegisterID
            if operands[0].value == 0
                unless operands[1] == operands[2]
                    $asm.puts "mov#{x86Suffix(kind)} #{operands[1].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
                end
            else
                $asm.puts "lea#{x86Suffix(kind)} #{operands[0].value}(#{operands[1].x86Operand(kind)}), #{operands[2].x86Operand(kind)}"
            end
        elsif operands.size == 3 and operands[0].is_a? RegisterID
            raise unless operands[1].is_a? RegisterID
            raise unless operands[2].is_a? RegisterID
            if operands[0] == operands[2]
                $asm.puts "add#{x86Suffix(kind)} #{operands[1].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
            else
                $asm.puts "lea#{x86Suffix(kind)} (#{operands[0].x86Operand(kind)}, #{operands[1].x86Operand(kind)}), #{operands[2].x86Operand(kind)}"
            end
        else
            unless Immediate.new(nil, 0) == operands[0]
                $asm.puts "add#{x86Suffix(kind)} #{x86Operands(kind, kind)}"
            end
        end
    end
    
    def handleX86Sub(kind)
        if operands.size == 3 and operands[1] == operands[2]
            $asm.puts "neg#{x86Suffix(kind)} #{operands[2].x86Operand(kind)}"
            $asm.puts "add#{x86Suffix(kind)} #{operands[0].x86Operand(kind)}, #{operands[2].x86Operand(kind)}"
        else
            handleX86Op("sub#{x86Suffix(kind)}", kind)
        end
    end
    
    def handleX86Mul(kind)
        if operands.size == 3 and operands[0].is_a? Immediate
            $asm.puts "imul#{x86Suffix(kind)} #{x86Operands(kind, kind, kind)}"
        else
            # FIXME: could do some peephole in case the left operand is immediate and it's
            # a power of two.
            handleX86Op("imul#{x86Suffix(kind)}", kind)
        end
    end
    
    def handleMove
        if Immediate.new(nil, 0) == operands[0] and operands[1].is_a? RegisterID
            if isX64
                $asm.puts "xor#{x86Suffix(:quad)} #{operands[1].x86Operand(:quad)}, #{operands[1].x86Operand(:quad)}"
            else
                $asm.puts "xor#{x86Suffix(:ptr)} #{operands[1].x86Operand(:ptr)}, #{operands[1].x86Operand(:ptr)}"
            end
        elsif operands[0] != operands[1]
            if isX64
                $asm.puts "mov#{x86Suffix(:quad)} #{x86Operands(:quad, :quad)}"
            else
                $asm.puts "mov#{x86Suffix(:ptr)} #{x86Operands(:ptr, :ptr)}"
            end
        end
    end
    
    def handleX87Compare(mode)
        case mode
        when :normal
            if (operands[0].x87DefaultStackPosition == 0)
                $asm.puts "fucomi #{operands[1].x87Operand(0)}"
            else
                $asm.puts "fld #{operands[0].x87Operand(0)}"
                $asm.puts "fucomip #{operands[1].x87Operand(1)}"
            end
        when :reverse
            if (operands[1].x87DefaultStackPosition == 0)
                $asm.puts "fucomi #{operands[0].x87Operand(0)}"
            else
                $asm.puts "fld #{operands[1].x87Operand(0)}"
                $asm.puts "fucomip #{operands[0].x87Operand(1)}"
            end
        else
            raise mode.inspect
        end
    end

    def handleX87BinOp(opcode, opcodereverse)
        if (operands[1].x87DefaultStackPosition == 0)
            $asm.puts "#{opcode} #{operands[0].x87Operand(0)}, %st"
        elsif (operands[0].x87DefaultStackPosition == 0)
            $asm.puts "#{opcodereverse} %st, #{operands[1].x87Operand(0)}"
        else
            $asm.puts "fld #{operands[0].x87Operand(0)}"
            $asm.puts "#{opcodereverse}p %st, #{operands[1].x87Operand(1)}"
        end
    end

    def lowerX86
        raise unless $activeBackend == "X86"
        lowerX86Common
    end
    
    def lowerX86_64
        raise unless $activeBackend == "X86_64"
        lowerX86Common
    end
    
    def lowerX86Common
        $asm.codeOrigin codeOriginString if $enableCodeOriginComments
        $asm.annotation annotation if $enableInstrAnnotations

        case opcode
        when "addi"
            handleX86Add(:int)
        when "addp"
            handleX86Add(:ptr)
        when "addq"
            handleX86Add(:quad)
        when "andi"
            handleX86Op("andl", :int)
        when "andp"
            handleX86Op("and#{x86Suffix(:ptr)}", :ptr)
        when "andq"
            handleX86Op("and#{x86Suffix(:quad)}", :quad)
        when "lshifti"
            handleX86Shift("sall", :int)
        when "lshiftp"
            handleX86Shift("sal#{x86Suffix(:ptr)}", :ptr)
        when "lshiftq"
            handleX86Shift("sal#{x86Suffix(:quad)}", :quad)
        when "muli"
            handleX86Mul(:int)
        when "mulp"
            handleX86Mul(:ptr)
        when "mulq"
            handleX86Mul(:quad)
        when "negi"
            $asm.puts "negl #{x86Operands(:int)}"
        when "negp"
            $asm.puts "neg#{x86Suffix(:ptr)} #{x86Operands(:ptr)}"
        when "negq"
            $asm.puts "neg#{x86Suffix(:quad)} #{x86Operands(:quad)}"
        when "noti"
            $asm.puts "notl #{x86Operands(:int)}"
        when "ori"
            handleX86Op("orl", :int)
        when "orp"
            handleX86Op("or#{x86Suffix(:ptr)}", :ptr)
        when "orq"
            handleX86Op("or#{x86Suffix(:quad)}", :quad)
        when "rshifti"
            handleX86Shift("sarl", :int)
        when "rshiftp"
            handleX86Shift("sar#{x86Suffix(:ptr)}", :ptr)
        when "rshiftq"
            handleX86Shift("sar#{x86Suffix(:quad)}", :quad)
        when "urshifti"
            handleX86Shift("shrl", :int)
        when "urshiftp"
            handleX86Shift("shr#{x86Suffix(:ptr)}", :ptr)
        when "urshiftq"
            handleX86Shift("shr#{x86Suffix(:quad)}", :quad)
        when "subi"
            handleX86Sub(:int)
        when "subp"
            handleX86Sub(:ptr)
        when "subq"
            handleX86Sub(:quad)
        when "xori"
            handleX86Op("xorl", :int)
        when "xorp"
            handleX86Op("xor#{x86Suffix(:ptr)}", :ptr)
        when "xorq"
            handleX86Op("xor#{x86Suffix(:quad)}", :quad)
        when "loadi", "storei"
            $asm.puts "movl #{x86Operands(:int, :int)}"
        when "loadis"
            if isX64
                $asm.puts "movslq #{x86Operands(:int, :quad)}"
            else
                $asm.puts "movl #{x86Operands(:int, :int)}"
            end
        when "loadp", "storep"
            $asm.puts "mov#{x86Suffix(:ptr)} #{x86Operands(:ptr, :ptr)}"
        when "loadq", "storeq"
            $asm.puts "mov#{x86Suffix(:quad)} #{x86Operands(:quad, :quad)}"
        when "loadb"
            $asm.puts "movzbl #{operands[0].x86Operand(:byte)}, #{operands[1].x86Operand(:int)}"
        when "loadbs"
            $asm.puts "movsbl #{operands[0].x86Operand(:byte)}, #{operands[1].x86Operand(:int)}"
        when "loadh"
            $asm.puts "movzwl #{operands[0].x86Operand(:half)}, #{operands[1].x86Operand(:int)}"
        when "loadhs"
            $asm.puts "movswl #{operands[0].x86Operand(:half)}, #{operands[1].x86Operand(:int)}"
        when "storeb"
            $asm.puts "movb #{x86Operands(:byte, :byte)}"
        when "loadd"
            if useX87
                $asm.puts "fldl #{operands[0].x86Operand(:double)}"
                $asm.puts "fstp #{operands[1].x87Operand(1)}"
            else
                $asm.puts "movsd #{x86Operands(:double, :double)}"
            end
        when "moved"
            if useX87
                if (operands[0].x87DefaultStackPosition == 0)
                    $asm.puts "fst #{operands[1].x87Operand(0)}"
                else
                    $asm.puts "fld #{operands[0].x87Operand(0)}"
                    $asm.puts "fstp #{operands[1].x87Operand(1)}"
                end
            else
                $asm.puts "movsd #{x86Operands(:double, :double)}"
            end
        when "stored"
            if useX87
                if (operands[0].x87DefaultStackPosition == 0)
                    $asm.puts "fstl #{operands[1].x86Operand(:double)}"
                else
                    $asm.puts "fld #{operands[0].x87Operand(0)}"
                    $asm.puts "fstpl #{operands[1].x86Operand(:double)}"
                end
            else
                $asm.puts "movsd #{x86Operands(:double, :double)}"
            end
        when "addd"
            if useX87
                handleX87BinOp("fadd", "fadd")
            else
                $asm.puts "addsd #{x86Operands(:double, :double)}"
            end
        when "muld"
            if useX87
                handleX87BinOp("fmul", "fmul")
            else
                $asm.puts "mulsd #{x86Operands(:double, :double)}"
            end
        when "subd"
            if useX87
                handleX87BinOp("fsub", "fsubr")
            else
                $asm.puts "subsd #{x86Operands(:double, :double)}"
            end
        when "divd"
            if useX87
                handleX87BinOp("fdiv", "fdivr")
            else
                $asm.puts "divsd #{x86Operands(:double, :double)}"
            end
        when "sqrtd"
            if useX87
                $asm.puts "fld #{operands[0].x87Operand(0)}"
                $asm.puts "fsqrtl"
                $asm.puts "fstp #{operands[1].x87Operand(1)}"
            else
                $asm.puts "sqrtsd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:double)}"
            end
        when "ci2d"
            if useX87
                sp = RegisterID.new(nil, "sp")
                $asm.puts "movl #{operands[0].x86Operand(:int)}, -4(#{sp.x86Operand(:ptr)})"
                $asm.puts "fildl -4(#{sp.x86Operand(:ptr)})"
                $asm.puts "fstp #{operands[1].x87Operand(1)}"
            else
                $asm.puts "cvtsi2sd #{operands[0].x86Operand(:int)}, #{operands[1].x86Operand(:double)}"
            end
        when "bdeq"
            if useX87
                handleX87Compare(:normal)
            else
                $asm.puts "ucomisd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:double)}"
            end
            if operands[0] == operands[1]
                # This is just a jump ordered, which is a jnp.
                $asm.puts "jnp #{operands[2].asmLabel}"
            else
                isUnordered = LocalLabel.unique("bdeq")
                $asm.puts "jp #{LabelReference.new(codeOrigin, isUnordered).asmLabel}"
                $asm.puts "je #{LabelReference.new(codeOrigin, operands[2]).asmLabel}"
                isUnordered.lower("X86")
            end
        when "bdneq"
            handleX86DoubleBranch("jne", :normal)
        when "bdgt"
            handleX86DoubleBranch("ja", :normal)
        when "bdgteq"
            handleX86DoubleBranch("jae", :normal)
        when "bdlt"
            handleX86DoubleBranch("ja", :reverse)
        when "bdlteq"
            handleX86DoubleBranch("jae", :reverse)
        when "bdequn"
            handleX86DoubleBranch("je", :normal)
        when "bdnequn"
            if useX87
                handleX87Compare(:normal)
            else
                $asm.puts "ucomisd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:double)}"
            end
            if operands[0] == operands[1]
                # This is just a jump unordered, which is a jp.
                $asm.puts "jp #{operands[2].asmLabel}"
            else
                isUnordered = LocalLabel.unique("bdnequn")
                isEqual = LocalLabel.unique("bdnequn")
                $asm.puts "jp #{LabelReference.new(codeOrigin, isUnordered).asmLabel}"
                $asm.puts "je #{LabelReference.new(codeOrigin, isEqual).asmLabel}"
                isUnordered.lower("X86")
                $asm.puts "jmp #{operands[2].asmLabel}"
                isEqual.lower("X86")
            end
        when "bdgtun"
            handleX86DoubleBranch("jb", :reverse)
        when "bdgtequn"
            handleX86DoubleBranch("jbe", :reverse)
        when "bdltun"
            handleX86DoubleBranch("jb", :normal)
        when "bdltequn"
            handleX86DoubleBranch("jbe", :normal)
        when "btd2i"
            # FIXME: unused and unimplemented for x87
            raise if useX87
            $asm.puts "cvttsd2si #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:int)}"
            $asm.puts "cmpl $0x80000000 #{operands[1].x86Operand(:int)}"
            $asm.puts "je #{operands[2].asmLabel}"
        when "td2i"
            # FIXME: unused and unimplemented for x87
            raise if useX87
            $asm.puts "cvttsd2si #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:int)}"
        when "bcd2i"
            if useX87
                sp = RegisterID.new(nil, "sp")
                if (operands[0].x87DefaultStackPosition == 0)
                    $asm.puts "fistl -4(#{sp.x86Operand(:ptr)})"
                else
                    $asm.puts "fld #{operands[0].x87Operand(0)}"
                    $asm.puts "fistpl -4(#{sp.x86Operand(:ptr)})"
                end
                $asm.puts "movl -4(#{sp.x86Operand(:ptr)}), #{operands[1].x86Operand(:int)}"
                $asm.puts "testl #{operands[1].x86Operand(:int)}, #{operands[1].x86Operand(:int)}"
                $asm.puts "je #{operands[2].asmLabel}"
                $asm.puts "fildl -4(#{sp.x86Operand(:ptr)})"
                $asm.puts "fucomip #{operands[0].x87Operand(1)}"
                $asm.puts "jp #{operands[2].asmLabel}"
                $asm.puts "jne #{operands[2].asmLabel}"
            else
                $asm.puts "cvttsd2si #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:int)}"
                $asm.puts "testl #{operands[1].x86Operand(:int)}, #{operands[1].x86Operand(:int)}"
                $asm.puts "je #{operands[2].asmLabel}"
                $asm.puts "cvtsi2sd #{operands[1].x86Operand(:int)}, %xmm7"
                $asm.puts "ucomisd #{operands[0].x86Operand(:double)}, %xmm7"
                $asm.puts "jp #{operands[2].asmLabel}"
                $asm.puts "jne #{operands[2].asmLabel}"
            end
        when "movdz"
            if useX87
                $asm.puts "fldzl"
                $asm.puts "fstp #{operands[0].x87Operand(1)}"
            else
                $asm.puts "xorpd #{operands[0].x86Operand(:double)}, #{operands[0].x86Operand(:double)}"
            end
        when "pop"
            $asm.puts "pop #{operands[0].x86Operand(:ptr)}"
        when "push"
            $asm.puts "push #{operands[0].x86Operand(:ptr)}"
        when "move"
            handleMove
        when "sxi2q"
            $asm.puts "movslq #{operands[0].x86Operand(:int)}, #{operands[1].x86Operand(:quad)}"
        when "zxi2q"
            $asm.puts "movl #{operands[0].x86Operand(:int)}, #{operands[1].x86Operand(:int)}"
        when "nop"
            $asm.puts "nop"
        when "bieq"
            handleX86IntBranch("je", :int)
        when "bpeq"
            handleX86IntBranch("je", :ptr)
        when "bqeq"
            handleX86IntBranch("je", :quad)
        when "bineq"
            handleX86IntBranch("jne", :int)
        when "bpneq"
            handleX86IntBranch("jne", :ptr)
        when "bqneq"
            handleX86IntBranch("jne", :quad)
        when "bia"
            handleX86IntBranch("ja", :int)
        when "bpa"
            handleX86IntBranch("ja", :ptr)
        when "bqa"
            handleX86IntBranch("ja", :quad)
        when "biaeq"
            handleX86IntBranch("jae", :int)
        when "bpaeq"
            handleX86IntBranch("jae", :ptr)
        when "bqaeq"
            handleX86IntBranch("jae", :quad)
        when "bib"
            handleX86IntBranch("jb", :int)
        when "bpb"
            handleX86IntBranch("jb", :ptr)
        when "bqb"
            handleX86IntBranch("jb", :quad)
        when "bibeq"
            handleX86IntBranch("jbe", :int)
        when "bpbeq"
            handleX86IntBranch("jbe", :ptr)
        when "bqbeq"
            handleX86IntBranch("jbe", :quad)
        when "bigt"
            handleX86IntBranch("jg", :int)
        when "bpgt"
            handleX86IntBranch("jg", :ptr)
        when "bqgt"
            handleX86IntBranch("jg", :quad)
        when "bigteq"
            handleX86IntBranch("jge", :int)
        when "bpgteq"
            handleX86IntBranch("jge", :ptr)
        when "bqgteq"
            handleX86IntBranch("jge", :quad)
        when "bilt"
            handleX86IntBranch("jl", :int)
        when "bplt"
            handleX86IntBranch("jl", :ptr)
        when "bqlt"
            handleX86IntBranch("jl", :quad)
        when "bilteq"
            handleX86IntBranch("jle", :int)
        when "bplteq"
            handleX86IntBranch("jle", :ptr)
        when "bqlteq"
            handleX86IntBranch("jle", :quad)
        when "bbeq"
            handleX86IntBranch("je", :byte)
        when "bbneq"
            handleX86IntBranch("jne", :byte)
        when "bba"
            handleX86IntBranch("ja", :byte)
        when "bbaeq"
            handleX86IntBranch("jae", :byte)
        when "bbb"
            handleX86IntBranch("jb", :byte)
        when "bbbeq"
            handleX86IntBranch("jbe", :byte)
        when "bbgt"
            handleX86IntBranch("jg", :byte)
        when "bbgteq"
            handleX86IntBranch("jge", :byte)
        when "bblt"
            handleX86IntBranch("jl", :byte)
        when "bblteq"
            handleX86IntBranch("jlteq", :byte)
        when "btis"
            handleX86BranchTest("js", :int)
        when "btps"
            handleX86BranchTest("js", :ptr)
        when "btqs"
            handleX86BranchTest("js", :quad)
        when "btiz"
            handleX86BranchTest("jz", :int)
        when "btpz"
            handleX86BranchTest("jz", :ptr)
        when "btqz"
            handleX86BranchTest("jz", :quad)
        when "btinz"
            handleX86BranchTest("jnz", :int)
        when "btpnz"
            handleX86BranchTest("jnz", :ptr)
        when "btqnz"
            handleX86BranchTest("jnz", :quad)
        when "btbs"
            handleX86BranchTest("js", :byte)
        when "btbz"
            handleX86BranchTest("jz", :byte)
        when "btbnz"
            handleX86BranchTest("jnz", :byte)
        when "jmp"
            $asm.puts "jmp #{operands[0].x86CallOperand(:ptr)}"
        when "baddio"
            handleX86OpBranch("addl", "jo", :int)
        when "baddpo"
            handleX86OpBranch("add#{x86Suffix(:ptr)}", "jo", :ptr)
        when "baddqo"
            handleX86OpBranch("add#{x86Suffix(:quad)}", "jo", :quad)
        when "baddis"
            handleX86OpBranch("addl", "js", :int)
        when "baddps"
            handleX86OpBranch("add#{x86Suffix(:ptr)}", "js", :ptr)
        when "baddqs"
            handleX86OpBranch("add#{x86Suffix(:quad)}", "js", :quad)
        when "baddiz"
            handleX86OpBranch("addl", "jz", :int)
        when "baddpz"
            handleX86OpBranch("add#{x86Suffix(:ptr)}", "jz", :ptr)
        when "baddqz"
            handleX86OpBranch("add#{x86Suffix(:quad)}", "jz", :quad)
        when "baddinz"
            handleX86OpBranch("addl", "jnz", :int)
        when "baddpnz"
            handleX86OpBranch("add#{x86Suffix(:ptr)}", "jnz", :ptr)
        when "baddqnz"
            handleX86OpBranch("add#{x86Suffix(:quad)}", "jnz", :quad)
        when "bsubio"
            handleX86SubBranch("jo", :int)
        when "bsubis"
            handleX86SubBranch("js", :int)
        when "bsubiz"
            handleX86SubBranch("jz", :int)
        when "bsubinz"
            handleX86SubBranch("jnz", :int)
        when "bmulio"
            handleX86OpBranch("imull", "jo", :int)
        when "bmulis"
            handleX86OpBranch("imull", "js", :int)
        when "bmuliz"
            handleX86OpBranch("imull", "jz", :int)
        when "bmulinz"
            handleX86OpBranch("imull", "jnz", :int)
        when "borio"
            handleX86OpBranch("orl", "jo", :int)
        when "boris"
            handleX86OpBranch("orl", "js", :int)
        when "boriz"
            handleX86OpBranch("orl", "jz", :int)
        when "borinz"
            handleX86OpBranch("orl", "jnz", :int)
        when "break"
            $asm.puts "int $3"
        when "call"
            if useX87
                2.times {
                    | offset |
                    $asm.puts "ffree %st(#{offset})"
                }
            end
            $asm.puts "call #{operands[0].x86CallOperand(:ptr)}"
        when "ret"
            $asm.puts "ret"
        when "cieq"
            handleX86IntCompareSet("sete", :int)
        when "cbeq"
            handleX86IntCompareSet("sete", :byte)
        when "cpeq"
            handleX86IntCompareSet("sete", :ptr)
        when "cqeq"
            handleX86IntCompareSet("sete", :quad)
        when "cineq"
            handleX86IntCompareSet("setne", :int)
        when "cbneq"
            handleX86IntCompareSet("setne", :byte)
        when "cpneq"
            handleX86IntCompareSet("setne", :ptr)
        when "cqneq"
            handleX86IntCompareSet("setne", :quad)
        when "cia"
            handleX86IntCompareSet("seta", :int)
        when "cba"
            handleX86IntCompareSet("seta", :byte)
        when "cpa"
            handleX86IntCompareSet("seta", :ptr)
        when "cqa"
            handleX86IntCompareSet("seta", :quad)
        when "ciaeq"
            handleX86IntCompareSet("setae", :int)
        when "cbaeq"
            handleX86IntCompareSet("setae", :byte)
        when "cpaeq"
            handleX86IntCompareSet("setae", :ptr)
        when "cqaeq"
            handleX86IntCompareSet("setae", :quad)
        when "cib"
            handleX86IntCompareSet("setb", :int)
        when "cbb"
            handleX86IntCompareSet("setb", :byte)
        when "cpb"
            handleX86IntCompareSet("setb", :ptr)
        when "cqb"
            handleX86IntCompareSet("setb", :quad)
        when "cibeq"
            handleX86IntCompareSet("setbe", :int)
        when "cbbeq"
            handleX86IntCompareSet("setbe", :byte)
        when "cpbeq"
            handleX86IntCompareSet("setbe", :ptr)
        when "cqbeq"
            handleX86IntCompareSet("setbe", :quad)
        when "cigt"
            handleX86IntCompareSet("setg", :int)
        when "cbgt"
            handleX86IntCompareSet("setg", :byte)
        when "cpgt"
            handleX86IntCompareSet("setg", :ptr)
        when "cqgt"
            handleX86IntCompareSet("setg", :quad)
        when "cigteq"
            handleX86IntCompareSet("setge", :int)
        when "cbgteq"
            handleX86IntCompareSet("setge", :byte)
        when "cpgteq"
            handleX86IntCompareSet("setge", :ptr)
        when "cqgteq"
            handleX86IntCompareSet("setge", :quad)
        when "cilt"
            handleX86IntCompareSet("setl", :int)
        when "cblt"
            handleX86IntCompareSet("setl", :byte)
        when "cplt"
            handleX86IntCompareSet("setl", :ptr)
        when "cqlt"
            handleX86IntCompareSet("setl", :quad)
        when "cilteq"
            handleX86IntCompareSet("setle", :int)
        when "cblteq"
            handleX86IntCompareSet("setle", :byte)
        when "cplteq"
            handleX86IntCompareSet("setle", :ptr)
        when "cqlteq"
            handleX86IntCompareSet("setle", :quad)
        when "tis"
            handleX86SetTest("sets", :int)
        when "tiz"
            handleX86SetTest("setz", :int)
        when "tinz"
            handleX86SetTest("setnz", :int)
        when "tps"
            handleX86SetTest("sets", :ptr)
        when "tpz"
            handleX86SetTest("setz", :ptr)
        when "tpnz"
            handleX86SetTest("setnz", :ptr)
        when "tqs"
            handleX86SetTest("sets", :quad)
        when "tqz"
            handleX86SetTest("setz", :quad)
        when "tqnz"
            handleX86SetTest("setnz", :quad)
        when "tbs"
            handleX86SetTest("sets", :byte)
        when "tbz"
            handleX86SetTest("setz", :byte)
        when "tbnz"
            handleX86SetTest("setnz", :byte)
        when "peek"
            sp = RegisterID.new(nil, "sp")
            $asm.puts "mov#{x86Suffix(:ptr)} #{operands[0].value * x86Bytes(:ptr)}(#{sp.x86Operand(:ptr)}), #{operands[1].x86Operand(:ptr)}"
        when "peekq"
            sp = RegisterID.new(nil, "sp")
            $asm.puts "mov#{x86Suffix(:quad)} #{operands[0].value * x86Bytes(:quad)}(#{sp.x86Operand(:ptr)}), #{operands[1].x86Operand(:quad)}"
        when "poke"
            sp = RegisterID.new(nil, "sp")
            $asm.puts "mov#{x86Suffix(:ptr)} #{operands[0].x86Operand(:ptr)}, #{operands[1].value * x86Bytes(:ptr)}(#{sp.x86Operand(:ptr)})"
        when "pokeq"
            sp = RegisterID.new(nil, "sp")
            $asm.puts "mov#{x86Suffix(:quad)} #{operands[0].x86Operand(:quad)}, #{operands[1].value * x86Bytes(:quad)}(#{sp.x86Operand(:ptr)})"
        when "cdqi"
            $asm.puts "cdq"
        when "idivi"
            $asm.puts "idivl #{operands[0].x86Operand(:int)}"
        when "fii2d"
            if useX87
                sp = RegisterID.new(nil, "sp")
                $asm.puts "movl #{operands[0].x86Operand(:int)}, -8(#{sp.x86Operand(:ptr)})"
                $asm.puts "movl #{operands[1].x86Operand(:int)}, -4(#{sp.x86Operand(:ptr)})"
                $asm.puts "fldl -8(#{sp.x86Operand(:ptr)})"
                $asm.puts "fstp #{operands[2].x87Operand(1)}"
            else
                $asm.puts "movd #{operands[0].x86Operand(:int)}, #{operands[2].x86Operand(:double)}"
                $asm.puts "movd #{operands[1].x86Operand(:int)}, %xmm7"
                $asm.puts "psllq $32, %xmm7"
                $asm.puts "por %xmm7, #{operands[2].x86Operand(:double)}"
            end
        when "fd2ii"
            if useX87
                sp = RegisterID.new(nil, "sp")
                if (operands[0].x87DefaultStackPosition == 0)
                    $asm.puts "fstl -8(#{sp.x86Operand(:ptr)})"
                else
                    $asm.puts "fld #{operands[0].x87Operand(0)}"
                    $asm.puts "fstpl -8(#{sp.x86Operand(:ptr)})"
                end
                $asm.puts "movl -8(#{sp.x86Operand(:ptr)}), #{operands[1].x86Operand(:int)}"
                $asm.puts "movl -4(#{sp.x86Operand(:ptr)}), #{operands[2].x86Operand(:int)}"
            else
                $asm.puts "movd #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:int)}"
                $asm.puts "movsd #{operands[0].x86Operand(:double)}, %xmm7"
                $asm.puts "psrlq $32, %xmm7"
                $asm.puts "movd %xmm7, #{operands[2].x86Operand(:int)}"
            end
        when "fq2d"
            if useX87
                sp = RegisterID.new(nil, "sp")
                $asm.puts "movq #{operands[0].x86Operand(:quad)}, -8(#{sp.x86Operand(:ptr)})"
                $asm.puts "fldl -8(#{sp.x86Operand(:ptr)})"
                $asm.puts "fstp #{operands[1].x87Operand(1)}"
            else
                $asm.puts "movq #{operands[0].x86Operand(:quad)}, #{operands[1].x86Operand(:double)}"
            end
        when "fd2q"
            if useX87
                sp = RegisterID.new(nil, "sp")
                if (operands[0].x87DefaultStackPosition == 0)
                    $asm.puts "fstl -8(#{sp.x86Operand(:ptr)})"
                else
                    $asm.puts "fld #{operands[0].x87Operand(0)}"
                    $asm.puts "fstpl -8(#{sp.x86Operand(:ptr)})"
                end
                $asm.puts "movq -8(#{sp.x86Operand(:ptr)}), #{operands[1].x86Operand(:quad)}"
            else
                $asm.puts "movq #{operands[0].x86Operand(:double)}, #{operands[1].x86Operand(:quad)}"
            end
        when "bo"
            $asm.puts "jo #{operands[0].asmLabel}"
        when "bs"
            $asm.puts "js #{operands[0].asmLabel}"
        when "bz"
            $asm.puts "jz #{operands[0].asmLabel}"
        when "bnz"
            $asm.puts "jnz #{operands[0].asmLabel}"
        when "leai"
            $asm.puts "leal #{operands[0].x86AddressOperand(:int)}, #{operands[1].x86Operand(:int)}"
        when "leap"
            $asm.puts "lea#{x86Suffix(:ptr)} #{operands[0].x86AddressOperand(:ptr)}, #{operands[1].x86Operand(:ptr)}"
        else
            lowerDefault
        end
    end
end

