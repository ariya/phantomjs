/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SamplingTool.h"

#include "CodeBlock.h"
#include "Interpreter.h"
#include "Opcode.h"

#if !OS(WINDOWS)
#include <unistd.h>
#endif

namespace JSC {

#if ENABLE(SAMPLING_FLAGS)

void SamplingFlags::sample()
{
    uint32_t mask = static_cast<uint32_t>(1 << 31);
    unsigned index;

    for (index = 0; index < 32; ++index) {
        if (mask & s_flags)
            break;
        mask >>= 1;
    }

    s_flagCounts[32 - index]++;
}

void SamplingFlags::start()
{
    for (unsigned i = 0; i <= 32; ++i)
        s_flagCounts[i] = 0;
}
void SamplingFlags::stop()
{
    uint64_t total = 0;
    for (unsigned i = 0; i <= 32; ++i)
        total += s_flagCounts[i];

    if (total) {
        dataLogF("\nSamplingFlags: sample counts with flags set: (%lld total)\n", total);
        for (unsigned i = 0; i <= 32; ++i) {
            if (s_flagCounts[i])
                dataLogF("  [ %02d ] : %lld\t\t(%03.2f%%)\n", i, s_flagCounts[i], (100.0 * s_flagCounts[i]) / total);
        }
        dataLogF("\n");
    } else
    dataLogF("\nSamplingFlags: no samples.\n\n");
}
uint64_t SamplingFlags::s_flagCounts[33];

#else
void SamplingFlags::start() {}
void SamplingFlags::stop() {}
#endif

#if ENABLE(SAMPLING_REGIONS)
volatile uintptr_t SamplingRegion::s_currentOrReserved;
Spectrum<const char*>* SamplingRegion::s_spectrum;
unsigned long SamplingRegion::s_noneOfTheAbove;
unsigned SamplingRegion::s_numberOfSamplesSinceDump;

SamplingRegion::Locker::Locker()
{
    uintptr_t previous;
    while (true) {
        previous = s_currentOrReserved;
        if (previous & 1) {
#if OS(UNIX)
            sched_yield();
#endif
            continue;
        }
        if (WTF::weakCompareAndSwapUIntPtr(&s_currentOrReserved, previous, previous | 1))
            break;
    }
}

SamplingRegion::Locker::~Locker()
{
    // We don't need the CAS, but we do it out of an
    // abundance of caution (and because it gives us a memory fence, which is
    // never bad).
    uintptr_t previous;
    do {
        previous = s_currentOrReserved;
    } while (!WTF::weakCompareAndSwapUIntPtr(&s_currentOrReserved, previous, previous & ~1));
}

void SamplingRegion::sample()
{
    // Make sure we lock s_current.
    Locker locker;
    
    // Create a spectrum if we don't have one already.
    if (!s_spectrum)
        s_spectrum = new Spectrum<const char*>();
    
    ASSERT(s_currentOrReserved & 1);
    
    // Walk the region stack, and record each region we see.
    SamplingRegion* region = bitwise_cast<SamplingRegion*>(s_currentOrReserved & ~1);
    if (region) {
        for (; region; region = region->m_previous)
            s_spectrum->add(region->m_name);
    } else
        s_noneOfTheAbove++;
    
    if (s_numberOfSamplesSinceDump++ == SamplingThread::s_hertz) {
        s_numberOfSamplesSinceDump = 0;
        dumpInternal();
    }
}

void SamplingRegion::dump()
{
    Locker locker;
    
    dumpInternal();
}

void SamplingRegion::dumpInternal()
{
    if (!s_spectrum) {
        dataLogF("\nSamplingRegion: was never sampled.\n\n");
        return;
    }
    
    Vector<Spectrum<const char*>::KeyAndCount> list = s_spectrum->buildList();
    
    unsigned long total = s_noneOfTheAbove;
    for (unsigned i = list.size(); i--;)
        total += list[i].count;
    
    dataLogF("\nSamplingRegion: sample counts for regions: (%lu samples)\n", total);

    for (unsigned i = list.size(); i--;)
        dataLogF("    %3.2lf%%  %s\n", (100.0 * list[i].count) / total, list[i].key);
}
#else // ENABLE(SAMPLING_REGIONS)
void SamplingRegion::dump() { }
#endif // ENABLE(SAMPLING_REGIONS)

/*
  Start with flag 16 set.
  By doing this the monitoring of lower valued flags will be masked out
  until flag 16 is explictly cleared.
*/
uint32_t SamplingFlags::s_flags = 1 << 15;


#if OS(WINDOWS)

static void sleepForMicroseconds(unsigned us)
{
    unsigned ms = us / 1000;
    if (us && !ms)
        ms = 1;
    Sleep(ms);
}

#else 

static void sleepForMicroseconds(unsigned us)
{
    usleep(us);
}

#endif

static inline unsigned hertz2us(unsigned hertz)
{
    return 1000000 / hertz;
}


SamplingTool* SamplingTool::s_samplingTool = 0;


bool SamplingThread::s_running = false;
unsigned SamplingThread::s_hertz = 10000;
ThreadIdentifier SamplingThread::s_samplingThread;

void SamplingThread::threadStartFunc(void*)
{
    while (s_running) {
        sleepForMicroseconds(hertz2us(s_hertz));

#if ENABLE(SAMPLING_FLAGS)
        SamplingFlags::sample();
#endif
#if ENABLE(SAMPLING_REGIONS)
        SamplingRegion::sample();
#endif
#if ENABLE(OPCODE_SAMPLING)
        SamplingTool::sample();
#endif
    }
}


void SamplingThread::start(unsigned hertz)
{
    ASSERT(!s_running);
    s_running = true;
    s_hertz = hertz;

    s_samplingThread = createThread(threadStartFunc, 0, "JavaScriptCore::Sampler");
}

void SamplingThread::stop()
{
    ASSERT(s_running);
    s_running = false;
    waitForThreadCompletion(s_samplingThread);
}


void ScriptSampleRecord::sample(CodeBlock* codeBlock, Instruction* vPC)
{
    if (!m_samples) {
        m_size = codeBlock->instructions().size();
        m_samples = static_cast<int*>(calloc(m_size, sizeof(int)));
        m_codeBlock = codeBlock;
    }

    ++m_sampleCount;

    unsigned offest = vPC - codeBlock->instructions().begin();
    // Since we don't read and write codeBlock and vPC atomically, this check
    // can fail if we sample mid op_call / op_ret.
    if (offest < m_size) {
        m_samples[offest]++;
        m_opcodeSampleCount++;
    }
}

void SamplingTool::doRun()
{
    Sample sample(m_sample, m_codeBlock);
    ++m_sampleCount;

    if (sample.isNull())
        return;

    if (!sample.inHostFunction()) {
        unsigned opcodeID = m_interpreter->getOpcodeID(sample.vPC()[0].u.opcode);

        ++m_opcodeSampleCount;
        ++m_opcodeSamples[opcodeID];

        if (sample.inCTIFunction())
            m_opcodeSamplesInCTIFunctions[opcodeID]++;
    }

#if ENABLE(CODEBLOCK_SAMPLING)
    if (CodeBlock* codeBlock = sample.codeBlock()) {
        MutexLocker locker(m_scriptSampleMapMutex);
        ScriptSampleRecord* record = m_scopeSampleMap->get(codeBlock->ownerExecutable());
        ASSERT(record);
        record->sample(codeBlock, sample.vPC());
    }
#endif
}

void SamplingTool::sample()
{
    s_samplingTool->doRun();
}

void SamplingTool::notifyOfScope(VM& vm, ScriptExecutable* script)
{
#if ENABLE(CODEBLOCK_SAMPLING)
    MutexLocker locker(m_scriptSampleMapMutex);
    m_scopeSampleMap->set(script, adoptPtr(new ScriptSampleRecord(vm, script)));
#else
    UNUSED_PARAM(vm);
    UNUSED_PARAM(script);
#endif
}

void SamplingTool::setup()
{
    s_samplingTool = this;
}

#if ENABLE(OPCODE_SAMPLING)

struct OpcodeSampleInfo {
    OpcodeID opcode;
    long long count;
    long long countInCTIFunctions;
};

struct LineCountInfo {
    unsigned line;
    unsigned count;
};

static int compareOpcodeIndicesSampling(const void* left, const void* right)
{
    const OpcodeSampleInfo* leftSampleInfo = reinterpret_cast<const OpcodeSampleInfo*>(left);
    const OpcodeSampleInfo* rightSampleInfo = reinterpret_cast<const OpcodeSampleInfo*>(right);

    return (leftSampleInfo->count < rightSampleInfo->count) ? 1 : (leftSampleInfo->count > rightSampleInfo->count) ? -1 : 0;
}

#if ENABLE(CODEBLOCK_SAMPLING)
static int compareLineCountInfoSampling(const void* left, const void* right)
{
    const LineCountInfo* leftLineCount = reinterpret_cast<const LineCountInfo*>(left);
    const LineCountInfo* rightLineCount = reinterpret_cast<const LineCountInfo*>(right);

    return (leftLineCount->line > rightLineCount->line) ? 1 : (leftLineCount->line < rightLineCount->line) ? -1 : 0;
}

static int compareScriptSampleRecords(const void* left, const void* right)
{
    const ScriptSampleRecord* const leftValue = *static_cast<const ScriptSampleRecord* const *>(left);
    const ScriptSampleRecord* const rightValue = *static_cast<const ScriptSampleRecord* const *>(right);

    return (leftValue->m_sampleCount < rightValue->m_sampleCount) ? 1 : (leftValue->m_sampleCount > rightValue->m_sampleCount) ? -1 : 0;
}
#endif

void SamplingTool::dump(ExecState* exec)
{
    // Tidies up SunSpider output by removing short scripts - such a small number of samples would likely not be useful anyhow.
    if (m_sampleCount < 10)
        return;
    
    // (1) Build and sort 'opcodeSampleInfo' array.

    OpcodeSampleInfo opcodeSampleInfo[numOpcodeIDs];
    for (int i = 0; i < numOpcodeIDs; ++i) {
        opcodeSampleInfo[i].opcode = static_cast<OpcodeID>(i);
        opcodeSampleInfo[i].count = m_opcodeSamples[i];
        opcodeSampleInfo[i].countInCTIFunctions = m_opcodeSamplesInCTIFunctions[i];
    }

    qsort(opcodeSampleInfo, numOpcodeIDs, sizeof(OpcodeSampleInfo), compareOpcodeIndicesSampling);

    // (2) Print Opcode sampling results.

    dataLogF("\nBytecode samples [*]\n");
    dataLogF("                             sample   %% of       %% of     |   cti     cti %%\n");
    dataLogF("opcode                       count     VM        total    |  count   of self\n");
    dataLogF("-------------------------------------------------------   |  ----------------\n");

    for (int i = 0; i < numOpcodeIDs; ++i) {
        long long count = opcodeSampleInfo[i].count;
        if (!count)
            continue;

        OpcodeID opcodeID = opcodeSampleInfo[i].opcode;
        
        const char* opcodeName = opcodeNames[opcodeID];
        const char* opcodePadding = padOpcodeName(opcodeID, 28);
        double percentOfVM = (static_cast<double>(count) * 100) / m_opcodeSampleCount;
        double percentOfTotal = (static_cast<double>(count) * 100) / m_sampleCount;
        long long countInCTIFunctions = opcodeSampleInfo[i].countInCTIFunctions;
        double percentInCTIFunctions = (static_cast<double>(countInCTIFunctions) * 100) / count;
        debugDebugPrintf("%s:%s%-6lld %.3f%%\t%.3f%%\t  |   %-6lld %.3f%%\n", opcodeName, opcodePadding, count, percentOfVM, percentOfTotal, countInCTIFunctions, percentInCTIFunctions);
    }
    
    dataLogF("\n[*] Samples inside host code are not charged to any Bytecode.\n\n");
    dataLogF("\tSamples inside VM:\t\t%lld / %lld (%.3f%%)\n", m_opcodeSampleCount, m_sampleCount, (static_cast<double>(m_opcodeSampleCount) * 100) / m_sampleCount);
    dataLogF("\tSamples inside host code:\t%lld / %lld (%.3f%%)\n\n", m_sampleCount - m_opcodeSampleCount, m_sampleCount, (static_cast<double>(m_sampleCount - m_opcodeSampleCount) * 100) / m_sampleCount);
    dataLogF("\tsample count:\tsamples inside this opcode\n");
    dataLogF("\t%% of VM:\tsample count / all opcode samples\n");
    dataLogF("\t%% of total:\tsample count / all samples\n");
    dataLogF("\t--------------\n");
    dataLogF("\tcti count:\tsamples inside a CTI function called by this opcode\n");
    dataLogF("\tcti %% of self:\tcti count / sample count\n");
    
#if ENABLE(CODEBLOCK_SAMPLING)

    // (3) Build and sort 'codeBlockSamples' array.

    int scopeCount = m_scopeSampleMap->size();
    Vector<ScriptSampleRecord*> codeBlockSamples(scopeCount);
    ScriptSampleRecordMap::iterator iter = m_scopeSampleMap->begin();
    for (int i = 0; i < scopeCount; ++i, ++iter)
        codeBlockSamples[i] = iter->value.get();

    qsort(codeBlockSamples.begin(), scopeCount, sizeof(ScriptSampleRecord*), compareScriptSampleRecords);

    // (4) Print data from 'codeBlockSamples' array.

    dataLogF("\nCodeBlock samples\n\n"); 

    for (int i = 0; i < scopeCount; ++i) {
        ScriptSampleRecord* record = codeBlockSamples[i];
        CodeBlock* codeBlock = record->m_codeBlock;

        double blockPercent = (record->m_sampleCount * 100.0) / m_sampleCount;

        if (blockPercent >= 1) {
            //Instruction* code = codeBlock->instructions().begin();
            dataLogF("#%d: %s:%d: %d / %lld (%.3f%%)\n", i + 1, record->m_executable->sourceURL().utf8().data(), codeBlock->lineNumberForBytecodeOffset(0), record->m_sampleCount, m_sampleCount, blockPercent);
            if (i < 10) {
                HashMap<unsigned,unsigned> lineCounts;
                codeBlock->dump(exec);

                dataLogF("    Opcode and line number samples [*]\n\n");
                for (unsigned op = 0; op < record->m_size; ++op) {
                    int count = record->m_samples[op];
                    if (count) {
                        dataLogF("    [% 4d] has sample count: % 4d\n", op, count);
                        unsigned line = codeBlock->lineNumberForBytecodeOffset(op);
                        lineCounts.set(line, (lineCounts.contains(line) ? lineCounts.get(line) : 0) + count);
                    }
                }
                dataLogF("\n");

                int linesCount = lineCounts.size();
                Vector<LineCountInfo> lineCountInfo(linesCount);
                int lineno = 0;
                for (HashMap<unsigned,unsigned>::iterator iter = lineCounts.begin(); iter != lineCounts.end(); ++iter, ++lineno) {
                    lineCountInfo[lineno].line = iter->key;
                    lineCountInfo[lineno].count = iter->value;
                }

                qsort(lineCountInfo.begin(), linesCount, sizeof(LineCountInfo), compareLineCountInfoSampling);

                for (lineno = 0; lineno < linesCount; ++lineno) {
                    dataLogF("    Line #%d has sample count %d.\n", lineCountInfo[lineno].line, lineCountInfo[lineno].count);
                }
                dataLogF("\n");
                dataLogF("    [*] Samples inside host code are charged to the calling Bytecode.\n");
                dataLogF("        Samples on a call / return boundary are not charged to a specific opcode or line.\n\n");
                dataLogF("            Samples on a call / return boundary: %d / %d (%.3f%%)\n\n", record->m_sampleCount - record->m_opcodeSampleCount, record->m_sampleCount, (static_cast<double>(record->m_sampleCount - record->m_opcodeSampleCount) * 100) / record->m_sampleCount);
            }
        }
    }
#else
    UNUSED_PARAM(exec);
#endif
}

#else

void SamplingTool::dump(ExecState*)
{
}

#endif

} // namespace JSC
