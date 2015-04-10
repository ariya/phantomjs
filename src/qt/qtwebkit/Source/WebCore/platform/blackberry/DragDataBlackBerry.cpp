/*
 * Copyright (C) 2009, 2010, 2011 Research In Motion Limited. All rights reserved.
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

#include "config.h"
#include "DragData.h"

#include "Color.h"
#include "DocumentFragment.h"
#include "NotImplemented.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

bool DragData::canSmartReplace() const
{
    notImplemented();
    return false;
}

bool DragData::containsColor() const
{
    notImplemented();
    return false;
}

bool DragData::containsCompatibleContent() const
{
    notImplemented();
    return false;
}

bool DragData::containsFiles() const
{
    notImplemented();
    return false;
}

bool DragData::containsPlainText() const
{
    notImplemented();
    return false;
}

bool DragData::containsURL(Frame*, FilenameConversionPolicy) const
{
    notImplemented();
    return false;
}

void DragData::asFilenames(Vector<String>&) const
{
    notImplemented();
}

Color DragData::asColor() const
{
    notImplemented();
    return Color();
}

String DragData::asPlainText(Frame*) const
{
    notImplemented();
    return String();
}

String DragData::asURL(Frame*, FilenameConversionPolicy, String*) const
{
    notImplemented();
    return String();
}

PassRefPtr<DocumentFragment> DragData::asFragment(Frame*, PassRefPtr<Range>, bool, bool&) const
{
    notImplemented();
    return 0;
}

#if ENABLE(FILE_SYSTEM)
String DragData::droppedFileSystemId() const
{
    notImplemented();
    return String();
}
#endif

} // namespace WebCore
