/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef AutofillBackingStore_h
#define AutofillBackingStore_h

#include "SQLiteDatabase.h"

#include <BlackBerryPlatformMisc.h>

namespace WebCore {

class AutofillBackingStore {
public:
    friend AutofillBackingStore& autofillBackingStore();

    ~AutofillBackingStore();
    bool open(const String& dbPath);
    bool add(const String& name, const String& value);
    Vector<String> get(const String& name, const String& valueHint);
    bool clear();

private:
    AutofillBackingStore();
    bool update(const String& name, const String& value);
    bool contains(const String& name, const String& value) const;

    SQLiteDatabase m_database;
    SQLiteStatement* m_addStatement;
    SQLiteStatement* m_updateStatement;
    SQLiteStatement* m_containsStatement;
    SQLiteStatement* m_getStatement;

    DISABLE_COPY(AutofillBackingStore)
};

AutofillBackingStore& autofillBackingStore();

} // namespace WebCore

#endif // AutofillBackingStore_h
