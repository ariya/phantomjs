/*
 *  Copyright (C) 2012 Samsung Electronics
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

#ifndef NavigatorVibration_h
#define NavigatorVibration_h

#if ENABLE(VIBRATION)

#include "ExceptionCode.h"
#include <wtf/Vector.h>

namespace WebCore {

class Navigator;
class Uint32Array;

class NavigatorVibration {
public:
    typedef Vector<unsigned> VibrationPattern;

    ~NavigatorVibration();

    static bool vibrate(Navigator*, unsigned time);
    static bool vibrate(Navigator*, const VibrationPattern&);

private:
    NavigatorVibration();
};

} // namespace WebCore

#endif // ENABLE(VIBRATION)

#endif // NavigatorVibration_h

