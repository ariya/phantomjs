/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "ScriptController.h"

#include "ContentSecurityPolicy.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "Page.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "UserGestureIndicator.h"
#include <wtf/text/TextPosition.h>

namespace WebCore {

bool ScriptController::canExecuteScripts(ReasonForCallingCanExecuteScripts reason)
{
    if (m_frame->document() && m_frame->document()->isSandboxed(SandboxScripts)) {
        // FIXME: This message should be moved off the console once a solution to https://bugs.webkit.org/show_bug.cgi?id=103274 exists.
        if (reason == AboutToExecuteScript)
            m_frame->document()->addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, "Blocked script execution in '" + m_frame->document()->url().stringCenterEllipsizedToLength() + "' because the document's frame is sandboxed and the 'allow-scripts' permission is not set.");
        return false;
    }

    if (m_frame->document() && m_frame->document()->isViewSource()) {
        ASSERT(m_frame->document()->securityOrigin()->isUnique());
        return true;
    }

    Settings* settings = m_frame->settings();
    const bool allowed = m_frame->loader()->client()->allowScript(settings && settings->isScriptEnabled());
    if (!allowed && reason == AboutToExecuteScript)
        m_frame->loader()->client()->didNotAllowScript();
    return allowed;
}

ScriptValue ScriptController::executeScript(const String& script, bool forceUserGesture)
{
    UserGestureIndicator gestureIndicator(forceUserGesture ? DefinitelyProcessingUserGesture : PossiblyProcessingUserGesture);
    return executeScript(ScriptSourceCode(script, m_frame->document()->url()));
}

ScriptValue ScriptController::executeScript(const ScriptSourceCode& sourceCode)
{
    if (!canExecuteScripts(AboutToExecuteScript) || isPaused())
        return ScriptValue();

    RefPtr<Frame> protect(m_frame); // Script execution can destroy the frame, and thus the ScriptController.

    return evaluate(sourceCode);
}

bool ScriptController::executeIfJavaScriptURL(const KURL& url, ShouldReplaceDocumentIfJavaScriptURL shouldReplaceDocumentIfJavaScriptURL)
{
    if (!protocolIsJavaScript(url))
        return false;

    if (!m_frame->page()
        || !m_frame->document()->contentSecurityPolicy()->allowJavaScriptURLs(m_frame->document()->url(), eventHandlerPosition().m_line))
        return true;

    // We need to hold onto the Frame here because executing script can
    // destroy the frame.
    RefPtr<Frame> protector(m_frame);
    RefPtr<Document> ownerDocument(m_frame->document());

    const int javascriptSchemeLength = sizeof("javascript:") - 1;

    String decodedURL = decodeURLEscapeSequences(url.string());
    ScriptValue result = executeScript(decodedURL.substring(javascriptSchemeLength));

    // If executing script caused this frame to be removed from the page, we
    // don't want to try to replace its document!
    if (!m_frame->page())
        return true;

    String scriptResult;
    JSDOMWindowShell* shell = windowShell(mainThreadNormalWorld());
    JSC::ExecState* exec = shell->window()->globalExec();
    if (!result.getString(exec, scriptResult))
        return true;

    // FIXME: We should always replace the document, but doing so
    //        synchronously can cause crashes:
    //        http://bugs.webkit.org/show_bug.cgi?id=16782
    if (shouldReplaceDocumentIfJavaScriptURL == ReplaceDocumentIfJavaScriptURL) {
        // We're still in a frame, so there should be a DocumentLoader.
        ASSERT(m_frame->document()->loader());
        
        // DocumentWriter::replaceDocument can cause the DocumentLoader to get deref'ed and possible destroyed,
        // so protect it with a RefPtr.
        if (RefPtr<DocumentLoader> loader = m_frame->document()->loader())
            loader->writer()->replaceDocument(scriptResult, ownerDocument.get());
    }
    return true;
}

} // namespace WebCore
