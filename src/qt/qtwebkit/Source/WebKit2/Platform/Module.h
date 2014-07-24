/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Module_h
#define Module_h

#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(QT)
#include <QLibrary>
#endif

#if PLATFORM(GTK)
typedef struct _GModule GModule;
#endif

#if PLATFORM(EFL)
#include <Eina.h>
#endif

namespace WebKit {

class Module {
    WTF_MAKE_NONCOPYABLE(Module);
public:
    Module(const String& path);
    ~Module();

    bool load();
    // Note: On Mac this leaks the CFBundle to avoid crashes when a bundle is unloaded and there are
    // live Objective-C objects whose methods come from that bundle.
    void unload();

#if PLATFORM(MAC)
    String bundleIdentifier() const;
#endif

    template<typename FunctionType> FunctionType functionPointer(const char* functionName) const;

#if PLATFORM(MAC) && !defined(__LP64__)
    CFBundleRefNum bundleResourceMap();
#endif

private:
    void* platformFunctionPointer(const char* functionName) const;

    String m_path;
#if PLATFORM(MAC)
    RetainPtr<CFBundleRef> m_bundle;
#if !defined(__LP64__)
    CFBundleRefNum m_bundleResourceMap;
#endif
#elif PLATFORM(QT)
    QLibrary m_lib;
#elif PLATFORM(GTK)
    GModule* m_handle;
#elif PLATFORM(EFL)
    OwnPtr<Eina_Module> m_module;
#endif
};

template<typename FunctionType> FunctionType Module::functionPointer(const char* functionName) const
{
    return reinterpret_cast<FunctionType>(platformFunctionPointer(functionName));
}

}

#endif
