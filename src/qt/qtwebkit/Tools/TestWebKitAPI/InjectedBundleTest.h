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

#ifndef InjectedBundleTest_h
#define InjectedBundleTest_h

#include "InjectedBundleController.h"

namespace TestWebKitAPI {

class InjectedBundleTest {
public:
    virtual ~InjectedBundleTest() { }

    virtual void initialize(WKBundleRef, WKTypeRef) { }

    virtual void didCreatePage(WKBundleRef, WKBundlePageRef) { }
    virtual void willDestroyPage(WKBundleRef, WKBundlePageRef) { }
    virtual void didInitializePageGroup(WKBundleRef, WKBundlePageGroupRef) { }
    virtual void didReceiveMessage(WKBundleRef, WKStringRef messageName, WKTypeRef messageBody) { }
    virtual void didReceiveMessageToPage(WKBundleRef, WKBundlePageRef, WKStringRef messageName, WKTypeRef messageBody) { }

    std::string name() const { return m_identifier; }
    
    template<typename TestClassTy> class Register {
    public:
        Register(const std::string& test)
        {
            InjectedBundleController::shared().registerCreateInjectedBundleTestFunction(test, Register::create);
        }

    private:
        static InjectedBundleTest* create(const std::string& identifier) 
        {
            return new TestClassTy(identifier);
        }
    };

protected:
    InjectedBundleTest(const std::string& identifier)
        : m_identifier(identifier)
    {
    }

    std::string m_identifier;
};

} // namespace TestWebKitAPI

#endif // InjectedBundleTest_h
