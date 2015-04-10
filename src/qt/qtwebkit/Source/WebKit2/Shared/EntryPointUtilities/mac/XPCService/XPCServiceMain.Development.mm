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

#import <crt_externs.h>
#import <dlfcn.h>
#import <mach-o/dyld.h>
#import <spawn.h> 
#import <stdio.h>
#import <stdlib.h>
#import <xpc/xpc.h>

namespace WebKit {

struct ReexecInfo {
    bool executableHeap;
    char** environment;
    cpu_type_t cpuType;
};

static NO_RETURN void reexec(ReexecInfo *info)
{
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    short flags = 0;

    // We just want to set the process state, not actually launch a new process,
    // so we are going to use the darwin extension to posix_spawn POSIX_SPAWN_SETEXEC
    // to act like a more full featured exec.
    flags |= POSIX_SPAWN_SETEXEC;

    sigset_t signalMaskSet;
    sigemptyset(&signalMaskSet);
    posix_spawnattr_setsigmask(&attr, &signalMaskSet);
    flags |= POSIX_SPAWN_SETSIGMASK;

    static const int allowExecutableHeapFlag = 0x2000;
    if (info->executableHeap)
        flags |= allowExecutableHeapFlag;

    posix_spawnattr_setflags(&attr, flags);

    size_t outCount = 0;
    posix_spawnattr_setbinpref_np(&attr, 1, &info->cpuType, &outCount);

    char path[4 * PATH_MAX];
    uint32_t pathLength = sizeof(path);
    _NSGetExecutablePath(path, &pathLength);

    char** argv = *_NSGetArgv();
    const char* programName = argv[0];
    const char* args[] = { programName, 0 };

    pid_t processIdentifier = 0;
    posix_spawn(&processIdentifier, path, 0, &attr, const_cast<char**>(args), info->environment);

    posix_spawnattr_destroy(&attr);

    NSLog(@"Unable to re-exec for path: %s", path);
    exit(EXIT_FAILURE);
}

static NO_RETURN void reexecCallBack(CFRunLoopTimerRef timer, void *info)
{
    reexec(static_cast<ReexecInfo *>(info));
}

static void XPCServiceEventHandler(xpc_connection_t peer)
{
    xpc_connection_set_target_queue(peer, dispatch_get_main_queue());
    xpc_connection_set_event_handler(peer, ^(xpc_object_t event) {
        xpc_type_t type = xpc_get_type(event);
        if (type == XPC_TYPE_ERROR) {
            if (event == XPC_ERROR_CONNECTION_INVALID || event == XPC_ERROR_TERMINATION_IMMINENT) {
                // FIXME: Handle this case more gracefully.
                exit(EXIT_FAILURE);
            }
        } else {
            assert(type == XPC_TYPE_DICTIONARY);

            if (!strcmp(xpc_dictionary_get_string(event, "message-name"), "re-exec")) {
                ReexecInfo *info = static_cast<ReexecInfo *>(malloc(sizeof(ReexecInfo)));

                info->executableHeap = xpc_dictionary_get_bool(event, "executable-heap");
                info->cpuType = (cpu_type_t)xpc_dictionary_get_uint64(event, "architecture");

                xpc_object_t environmentArray = xpc_dictionary_get_value(event, "environment");
                size_t numberOfEnvironmentVariables = xpc_array_get_count(environmentArray);
                char** environment = static_cast<char **>(malloc(numberOfEnvironmentVariables * sizeof(char*) + 1));
                for (size_t i = 0; i < numberOfEnvironmentVariables; ++i)
                    environment[i] = strdup(xpc_array_get_string(environmentArray, i));
                environment[numberOfEnvironmentVariables] = 0;
                info->environment = environment;

                CFRunLoopTimerContext context = { 0, info, NULL, NULL, NULL };
                CFRunLoopTimerRef timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(), 0, 0, 0, reexecCallBack, &context);
                CFRunLoopAddTimer(CFRunLoopGetCurrent(), timer, kCFRunLoopCommonModes);
            }

            if (!strcmp(xpc_dictionary_get_string(event, "message-name"), "bootstrap")) {
                static void* frameworkLibrary = dlopen(xpc_dictionary_get_string(event, "framework-executable-path"), RTLD_NOW);
                if (!frameworkLibrary) {
                    NSLog(@"Unable to load WebKit2.framework at path: %s (Error: %s)", xpc_dictionary_get_string(event, "framework-executable-path"), dlerror());
                    exit(EXIT_FAILURE);
                }

                CFBundleRef webKit2Bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.WebKit2"));
                CFStringRef entryPointFunctionName = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("WebKitEntryPoint"));

                typedef void (*InitializerFunction)(xpc_connection_t, xpc_object_t);
                InitializerFunction initializerFunctionPtr = reinterpret_cast<InitializerFunction>(CFBundleGetFunctionPointerForName(webKit2Bundle, entryPointFunctionName));
                if (!initializerFunctionPtr) {
                    NSLog(@"Unable to find entry point in WebKit2.framework with name: %@", (NSString *)entryPointFunctionName);
                    exit(EXIT_FAILURE);
                }

                xpc_object_t reply = xpc_dictionary_create_reply(event);
                xpc_dictionary_set_string(reply, "message-name", "process-finished-launching");
                xpc_connection_send_message(xpc_dictionary_get_remote_connection(event), reply);
                xpc_release(reply);

                dup2(xpc_dictionary_dup_fd(event, "stdout"), STDOUT_FILENO);
                dup2(xpc_dictionary_dup_fd(event, "stderr"), STDERR_FILENO);

                initializerFunctionPtr(peer, event);
            }

            if (!strcmp(xpc_dictionary_get_string(event, "message-name"), "pre-bootstrap")) {
                // Hold on to the pre-bootstrap message.
                xpc_retain(event);
            }
        }
    });

    xpc_connection_resume(peer);
}

} // namespace WebKit;

using namespace WebKit;

int main(int argc, char** argv)
{
    xpc_main(XPCServiceEventHandler);
    return 0;
}
