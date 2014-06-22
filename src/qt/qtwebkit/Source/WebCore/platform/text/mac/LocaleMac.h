/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LocaleMac_h
#define LocaleMac_h

#include "PlatformLocale.h"
#include <wtf/Forward.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

OBJC_CLASS NSCalendar;
OBJC_CLASS NSDateFormatter;
OBJC_CLASS NSLocale;

namespace WebCore {

class DateComponents;

class LocaleMac : public Locale {
public:
    static PassOwnPtr<LocaleMac> create(const String&);
    static PassOwnPtr<LocaleMac> create(NSLocale*);
    ~LocaleMac();

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    virtual String dateFormat() OVERRIDE;
    virtual String monthFormat() OVERRIDE;
    virtual String shortMonthFormat() OVERRIDE;
    virtual String timeFormat() OVERRIDE;
    virtual String shortTimeFormat() OVERRIDE;
    virtual String dateTimeFormatWithSeconds() OVERRIDE;
    virtual String dateTimeFormatWithoutSeconds() OVERRIDE;
    virtual const Vector<String>& monthLabels() OVERRIDE;
    virtual const Vector<String>& shortMonthLabels() OVERRIDE;
    virtual const Vector<String>& standAloneMonthLabels() OVERRIDE;
    virtual const Vector<String>& shortStandAloneMonthLabels() OVERRIDE;
    virtual const Vector<String>& timeAMPMLabels() OVERRIDE;
#endif

private:
    explicit LocaleMac(NSLocale*);
    RetainPtr<NSDateFormatter> shortDateFormatter();
    virtual void initializeLocaleData() OVERRIDE;

    RetainPtr<NSLocale> m_locale;
    RetainPtr<NSCalendar> m_gregorianCalendar;
#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
    Vector<String> m_monthLabels;
    RetainPtr<NSDateFormatter> timeFormatter();
    RetainPtr<NSDateFormatter> shortTimeFormatter();
    RetainPtr<NSDateFormatter> dateTimeFormatterWithSeconds();
    RetainPtr<NSDateFormatter> dateTimeFormatterWithoutSeconds();

    String m_dateFormat;
    String m_monthFormat;
    String m_shortMonthFormat;
    String m_timeFormatWithSeconds;
    String m_timeFormatWithoutSeconds;
    String m_dateTimeFormatWithSeconds;
    String m_dateTimeFormatWithoutSeconds;
    Vector<String> m_shortMonthLabels;
    Vector<String> m_standAloneMonthLabels;
    Vector<String> m_shortStandAloneMonthLabels;
    Vector<String> m_timeAMPMLabels;
#endif
    bool m_didInitializeNumberData;
};

} // namespace WebCore
#endif
