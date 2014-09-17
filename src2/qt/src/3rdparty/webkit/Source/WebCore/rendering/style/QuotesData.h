/*
 * Copyright (C) 2011 Nokia Inc. All rights reserved.
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

#ifndef QuotesData_h
#define QuotesData_h

#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class QuotesData : public RefCounted<QuotesData> {
public:
    virtual ~QuotesData();
    static QuotesData* create(int stringCount);
    String* data() { return reinterpret_cast<String*>(this+1); }
    const String* data() const { return reinterpret_cast<const String*>(this+1); }
    int length;
    bool operator==(const QuotesData&) const;
    void operator delete(void* p) { delete[] static_cast<char*>(p); }
private:
    QuotesData(int stringCount) : length(stringCount) {}
};

}
#endif // QuotesData_h
