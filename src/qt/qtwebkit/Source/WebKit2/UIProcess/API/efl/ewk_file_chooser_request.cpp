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

#include "config.h"
#include "ewk_file_chooser_request.h"

#include "WKArray.h"
#include "WKOpenPanelParameters.h"
#include "WKOpenPanelResultListener.h"
#include "WKSharedAPICast.h"
#include "WKString.h"
#include "WKURL.h"
#include "ewk_file_chooser_request_private.h"
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

EwkFileChooserRequest::EwkFileChooserRequest(WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener)
    : m_parameters(parameters)
    , m_listener(listener)
    , m_wasRequestHandled(false)
{
    ASSERT(parameters);
    ASSERT(listener);
}

EwkFileChooserRequest::~EwkFileChooserRequest()
{
    if (!m_wasRequestHandled)
        WKOpenPanelResultListenerCancel(m_listener.get());
}

bool EwkFileChooserRequest::allowMultipleFiles() const
{
    return WKOpenPanelParametersGetAllowsMultipleFiles(m_parameters.get());
}

WKRetainPtr<WKArrayRef> EwkFileChooserRequest::acceptedMIMETypes() const
{
    return adoptWK(WKOpenPanelParametersCopyAcceptedMIMETypes(m_parameters.get()));
}

void EwkFileChooserRequest::cancel()
{
    m_wasRequestHandled = true;

    return WKOpenPanelResultListenerCancel(m_listener.get());
}

void EwkFileChooserRequest::chooseFiles(WKArrayRef fileURLs)
{
    m_wasRequestHandled = true;
    WKOpenPanelResultListenerChooseFiles(m_listener.get(), fileURLs);
}

Eina_Bool ewk_file_chooser_request_allow_multiple_files_get(const Ewk_File_Chooser_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkFileChooserRequest, request, impl, false);

    return impl->allowMultipleFiles();
}

Eina_List* ewk_file_chooser_request_accepted_mimetypes_get(const Ewk_File_Chooser_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkFileChooserRequest, request, impl, 0);

    Eina_List* mimeTypeList = 0;
    WKRetainPtr<WKArrayRef> mimeTypes = impl->acceptedMIMETypes();

    const size_t size = WKArrayGetSize(mimeTypes.get());
    for (size_t i = 0; i < size; ++i) {
        WKRetainPtr<WKStringRef> mimeType = static_cast<WKStringRef>(WKArrayGetItemAtIndex(mimeTypes.get(), i));
        if (!mimeType || WKStringIsEmpty(mimeType.get()))
            continue;
        mimeTypeList = eina_list_append(mimeTypeList, eina_stringshare_add(toWTFString(mimeType.get()).utf8().data()));
    }

    return mimeTypeList;
}

Eina_Bool ewk_file_chooser_request_cancel(Ewk_File_Chooser_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkFileChooserRequest, request, impl, false);
    EINA_SAFETY_ON_TRUE_RETURN_VAL(impl->wasHandled(), false);

    impl->cancel();

    return true;
}

Eina_Bool ewk_file_chooser_request_files_choose(Ewk_File_Chooser_Request* request, const Eina_List* chosenFiles)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkFileChooserRequest, request, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(chosenFiles, false);
    EINA_SAFETY_ON_TRUE_RETURN_VAL(impl->wasHandled(), false);

    const unsigned urlCount = eina_list_count(chosenFiles);
    EINA_SAFETY_ON_FALSE_RETURN_VAL(urlCount == 1 || (urlCount > 1 && impl->allowMultipleFiles()), false);

    OwnArrayPtr<WKTypeRef> filesURLs = adoptArrayPtr(new WKTypeRef[urlCount]);

    for (unsigned i = 0; i < urlCount; ++i) {
        const char* url = static_cast<char*>(eina_list_nth(chosenFiles, i));
        EINA_SAFETY_ON_NULL_RETURN_VAL(url, false);
        String fileURL = ASCIILiteral("file://") + String::fromUTF8(url);
        filesURLs[i] = toCopiedURLAPI(fileURL);
    }

    WKRetainPtr<WKArrayRef> wkFileURLs(AdoptWK, WKArrayCreateAdoptingValues(filesURLs.get(), urlCount));
    impl->chooseFiles(wkFileURLs.get());

    return true;
}

Eina_Bool ewk_file_chooser_request_file_choose(Ewk_File_Chooser_Request* request, const char* chosenFile)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkFileChooserRequest, request, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(chosenFile, false);
    EINA_SAFETY_ON_TRUE_RETURN_VAL(impl->wasHandled(), false);

    String fileURL = ASCIILiteral("file://") + String::fromUTF8(chosenFile);
    WKRetainPtr<WKURLRef> wkURL(AdoptWK, toCopiedURLAPI(fileURL));

    WKTypeRef wkURLPtr = wkURL.get();
    WKRetainPtr<WKArrayRef> wkFileURLs(AdoptWK, WKArrayCreate(&wkURLPtr, 1));
    impl->chooseFiles(wkFileURLs.get());

    return true;
}
