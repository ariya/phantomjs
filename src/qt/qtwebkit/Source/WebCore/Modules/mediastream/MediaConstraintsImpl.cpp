/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "MediaConstraintsImpl.h"

#include "ArrayValue.h"
#include "Dictionary.h"
#include "ExceptionCode.h"
#include <wtf/HashMap.h>

namespace WebCore {

PassRefPtr<MediaConstraintsImpl> MediaConstraintsImpl::create(const Dictionary& constraints, ExceptionCode& ec)
{
    RefPtr<MediaConstraintsImpl> object = adoptRef(new MediaConstraintsImpl());
    if (!object->initialize(constraints)) {
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }
    return object.release();
}

PassRefPtr<MediaConstraintsImpl> MediaConstraintsImpl::create()
{
    return adoptRef(new MediaConstraintsImpl());
}

bool MediaConstraintsImpl::initialize(const Dictionary& constraints)
{
    if (constraints.isUndefinedOrNull())
        return true;

    Vector<String> names;
    constraints.getOwnPropertyNames(names);

    String mandatory = ASCIILiteral("mandatory");
    String optional = ASCIILiteral("optional");

    for (Vector<String>::iterator it = names.begin(); it != names.end(); ++it) {
        if (*it != mandatory && *it != optional)
            return false;
    }

    if (names.contains(mandatory)) {
        Dictionary mandatoryConstraints;
        bool ok = constraints.get(mandatory, mandatoryConstraints);
        if (!ok || mandatoryConstraints.isUndefinedOrNull())
            return false;

        ok = mandatoryConstraints.getOwnPropertiesAsStringHashMap(m_mandatoryConstraints);
        if (!ok)
            return false;
    }

    if (names.contains(optional)) {
        ArrayValue optionalConstraints;
        bool ok = constraints.get(optional, optionalConstraints);
        if (!ok || optionalConstraints.isUndefinedOrNull())
            return false;

        size_t numberOfConstraints;
        ok = optionalConstraints.length(numberOfConstraints);
        if (!ok)
            return false;

        for (size_t i = 0; i < numberOfConstraints; ++i) {
            Dictionary constraint;
            ok = optionalConstraints.get(i, constraint);
            if (!ok || constraint.isUndefinedOrNull())
                return false;
            Vector<String> localNames;
            constraint.getOwnPropertyNames(localNames);
            if (localNames.size() != 1)
                return false;
            String key = localNames[0];
            String value;
            ok = constraint.get(key, value);
            if (!ok)
                return false;
            m_optionalConstraints.append(MediaConstraint(key, value));
        }
    }

    return true;
}

MediaConstraintsImpl::~MediaConstraintsImpl()
{
}

void MediaConstraintsImpl::getMandatoryConstraints(Vector<MediaConstraint>& constraints) const
{
    constraints.clear();
    HashMap<String, String>::const_iterator i = m_mandatoryConstraints.begin();
    for (; i != m_mandatoryConstraints.end(); ++i)
        constraints.append(MediaConstraint(i->key, i->value));
}

void MediaConstraintsImpl::getOptionalConstraints(Vector<MediaConstraint>& constraints) const
{
    constraints.clear();
    constraints.append(m_optionalConstraints);
}

bool MediaConstraintsImpl::getMandatoryConstraintValue(const String& name, String& value) const
{
    HashMap<String, String>::const_iterator i = m_mandatoryConstraints.find(name);
    if (i == m_mandatoryConstraints.end())
        return false;

    value = i->value;
    return true;
}

bool MediaConstraintsImpl::getOptionalConstraintValue(const String& name, String& value) const
{
    Vector<MediaConstraint>::const_iterator i = m_optionalConstraints.begin();
    for (; i != m_optionalConstraints.end(); ++i) {
        if (i->m_name == name) {
            value = i->m_value;
            return true;
        }
    }

    return false;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
