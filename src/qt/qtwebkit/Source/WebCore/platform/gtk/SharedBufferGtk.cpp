/*
 * Copyright (C) 2008 Collabora Ltd.
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
#include "SharedBuffer.h"

#include "FileSystem.h"
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#include <glib.h>


namespace WebCore {

PassRefPtr<SharedBuffer> SharedBuffer::createWithContentsOfFile(const String& filePath)
{
    if (filePath.isEmpty())
        return 0;

    CString filename = fileSystemRepresentation(filePath);
    GOwnPtr<gchar> contents;
    gsize size;
    GOwnPtr<GError> error;
    if (!g_file_get_contents(filename.data(), &contents.outPtr(), &size, &error.outPtr())) {
        LOG_ERROR("Failed to fully read contents of file %s - %s", filenameForDisplay(filePath).utf8().data(), error->message);
        return 0;
    }

    RefPtr<SharedBuffer> result = SharedBuffer::create(contents.get(), size);

    return result.release();
}

} // namespace WebCore
