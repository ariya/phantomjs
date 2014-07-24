/*
 * Copyright (C) 2005, 2006, 2007, 2012 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "FormDataStreamCFNet.h"

#include "BlobData.h"
#include "FileSystem.h"
#include "FormData.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/MainThread.h>
#include <wtf/RetainPtr.h>
#include <wtf/SchedulePair.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Threading.h>

#if PLATFORM(IOS)
#include <MacErrors.h>
#elif PLATFORM(MAC)
#include <CoreServices/CoreServices.h>
#endif

#if PLATFORM(MAC)
extern "C" void CFURLRequestSetHTTPRequestBody(CFMutableURLRequestRef mutableHTTPRequest, CFDataRef httpBody);
extern "C" void CFURLRequestSetHTTPHeaderFieldValue(CFMutableURLRequestRef mutableHTTPRequest, CFStringRef httpHeaderField, CFStringRef httpHeaderFieldValue);
extern "C" void CFURLRequestSetHTTPRequestBodyStream(CFMutableURLRequestRef req, CFReadStreamRef bodyStream);
#elif PLATFORM(WIN)
#include <CFNetwork/CFURLRequest.h>
#endif

typedef struct {
    CFIndex version; /* == 1 */
    void *(*create)(CFReadStreamRef stream, void *info);
    void (*finalize)(CFReadStreamRef stream, void *info);
    CFStringRef (*copyDescription)(CFReadStreamRef stream, void *info);
    Boolean (*open)(CFReadStreamRef stream, CFStreamError *error, Boolean *openComplete, void *info);
    Boolean (*openCompleted)(CFReadStreamRef stream, CFStreamError *error, void *info);
    CFIndex (*read)(CFReadStreamRef stream, UInt8 *buffer, CFIndex bufferLength, CFStreamError *error, Boolean *atEOF, void *info);
    const UInt8 *(*getBuffer)(CFReadStreamRef stream, CFIndex maxBytesToRead, CFIndex *numBytesRead, CFStreamError *error, Boolean *atEOF, void *info);
    Boolean (*canRead)(CFReadStreamRef stream, void *info);
    void (*close)(CFReadStreamRef stream, void *info);
    CFTypeRef (*copyProperty)(CFReadStreamRef stream, CFStringRef propertyName, void *info);
    Boolean (*setProperty)(CFReadStreamRef stream, CFStringRef propertyName, CFTypeRef propertyValue, void *info);
    void (*requestEvents)(CFReadStreamRef stream, CFOptionFlags streamEvents, void *info);
    void (*schedule)(CFReadStreamRef stream, CFRunLoopRef runLoop, CFStringRef runLoopMode, void *info);
    void (*unschedule)(CFReadStreamRef stream, CFRunLoopRef runLoop, CFStringRef runLoopMode, void *info);
} CFReadStreamCallBacksV1;

#if PLATFORM(WIN)
#define EXTERN extern "C" __declspec(dllimport)
#else
#define EXTERN extern "C"
#endif

EXTERN void CFReadStreamSignalEvent(CFReadStreamRef stream, CFStreamEventType event, const void *error);
EXTERN CFReadStreamRef CFReadStreamCreate(CFAllocatorRef alloc, const void *callbacks, void *info);

namespace WebCore {

static void formEventCallback(CFReadStreamRef stream, CFStreamEventType type, void* context);

static CFStringRef formDataPointerPropertyName = CFSTR("WebKitFormDataPointer");

CFStringRef formDataStreamLengthPropertyName()
{
    return CFSTR("WebKitFormDataStreamLength");
}

struct FormCreationContext {
    RefPtr<FormData> formData;
    unsigned long long streamLength;
};

struct FormStreamFields {
    RefPtr<FormData> formData;
    SchedulePairHashSet scheduledRunLoopPairs;
    Vector<FormDataElement> remainingElements; // in reverse order
    CFReadStreamRef currentStream;
#if ENABLE(BLOB)
    long long currentStreamRangeLength;
#endif
    char* currentData;
    CFReadStreamRef formStream;
    unsigned long long streamLength;
    unsigned long long bytesSent;
};

static void closeCurrentStream(FormStreamFields* form)
{
    if (form->currentStream) {
        CFReadStreamClose(form->currentStream);
        CFReadStreamSetClient(form->currentStream, kCFStreamEventNone, 0, 0);
        CFRelease(form->currentStream);
        form->currentStream = 0;
#if ENABLE(BLOB)
        form->currentStreamRangeLength = BlobDataItem::toEndOfFile;
#endif
    }
    if (form->currentData) {
        fastFree(form->currentData);
        form->currentData = 0;
    }
}

// Return false if we cannot advance the stream. Currently the only possible failure is that the underlying file has been removed or changed since File.slice.
static bool advanceCurrentStream(FormStreamFields* form)
{
    closeCurrentStream(form);

    if (form->remainingElements.isEmpty())
        return true;

    // Create the new stream.
    FormDataElement& nextInput = form->remainingElements.last();

    if (nextInput.m_type == FormDataElement::data) {
        size_t size = nextInput.m_data.size();
        char* data = nextInput.m_data.releaseBuffer();
        form->currentStream = CFReadStreamCreateWithBytesNoCopy(0, reinterpret_cast<const UInt8*>(data), size, kCFAllocatorNull);
        form->currentData = data;
    } else {
#if ENABLE(BLOB)
        // Check if the file has been changed or not if required.
        if (isValidFileTime(nextInput.m_expectedFileModificationTime)) {
            time_t fileModificationTime;
            if (!getFileModificationTime(nextInput.m_filename, fileModificationTime) || fileModificationTime != static_cast<time_t>(nextInput.m_expectedFileModificationTime))
                return false;
        }
#endif
        const String& path = nextInput.m_shouldGenerateFile ? nextInput.m_generatedFilename : nextInput.m_filename;
        form->currentStream = CFReadStreamCreateWithFile(0, pathAsURL(path).get());
        if (!form->currentStream) {
            // The file must have been removed or become unreadable.
            return false;
        }
#if ENABLE(BLOB)
        if (nextInput.m_fileStart > 0) {
            RetainPtr<CFNumberRef> position = adoptCF(CFNumberCreate(0, kCFNumberLongLongType, &nextInput.m_fileStart));
            CFReadStreamSetProperty(form->currentStream, kCFStreamPropertyFileCurrentOffset, position.get());
        }
        form->currentStreamRangeLength = nextInput.m_fileLength;
#endif
    }
    form->remainingElements.removeLast();

    // Set up the callback.
    CFStreamClientContext context = { 0, form, 0, 0, 0 };
    CFReadStreamSetClient(form->currentStream, kCFStreamEventHasBytesAvailable | kCFStreamEventErrorOccurred | kCFStreamEventEndEncountered,
        formEventCallback, &context);

    // Schedule with the current set of run loops.
    SchedulePairHashSet::iterator end = form->scheduledRunLoopPairs.end();
    for (SchedulePairHashSet::iterator it = form->scheduledRunLoopPairs.begin(); it != end; ++it)
        CFReadStreamScheduleWithRunLoop(form->currentStream, (*it)->runLoop(), (*it)->mode());

    return true;
}

static bool openNextStream(FormStreamFields* form)
{
    // Skip over any streams we can't open.
    if (!advanceCurrentStream(form))
        return false;
    while (form->currentStream && !CFReadStreamOpen(form->currentStream)) {
        if (!advanceCurrentStream(form))
            return false;
    }
    return true;
}

static void* formCreate(CFReadStreamRef stream, void* context)
{
    FormCreationContext* formContext = static_cast<FormCreationContext*>(context);

    FormStreamFields* newInfo = new FormStreamFields;
    newInfo->formData = formContext->formData.release();
    newInfo->currentStream = 0;
#if ENABLE(BLOB)
    newInfo->currentStreamRangeLength = BlobDataItem::toEndOfFile;
#endif
    newInfo->currentData = 0;
    newInfo->formStream = stream; // Don't retain. That would create a reference cycle.
    newInfo->streamLength = formContext->streamLength;
    newInfo->bytesSent = 0;

    // Append in reverse order since we remove elements from the end.
    size_t size = newInfo->formData->elements().size();
    newInfo->remainingElements.reserveInitialCapacity(size);
    for (size_t i = 0; i < size; ++i)
        newInfo->remainingElements.append(newInfo->formData->elements()[size - i - 1]);

    return newInfo;
}

static void formFinishFinalizationOnMainThread(void* context)
{
    OwnPtr<FormStreamFields> form = adoptPtr(static_cast<FormStreamFields*>(context));

    closeCurrentStream(form.get());
}

static void formFinalize(CFReadStreamRef stream, void* context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);
    ASSERT_UNUSED(stream, form->formStream == stream);

    callOnMainThread(formFinishFinalizationOnMainThread, form);
}

static Boolean formOpen(CFReadStreamRef, CFStreamError* error, Boolean* openComplete, void* context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);

    bool opened = openNextStream(form);

    *openComplete = opened;
    error->error = opened ? 0 :
#if PLATFORM(WIN)
        ENOENT;
#else
        fnfErr;
#endif
    return opened;
}

static CFIndex formRead(CFReadStreamRef, UInt8* buffer, CFIndex bufferLength, CFStreamError* error, Boolean* atEOF, void* context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);

    while (form->currentStream) {
        CFIndex bytesToRead = bufferLength;
#if ENABLE(BLOB)
        if (form->currentStreamRangeLength != BlobDataItem::toEndOfFile && form->currentStreamRangeLength < bytesToRead)
            bytesToRead = static_cast<CFIndex>(form->currentStreamRangeLength);
#endif
        CFIndex bytesRead = CFReadStreamRead(form->currentStream, buffer, bytesToRead);
        if (bytesRead < 0) {
            *error = CFReadStreamGetError(form->currentStream);
            return -1;
        }
        if (bytesRead > 0) {
            error->error = 0;
            *atEOF = FALSE;
            form->bytesSent += bytesRead;
#if ENABLE(BLOB)
            if (form->currentStreamRangeLength != BlobDataItem::toEndOfFile)
                form->currentStreamRangeLength -= bytesRead;
#endif

            return bytesRead;
        }
        openNextStream(form);
    }

    error->error = 0;
    *atEOF = TRUE;
    return 0;
}

static Boolean formCanRead(CFReadStreamRef stream, void* context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);

    while (form->currentStream && CFReadStreamGetStatus(form->currentStream) == kCFStreamStatusAtEnd)
        openNextStream(form);

    if (!form->currentStream) {
        CFReadStreamSignalEvent(stream, kCFStreamEventEndEncountered, 0);
        return FALSE;
    }
    return CFReadStreamHasBytesAvailable(form->currentStream);
}

static void formClose(CFReadStreamRef, void* context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);

    closeCurrentStream(form);
}

static CFTypeRef formCopyProperty(CFReadStreamRef, CFStringRef propertyName, void *context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);

    if (kCFCompareEqualTo == CFStringCompare(propertyName, formDataPointerPropertyName, 0)) {
        long formDataAsNumber = static_cast<long>(reinterpret_cast<intptr_t>(form->formData.get()));
        return CFNumberCreate(0, kCFNumberLongType, &formDataAsNumber);
    }

    if (kCFCompareEqualTo == CFStringCompare(propertyName, formDataStreamLengthPropertyName(), 0))
        return CFStringCreateWithFormat(0, 0, CFSTR("%llu"), form->streamLength);

    return 0;
}

static void formSchedule(CFReadStreamRef, CFRunLoopRef runLoop, CFStringRef runLoopMode, void* context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);

    if (form->currentStream)
        CFReadStreamScheduleWithRunLoop(form->currentStream, runLoop, runLoopMode);
    form->scheduledRunLoopPairs.add(SchedulePair::create(runLoop, runLoopMode));
}

static void formUnschedule(CFReadStreamRef, CFRunLoopRef runLoop, CFStringRef runLoopMode, void* context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);

    if (form->currentStream)
        CFReadStreamUnscheduleFromRunLoop(form->currentStream, runLoop, runLoopMode);
    form->scheduledRunLoopPairs.remove(SchedulePair::create(runLoop, runLoopMode));
}

static void formEventCallback(CFReadStreamRef stream, CFStreamEventType type, void* context)
{
    FormStreamFields* form = static_cast<FormStreamFields*>(context);

    switch (type) {
    case kCFStreamEventHasBytesAvailable:
        CFReadStreamSignalEvent(form->formStream, kCFStreamEventHasBytesAvailable, 0);
        break;
    case kCFStreamEventErrorOccurred: {
        CFStreamError readStreamError = CFReadStreamGetError(stream);
        CFReadStreamSignalEvent(form->formStream, kCFStreamEventErrorOccurred, &readStreamError);
        break;
    }
    case kCFStreamEventEndEncountered:
        openNextStream(form);
        if (!form->currentStream)
            CFReadStreamSignalEvent(form->formStream, kCFStreamEventEndEncountered, 0);
        break;
    case kCFStreamEventNone:
        LOG_ERROR("unexpected kCFStreamEventNone");
        break;
    case kCFStreamEventOpenCompleted:
        LOG_ERROR("unexpected kCFStreamEventOpenCompleted");
        break;
    case kCFStreamEventCanAcceptBytes:
        LOG_ERROR("unexpected kCFStreamEventCanAcceptBytes");
        break;
    }
}

void setHTTPBody(CFMutableURLRequestRef request, PassRefPtr<FormData> prpFormData)
{
    RefPtr<FormData> formData = prpFormData;

    if (!formData)
        return;
        
    size_t count = formData->elements().size();

    // Handle the common special case of one piece of form data, with no files.
    if (count == 1 && !formData->alwaysStream()) {
        const FormDataElement& element = formData->elements()[0];
        if (element.m_type == FormDataElement::data) {
            RetainPtr<CFDataRef> data = adoptCF(CFDataCreate(0, reinterpret_cast<const UInt8 *>(element.m_data.data()), element.m_data.size()));
            CFURLRequestSetHTTPRequestBody(request, data.get());
            return;
        }
    }

#if ENABLE(BLOB)
    formData = formData->resolveBlobReferences();
    count = formData->elements().size();
#endif

    // Precompute the content length so NSURLConnection doesn't use chunked mode.
    unsigned long long length = 0;
    for (size_t i = 0; i < count; ++i) {
        const FormDataElement& element = formData->elements()[i];
        if (element.m_type == FormDataElement::data)
            length += element.m_data.size();
        else {
#if ENABLE(BLOB)
            // If we're sending the file range, use the existing range length for now. We will detect if the file has been changed right before we read the file and abort the operation if necessary.
            if (element.m_fileLength != BlobDataItem::toEndOfFile) {
                length += element.m_fileLength;
                continue;
            }
#endif
            long long fileSize;
            if (getFileSize(element.m_shouldGenerateFile ? element.m_generatedFilename : element.m_filename, fileSize))
                length += fileSize;
        }
    }

    // Create and set the stream.

    // Pass the length along with the formData so it does not have to be recomputed.
    FormCreationContext formContext = { formData.release(), length };

    CFReadStreamCallBacksV1 callBacks = { 1, formCreate, formFinalize, 0, formOpen, 0, formRead, 0, formCanRead, formClose, formCopyProperty, 0, 0, formSchedule, formUnschedule
    };
    RetainPtr<CFReadStreamRef> stream = adoptCF(CFReadStreamCreate(0, static_cast<const void*>(&callBacks), &formContext));

    CFURLRequestSetHTTPRequestBodyStream(request, stream.get());
}

FormData* httpBodyFromStream(CFReadStreamRef stream)
{
    if (!stream)
        return 0;

    // Passing the pointer as property appears to be the only way to associate a stream with FormData.
    // A new stream is always created in CFURLRequestCopyHTTPRequestBodyStream (or -[NSURLRequest HTTPBodyStream]),
    // so a side HashMap wouldn't work.
    // Even the stream's context pointer is different from the one we returned from formCreate().

    RetainPtr<CFNumberRef> formDataPointerAsCFNumber = adoptCF(static_cast<CFNumberRef>(CFReadStreamCopyProperty(stream, formDataPointerPropertyName)));
    if (!formDataPointerAsCFNumber)
        return 0;

    long formDataPointerAsNumber;
    if (!CFNumberGetValue(formDataPointerAsCFNumber.get(), kCFNumberLongType, &formDataPointerAsNumber))
        return 0;

    return reinterpret_cast<FormData*>(static_cast<intptr_t>(formDataPointerAsNumber));
}

} // namespace WebCore
