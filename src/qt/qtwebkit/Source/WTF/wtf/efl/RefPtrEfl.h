/*
 *  Copyright (C) 2011 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef RefPtrEfl_h
#define RefPtrEfl_h

#include <wtf/RefPtr.h>

#if USE(EO)
typedef struct _Eo Evas_Object;
#else
typedef struct _Evas_Object Evas_Object;
#endif

namespace WTF {

template<> void refIfNotNull(Evas_Object* ptr);
template<> void derefIfNotNull(Evas_Object* ptr);

}

#endif // RefPtrEfl_h
