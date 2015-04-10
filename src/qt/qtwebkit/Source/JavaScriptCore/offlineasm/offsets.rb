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

def to32Bit(value)
    if value > 0x7fffffff
        value -= 1 << 32
    end
    value
end

OFFSET_HEADER_MAGIC_NUMBERS = [ to32Bit(0x9e43fd66), to32Bit(0x4379bfba) ]
OFFSET_MAGIC_NUMBERS = [ to32Bit(0xec577ac7), to32Bit(0x0ff5e755) ]

#
# MissingMagicValuesException
#
# Thrown when magic values are missing from the binary.
#

class MissingMagicValuesException < Exception
end

#
# offsetsList(ast)
# sizesList(ast)
#
# Returns a list of offsets and sizes used by the AST.
#

def offsetsList(ast)
    ast.filter(StructOffset).uniq.sort
end

def sizesList(ast)
    ast.filter(Sizeof).uniq.sort
end

#
# offsetsAndConfigurationIndex(ast, file) ->
#     [[offsets, index], ...]
#
# Parses the offsets from a file and returns a list of offsets and the
# index of the configuration that is valid in this build target.
#

def offsetsAndConfigurationIndex(file)
    endiannessMarkerBytes = nil
    result = {}
    
    def readInt(endianness, bytes)
        if endianness == :little
            # Little endian
            (bytes[0] << 0 |
             bytes[1] << 8 |
             bytes[2] << 16 |
             bytes[3] << 24)
        else
            # Big endian
            (bytes[0] << 24 |
             bytes[1] << 16 |
             bytes[2] << 8 |
             bytes[3] << 0)
        end
    end
    
    def prepareMagic(endianness, numbers)
        magicBytes = []
        numbers.each {
            | number |
            currentBytes = []
            4.times {
                currentBytes << (number & 0xff)
                number >>= 8
            }
            if endianness == :big
                currentBytes.reverse!
            end
            magicBytes += currentBytes
        }
        magicBytes
    end
    
    fileBytes = []
    
    File.open(file, "rb") {
        | inp |
        loop {
            byte = inp.getbyte
            break unless byte
            fileBytes << byte
        }
    }
    
    def sliceByteArrays(byteArray, pattern)
        result = []
        lastSlicePoint = 0
        (byteArray.length - pattern.length + 1).times {
            | index |
            foundOne = true
            pattern.length.times {
                | subIndex |
                if byteArray[index + subIndex] != pattern[subIndex]
                    foundOne = false
                    break
                end
            }
            if foundOne
                result << byteArray[lastSlicePoint...index]
                lastSlicePoint = index + pattern.length
            end
        }
        
        result << byteArray[lastSlicePoint...(byteArray.length)]
        
        result
    end
    
    [:little, :big].each {
        | endianness |
        headerMagicBytes = prepareMagic(endianness, OFFSET_HEADER_MAGIC_NUMBERS)
        magicBytes = prepareMagic(endianness, OFFSET_MAGIC_NUMBERS)
        
        bigArray = sliceByteArrays(fileBytes, headerMagicBytes)
        unless bigArray.size <= 1
            bigArray[1..-1].each {
                | configArray |
                array = sliceByteArrays(configArray, magicBytes)
                index = readInt(endianness, array[1])
                offsets = []
                array[2..-1].each {
                    | data |
                    offsets << readInt(endianness, data)
                }
                result[index] = offsets
            }
        end
    }
    
    raise MissingMagicValuesException unless result.length >= 1
    
    # result is {index1=>offsets1, index2=>offsets2} but we want to return
    # [[offsets1, index1], [offsets2, index2]].
    return result.map {
        | pair |
        pair.reverse
    }
end

#
# buildOffsetsMap(ast, offsetsList) -> [offsets, sizes]
#
# Builds a mapping between StructOffset nodes and their values.
#

def buildOffsetsMap(ast, offsetsList)
    offsetsMap = {}
    sizesMap = {}
    astOffsetsList = offsetsList(ast)
    astSizesList = sizesList(ast)
    raise unless astOffsetsList.size + astSizesList.size == offsetsList.size
    offsetsList(ast).each_with_index {
        | structOffset, index |
        offsetsMap[structOffset] = offsetsList.shift
    }
    sizesList(ast).each_with_index {
        | sizeof, index |
        sizesMap[sizeof] = offsetsList.shift
    }
    [offsetsMap, sizesMap]
end

