/*
* Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef WTF_ByteOrder_h
#define WTF_ByteOrder_h

#include <wtf/Platform.h>

#if OS(UNIX)
#include <arpa/inet.h>
#endif

#if OS(WINDOWS)

namespace WTF {
inline uint32_t wswap32(uint32_t x) { return ((x & 0xffff0000) >> 16) | ((x & 0x0000ffff) << 16); }
inline uint32_t bswap32(uint32_t x) { return ((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8) | ((x & 0x000000ff) << 24); }
inline uint16_t bswap16(uint16_t x) { return ((x & 0xff00) >> 8) | ((x & 0x00ff) << 8); }
} // namespace WTF

#if CPU(BIG_ENDIAN)
inline uint16_t ntohs(uint16_t x) { return x; }
inline uint16_t htons(uint16_t x) { return x; }
inline uint32_t ntohl(uint32_t x) { return x; }
inline uint32_t htonl(uint32_t x) { return x; }
#elif CPU(MIDDLE_ENDIAN)
inline uint16_t ntohs(uint16_t x) { return x; }
inline uint16_t htons(uint16_t x) { return x; }
inline uint32_t ntohl(uint32_t x) { return WTF::wswap32(x); }
inline uint32_t htonl(uint32_t x) { return WTF::wswap32(x); }
#else
inline uint16_t ntohs(uint16_t x) { return WTF::bswap16(x); }
inline uint16_t htons(uint16_t x) { return WTF::bswap16(x); }
inline uint32_t ntohl(uint32_t x) { return WTF::bswap32(x); }
inline uint32_t htonl(uint32_t x) { return WTF::bswap32(x); }
#endif

#endif // OS(WINDOWS)

#endif // WTF_ByteOrder_h
