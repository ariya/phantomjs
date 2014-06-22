/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef Options_h
#define Options_h

#include "JSExportMacros.h"
#include <stdint.h>
#include <stdio.h>

namespace JSC {

// How do JSC VM options work?
// ===========================
// The JSC_OPTIONS() macro below defines a list of all JSC options in use,
// along with their types and default values. The options values are actually
// realized as an array of Options::Entry elements.
//
//     Options::initialize() will initialize the array of options values with
// the defaults specified in JSC_OPTIONS() below. After that, the values can
// be programmatically read and written to using an accessor method with the
// same name as the option. For example, the option "useJIT" can be read and
// set like so:
//
//     bool jitIsOn = Options::useJIT();  // Get the option value.
//     Options::useJIT() = false;         // Sets the option value.
//
//     If you want to tweak any of these values programmatically for testing
// purposes, you can do so in Options::initialize() after the default values
// are set.
//
//     Alternatively, you can override the default values by specifying
// environment variables of the form: JSC_<name of JSC option>.
//
// Note: Options::initialize() tries to ensure some sanity on the option values
// which are set by doing some range checks, and value corrections. These
// checks are done after the option values are set. If you alter the option
// values after the sanity checks (for your own testing), then you're liable to
// ensure that the new values set are sane and reasonable for your own run.

class OptionRange {
private:
    enum RangeState { Uninitialized, InitError, Normal, Inverted };
public:
    OptionRange& operator= (const int& rhs)
    { // Only needed for initialization
        if (!rhs) {
            m_state = Uninitialized;
            m_rangeString = 0;
            m_lowLimit = 0;
            m_highLimit = 0;
        }
        return *this;
    }

    bool init(const char*);
    bool isInRange(unsigned);
    const char* rangeString() { return (m_state > InitError) ? m_rangeString : "<null>"; }

private:
    RangeState m_state;
    const char* m_rangeString;
    unsigned m_lowLimit;
    unsigned m_highLimit;
};

typedef OptionRange optionRange;

#define JSC_OPTIONS(v) \
    v(bool, useJIT,    true) \
    v(bool, useDFGJIT, true) \
    v(bool, useRegExpJIT, true) \
    \
    v(bool, forceDFGCodeBlockLiveness, false) \
    \
    v(bool, dumpGeneratedBytecodes, false) \
    \
    /* showDisassembly implies showDFGDisassembly. */ \
    v(bool, showDisassembly, false) \
    v(bool, showDFGDisassembly, false) \
    v(bool, showAllDFGNodes, false) \
    v(optionRange, bytecodeRangeToDFGCompile, 0) \
    v(bool, dumpBytecodeAtDFGTime, false) \
    v(bool, dumpGraphAtEachPhase, false) \
    v(bool, verboseCompilation, false) \
    v(bool, logCompilationChanges, false) \
    v(bool, printEachOSRExit, false) \
    v(bool, validateGraph, false) \
    v(bool, validateGraphAtEachPhase, false) \
    \
    v(bool, enableProfiler, false) \
    \
    v(unsigned, maximumOptimizationCandidateInstructionCount, 10000) \
    \
    v(unsigned, maximumFunctionForCallInlineCandidateInstructionCount, 180) \
    v(unsigned, maximumFunctionForClosureCallInlineCandidateInstructionCount, 100) \
    v(unsigned, maximumFunctionForConstructInlineCandidateInstructionCount, 100) \
    \
    /* Depth of inline stack, so 1 = no inlining, 2 = one level, etc. */ \
    v(unsigned, maximumInliningDepth, 5) \
    \
    v(int32, thresholdForJITAfterWarmUp, 100) \
    v(int32, thresholdForJITSoon, 100) \
    \
    v(int32, thresholdForOptimizeAfterWarmUp, 1000) \
    v(int32, thresholdForOptimizeAfterLongWarmUp, 1000) \
    v(int32, thresholdForOptimizeSoon, 1000) \
    \
    v(int32, executionCounterIncrementForLoop, 1) \
    v(int32, executionCounterIncrementForReturn, 15) \
    \
    v(int32, evalThresholdMultiplier, 10) \
    \
    v(bool, randomizeExecutionCountsBetweenCheckpoints, false) \
    v(int32, maximumExecutionCountsBetweenCheckpoints, 1000) \
    \
    v(unsigned, likelyToTakeSlowCaseMinimumCount, 100) \
    v(unsigned, couldTakeSlowCaseMinimumCount, 10) \
    \
    v(unsigned, osrExitCountForReoptimization, 100) \
    v(unsigned, osrExitCountForReoptimizationFromLoop, 5) \
    \
    v(unsigned, reoptimizationRetryCounterMax, 0)  \
    v(unsigned, reoptimizationRetryCounterStep, 1) \
    \
    v(unsigned, minimumOptimizationDelay, 1) \
    v(unsigned, maximumOptimizationDelay, 5) \
    v(double, desiredProfileLivenessRate, 0.75) \
    v(double, desiredProfileFullnessRate, 0.35) \
    \
    v(double, doubleVoteRatioForDoubleFormat, 2) \
    v(double, structureCheckVoteRatioForHoisting, 1) \
    \
    v(unsigned, minimumNumberOfScansBetweenRebalance, 100) \
    v(unsigned, numberOfGCMarkers, computeNumberOfGCMarkers(7)) \
    v(unsigned, opaqueRootMergeThreshold, 1000) \
    v(double, minHeapUtilization, 0.8) \
    v(double, minCopiedBlockUtilization, 0.9) \
    \
    v(bool, forceWeakRandomSeed, false) \
    v(unsigned, forcedWeakRandomSeed, 0) \
    \
    v(bool, useZombieMode, false) \
    v(bool, objectsAreImmortal, false) \
    v(bool, showObjectStatistics, false) \
    \
    v(unsigned, gcMaxHeapSize, 0) \
    v(bool, recordGCPauseTimes, false) \
    v(bool, logHeapStatisticsAtExit, false) 

class Options {
public:
    // This typedef is to allow us to eliminate the '_' in the field name in
    // union inside Entry. This is needed to keep the style checker happy.
    typedef int32_t int32;

    // Declare the option IDs:
    enum OptionID {
#define FOR_EACH_OPTION(type_, name_, defaultValue_) \
        OPT_##name_,
        JSC_OPTIONS(FOR_EACH_OPTION)
#undef FOR_EACH_OPTION
        numberOfOptions
    };


    static void initialize();

    // Parses a single command line option in the format "<optionName>=<value>"
    // (no spaces allowed) and set the specified option if appropriate.
    JS_EXPORT_PRIVATE static bool setOption(const char* arg);
    JS_EXPORT_PRIVATE static void dumpAllOptions(FILE* stream = stdout);
    static void dumpOption(OptionID id, FILE* stream = stdout, const char* header = "", const char* footer = "");

    // Declare accessors for each option:
#define FOR_EACH_OPTION(type_, name_, defaultValue_) \
    ALWAYS_INLINE static type_& name_() { return s_options[OPT_##name_].u.type_##Val; }

    JSC_OPTIONS(FOR_EACH_OPTION)
#undef FOR_EACH_OPTION

private:
    enum EntryType {
        boolType,
        unsignedType,
        doubleType,
        int32Type,
        optionRangeType,
    };

    // For storing for an option value:
    struct Entry {
        union {
            bool boolVal;
            unsigned unsignedVal;
            double doubleVal;
            int32 int32Val;
            OptionRange optionRangeVal;
        } u;
    };

    // For storing constant meta data about each option:
    struct EntryInfo {
        const char* name;
        EntryType type;
    };

    Options();

    // Declare the options:
#define FOR_EACH_OPTION(type_, name_, defaultValue_) \
    type_ m_##name_;
    JSC_OPTIONS(FOR_EACH_OPTION)
#undef FOR_EACH_OPTION

    // Declare the singleton instance of the options store:
    JS_EXPORTDATA static Entry s_options[numberOfOptions];
    static const EntryInfo s_optionsInfo[numberOfOptions];
};

} // namespace JSC

#endif // Options_h
