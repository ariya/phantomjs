/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef NetworkInfoClientBlackBerry_h
#define NetworkInfoClientBlackBerry_h

#include "NetworkInfoClient.h"
#include "NetworkInfoController.h"

#include <BlackBerryPlatformNetworkInfo.h>

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {

class NetworkInfoClientBlackBerry : public NetworkInfoClient, public BlackBerry::Platform::NetworkInfoListener {
public:

    NetworkInfoClientBlackBerry(BlackBerry::WebKit::WebPagePrivate*);
    ~NetworkInfoClientBlackBerry() { }

    virtual void networkInfoControllerDestroyed();

    void startUpdating();
    void stopUpdating();
    double bandwidth() const;
    bool metered() const;

    void onCurrentNetworkTypeChange(BlackBerry::Platform::InternalNetworkConnectionType newNetworkType);
    void onCurrentCellularTypeChange(BlackBerry::Platform::InternalCellularConnectionType newCellularType);

private:
    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
    NetworkInfoController* m_controller;
    bool m_isActive;
};
}

#endif
