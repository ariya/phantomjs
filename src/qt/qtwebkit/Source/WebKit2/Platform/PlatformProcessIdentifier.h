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


#ifndef PlatformProcessIdentifier_h
#define PlatformProcessIdentifier_h

#if PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE
#elif PLATFORM(EFL)
#include <unistd.h>
#endif

namespace WebKit {

#if PLATFORM(MAC)
typedef pid_t PlatformProcessIdentifier;
#elif PLATFORM(QT)
typedef QProcess* PlatformProcessIdentifier;
#elif PLATFORM(GTK)
#ifdef G_OS_WIN32
typedef void* GPid;
#else
typedef int GPid;
#endif
typedef GPid PlatformProcessIdentifier;
#elif PLATFORM(EFL)
typedef pid_t PlatformProcessIdentifier;
#endif

} // namespace WebKit 

#endif // PlatformProcessIdentifier_h
