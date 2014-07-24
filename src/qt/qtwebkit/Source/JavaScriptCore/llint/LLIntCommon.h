/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef LLIntCommon_h
#define LLIntCommon_h

// Print every instruction executed.
#define LLINT_EXECUTION_TRACING 0

// Print some information for some of the more subtle slow paths.
#define LLINT_SLOW_PATH_TRACING 0

// Disable inline allocation in the interpreter. This is great if you're changing
// how the GC allocates.
#define LLINT_ALWAYS_ALLOCATE_SLOW 0

// Disable inline caching of get_by_id and put_by_id.
#define LLINT_ALWAYS_ACCESS_SLOW 0

// Enable OSR into the JIT. Disabling this while the LLInt is enabled effectively
// turns off all JIT'ing, since in LLInt's parlance, OSR subsumes any form of JIT
// invocation.
#if ENABLE(JIT)
#define LLINT_OSR_TO_JIT 1
#else
#define LLINT_OSR_TO_JIT 0
#endif

#endif // LLIntCommon_h

