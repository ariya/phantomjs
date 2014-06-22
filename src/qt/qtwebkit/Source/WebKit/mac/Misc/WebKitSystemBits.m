/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#import "WebKitSystemBits.h"

#import "WebNSFileManagerExtras.h"
#import <mach/host_info.h>
#import <mach/mach.h>
#import <mach/mach_error.h>
#import <sys/sysctl.h>
#import <sys/types.h>
#import <wtf/Assertions.h>

static host_basic_info_data_t gHostBasicInfo;
static pthread_once_t initControl = PTHREAD_ONCE_INIT;

static void initCapabilities(void)
{
    mach_msg_type_number_t  count;
    kern_return_t r;
    mach_port_t host;

    /* Discover our CPU type */
    host = mach_host_self();
    count = HOST_BASIC_INFO_COUNT;
    r = host_info(host, HOST_BASIC_INFO, (host_info_t) &gHostBasicInfo, &count);
    mach_port_deallocate(mach_task_self(), host);
    if (r != KERN_SUCCESS) {
        LOG_ERROR("%s : host_info(%d) : %s.\n", __FUNCTION__, r, mach_error_string(r));
    }
}

uint64_t WebMemorySize(void)
{
    pthread_once(&initControl, initCapabilities);
    return gHostBasicInfo.max_mem;
}

int WebNumberOfCPUs(void)
{
    static int numCPUs = 0;
    
    if (numCPUs == 0) {
        int mib[2];
        size_t len;

        mib[0] = CTL_HW;
        mib[1] = HW_NCPU;
        len = sizeof(numCPUs);
        sysctl(mib, 2, &numCPUs, &len, NULL, 0);
    }
    return numCPUs;
}

unsigned long long WebVolumeFreeSize(NSString *path)
{
    NSDictionary *fileSystemAttributesDictionary = [[NSFileManager defaultManager] attributesOfFileSystemForPath:path error:NULL];
    return [[fileSystemAttributesDictionary objectForKey:NSFileSystemFreeSize] unsignedLongLongValue];
}
