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

#import "config.h"
#import "Module.h"

namespace WebKit {

bool Module::load()
{
    RetainPtr<CFURLRef> bundleURL = adoptCF(CFURLCreateWithFileSystemPath(kCFAllocatorDefault, m_path.createCFString().get(), kCFURLPOSIXPathStyle, FALSE));
    if (!bundleURL)
        return false;

    RetainPtr<CFBundleRef> bundle = adoptCF(CFBundleCreate(kCFAllocatorDefault, bundleURL.get()));
    if (!bundle)
        return false;

    if (!CFBundleLoadExecutable(bundle.get()))
        return false;

    m_bundle = adoptCF(bundle.leakRef());
    return true;
}

void Module::unload()
{
    if (!m_bundle)
        return;

#if !defined(__LP64__)
    if (m_bundleResourceMap != -1)
        CFBundleCloseBundleResourceMap(m_bundle.get(), m_bundleResourceMap);
#endif

    // See the comment in Module.h for why we leak the bundle here.
    CFBundleRef unused = m_bundle.leakRef();
    (void)unused;
}

void* Module::platformFunctionPointer(const char* functionName) const
{
    if (!m_bundle)
        return 0;
    RetainPtr<CFStringRef> functionNameString = adoptCF(CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, functionName, kCFStringEncodingASCII, kCFAllocatorNull));
    return CFBundleGetFunctionPointerForName(m_bundle.get(), functionNameString.get());
}

String Module::bundleIdentifier() const
{
    return CFBundleGetIdentifier(m_bundle.get());
}

#if !defined(__LP64__)
CFBundleRefNum Module::bundleResourceMap()
{
    if (m_bundleResourceMap == -1)
        m_bundleResourceMap = CFBundleOpenBundleResourceMap(m_bundle.get());

    return m_bundleResourceMap;
}
#endif

}
