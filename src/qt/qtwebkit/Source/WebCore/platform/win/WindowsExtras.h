/*
 * Copyright (C) 2012 Patrick Gansterer <paroga@paroga.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WindowsExtras_h
#define WindowsExtras_h

#include <windows.h>
#include <objbase.h>
#include <shlwapi.h>

namespace WebCore {

#ifndef HWND_MESSAGE
const HWND HWND_MESSAGE = 0;
#endif

inline HRESULT getRegistryValue(HKEY hkey, LPCWSTR pszSubKey, LPCWSTR pszValue, LPDWORD pdwType, LPVOID pvData, LPDWORD pcbData)
{
#if OS(WINCE)
    HKEY key;
    if (::RegOpenKeyExW(hkey, pszSubKey, 0, 0, &key) != ERROR_SUCCESS)
        return ERROR_INVALID_NAME;
    HRESULT result = ::RegQueryValueExW(key, pszValue, 0, pdwType, static_cast<LPBYTE>(pvData), pcbData);
    ::RegCloseKey(key);
    return result;
#else
    return ::SHGetValueW(hkey, pszSubKey, pszValue, pdwType, pvData, pcbData);
#endif
}

inline void* getWindowPointer(HWND hWnd, int index)
{
#if OS(WINCE)
    return reinterpret_cast<void*>(::GetWindowLong(hWnd, index));
#else
    return reinterpret_cast<void*>(::GetWindowLongPtr(hWnd, index));
#endif
}

inline void* setWindowPointer(HWND hWnd, int index, void* value)
{
#if OS(WINCE)
    return reinterpret_cast<void*>(::SetWindowLong(hWnd, index, reinterpret_cast<LONG>(value)));
#else
    return reinterpret_cast<void*>(::SetWindowLongPtr(hWnd, index, reinterpret_cast<LONG_PTR>(value)));
#endif
}

} // namespace WebCore

#endif // WindowsExtras_h
