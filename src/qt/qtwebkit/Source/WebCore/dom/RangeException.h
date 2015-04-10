/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Gunnstein Lye (gunnstein@netcom.no)
 * (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef RangeException_h
#define RangeException_h

#include "ExceptionBase.h"

namespace WebCore {

class RangeException : public ExceptionBase {
public:
    static PassRefPtr<RangeException> create(const ExceptionCodeDescription& description)
    {
        return adoptRef(new RangeException(description));
    }

    static const int RangeExceptionOffset = 200;
    static const int RangeExceptionMax = 299;

    enum RangeExceptionCode {
        BAD_BOUNDARYPOINTS_ERR = RangeExceptionOffset + 1,
        INVALID_NODE_TYPE_ERR
    };

    static bool initializeDescription(ExceptionCode, ExceptionCodeDescription*);

private:
    explicit RangeException(const ExceptionCodeDescription& description)
        : ExceptionBase(description)
    {
    }
};

} // namespace WebCore

#endif // RangeException_h
