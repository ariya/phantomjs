/* Based on nsURLParsers.cc from Mozilla
 * -------------------------------------
 * Copyright (C) 1998 Netscape Communications Corporation.
 *
 * Other contributors:
 *   Darin Fisher (original author)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#include "config.h"
#include "URLSegments.h"

namespace WTF {

int URLSegments::length() const
{
    if (fragment.isValid())
        return fragment.end();
    return charactersBefore(Fragment, false);
}

int URLSegments::charactersBefore(ComponentType type, bool includeDelimiter) const
{
    if (type == Scheme)
        return scheme.begin();

    int current = 0;
    if (scheme.isValid())
        current = scheme.end() + 1; // Advance over the ':' at the end of the scheme.

    if (username.isValid()) {
        if (type <= Username)
            return username.begin();
        current = username.end() + 1; // Advance over the '@' or ':' at the end.
    }

    if (password.isValid()) {
        if (type <= Password)
            return password.begin();
        current = password.end() + 1; // Advance over the '@' at the end.
    }

    if (host.isValid()) {
        if (type <= Host)
            return host.begin();
        current = host.end();
    }

    if (port.isValid()) {
        if (type < Port || (type == Port && includeDelimiter))
            return port.begin() - 1; // Back over delimiter.
        if (type == Port)
            return port.begin(); // Don't want delimiter counted.
        current = port.end();
    }

    if (path.isValid()) {
        if (type <= Path)
            return path.begin();
        current = path.end();
    }

    if (query.isValid()) {
        if (type < Query || (type == Query && includeDelimiter))
            return query.begin() - 1; // Back over delimiter.
        if (type == Query)
            return query.begin(); // Don't want delimiter counted.
        current = query.end();
    }

    if (fragment.isValid()) {
        if (type == Fragment && !includeDelimiter)
            return fragment.begin(); // Back over delimiter.

        // When there is a fragment and we get here, the component we wanted was before
        // this and not found, so we always know the beginning of the fragment is right.
        return fragment.begin() - 1; // Don't want delimiter counted.
    }

    return current;
}

} // namespace WTF
