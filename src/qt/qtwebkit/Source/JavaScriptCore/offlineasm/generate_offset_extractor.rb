#!/usr/bin/env ruby

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

$: << File.dirname(__FILE__)

require "config"
require "backends"
require "digest/sha1"
require "offsets"
require "parser"
require "self_hash"
require "settings"
require "transform"

inputFlnm = ARGV.shift
outputFlnm = ARGV.shift

$stderr.puts "offlineasm: Parsing #{inputFlnm} and creating offset extractor #{outputFlnm}."

def emitMagicNumber
    OFFSET_MAGIC_NUMBERS.each {
        | number |
        $output.puts "unsigned(#{number}),"
    }
end

inputHash = "// offlineasm input hash: #{parseHash(inputFlnm)} #{selfHash}"

if FileTest.exist? outputFlnm
    File.open(outputFlnm, "r") {
        | inp |
        firstLine = inp.gets
        if firstLine and firstLine.chomp == inputHash
            $stderr.puts "offlineasm: Nothing changed."
            exit 0
        end
    }
end

originalAST = parse(inputFlnm)

#
# Optimize the AST to make configuration extraction faster. This reduces the AST to a form
# that only contains the things that matter for our purposes: offsets, sizes, and if
# statements.
#

class Node
    def offsetsPruneTo(sequence)
        children.each {
            | child |
            child.offsetsPruneTo(sequence)
        }
    end
    
    def offsetsPrune
        result = Sequence.new(codeOrigin, [])
        offsetsPruneTo(result)
        result
    end
end

class IfThenElse
    def offsetsPruneTo(sequence)
        ifThenElse = IfThenElse.new(codeOrigin, predicate, thenCase.offsetsPrune)
        ifThenElse.elseCase = elseCase.offsetsPrune
        sequence.list << ifThenElse
    end
end

class StructOffset
    def offsetsPruneTo(sequence)
        sequence.list << self
    end
end

class Sizeof
    def offsetsPruneTo(sequence)
        sequence.list << self
    end
end

prunedAST = originalAST.offsetsPrune

File.open(outputFlnm, "w") {
    | outp |
    $output = outp
    outp.puts inputHash
    length = 0
    emitCodeInAllConfigurations(prunedAST) {
        | settings, ast, backend, index |
        offsetsList = ast.filter(StructOffset).uniq.sort
        sizesList = ast.filter(Sizeof).uniq.sort
        length += OFFSET_HEADER_MAGIC_NUMBERS.size + (OFFSET_MAGIC_NUMBERS.size + 1) * (1 + offsetsList.size + sizesList.size)
    }
    outp.puts "static const unsigned extractorTable[#{length}] = {"
    emitCodeInAllConfigurations(prunedAST) {
        | settings, ast, backend, index |
        OFFSET_HEADER_MAGIC_NUMBERS.each {
            | number |
            $output.puts "unsigned(#{number}),"
        }

        offsetsList = ast.filter(StructOffset).uniq.sort
        sizesList = ast.filter(Sizeof).uniq.sort
        
        emitMagicNumber
        outp.puts "#{index},"
        offsetsList.each {
            | offset |
            emitMagicNumber
            outp.puts "OFFLINE_ASM_OFFSETOF(#{offset.struct}, #{offset.field}),"
        }
        sizesList.each {
            | offset |
            emitMagicNumber
            outp.puts "sizeof(#{offset.struct}),"
        }
    }
    outp.puts "};"
}

$stderr.puts "offlineasm: offset extractor #{outputFlnm} successfully generated."

