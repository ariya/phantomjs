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

class Assembler
    def initialize(outp)
        @outp = outp
        @state = :cpp
        @commentState = :none
        @comment = nil
        @internalComment = nil
        @annotation = nil
        @codeOrigin = nil
        @numLocalLabels = 0
        @numGlobalLabels = 0

        @newlineSpacerState = :none
    end
    
    def enterAsm
        @outp.puts "OFFLINE_ASM_BEGIN"
        @state = :asm
    end
    
    def leaveAsm
        putsLastComment
        @outp.puts "OFFLINE_ASM_END"
        @state = :cpp
    end
    
    def inAsm
        enterAsm
        yield
        leaveAsm
    end
    
    # Concatenates all the various components of the comment to dump.
    def lastComment
        separator = " "
        result = ""
        result = "#{@comment}" if @comment
        if @annotation and $enableInstrAnnotations
            result += separator if result != ""
            result += "#{@annotation}"
        end
        if @internalComment
            result += separator if result != ""
            result += "#{@internalComment}"
        end
        if @codeOrigin and $enableCodeOriginComments
            result += separator if result != ""
            result += "#{@codeOrigin}"
        end
        if result != ""
            result = "// " + result
        end

        # Reset all the components that we've just sent to be dumped.
        @commentState = :none
        @comment = nil
        @annotation = nil
        @codeOrigin = nil
        @internalComment = nil
        result
    end
    
    # Puts a C Statement in the output stream.
    def putc(*line)
        raise unless @state == :asm
        @outp.puts(formatDump("    " + line.join(''), lastComment))
    end
    
    def formatDump(dumpStr, comment, commentColumns=$preferredCommentStartColumn)
        if comment.length > 0
            "%-#{commentColumns}s %s" % [dumpStr, comment]
        else
            dumpStr
        end
    end

    # private method for internal use only.
    def putAnnotation(text)
        raise unless @state == :asm
        if $enableInstrAnnotations
            @outp.puts text
            @annotation = nil
        end
    end

    def putLocalAnnotation()
        putAnnotation "    // #{@annotation}" if @annotation
    end

    def putGlobalAnnotation()
        putsNewlineSpacerIfAppropriate(:annotation)
        putAnnotation "// #{@annotation}" if @annotation
    end

    def putsLastComment
        comment = lastComment
        unless comment.empty?
            @outp.puts comment
        end
    end
    
    def puts(*line)
        raise unless @state == :asm
        @outp.puts(formatDump("    \"\\t" + line.join('') + "\\n\"", lastComment))
    end
    
    def print(line)
        raise unless @state == :asm
        @outp.print("\"" + line + "\"")
    end
    
    def putsNewlineSpacerIfAppropriate(state)
        if @newlineSpacerState != state
            @outp.puts("\n")
            @newlineSpacerState = state
        end
    end

    def putsLabel(labelName)
        raise unless @state == :asm
        @numGlobalLabels += 1
        putsNewlineSpacerIfAppropriate(:global)
        @internalComment = $enableLabelCountComments ? "Global Label #{@numGlobalLabels}" : nil
        if /\Allint_op_/.match(labelName)
            @outp.puts(formatDump("OFFLINE_ASM_OPCODE_LABEL(op_#{$~.post_match})", lastComment))
        else
            @outp.puts(formatDump("OFFLINE_ASM_GLUE_LABEL(#{labelName})", lastComment))
        end
        @newlineSpacerState = :none # After a global label, we can use another spacer.
    end
    
    def putsLocalLabel(labelName)
        raise unless @state == :asm
        @numLocalLabels += 1
        @outp.puts("\n")
        @internalComment = $enableLabelCountComments ? "Local Label #{@numLocalLabels}" : nil
        @outp.puts(formatDump("  OFFLINE_ASM_LOCAL_LABEL(#{labelName})", lastComment))
    end
    
    def self.labelReference(labelName)
        "\" LOCAL_REFERENCE(#{labelName}) \""
    end
    
    def self.localLabelReference(labelName)
        "\" LOCAL_LABEL_STRING(#{labelName}) \""
    end
    
    def self.cLabelReference(labelName)
        if /\Allint_op_/.match(labelName)
            "op_#{$~.post_match}"  # strip opcodes of their llint_ prefix.
        else
            "#{labelName}"
        end
    end
    
    def self.cLocalLabelReference(labelName)
        "#{labelName}"
    end
    
    def codeOrigin(text)
        case @commentState
        when :none
            @codeOrigin = text
            @commentState = :one
        when :one
            if $enableCodeOriginComments
                @outp.puts "    // #{@codeOrigin}"
                @outp.puts "    // #{text}"
            end
            @codeOrigin = nil
            @commentState = :many
        when :many
            @outp.puts "// #{text}" if $enableCodeOriginComments
        else
            raise
        end
    end

    def comment(text)
        @comment = text
    end
    def annotation(text)
        @annotation = text
    end
end

asmFile = ARGV.shift
offsetsFile = ARGV.shift
outputFlnm = ARGV.shift

$stderr.puts "offlineasm: Parsing #{asmFile} and #{offsetsFile} and creating assembly file #{outputFlnm}."

begin
    configurationList = offsetsAndConfigurationIndex(offsetsFile)
rescue MissingMagicValuesException
    $stderr.puts "offlineasm: No magic values found. Skipping assembly file generation."
    exit 0
end

inputHash =
    "// offlineasm input hash: " + parseHash(asmFile) +
    " " + Digest::SHA1.hexdigest(configurationList.map{|v| (v[0] + [v[1]]).join(' ')}.join(' ')) +
    " " + selfHash

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

File.open(outputFlnm, "w") {
    | outp |
    $output = outp
    $output.puts inputHash
    
    $asm = Assembler.new($output)
    
    ast = parse(asmFile)
    
    configurationList.each {
        | configuration |
        offsetsList = configuration[0]
        configIndex = configuration[1]
        forSettings(computeSettingsCombinations(ast)[configIndex], ast) {
            | concreteSettings, lowLevelAST, backend |
            lowLevelAST = lowLevelAST.resolve(*buildOffsetsMap(lowLevelAST, offsetsList))
            lowLevelAST.validate
            emitCodeInConfiguration(concreteSettings, lowLevelAST, backend) {
                $asm.inAsm {
                    lowLevelAST.lower(backend)
                }
            }
        }
    }
}

$stderr.puts "offlineasm: Assembly file #{outputFlnm} successfully generated."

