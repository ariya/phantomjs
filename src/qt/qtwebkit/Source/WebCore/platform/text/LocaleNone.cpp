/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformLocale.h"
#include <wtf/DateMath.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class LocaleNone : public Locale {
public:
    virtual ~LocaleNone();

private:
    virtual void initializeLocaleData() OVERRIDE FINAL;
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

    Vector<String> m_timeAMPMLabels;
    Vector<String> m_shortMonthLabels;
    Vector<String> m_monthLabels;
#endif
};

PassOwnPtr<Locale> Locale::create(const AtomicString&)
{
    return adoptPtr(new LocaleNone());
}

LocaleNone::~LocaleNone()
{
}

void LocaleNone::initializeLocaleData()
{
}

#if ENABLE(DATE_AND_TIME_INPUT_TYPES)
const Vector<String>& LocaleNone::monthLabels()
{
    if (!m_monthLabels.isEmpty())
        return m_monthLabels;
    m_monthLabels.reserveCapacity(WTF_ARRAY_LENGTH(WTF::monthFullName));
    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(WTF::monthFullName); ++i)
        m_monthLabels.append(WTF::monthFullName[i]);
    return m_monthLabels;
}

String LocaleNone::dateFormat()
{
    return ASCIILiteral("yyyy-MM-dd");
}

String LocaleNone::monthFormat()
{
    return ASCIILiteral("yyyy-MM");
}

String LocaleNone::shortMonthFormat()
{
    return ASCIILiteral("yyyy-MM");
}

String LocaleNone::timeFormat()
{
    return ASCIILiteral("HH:mm:ss");
}

String LocaleNone::shortTimeFormat()
{
    return ASCIILiteral("HH:mm");
}

String LocaleNone::dateTimeFormatWithSeconds()
{
    return ASCIILiteral("yyyy-MM-dd'T'HH:mm:ss");
}

String LocaleNone::dateTimeFormatWithoutSeconds()
{
    return ASCIILiteral("yyyy-MM-dd'T'HH:mm");
}

const Vector<String>& LocaleNone::shortMonthLabels()
{
    if (!m_shortMonthLabels.isEmpty())
        return m_shortMonthLabels;
    m_shortMonthLabels.reserveCapacity(WTF_ARRAY_LENGTH(WTF::monthName));
    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(WTF::monthName); ++i)
        m_shortMonthLabels.append(WTF::monthName[i]);
    return m_shortMonthLabels;
}

const Vector<String>& LocaleNone::shortStandAloneMonthLabels()
{
    return shortMonthLabels();
}

const Vector<String>& LocaleNone::standAloneMonthLabels()
{
    return monthLabels();
}

const Vector<String>& LocaleNone::timeAMPMLabels()
{
    if (!m_timeAMPMLabels.isEmpty())
        return m_timeAMPMLabels;
    m_timeAMPMLabels.reserveCapacity(2);
    m_timeAMPMLabels.append("AM");
    m_timeAMPMLabels.append("PM");
    return m_timeAMPMLabels;
}

#endif

} // namespace WebCore
