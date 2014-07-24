/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
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

#ifndef WorkQueueItem_h
#define WorkQueueItem_h

#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSBase.h>

class WorkQueueItem {
public:
    virtual ~WorkQueueItem() { }
    virtual bool invoke() const = 0; // Returns true if this started a load.
};

class LoadItem : public WorkQueueItem {
public:
    LoadItem(const JSStringRef url, const JSStringRef target)
        : m_url(url)
        , m_target(target)
    {
    }

private:
    virtual bool invoke() const;

    JSRetainPtr<JSStringRef> m_url;
    JSRetainPtr<JSStringRef> m_target;
};

class LoadHTMLStringItem : public WorkQueueItem {
public:
    LoadHTMLStringItem(const JSStringRef content, const JSStringRef baseURL)
        : m_content(content)
        , m_baseURL(baseURL)
    {
    }

    LoadHTMLStringItem(const JSStringRef content, const JSStringRef baseURL, const JSStringRef unreachableURL)
        : m_content(content)
        , m_baseURL(baseURL)
        , m_unreachableURL(unreachableURL)
    {
    }

private:
    virtual bool invoke() const;

    JSRetainPtr<JSStringRef> m_content;
    JSRetainPtr<JSStringRef> m_baseURL;
    JSRetainPtr<JSStringRef> m_unreachableURL;
};

class ReloadItem : public WorkQueueItem {
private:
    virtual bool invoke() const;
};

class ScriptItem : public WorkQueueItem {
protected:
    ScriptItem(const JSStringRef script)
        : m_script(script)
    {
    }

protected:
    virtual bool invoke() const;

private:
    JSRetainPtr<JSStringRef> m_script;
};

class LoadingScriptItem : public ScriptItem {
public:
    LoadingScriptItem(const JSStringRef script)
        : ScriptItem(script)
    {
    }

private:
    virtual bool invoke() const { return ScriptItem::invoke(); }
};

class NonLoadingScriptItem : public ScriptItem {
public:
    NonLoadingScriptItem(const JSStringRef script)
        : ScriptItem(script)
    {
    }

private:
    virtual bool invoke() const { ScriptItem::invoke(); return false; }
};

class BackForwardItem : public WorkQueueItem {
protected:
    BackForwardItem(int howFar)
        : m_howFar(howFar)
    {
    }

private:
    virtual bool invoke() const;

    int m_howFar;
};

class BackItem : public BackForwardItem {
public:
    BackItem(unsigned howFar)
        : BackForwardItem(-howFar)
    {
    }
};

class ForwardItem : public BackForwardItem {
public:
    ForwardItem(unsigned howFar)
        : BackForwardItem(howFar)
    {
    }
};

#endif // !defined(WorkQueueItem_h)
