/*
 * Copyright (C) 2013 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLParserOptions.h"

#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "ScriptController.h"
#include "Settings.h"

namespace WebCore {

HTMLParserOptions::HTMLParserOptions(Document* document)
{
    Frame* frame = document ? document->frame() : 0;
    scriptEnabled = frame && frame->script()->canExecuteScripts(NotAboutToExecuteScript);
    pluginsEnabled = frame && frame->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin);

    Settings* settings = document ? document->settings() : 0;
    usePreHTML5ParserQuirks = settings && settings->usePreHTML5ParserQuirks();
#if ENABLE(THREADED_HTML_PARSER)
    // We force the main-thread parser for about:blank, javascript: and data: urls for compatibility
    // with historical synchronous loading/parsing behavior of those schemes.
    useThreading = settings && settings->threadedHTMLParser() && !document->url().isBlankURL()
        && (settings->useThreadedHTMLParserForDataURLs() || !document->url().protocolIsData());
#else
    useThreading = false;
#endif
    maximumDOMTreeDepth = settings ? settings->maximumHTMLParserDOMTreeDepth() : Settings::defaultMaximumHTMLParserDOMTreeDepth;
}

}
