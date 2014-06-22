/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#import <wtf/Platform.h>
#import "PluginProcessShim.h"

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import <WebCore/DynamicLinkerInterposing.h>
#import <WebKitSystemInterface.h>
#import <stdio.h>
#import <objc/message.h>

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/mman.h>

namespace WebKit {

extern "C" void WebKitPluginProcessShimInitialize(const PluginProcessShimCallbacks& callbacks);

static PluginProcessShimCallbacks pluginProcessShimCallbacks;

#ifndef __LP64__

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

static void shimDebugger(void)
{
    if (!pluginProcessShimCallbacks.shouldCallRealDebugger())
        return;
    
    Debugger();
}

static UInt32 shimGetCurrentEventButtonState()
{
    return pluginProcessShimCallbacks.getCurrentEventButtonState();
}

static Boolean shimIsWindowActive(WindowRef window)
{
    bool result;
    if (pluginProcessShimCallbacks.isWindowActive(window, result))
        return result;
    
    return IsWindowActive(window);
}

static void shimModalDialog(ModalFilterUPP modalFilter, DialogItemIndex *itemHit)
{
    pluginProcessShimCallbacks.beginModal();
    ModalDialog(modalFilter, itemHit);
    pluginProcessShimCallbacks.endModal();
}

static DialogItemIndex shimAlert(SInt16 alertID, ModalFilterUPP modalFilter)
{
    pluginProcessShimCallbacks.beginModal();
    DialogItemIndex index = Alert(alertID, modalFilter);
    pluginProcessShimCallbacks.endModal();
    
    return index;
}

static void shimShowWindow(WindowRef window)
{
    pluginProcessShimCallbacks.carbonWindowShown(window);
    ShowWindow(window);
}

static void shimHideWindow(WindowRef window)
{
    pluginProcessShimCallbacks.carbonWindowHidden(window);
    HideWindow(window);
}

static OSStatus
shimLSOpenCFURLRef(CFURLRef url, CFURLRef* launchedURL)
{
    int32_t returnValue;
    if (pluginProcessShimCallbacks.openCFURLRef(url, returnValue, launchedURL))
        return returnValue;

    return LSOpenCFURLRef(url, launchedURL);
}

DYLD_INTERPOSE(shimDebugger, Debugger);
DYLD_INTERPOSE(shimGetCurrentEventButtonState, GetCurrentEventButtonState);
DYLD_INTERPOSE(shimIsWindowActive, IsWindowActive);
DYLD_INTERPOSE(shimModalDialog, ModalDialog);
DYLD_INTERPOSE(shimAlert, Alert);
DYLD_INTERPOSE(shimShowWindow, ShowWindow);
DYLD_INTERPOSE(shimHideWindow, HideWindow);
DYLD_INTERPOSE(shimLSOpenCFURLRef, LSOpenCFURLRef);

#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

#endif

// Simple Fake System V shared memory. This replacement API implements
// usable system V shared memory for use within a single process. The memory
// is not shared outside of the scope of the process.
struct FakeSharedMemoryDescriptor {
    FakeSharedMemoryDescriptor* next;
    int referenceCount;
    key_t key;
    size_t requestedSize;
    size_t mmapedSize;
    int sharedMemoryFlags;
    int sharedMemoryIdentifier;
    void* mmapedAddress;
};

static FakeSharedMemoryDescriptor* shmDescriptorList = 0;
static int fakeSharedMemoryIdentifier = 0;

static FakeSharedMemoryDescriptor* findBySharedMemoryIdentifier(int sharedMemoryIdentifier)
{
    FakeSharedMemoryDescriptor* descriptorPtr = shmDescriptorList;

    while (descriptorPtr) {
        if (descriptorPtr->sharedMemoryIdentifier == sharedMemoryIdentifier)
            break;
        descriptorPtr = descriptorPtr->next;
    }
    return descriptorPtr;
}

static FakeSharedMemoryDescriptor* findBySharedMemoryAddress(const void* mmapedAddress)
{
    FakeSharedMemoryDescriptor* descriptorPtr = shmDescriptorList;

    while (descriptorPtr) {
        if (descriptorPtr->mmapedAddress == mmapedAddress)
            break;

        descriptorPtr = descriptorPtr->next;
    }
    return descriptorPtr;
}

static Boolean shim_disabled(void)
{
    static Boolean isFakeSHMDisabled;

    static dispatch_once_t once;
    dispatch_once(&once, ^() {
        Boolean keyExistsAndHasValidFormat = false;
        Boolean prefValue = CFPreferencesGetAppBooleanValue(CFSTR("WebKitDisableFakeSYSVSHM"), kCFPreferencesCurrentApplication, &keyExistsAndHasValidFormat);

        if (keyExistsAndHasValidFormat && prefValue)
            isFakeSHMDisabled = true;
        else
            isFakeSHMDisabled = false;
    });

    return isFakeSHMDisabled;
}

static int shim_shmdt(const void* sharedAddress)
{
    if (shim_disabled())
        return shmdt(sharedAddress);

    FakeSharedMemoryDescriptor* descriptorPtr = findBySharedMemoryAddress(sharedAddress);
    if (!descriptorPtr) {
        errno = EINVAL;
        return -1;
    }

    descriptorPtr->referenceCount--;
    if (!descriptorPtr->referenceCount) {
        munmap(descriptorPtr->mmapedAddress, descriptorPtr->mmapedSize);
        descriptorPtr->mmapedAddress = 0;
    }

    return 0;
}

static void* shim_shmat(int sharedMemoryIdentifier, const void* requestedSharedAddress, int shmflg)
{
    if (shim_disabled())
        return shmat(sharedMemoryIdentifier, requestedSharedAddress, shmflg);

    FakeSharedMemoryDescriptor* descriptorPtr = findBySharedMemoryIdentifier(sharedMemoryIdentifier);
    void* mappedAddress = (void*)-1;

    if (!descriptorPtr) {
        errno = EINVAL;
        return mappedAddress;
    }

    if (descriptorPtr->mmapedAddress) {
        if (!requestedSharedAddress || requestedSharedAddress == descriptorPtr->mmapedAddress) {
            mappedAddress = descriptorPtr->mmapedAddress;
            descriptorPtr->referenceCount++;
        }
    } else {
        descriptorPtr->mmapedSize = (descriptorPtr->requestedSize + PAGE_SIZE) & ~(PAGE_SIZE - 1);
        mappedAddress = descriptorPtr->mmapedAddress = mmap((void*)requestedSharedAddress,
            descriptorPtr->mmapedSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        descriptorPtr->referenceCount++;
    }

    return mappedAddress;
}

static int shim_shmget(key_t key, size_t requestedSizeOfSharedMemory, int sharedMemoryFlags)
{
    if (shim_disabled())
        return shmget(key, requestedSizeOfSharedMemory, sharedMemoryFlags);

    FakeSharedMemoryDescriptor* descriptorPtr = shmDescriptorList;

    while (descriptorPtr) {
        // Are we looking for something we've already created?
        if (descriptorPtr->key == key
            && descriptorPtr->requestedSize == requestedSizeOfSharedMemory
            && !((descriptorPtr->sharedMemoryFlags ^ sharedMemoryFlags) & 0777))
            break;
        descriptorPtr = descriptorPtr->next;
    }

    if (!descriptorPtr) {
        descriptorPtr = (FakeSharedMemoryDescriptor*)malloc(sizeof(FakeSharedMemoryDescriptor));
        if (!descriptorPtr) {
            errno = ENOMEM;
            return -1;
        }
        descriptorPtr->key = key;
        descriptorPtr->requestedSize = requestedSizeOfSharedMemory;
        descriptorPtr->sharedMemoryFlags = sharedMemoryFlags;
        descriptorPtr->sharedMemoryIdentifier = ++fakeSharedMemoryIdentifier;
        descriptorPtr->referenceCount = 0;
        descriptorPtr->mmapedAddress = 0;
        descriptorPtr->mmapedSize = 0;
        descriptorPtr->next = shmDescriptorList;
        shmDescriptorList = descriptorPtr;
    }
    return descriptorPtr->sharedMemoryIdentifier;
}

static int shim_shmctl(int sharedMemoryIdentifier, int cmd, struct shmid_ds* outputDescriptor)
{
    if (shim_disabled())
        return shmctl(sharedMemoryIdentifier, cmd, outputDescriptor);

    FakeSharedMemoryDescriptor* descriptorPtr = findBySharedMemoryIdentifier(sharedMemoryIdentifier);

    if (!descriptorPtr) {
        errno = EINVAL;
        return -1;
    }

    switch (cmd) {
    case IPC_SET:
    case IPC_RMID:
        errno = EPERM;
        return -1;

    case IPC_STAT:
        outputDescriptor->shm_perm.cuid = outputDescriptor->shm_perm.uid = getuid();
        outputDescriptor->shm_perm.cgid = outputDescriptor->shm_perm.gid = getgid();
        outputDescriptor->shm_perm.mode = descriptorPtr->sharedMemoryFlags & 0777;

        outputDescriptor->shm_segsz = descriptorPtr->requestedSize;

        outputDescriptor->shm_cpid = outputDescriptor->shm_lpid = getpid();

        outputDescriptor->shm_nattch = descriptorPtr->referenceCount;

        outputDescriptor->shm_ctime = outputDescriptor->shm_atime = outputDescriptor->shm_dtime = time(0);

        return 0;
    }

    errno = EINVAL;
    return -1;
}

DYLD_INTERPOSE(shim_shmat, shmat);
DYLD_INTERPOSE(shim_shmdt, shmdt);
DYLD_INTERPOSE(shim_shmget, shmget);
DYLD_INTERPOSE(shim_shmctl, shmctl);

__attribute__((visibility("default")))
void WebKitPluginProcessShimInitialize(const PluginProcessShimCallbacks& callbacks)
{
    pluginProcessShimCallbacks = callbacks;
}

} // namespace WebKit

