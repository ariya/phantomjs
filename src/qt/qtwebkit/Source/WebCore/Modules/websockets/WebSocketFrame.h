/*
 * Copyright (C) 2012 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebSocketFrame_h
#define WebSocketFrame_h

#if ENABLE(WEB_SOCKETS)

#include <wtf/text/WTFString.h>

namespace WebCore {

struct WebSocketFrame {
    // RFC6455 opcodes.
    enum OpCode {
        OpCodeContinuation = 0x0,
        OpCodeText = 0x1,
        OpCodeBinary = 0x2,
        OpCodeClose = 0x8,
        OpCodePing = 0x9,
        OpCodePong = 0xA,
        OpCodeInvalid = 0x10
    };

    enum ParseFrameResult {
        FrameOK,
        FrameIncomplete,
        FrameError
    };

    static bool isNonControlOpCode(OpCode opCode) { return opCode == OpCodeContinuation || opCode == OpCodeText || opCode == OpCodeBinary; }
    static bool isControlOpCode(OpCode opCode) { return opCode == OpCodeClose || opCode == OpCodePing || opCode == OpCodePong; }
    static bool isReservedOpCode(OpCode opCode) { return !isNonControlOpCode(opCode) && !isControlOpCode(opCode); }
    static bool needsExtendedLengthField(size_t payloadLength);
    static ParseFrameResult parseFrame(char* data, size_t dataLength, WebSocketFrame&, const char*& frameEnd, String& errorString); // May modify part of data to unmask the frame.

    WebSocketFrame(OpCode = OpCodeInvalid, bool final = false, bool compress = false, bool masked = false, const char* payload = 0, size_t payloadLength = 0);
    void makeFrameData(Vector<char>& frameData);

    OpCode opCode;
    bool final;
    bool compress;
    bool reserved2;
    bool reserved3;
    bool masked;
    const char* payload;
    size_t payloadLength;
};

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketFrame_h
