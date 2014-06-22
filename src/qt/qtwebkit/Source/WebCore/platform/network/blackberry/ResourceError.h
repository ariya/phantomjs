/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#ifndef ResourceError_h
#define ResourceError_h

#include "ResourceErrorBase.h"

namespace WebCore {

class ResourceError : public ResourceErrorBase {
public:
    static const char* const httpErrorDomain;
    static const char* const platformErrorDomain;

    ResourceError()
    {
    }

    ResourceError(const String& domain, int errorCode, const String& failingURL, const String& localizedDescription)
        : ResourceErrorBase(domain, errorCode, failingURL, localizedDescription)
    {
    }
};

} // namespace WebCore

#endif // ResourceError_h
