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

#include "config.h"
#include "MachUtilities.h"

#include <mach/mach_init.h>
#include <mach/task.h>

void setMachPortQueueLength(mach_port_t receivePort, mach_port_msgcount_t queueLength)
{
    mach_port_limits_t portLimits;
    portLimits.mpl_qlimit = queueLength;

    mach_port_set_attributes(mach_task_self(), receivePort, MACH_PORT_LIMITS_INFO, reinterpret_cast<mach_port_info_t>(&portLimits), MACH_PORT_LIMITS_INFO_COUNT);
}

mach_port_t machExceptionPort()
{
    exception_mask_t exceptionMasks[EXC_TYPES_COUNT];
    exception_port_t exceptionHandlers[EXC_TYPES_COUNT];
    exception_behavior_t exceptionBehaviors[EXC_TYPES_COUNT];
    thread_state_flavor_t exceptionFlavors[EXC_TYPES_COUNT];
    mach_msg_type_number_t numExceptionMasks;

    kern_return_t kr = task_get_exception_ports(mach_task_self(), EXC_MASK_CRASH, exceptionMasks, &numExceptionMasks, exceptionHandlers, exceptionBehaviors, exceptionFlavors);
    if (kr != KERN_SUCCESS) {
        ASSERT_NOT_REACHED();
        return MACH_PORT_NULL;
    }

    // We're just interested in the first exception handler.
    return exceptionHandlers[0];
}

void setMachExceptionPort(mach_port_t exceptionPort)
{
    // Assert that we dont try to call setMachExceptionPort more than once per process.
#if !ASSERT_DISABLED
    static mach_port_t taskExceptionPort = MACH_PORT_NULL;
    ASSERT(taskExceptionPort == MACH_PORT_NULL);
    taskExceptionPort = exceptionPort;
#endif

    if (task_set_exception_ports(mach_task_self(), EXC_MASK_CRASH, exceptionPort, EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES, MACHINE_THREAD_STATE) != KERN_SUCCESS)
        ASSERT_NOT_REACHED();
}
