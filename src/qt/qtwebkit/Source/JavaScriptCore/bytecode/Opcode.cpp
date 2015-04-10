/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
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
#include "Opcode.h"

#if ENABLE(OPCODE_STATS)
#include <stdio.h>
#include <wtf/FixedArray.h>
#endif

using namespace std;

namespace JSC {

const char* const opcodeNames[] = {
#define OPCODE_NAME_ENTRY(opcode, size) #opcode,
    FOR_EACH_OPCODE_ID(OPCODE_NAME_ENTRY)
#undef OPCODE_NAME_ENTRY
};

#if ENABLE(OPCODE_STATS)

long long OpcodeStats::opcodeCounts[numOpcodeIDs];
long long OpcodeStats::opcodePairCounts[numOpcodeIDs][numOpcodeIDs];
int OpcodeStats::lastOpcode = -1;

static OpcodeStats logger;

OpcodeStats::OpcodeStats()
{
    for (int i = 0; i < numOpcodeIDs; ++i)
        opcodeCounts[i] = 0;
    
    for (int i = 0; i < numOpcodeIDs; ++i)
        for (int j = 0; j < numOpcodeIDs; ++j)
            opcodePairCounts[i][j] = 0;
}

static int compareOpcodeIndices(const void* left, const void* right)
{
    long long leftValue = OpcodeStats::opcodeCounts[*(int*) left];
    long long rightValue = OpcodeStats::opcodeCounts[*(int*) right];
    
    if (leftValue < rightValue)
        return 1;
    else if (leftValue > rightValue)
        return -1;
    else
        return 0;
}

static int compareOpcodePairIndices(const void* left, const void* right)
{
    pair<int, int> leftPair = *(pair<int, int>*) left;
    long long leftValue = OpcodeStats::opcodePairCounts[leftPair.first][leftPair.second];
    pair<int, int> rightPair = *(pair<int, int>*) right;
    long long rightValue = OpcodeStats::opcodePairCounts[rightPair.first][rightPair.second];
    
    if (leftValue < rightValue)
        return 1;
    else if (leftValue > rightValue)
        return -1;
    else
        return 0;
}

OpcodeStats::~OpcodeStats()
{
    long long totalInstructions = 0;
    for (int i = 0; i < numOpcodeIDs; ++i)
        totalInstructions += opcodeCounts[i];
    
    long long totalInstructionPairs = 0;
    for (int i = 0; i < numOpcodeIDs; ++i)
        for (int j = 0; j < numOpcodeIDs; ++j)
            totalInstructionPairs += opcodePairCounts[i][j];

    FixedArray<int, numOpcodeIDs> sortedIndices;
    for (int i = 0; i < numOpcodeIDs; ++i)
        sortedIndices[i] = i;
    qsort(sortedIndices.data(), numOpcodeIDs, sizeof(int), compareOpcodeIndices);
    
    pair<int, int> sortedPairIndices[numOpcodeIDs * numOpcodeIDs];
    pair<int, int>* currentPairIndex = sortedPairIndices;
    for (int i = 0; i < numOpcodeIDs; ++i)
        for (int j = 0; j < numOpcodeIDs; ++j)
            *(currentPairIndex++) = make_pair(i, j);
    qsort(sortedPairIndices, numOpcodeIDs * numOpcodeIDs, sizeof(pair<int, int>), compareOpcodePairIndices);
    
    dataLogF("\nExecuted opcode statistics\n"); 
    
    dataLogF("Total instructions executed: %lld\n\n", totalInstructions);

    dataLogF("All opcodes by frequency:\n\n");

    for (int i = 0; i < numOpcodeIDs; ++i) {
        int index = sortedIndices[i];
        dataLogF("%s:%s %lld - %.2f%%\n", opcodeNames[index], padOpcodeName((OpcodeID)index, 28), opcodeCounts[index], ((double) opcodeCounts[index]) / ((double) totalInstructions) * 100.0);    
    }
    
    dataLogF("\n");
    dataLogF("2-opcode sequences by frequency: %lld\n\n", totalInstructions);
    
    for (int i = 0; i < numOpcodeIDs * numOpcodeIDs; ++i) {
        pair<int, int> indexPair = sortedPairIndices[i];
        long long count = opcodePairCounts[indexPair.first][indexPair.second];
        
        if (!count)
            break;
        
        dataLogF("%s%s %s:%s %lld %.2f%%\n", opcodeNames[indexPair.first], padOpcodeName((OpcodeID)indexPair.first, 28), opcodeNames[indexPair.second], padOpcodeName((OpcodeID)indexPair.second, 28), count, ((double) count) / ((double) totalInstructionPairs) * 100.0);
    }
    
    dataLogF("\n");
    dataLogF("Most common opcodes and sequences:\n");

    for (int i = 0; i < numOpcodeIDs; ++i) {
        int index = sortedIndices[i];
        long long opcodeCount = opcodeCounts[index];
        double opcodeProportion = ((double) opcodeCount) / ((double) totalInstructions);
        if (opcodeProportion < 0.0001)
            break;
        dataLogF("\n%s:%s %lld - %.2f%%\n", opcodeNames[index], padOpcodeName((OpcodeID)index, 28), opcodeCount, opcodeProportion * 100.0);

        for (int j = 0; j < numOpcodeIDs * numOpcodeIDs; ++j) {
            pair<int, int> indexPair = sortedPairIndices[j];
            long long pairCount = opcodePairCounts[indexPair.first][indexPair.second];
            double pairProportion = ((double) pairCount) / ((double) totalInstructionPairs);
        
            if (!pairCount || pairProportion < 0.0001 || pairProportion < opcodeProportion / 100)
                break;

            if (indexPair.first != index && indexPair.second != index)
                continue;

            dataLogF("    %s%s %s:%s %lld - %.2f%%\n", opcodeNames[indexPair.first], padOpcodeName((OpcodeID)indexPair.first, 28), opcodeNames[indexPair.second], padOpcodeName((OpcodeID)indexPair.second, 28), pairCount, pairProportion * 100.0);
        }
        
    }
    dataLogF("\n");
}

void OpcodeStats::recordInstruction(int opcode)
{
    opcodeCounts[opcode]++;
    
    if (lastOpcode != -1)
        opcodePairCounts[lastOpcode][opcode]++;
    
    lastOpcode = opcode;
}

void OpcodeStats::resetLastInstruction()
{
    lastOpcode = -1;
}

#endif

} // namespace JSC
