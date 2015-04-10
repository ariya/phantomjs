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
# "Optimization" passes. These are used to lower the representation for
# backends that cannot handle some of our higher-level instructions.
#

#
# A temporary - a variable that will be allocated to a register after we're
# done.
#

class Node
    def replaceTemporariesWithRegisters(kind)
        mapChildren {
            | node |
            node.replaceTemporariesWithRegisters(kind)
        }
    end
end

class Tmp < NoChildren
    attr_reader :firstMention, :lastMention
    attr_reader :kind
    attr_accessor :register

    def initialize(codeOrigin, kind)
        super(codeOrigin)
        @kind = kind
    end
    
    def dump
        "$tmp#{object_id}"
    end
    
    def mention!(position)
        if not @firstMention or position < @firstMention
            @firstMention = position
        end
        if not @lastMention or position > @lastMention
            @lastMention = position
        end
    end
    
    def replaceTemporariesWithRegisters(kind)
        if @kind == kind
            raise "Did not allocate register to temporary at #{codeOriginString}" unless @register
            @register
        else
            self
        end
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

# Assign registers to temporaries, by finding which temporaries interfere
# with each other. Note that this relies on temporary live ranges not crossing
# basic block boundaries.

def assignRegistersToTemporaries(list, kind, registers)
    list.each_with_index {
        | node, index |
        node.filter(Tmp).uniq.each {
            | tmp |
            if tmp.kind == kind
                tmp.mention! index
            end
        }
    }
    
    freeRegisters = registers.dup
    list.each_with_index {
        | node, index |
        tmpList = node.filter(Tmp).uniq
        tmpList.each {
            | tmp |
            if tmp.kind == kind and tmp.firstMention == index
                raise "Could not allocate register to temporary at #{node.codeOriginString}" if freeRegisters.empty?
                tmp.register = freeRegisters.pop
            end
        }
        tmpList.each {
            | tmp |
            if tmp.kind == kind and tmp.lastMention == index
                freeRegisters.push tmp.register
                raise "Register allocation inconsistency at #{node.codeOriginString}" if freeRegisters.size > registers.size
            end
        }
    }
    
    list.map {
        | node |
        node.replaceTemporariesWithRegisters(kind)
    }
end

