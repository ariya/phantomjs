/*
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
#include "Clipboard.h"

#include "CachedImage.h"
#include "Document.h"
#include "Editor.h"
#include "Element.h"
#include "Frame.h"
#include "Pasteboard.h"

namespace WebCore {

DragImageRef Clipboard::createDragImage(IntPoint& location) const
{
    location = m_dragLoc;

    if (m_dragImage)
        return createDragImageFromImage(m_dragImage->image());

    if (m_dragImageElement) {
        Document* document = m_dragImageElement->document();
        if (Frame* frame = document->frame())
            return frame->nodeImage(m_dragImageElement.get());
    }

    return 0; // We do not have enough information to create a drag image, use the default icon.
}

void Clipboard::declareAndWriteDragImage(Element* element, const KURL& url, const String& label, Frame* frame)
{
    m_pasteboard->writeImage(element->toNode(), url, label);
}

}
