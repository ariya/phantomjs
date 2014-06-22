/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef WKBaseEfl_h
#define WKBaseEfl_h

#ifndef WKBase_h
#error "Please #include \"WKBase.h\" instead of this file directly."
#endif

typedef const struct OpaqueWKView* WKViewRef;
typedef const struct OpaqueWKPopupItem* WKPopupItemRef;
typedef const struct OpaqueWKPopupMenuListener* WKPopupMenuListenerRef;
typedef const struct OpaqueWKTouchPoint* WKTouchPointRef;
typedef const struct OpaqueWKTouchEvent* WKTouchEventRef;

#endif /* WKBaseEfl_h */
