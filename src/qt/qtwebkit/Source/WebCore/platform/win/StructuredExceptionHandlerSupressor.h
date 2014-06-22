/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef StructuredExceptionHandlerSupressor_h
#define StructuredExceptionHandlerSupressor_h

namespace WebCore {

#pragma warning(push)
#pragma warning(disable: 4733) // Disable "not registered as safe handler" warning

class StructuredExceptionHandlerSupressor {
    WTF_MAKE_NONCOPYABLE(StructuredExceptionHandlerSupressor);
public:
    StructuredExceptionHandlerSupressor()
    {
        // Windows puts an __try/__except block around some calls, such as hooks.
        // The exception handler then ignores system exceptions like invalid addresses
        // and null pointers. This class can be used to remove this block and prevent
        // it from catching the exception. Typically this will cause the exception to crash 
        // which is often desirable to allow crashlogs to be recorded for debugging purposed.
        // While this class is in scope we replace the Windows exception handler with 0xffffffff, 
        // which indicates that the exception should not be handled.
        //
        // See http://www.microsoft.com/msj/0197/Exception/Exception.aspx

        // Windows doesn't like assigning to member variables, so we need to get the value into
        // a local variable and store it afterwards.
        void* registration;

        __asm mov eax, FS:[0]
        __asm mov [registration], eax
        __asm mov eax, 0xffffffff
        __asm mov FS:[0], eax

        m_savedExceptionRegistration = registration;
    }

    ~StructuredExceptionHandlerSupressor()
    {
        // Restore the exception handler
        __asm mov eax, [m_savedExceptionRegistration]
        __asm mov FS:[0], eax
    }

private:
    void* m_savedExceptionRegistration;
};

#pragma warning(pop)

} // namespace WebCore

#endif // StructuredExceptionHandlerSupressor_h
