/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
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

#include "config.h"
#include "NetworkInfoClientBlackBerry.h"

#include "Event.h"
#include "NetworkInfo.h"
#include "WebPage_p.h"

#include <math.h>

namespace WebCore {

static const double  networkSpeedNone = 0;
static const double  networkSpeedEthernet = 20.0;
static const double  networkSpeedWifi = 20.0;
static const double  networkSpeedGPRS = 7.2 / 1024.0;
static const double  networkSpeedEDGE = 30.0 / 1024.0;
static const double  networkSpeedUMTS = 48.0 / 1024.0;
static const double  networkSpeedHSPA = 1.7;
static const double  networkSpeedHSPAP = 5.25;
static const double  networkSpeedCDMA = 18.0 / 1024.0;
static const double  networkSpeedEVDO = 307.0 / 1024.0;
static const double  networkSpeedEVDOA = 397.0 / 1024.0;
static const double  networkSpeedEHRPD = 21.625;
static const double  networkSpeedLTE = 21.625;
static const double  networkSpeedDefault = INFINITY; // w3c draft states it should be infinity

NetworkInfoClientBlackBerry::NetworkInfoClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
    , m_controller(0)
    , m_isActive(false)
{
}

void NetworkInfoClientBlackBerry::networkInfoControllerDestroyed()
{
    delete this;
}

void NetworkInfoClientBlackBerry::startUpdating()
{
    if (!m_isActive)
        // Add ourselves to listener so our values get updated whenever PPS calls.
        BlackBerry::Platform::NetworkInfo::instance()->addListener(this);
    m_isActive = true;
}

void NetworkInfoClientBlackBerry::stopUpdating()
{
    if (m_isActive)
        BlackBerry::Platform::NetworkInfo::instance()->removeListener(this);
    m_isActive = false;
}

double NetworkInfoClientBlackBerry::bandwidth() const
{
    switch (BlackBerry::Platform::NetworkInfo::instance()->getCurrentNetworkType()) {
    case BlackBerry::Platform::NetworkTypeWifi:
        return networkSpeedWifi;
    case BlackBerry::Platform::NetworkTypeCellular:
        switch (BlackBerry::Platform::NetworkInfo::instance()->getCurrentCellularType()) {
        case BlackBerry::Platform::CellularTypeGPRS:
            return networkSpeedGPRS;
        case BlackBerry::Platform::CellularTypeEDGE:
            return networkSpeedEDGE;
        case BlackBerry::Platform::CellularTypeUMTS:
            return networkSpeedUMTS;
        case BlackBerry::Platform::CellularTypeHSPA:
            return networkSpeedHSPA;
        case BlackBerry::Platform::CellularTypeHSPAP:
            return networkSpeedHSPAP;
        case BlackBerry::Platform::CellularTypeCDMA:
            return networkSpeedCDMA;
        case BlackBerry::Platform::CellularTypeEVDO:
            return networkSpeedEVDO;
        case BlackBerry::Platform::CellularTypeEVDOA:
            return networkSpeedEVDOA;
        case BlackBerry::Platform::CellularTypeEHRPD:
            return networkSpeedEHRPD;
        case BlackBerry::Platform::CellularTypeLTE:
            return networkSpeedLTE;
        case BlackBerry::Platform::CellularTypeUnknown:
            return networkSpeedDefault;
        }
        break;
    case BlackBerry::Platform::NetworkTypeWired:
        return networkSpeedEthernet;
    case BlackBerry::Platform::NetworkTypeNone:
        return networkSpeedNone;
    default:
        break;
    }

    return networkSpeedDefault;
}

bool NetworkInfoClientBlackBerry::metered() const
{
    BlackBerry::Platform::InternalNetworkConnectionType cellType = BlackBerry::Platform::NetworkInfo::instance()->getCurrentNetworkType();
    if (cellType == BlackBerry::Platform::NetworkTypeCellular || cellType == BlackBerry::Platform::NetworkTypeBB)
        return true;
    return false;
}

void NetworkInfoClientBlackBerry::onCurrentNetworkTypeChange(BlackBerry::Platform::InternalNetworkConnectionType)
{
    if (m_isActive) {
        RefPtr<NetworkInfo> newNetworkInfo = NetworkInfo::create(bandwidth(), metered());
        NetworkInfoController::from(m_webPagePrivate->m_page)->didChangeNetworkInformation(eventNames().webkitnetworkinfochangeEvent , newNetworkInfo.get());
    }
}

void NetworkInfoClientBlackBerry::onCurrentCellularTypeChange(BlackBerry::Platform::InternalCellularConnectionType)
{
    // Only dispatch to listeners if the current type is cellular.
    if (BlackBerry::Platform::NetworkInfo::instance()->getCurrentNetworkType() == BlackBerry::Platform::NetworkTypeCellular && m_isActive) {
        RefPtr<NetworkInfo> newNetworkInfo = NetworkInfo::create(bandwidth(), metered());
        NetworkInfoController::from(m_webPagePrivate->m_page)->didChangeNetworkInformation(eventNames().webkitnetworkinfochangeEvent , newNetworkInfo.get());
    }
}

}
