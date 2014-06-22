/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef ProfilerDatabase_h
#define ProfilerDatabase_h

#include "JSCJSValue.h"
#include "ProfilerBytecodes.h"
#include "ProfilerCompilation.h"
#include "ProfilerCompilationKind.h"
#include <wtf/FastAllocBase.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/SegmentedVector.h>
#include <wtf/text/WTFString.h>

namespace JSC { namespace Profiler {

class Database {
    WTF_MAKE_FAST_ALLOCATED; WTF_MAKE_NONCOPYABLE(Database);
public:
    JS_EXPORT_PRIVATE Database(VM&);
    JS_EXPORT_PRIVATE ~Database();
    
    int databaseID() const { return m_databaseID; }
    
    Bytecodes* ensureBytecodesFor(CodeBlock*);
    void notifyDestruction(CodeBlock*);
    
    PassRefPtr<Compilation> newCompilation(CodeBlock*, CompilationKind);
    PassRefPtr<Compilation> newCompilation(Bytecodes*, CompilationKind);
    
    // Converts the database to a JavaScript object that is suitable for JSON stringification.
    // Note that it's probably a good idea to use an ExecState* associated with a global
    // object that is "clean" - i.e. array and object prototypes haven't had strange things
    // done to them. And yes, it should be appropriate to just use a globalExec here.
    JS_EXPORT_PRIVATE JSValue toJS(ExecState*) const;
    
    // Converts the database to a JavaScript object using a private temporary global object,
    // and then returns the JSON representation of that object.
    JS_EXPORT_PRIVATE String toJSON() const;
    
    // Saves the JSON representation (from toJSON()) to the given file. Returns false if the
    // save failed.
    JS_EXPORT_PRIVATE bool save(const char* filename) const;

    void registerToSaveAtExit(const char* filename);
    
private:

    void addDatabaseToAtExit();
    void removeDatabaseFromAtExit();
    void performAtExitSave() const;
    static Database* removeFirstAtExitDatabase();
    static void atExitCallback();
    
    int m_databaseID;
    VM& m_vm;
    SegmentedVector<Bytecodes> m_bytecodes;
    HashMap<CodeBlock*, Bytecodes*> m_bytecodesMap;
    Vector<RefPtr<Compilation> > m_compilations;
    bool m_shouldSaveAtExit;
    CString m_atExitSaveFilename;
    Database* m_nextRegisteredDatabase;
};

} } // namespace JSC::Profiler

#endif // ProfilerDatabase_h

