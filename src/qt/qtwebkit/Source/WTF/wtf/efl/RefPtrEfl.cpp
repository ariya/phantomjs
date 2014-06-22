/*
 *  Copyright (C) 2011 Samsung Electronics
 *  Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "RefPtrEfl.h"

#include <Evas.h>

namespace WTF {

template<> void refIfNotNull(Evas_Object* ptr)
{
    if (LIKELY(!!ptr))
        evas_object_ref(ptr);
}

template<> void derefIfNotNull(Evas_Object* ptr)
{
    // Refcounting in Evas_Object is different that normal. The object is created
    // with an external ref count of 0, but with one internal count of 1 which can
    // only be removed by calling _del. Calling _del with an external ref count > 0,
    // postposes deletion until it reaches 0.

    if (LIKELY(!!ptr)) {
        if (evas_object_ref_get(ptr) > 0)
            evas_object_unref(ptr);
        else
            evas_object_del(ptr);
    }
}

}
