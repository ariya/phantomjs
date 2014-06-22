/*
 *  Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 *  Copyright (C) 2009-2010 ProFUSION embedded systems
 *  Copyright (C) 2009-2010 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DragData.h"

#include "Document.h"
#include "DocumentFragment.h"

namespace WebCore {

bool DragData::canSmartReplace() const
{
    return false;
}

bool DragData::containsColor() const
{
    return false;
}

bool DragData::containsFiles() const
{
    return false;
}

unsigned DragData::numberOfFiles() const
{
    return 0;
}

void DragData::asFilenames(Vector<String>&) const
{
}

bool DragData::containsPlainText() const
{
    return false;
}

String DragData::asPlainText(Frame*) const
{
    return String();
}

Color DragData::asColor() const
{
    return Color();
}

bool DragData::containsCompatibleContent() const
{
    return false;
}

bool DragData::containsURL(Frame*, FilenameConversionPolicy) const
{
    return false;
}

String DragData::asURL(Frame*, FilenameConversionPolicy, String*) const
{
    return String();
}

PassRefPtr<DocumentFragment> DragData::asFragment(Frame*, PassRefPtr<Range>, bool, bool&) const
{
    return 0;
}

}
