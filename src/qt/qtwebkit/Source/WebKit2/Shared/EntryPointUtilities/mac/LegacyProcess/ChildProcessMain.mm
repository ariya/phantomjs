/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#import <Foundation/Foundation.h>
#import <dlfcn.h>
#import <stdio.h>
#import <stdlib.h>
#import <sys/event.h>
#import <unistd.h>

namespace WebKit {

static void closeUnusedFileDescriptors()
{
    int numFDs = getdtablesize();

    // Close all file descriptors except stdin, stdout and stderr.
    for (int fd = 3; fd < numFDs; ++fd) {
        // Check if this is a kqueue file descriptor. If it is, we don't want to close it because it has
        // been created by initializing libdispatch from a global initializer. See <rdar://problem/9828476> for more details.
        struct timespec timeSpec = { 0, 0 };
        if (!kevent(fd, 0, 0, 0, 0, &timeSpec))
            continue;

        close(fd);
    }
}

typedef int (*BootstrapMainFunction)(int argc, char** argv);

static int BootstrapMain(int argc, char** argv)
{
    closeUnusedFileDescriptors();

    if (argc < 2)
        return EXIT_FAILURE;

    static void* frameworkLibrary = dlopen(argv[1], RTLD_NOW);
    if (!frameworkLibrary) {
        NSLog(@"Unable to load WebKit2.framework: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    BootstrapMainFunction bootstrapMainFunction;
    @autoreleasepool {
        NSString *entryPointFunctionName = (NSString *)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("WebKitEntryPoint"));
        bootstrapMainFunction = reinterpret_cast<BootstrapMainFunction>(dlsym(frameworkLibrary, [entryPointFunctionName UTF8String]));
        if (!bootstrapMainFunction) {
            NSLog(@"Unable to find entry point '%s' in WebKit2.framework: %s\n", [entryPointFunctionName UTF8String], dlerror());
            return EXIT_FAILURE;
        }
    }

    return bootstrapMainFunction(argc, argv);
}

} // namespace WebKit

int main(int argc, char** argv)
{
    return WebKit::BootstrapMain(argc, argv);
}
