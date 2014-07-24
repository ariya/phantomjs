/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include "NetworkInfoProviderEfl.h"

#if ENABLE(NETWORK_INFO)

#include <Eeze.h>
#include <Eeze_Net.h>
#include <limits>
#include <wtf/text/WTFString.h>

namespace WebCore {

static const char ethernetInterface[] = "eth0";

NetworkInfoProviderEfl::NetworkInfoProviderEfl()
{
}

void NetworkInfoProviderEfl::startUpdating()
{
    if (!eeze_init())
        EINA_LOG_ERR("Fail to start network information client.");
}

void NetworkInfoProviderEfl::stopUpdating()
{
    eeze_shutdown();
}

double NetworkInfoProviderEfl::bandwidth() const
{
    // FIXME : This function should consider cellular network as well. For example, 2G, 3G and 4G.
    // See https://bugs.webkit.org/show_bug.cgi?id=89851 for detail.
    Eeze_Net* ethNet = eeze_net_new(ethernetInterface);
    // FIXME: Ethernet is only supported now. If ethNet is null, network is offline.
    if (!ethNet)
        return 0; // If network is offline, return 0.

    eeze_net_scan(ethNet);

    // FIXME : The eeze library doesn't support EEZE_NET_ADDR_TYPE_IP type yet. So, EEZE_NET_ADDR_TYPE_BROADCAST
    // is used for now.
    // See https://bugs.webkit.org/show_bug.cgi?id=89852 for detail.
    const char* address = eeze_net_addr_get(ethNet, EEZE_NET_ADDR_TYPE_BROADCAST);
    if (!address)
        return 0; // If network is offline, return 0.

    double bandwidth;
    const char* attribute = eeze_net_attribute_get(ethNet, "speed");
    if (attribute) {
        bool ok;
        bandwidth = String::fromUTF8(attribute).toUIntStrict(&ok);
    } else
        bandwidth = std::numeric_limits<double>::infinity(); // If bandwidth is unknown, return infinity value.

    eeze_net_free(ethNet);

    return bandwidth / 8; // MB/s
}

} // namespace WebCore

#endif // ENABLE(NETWORK_INFO)
