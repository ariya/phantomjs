/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef EwkViewCallbacks_h
#define EwkViewCallbacks_h

#include "WKEinaSharedString.h"
#include "ewk_private.h"
#include "ewk_view.h"
#include <Evas.h>
#include <WebKit2/WKGeometry.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

typedef struct EwkObject Ewk_Auth_Request;
typedef struct EwkObject Ewk_Download_Job;
typedef struct EwkObject Ewk_File_Chooser_Request;
typedef struct EwkObject Ewk_Form_Submission_Request;
typedef struct EwkObject Ewk_Navigation_Policy_Decision;
typedef struct EwkError Ewk_Error;

namespace EwkViewCallbacks {

enum CallbackType {
    AuthenticationRequest,
    BackForwardListChange,
    CancelVibration,
    ContentsSizeChanged,
    DownloadJobCancelled,
    DownloadJobFailed,
    DownloadJobFinished,
    DownloadJobRequested,
    FileChooserRequest,
    NewFormSubmissionRequest,
    FaviconChanged,
    LoadError,
    LoadFinished,
    LoadProgress,
    MenuBarVisible,
    ProvisionalLoadFailed,
    ProvisionalLoadRedirect,
    ProvisionalLoadStarted,
    StatusBarVisible,
    NavigationPolicyDecision,
    NewWindowPolicyDecision,
    TextFound,
    TitleChange,
    ToolbarVisible,
    TooltipTextUnset,
    TooltipTextSet,
    URLChanged,
    Vibrate,
    WebProcessCrashed,
    WindowResizable
};

template <CallbackType>
struct CallBackInfo;

class EvasObjectHolder {
protected:
    explicit EvasObjectHolder(Evas_Object* object)
        : m_object(object)
    {
        ASSERT(m_object);
    }

    Evas_Object* m_object;
};

template <CallbackType callbackType, typename ArgType = typename CallBackInfo<callbackType>::Type>
struct CallBack: public EvasObjectHolder {
    explicit CallBack(Evas_Object* view) : EvasObjectHolder(view) { }

    void call(ArgType argument)
    {
        evas_object_smart_callback_call(m_object, CallBackInfo<callbackType>::name(), static_cast<void*>(argument));
    }
};

template <CallbackType callbackType>
struct CallBack <callbackType, void> : public EvasObjectHolder {
    explicit CallBack(Evas_Object* view) : EvasObjectHolder(view) { }

    void call()
    {
        evas_object_smart_callback_call(m_object, CallBackInfo<callbackType>::name(), 0);
    }
};

template <CallbackType callbackType>
struct CallBack <callbackType, const char*> : public EvasObjectHolder {
    explicit CallBack(Evas_Object* view) : EvasObjectHolder(view) { }

    void call(const char* arg)
    {
        evas_object_smart_callback_call(m_object, CallBackInfo<callbackType>::name(), const_cast<char*>(arg));
    }

    void call(const String& arg)
    {
        call(arg.utf8().data());
    }

    void call(const WKEinaSharedString& arg)
    {
        call(static_cast<const char*>(arg));
    }
};

template <CallbackType callbackType>
struct CallBack <callbackType, Ewk_CSS_Size*> : public EvasObjectHolder {
    explicit CallBack(Evas_Object* view) : EvasObjectHolder(view) { }

    void call(Ewk_CSS_Size* size)
    {
        evas_object_smart_callback_call(m_object, CallBackInfo<callbackType>::name(), size);
    }

    void call(const WKSize& arg)
    {
        Ewk_CSS_Size size = { static_cast<Evas_Coord>(arg.width), static_cast<Evas_Coord>(arg.height) };
        call(&size);
    }
};

#define DECLARE_EWK_VIEW_CALLBACK(callbackType, string, type) \
template <>                                                   \
struct CallBackInfo<callbackType> {                           \
    typedef type Type;                                        \
    static inline const char* name() { return string; }       \
}

// Note: type 'void' means that no arguments are expected.
DECLARE_EWK_VIEW_CALLBACK(AuthenticationRequest, "authentication,request", Ewk_Auth_Request*);
DECLARE_EWK_VIEW_CALLBACK(BackForwardListChange, "back,forward,list,changed", void);
DECLARE_EWK_VIEW_CALLBACK(CancelVibration, "cancel,vibration", void);
DECLARE_EWK_VIEW_CALLBACK(ContentsSizeChanged, "contents,size,changed", Ewk_CSS_Size*);
DECLARE_EWK_VIEW_CALLBACK(DownloadJobCancelled, "download,cancelled", Ewk_Download_Job*);
DECLARE_EWK_VIEW_CALLBACK(DownloadJobFailed, "download,failed", Ewk_Download_Job_Error*);
DECLARE_EWK_VIEW_CALLBACK(DownloadJobFinished, "download,finished", Ewk_Download_Job*);
DECLARE_EWK_VIEW_CALLBACK(DownloadJobRequested, "download,request", Ewk_Download_Job*);
DECLARE_EWK_VIEW_CALLBACK(FileChooserRequest, "file,chooser,request", Ewk_File_Chooser_Request*);
DECLARE_EWK_VIEW_CALLBACK(NewFormSubmissionRequest, "form,submission,request", Ewk_Form_Submission_Request*);
DECLARE_EWK_VIEW_CALLBACK(FaviconChanged, "favicon,changed", void);
DECLARE_EWK_VIEW_CALLBACK(LoadError, "load,error", Ewk_Error*);
DECLARE_EWK_VIEW_CALLBACK(LoadFinished, "load,finished", void);
DECLARE_EWK_VIEW_CALLBACK(LoadProgress, "load,progress", double*);
DECLARE_EWK_VIEW_CALLBACK(ProvisionalLoadFailed, "load,provisional,failed", Ewk_Error*);
DECLARE_EWK_VIEW_CALLBACK(ProvisionalLoadRedirect, "load,provisional,redirect", void);
DECLARE_EWK_VIEW_CALLBACK(ProvisionalLoadStarted, "load,provisional,started", void);
DECLARE_EWK_VIEW_CALLBACK(MenuBarVisible, "menubar,visible", bool*);
DECLARE_EWK_VIEW_CALLBACK(NavigationPolicyDecision, "policy,decision,navigation", Ewk_Navigation_Policy_Decision*);
DECLARE_EWK_VIEW_CALLBACK(NewWindowPolicyDecision, "policy,decision,new,window", Ewk_Navigation_Policy_Decision*);
DECLARE_EWK_VIEW_CALLBACK(StatusBarVisible, "statusbar,visible", bool*);
DECLARE_EWK_VIEW_CALLBACK(TextFound, "text,found", unsigned*);
DECLARE_EWK_VIEW_CALLBACK(TitleChange, "title,changed", const char*);
DECLARE_EWK_VIEW_CALLBACK(ToolbarVisible, "toolbar,visible", bool*);
DECLARE_EWK_VIEW_CALLBACK(TooltipTextUnset, "tooltip,text,unset", void);
DECLARE_EWK_VIEW_CALLBACK(TooltipTextSet, "tooltip,text,set", const char*);
DECLARE_EWK_VIEW_CALLBACK(URLChanged, "url,changed", const char*);
DECLARE_EWK_VIEW_CALLBACK(Vibrate, "vibrate", uint32_t*);
DECLARE_EWK_VIEW_CALLBACK(WebProcessCrashed, "webprocess,crashed", bool*);
DECLARE_EWK_VIEW_CALLBACK(WindowResizable, "window,resizable", bool*);

}

#endif // #ifndef EwkViewCallbacks_h
