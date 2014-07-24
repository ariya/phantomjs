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

#ifndef VibrationClient_h
#define VibrationClient_h

namespace WebCore {

class Page;

class VibrationClient {
public:
    virtual ~VibrationClient() { }

    virtual void vibrate(const unsigned& time) = 0;
    virtual void cancelVibration() = 0;

    virtual void vibrationDestroyed() = 0;
};

void provideVibrationTo(Page*, VibrationClient*);

} // namespace WebCore

#endif // VibrationClient_h

