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

#ifndef Vibration_h
#define Vibration_h

#if ENABLE(VIBRATION)

#include "Page.h"
#include "Timer.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class VibrationClient;

class Vibration : public Supplement<Page> {
public:
    typedef Vector<unsigned> VibrationPattern;

    explicit Vibration(VibrationClient*);
    ~Vibration();

    static PassOwnPtr<Vibration> create(VibrationClient*);

    bool vibrate(const VibrationPattern&);
    void cancelVibration();

    // FIXME : Add suspendVibration() and resumeVibration() to the page visibility feature, when the document.hidden attribute is changed.
    void suspendVibration();
    void resumeVibration();
    void timerStartFired(Timer<Vibration>*);
    void timerStopFired(Timer<Vibration>*);

    static const char* supplementName();
    static Vibration* from(Page* page) { return static_cast<Vibration*>(Supplement<Page>::from(page, supplementName())); }
    static bool isActive(Page*);

    bool isVibrating() { return m_isVibrating; }

private:
    void stopVibration();

    VibrationClient* m_vibrationClient;
    Timer<Vibration> m_timerStart;
    Timer<Vibration> m_timerStop;
    bool m_isVibrating;
    VibrationPattern m_pattern;
};

} // namespace WebCore

#endif // ENABLE(VIBRATION)

#endif // Vibration_h

