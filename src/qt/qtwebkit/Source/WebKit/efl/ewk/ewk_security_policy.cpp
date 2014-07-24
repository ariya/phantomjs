/*
    Copyright (C) 2012 ProFUSION embedded systems

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_security_policy.h"

#include "KURL.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"

using namespace WebCore;

void ewk_security_policy_whitelist_origin_add(const char* sourceUrl, const char* destinationUrl, Eina_Bool allowSubdomains)
{
    const RefPtr<SecurityOrigin> source = SecurityOrigin::createFromString(sourceUrl);
    const KURL destination(KURL(), destinationUrl);

    SecurityPolicy::addOriginAccessWhitelistEntry(*source, destination.protocol(), destination.host(), allowSubdomains);
}

void ewk_security_policy_whitelist_origin_del(const char* sourceUrl, const char* destinationUrl, Eina_Bool allowSubdomains)
{
    const RefPtr<SecurityOrigin> source = SecurityOrigin::createFromString(sourceUrl);
    const KURL destination(KURL(), destinationUrl);

    SecurityPolicy::removeOriginAccessWhitelistEntry(*source, destination.protocol(), destination.host(), allowSubdomains);
}

void ewk_security_policy_whitelist_origin_reset()
{
    SecurityPolicy::resetOriginAccessWhitelists();
}
