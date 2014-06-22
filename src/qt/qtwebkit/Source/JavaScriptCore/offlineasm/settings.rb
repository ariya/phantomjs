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
require "backends"
require "parser"
require "transform"

#
# computeSettingsCombinations(ast) -> settingsCombiations
#
# Computes an array of settings maps, where a settings map constitutes
# a configuration for the assembly code being generated. The map
# contains key value pairs where keys are settings names (strings) and
# the values are booleans (true for enabled, false for disabled).
#

def computeSettingsCombinations(ast)
    settingsCombinations = []
    
    def settingsCombinator(settingsCombinations, mapSoFar, remaining)
        if remaining.empty?
            settingsCombinations << mapSoFar
            return
        end
        
        newMap = mapSoFar.dup
        newMap[remaining[0]] = true
        settingsCombinator(settingsCombinations, newMap, remaining[1..-1])
        
        newMap = mapSoFar.dup
        newMap[remaining[0]] = false
        settingsCombinator(settingsCombinations, newMap, remaining[1..-1])
    end
    
    settingsCombinator(settingsCombinations, {}, (ast.filter(Setting).uniq.collect{|v| v.name} + BACKENDS).uniq)
    
    settingsCombinations
end

#
# forSettings(concreteSettings, ast) {
#     | concreteSettings, lowLevelAST, backend | ... }
#
# Determines if the settings combination is valid, and if so, calls
# the block with the information you need to generate code.
#

def forSettings(concreteSettings, ast)
    # Check which architectures this combinator claims to support.
    numClaimedBackends = 0
    selectedBackend = nil
    BACKENDS.each {
        | backend |
        isSupported = concreteSettings[backend]
        raise unless isSupported != nil
        numClaimedBackends += if isSupported then 1 else 0 end
        if isSupported
            selectedBackend = backend
        end
    }
    
    return if numClaimedBackends > 1
    
    # Resolve the AST down to a low-level form (no macros or conditionals).
    lowLevelAST = ast.resolveSettings(concreteSettings)
    
    yield concreteSettings, lowLevelAST, selectedBackend
end

#
# forEachValidSettingsCombination(ast) {
#     | concreteSettings, ast, backend, index | ... }
#
# forEachValidSettingsCombination(ast, settingsCombinations) {
#     | concreteSettings, ast, backend, index | ... }
#
# Executes the given block for each valid settings combination in the
# settings map. The ast passed into the block is resolved
# (ast.resolve) against the settings.
#
# The first form will call computeSettingsCombinations(ast) for you.
#

def forEachValidSettingsCombination(ast, *optionalSettingsCombinations)
    raise if optionalSettingsCombinations.size > 1
    
    if optionalSettingsCombinations.empty?
        settingsCombinations = computeSettingsCombinations(ast)
    else
        settingsCombinations = optionalSettingsCombiations[0]
    end
    
    settingsCombinations.each_with_index {
        | concreteSettings, index |
        forSettings(concreteSettings, ast) {
            | concreteSettings_, lowLevelAST, backend |
            yield concreteSettings, lowLevelAST, backend, index
        }
    }
end

#
# cppSettingsTest(concreteSettings)
#
# Returns the C++ code used to test if we are in a configuration that
# corresponds to the given concrete settings.
#

def cppSettingsTest(concreteSettings)
    "#if " + concreteSettings.to_a.collect{
        | pair |
        (if pair[1]
             ""
         else
             "!"
         end) + "OFFLINE_ASM_" + pair[0]
    }.join(" && ")
end

#
# isASTErroneous(ast)
#
# Tests to see if the AST claims that there is an error - i.e. if the
# user's code, after settings resolution, has Error nodes.
#

def isASTErroneous(ast)
    not ast.filter(Error).empty?
end

#
# assertConfiguration(concreteSettings)
#
# Emits a check that asserts that we're using the given configuration.
#

def assertConfiguration(concreteSettings)
    $output.puts cppSettingsTest(concreteSettings)
    $output.puts "#else"
    $output.puts "#error \"Configuration mismatch.\""
    $output.puts "#endif"
end

#
# emitCodeInConfiguration(concreteSettings, ast, backend) {
#     | concreteSettings, ast, backend | ... }
#
# Emits all relevant guards to see if the configuration holds and
# calls the block if the configuration is not erroneous.
#

def emitCodeInConfiguration(concreteSettings, ast, backend)
    $output.puts cppSettingsTest(concreteSettings)
    
    if isASTErroneous(ast)
        $output.puts "#error \"Invalid configuration.\""
    elsif not WORKING_BACKENDS.include? backend
        $output.puts "#error \"This backend is not supported yet.\""
    else
        yield concreteSettings, ast, backend
    end
    
    $output.puts "#endif"
end

#
# emitCodeInAllConfigurations(ast) {
#     | concreteSettings, ast, backend, index | ... }
#
# Emits guard codes for all valid configurations, and calls the block
# for those configurations that are valid and not erroneous.
#

def emitCodeInAllConfigurations(ast)
    forEachValidSettingsCombination(ast) {
        | concreteSettings, lowLevelAST, backend, index |
        $output.puts cppSettingsTest(concreteSettings)
        yield concreteSettings, lowLevelAST, backend, index
        $output.puts "#endif"
    }
end



