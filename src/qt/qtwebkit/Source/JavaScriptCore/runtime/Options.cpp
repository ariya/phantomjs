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

#include "config.h"
#include "Options.h"

#include "HeapStatistics.h"
#include <algorithm>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wtf/NumberOfCores.h>
#include <wtf/PageBlock.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringExtras.h>

#if OS(DARWIN) && ENABLE(PARALLEL_GC)
#include <sys/sysctl.h>
#endif

namespace JSC {

static bool parse(const char* string, bool& value)
{
    if (!strcasecmp(string, "true") || !strcasecmp(string, "yes") || !strcmp(string, "1")) {
        value = true;
        return true;
    }
    if (!strcasecmp(string, "false") || !strcasecmp(string, "no") || !strcmp(string, "0")) {
        value = false;
        return true;
    }
    return false;
}

static bool parse(const char* string, int32_t& value)
{
    return sscanf(string, "%d", &value) == 1;
}

static bool parse(const char* string, unsigned& value)
{
    return sscanf(string, "%u", &value) == 1;
}

static bool parse(const char* string, double& value)
{
    return sscanf(string, "%lf", &value) == 1;
}

static bool parse(const char* string, OptionRange& value)
{
    return value.init(string);
}

template<typename T>
void overrideOptionWithHeuristic(T& variable, const char* name)
{
#if !OS(WINCE)
    const char* stringValue = getenv(name);
    if (!stringValue)
        return;
    
    if (parse(stringValue, variable))
        return;
    
    fprintf(stderr, "WARNING: failed to parse %s=%s\n", name, stringValue);
#endif
}

static unsigned computeNumberOfGCMarkers(int maxNumberOfGCMarkers)
{
    int cpusToUse = 1;

#if ENABLE(PARALLEL_GC)
    cpusToUse = std::min(WTF::numberOfProcessorCores(), maxNumberOfGCMarkers);

    // Be paranoid, it is the OS we're dealing with, after all.
    ASSERT(cpusToUse >= 1);
    if (cpusToUse < 1)
        cpusToUse = 1;
#else
    UNUSED_PARAM(maxNumberOfGCMarkers);
#endif

    return cpusToUse;
}

bool OptionRange::init(const char* rangeString)
{
    // rangeString should be in the form of [!]<low>[:<high>]
    // where low and high are unsigned

    bool invert = false;

    if (m_state > Uninitialized)
        return true;

    if (!rangeString) {
        m_state = InitError;
        return false;
    }

    m_rangeString = rangeString;

    if (*rangeString == '!') {
        invert = true;
        rangeString++;
    }

    int scanResult = sscanf(rangeString, " %u:%u", &m_lowLimit, &m_highLimit);

    if (!scanResult || scanResult == EOF) {
        m_state = InitError;
        return false;
    }

    if (scanResult == 1)
        m_highLimit = m_lowLimit;

    if (m_lowLimit > m_highLimit) {
        m_state = InitError;
        return false;
    }

    m_state = invert ? Inverted : Normal;

    return true;
}

bool OptionRange::isInRange(unsigned count)
{
    if (m_state < Normal)
        return true;

    if ((m_lowLimit <= count) && (count <= m_highLimit))
        return m_state == Normal ? true : false;

    return m_state == Normal ? false : true;
}

Options::Entry Options::s_options[Options::numberOfOptions];

// Realize the names for each of the options:
const Options::EntryInfo Options::s_optionsInfo[Options::numberOfOptions] = {
#define FOR_EACH_OPTION(type_, name_, defaultValue_) \
    { #name_, Options::type_##Type },
    JSC_OPTIONS(FOR_EACH_OPTION)
#undef FOR_EACH_OPTION
};

void Options::initialize()
{
    // Initialize each of the options with their default values:
#define FOR_EACH_OPTION(type_, name_, defaultValue_) \
    name_() = defaultValue_;
    JSC_OPTIONS(FOR_EACH_OPTION)
#undef FOR_EACH_OPTION
        
#if USE(CF) || OS(UNIX)
    objectsAreImmortal() = !!getenv("JSImmortalZombieEnabled");
    useZombieMode() = !!getenv("JSImmortalZombieEnabled") || !!getenv("JSZombieEnabled");

    gcMaxHeapSize() = getenv("GCMaxHeapSize") ? HeapStatistics::parseMemoryAmount(getenv("GCMaxHeapSize")) : 0;
    recordGCPauseTimes() = !!getenv("JSRecordGCPauseTimes");
    logHeapStatisticsAtExit() = gcMaxHeapSize() || recordGCPauseTimes();
#endif

    // Allow environment vars to override options if applicable.
    // The evn var should be the name of the option prefixed with
    // "JSC_".
#define FOR_EACH_OPTION(type_, name_, defaultValue_) \
    overrideOptionWithHeuristic(name_(), "JSC_" #name_);
    JSC_OPTIONS(FOR_EACH_OPTION)
#undef FOR_EACH_OPTION

#if 0
    ; // Deconfuse editors that do auto indentation
#endif
    
#if !ENABLE(JIT)
    useJIT() = false;
    useDFGJIT() = false;
#endif
#if !ENABLE(YARR_JIT)
    useRegExpJIT() = false;
#endif
    
    // Do range checks where needed and make corrections to the options:
    ASSERT(thresholdForOptimizeAfterLongWarmUp() >= thresholdForOptimizeAfterWarmUp());
    ASSERT(thresholdForOptimizeAfterWarmUp() >= thresholdForOptimizeSoon());
    ASSERT(thresholdForOptimizeAfterWarmUp() >= 0);

    // Compute the maximum value of the reoptimization retry counter. This is simply
    // the largest value at which we don't overflow the execute counter, when using it
    // to left-shift the execution counter by this amount. Currently the value ends
    // up being 18, so this loop is not so terrible; it probably takes up ~100 cycles
    // total on a 32-bit processor.
    reoptimizationRetryCounterMax() = 0;
    while ((static_cast<int64_t>(thresholdForOptimizeAfterLongWarmUp()) << (reoptimizationRetryCounterMax() + 1)) <= static_cast<int64_t>(std::numeric_limits<int32>::max()))
        reoptimizationRetryCounterMax()++;

    ASSERT((static_cast<int64_t>(thresholdForOptimizeAfterLongWarmUp()) << reoptimizationRetryCounterMax()) > 0);
    ASSERT((static_cast<int64_t>(thresholdForOptimizeAfterLongWarmUp()) << reoptimizationRetryCounterMax()) <= static_cast<int64_t>(std::numeric_limits<int32>::max()));
}

// Parses a single command line option in the format "<optionName>=<value>"
// (no spaces allowed) and set the specified option if appropriate.
bool Options::setOption(const char* arg)
{
    // arg should look like this:
    //   <jscOptionName>=<appropriate value>
    const char* equalStr = strchr(arg, '=');
    if (!equalStr)
        return false;

    const char* valueStr = equalStr + 1;

    // For each option, check if the specify arg is a match. If so, set the arg
    // if the value makes sense. Otherwise, move on to checking the next option.
#define FOR_EACH_OPTION(type_, name_, defaultValue_)    \
    if (!strncmp(arg, #name_, equalStr - arg)) {        \
        type_ value;                                    \
        value = 0;                                      \
        bool success = parse(valueStr, value);          \
        if (success) {                                  \
            name_() = value;                            \
            return true;                                \
        }                                               \
        return false;                                   \
    }

    JSC_OPTIONS(FOR_EACH_OPTION)
#undef FOR_EACH_OPTION

        return false; // No option matched.
}

void Options::dumpAllOptions(FILE* stream)
{
    fprintf(stream, "JSC runtime options:\n");
    for (int id = 0; id < numberOfOptions; id++)
        dumpOption(static_cast<OptionID>(id), stream, "   ", "\n");
}

void Options::dumpOption(OptionID id, FILE* stream, const char* header, const char* footer)
{
    if (id >= numberOfOptions)
        return; // Illegal option.

    fprintf(stream, "%s%s: ", header, s_optionsInfo[id].name);
    switch (s_optionsInfo[id].type) {
    case boolType:
        fprintf(stream, "%s", s_options[id].u.boolVal?"true":"false");
        break;
    case unsignedType:
        fprintf(stream, "%u", s_options[id].u.unsignedVal);
        break;
    case doubleType:
        fprintf(stream, "%lf", s_options[id].u.doubleVal);
        break;
    case int32Type:
        fprintf(stream, "%d", s_options[id].u.int32Val);
        break;
    case optionRangeType:
        fprintf(stream, "%s", s_options[id].u.optionRangeVal.rangeString());
        break;
    }
    fprintf(stream, "%s", footer);
}

} // namespace JSC

