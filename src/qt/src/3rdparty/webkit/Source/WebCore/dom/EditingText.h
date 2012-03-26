/*
 * Copyright (C) 2003, 2009 Apple Inc. All rights reserved.
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

#ifndef EditingText_h
#define EditingText_h

#include "Text.h"

namespace WebCore {

class EditingText : public Text {
public:
    static PassRefPtr<EditingText> create(Document*, const String&);

private:
    virtual bool rendererIsNeeded(RenderStyle *);

    EditingText(Document*, const String&);
};

} // namespace WebCore

#endif // EditingText_h
