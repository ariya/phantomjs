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

#ifndef SandboxExtension_h
#define SandboxExtension_h

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

#if ENABLE(WEB_PROCESS_SANDBOX)
typedef struct __WKSandboxExtension* WKSandboxExtensionRef;
#endif

namespace CoreIPC {
    class ArgumentEncoder;
    class ArgumentDecoder;
}

namespace WebKit {

class SandboxExtension : public RefCounted<SandboxExtension> {
public:
    enum Type {
        ReadOnly,
        ReadWrite
    };

    class Handle {
        WTF_MAKE_NONCOPYABLE(Handle);
    
    public:
        Handle();
        ~Handle();

        void encode(CoreIPC::ArgumentEncoder&) const;
        static bool decode(CoreIPC::ArgumentDecoder&, Handle&);

    private:
        friend class SandboxExtension;
#if ENABLE(WEB_PROCESS_SANDBOX)
        mutable WKSandboxExtensionRef m_sandboxExtension;
#endif
    };

    class HandleArray {
        WTF_MAKE_NONCOPYABLE(HandleArray);
        
    public:
        HandleArray();
        ~HandleArray();
        void allocate(size_t);
        Handle& operator[](size_t i);
        const Handle& operator[](size_t i) const;
        size_t size() const;
        void encode(CoreIPC::ArgumentEncoder&) const;
        static bool decode(CoreIPC::ArgumentDecoder&, HandleArray&);
       
    private:
#if ENABLE(WEB_PROCESS_SANDBOX)
        Handle* m_data;
        size_t m_size;
#else
        Handle m_emptyHandle;
#endif
    };
    
    static PassRefPtr<SandboxExtension> create(const Handle&);
    static void createHandle(const String& path, Type type, Handle&);
    static void createHandleForReadWriteDirectory(const String& path, Handle&); // Will attempt to create the directory.
    static String createHandleForTemporaryFile(const String& prefix, Type type, Handle&);
    ~SandboxExtension();

    bool consume();
    bool revoke();

    bool consumePermanently();
    static bool consumePermanently(const Handle&);

private:
    explicit SandboxExtension(const Handle&);
                     
#if ENABLE(WEB_PROCESS_SANDBOX)
    mutable WKSandboxExtensionRef m_sandboxExtension;
    size_t m_useCount;
#endif
};

#if !ENABLE(WEB_PROCESS_SANDBOX)
inline SandboxExtension::Handle::Handle() { }
inline SandboxExtension::Handle::~Handle() { }
inline void SandboxExtension::Handle::encode(CoreIPC::ArgumentEncoder&) const { }
inline bool SandboxExtension::Handle::decode(CoreIPC::ArgumentDecoder&, Handle&) { return true; }
inline SandboxExtension::HandleArray::HandleArray() { }
inline SandboxExtension::HandleArray::~HandleArray() { }
inline void SandboxExtension::HandleArray::allocate(size_t) { }
inline size_t SandboxExtension::HandleArray::size() const { return 0; }    
inline const SandboxExtension::Handle& SandboxExtension::HandleArray::operator[](size_t) const { return m_emptyHandle; }
inline SandboxExtension::Handle& SandboxExtension::HandleArray::operator[](size_t) { return m_emptyHandle; }
inline void SandboxExtension::HandleArray::encode(CoreIPC::ArgumentEncoder&) const { }
inline bool SandboxExtension::HandleArray::decode(CoreIPC::ArgumentDecoder&, HandleArray&) { return true; }
inline PassRefPtr<SandboxExtension> SandboxExtension::create(const Handle&) { return 0; }
inline void SandboxExtension::createHandle(const String&, Type, Handle&) { }
inline void SandboxExtension::createHandleForReadWriteDirectory(const String&, Handle&) { }
inline String SandboxExtension::createHandleForTemporaryFile(const String& /*prefix*/, Type, Handle&) {return String();}
inline SandboxExtension::~SandboxExtension() { }
inline bool SandboxExtension::revoke() { return true; }
inline bool SandboxExtension::consume() { return true; }
inline bool SandboxExtension::consumePermanently() { return true; }
inline bool SandboxExtension::consumePermanently(const Handle&) { return true; }
#endif

} // namespace WebKit


#endif // SandboxExtension_h
