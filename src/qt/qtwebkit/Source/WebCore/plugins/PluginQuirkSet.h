/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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

#ifndef PluginQuirkSet_h
#define PluginQuirkSet_h


namespace WebCore {

    enum PluginQuirk {
        PluginQuirkWantsMozillaUserAgent = 1 << 0,
        PluginQuirkDeferFirstSetWindowCall = 1 << 1,
        PluginQuirkThrottleInvalidate = 1 << 2, 
        PluginQuirkRemoveWindowlessVideoParam = 1 << 3,
        PluginQuirkThrottleWMUserPlusOneMessages = 1 << 4,
        PluginQuirkDontUnloadPlugin = 1 << 5,
        PluginQuirkDontCallWndProcForSameMessageRecursively = 1 << 6,
        PluginQuirkHasModalMessageLoop = 1 << 7,
        PluginQuirkFlashURLNotifyBug = 1 << 8,
        PluginQuirkDontClipToZeroRectWhenScrolling = 1 << 9,
        PluginQuirkDontSetNullWindowHandleOnDestroy = 1 << 10,
        PluginQuirkDontAllowMultipleInstances = 1 << 11,
        PluginQuirkRequiresGtkToolKit = 1 << 12,
        PluginQuirkRequiresDefaultScreenDepth = 1 << 13,
        PluginQuirkDontCallSetWindowMoreThanOnce = 1 << 14,
        PluginQuirkIgnoreRightClickInWindowlessMode = 1 << 15,
        PluginQuirkWantsChromeUserAgent = 1 << 16
    };

    class PluginQuirkSet {
        public:
            PluginQuirkSet() : m_quirks(0) { }
            void add(PluginQuirk quirk) { m_quirks |= quirk; }
            bool contains(PluginQuirk quirk) const { return m_quirks & quirk; }
        private:
            unsigned m_quirks;
    };

} // namespace WebCore

#endif // PluginQuirkSet_h
