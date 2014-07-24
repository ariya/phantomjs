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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#include "config.h"
#if ENABLE(FILE_SYSTEM)
#include "AsyncFileWriterBlackBerry.h"

#include "PlatformBlob.h"

#include <wtf/text/CString.h>

namespace WebCore {

void AsyncFileWriterBlackBerry::write(long long position, Blob* blob)
{
    ASSERT(blob);

    beginWriteOrTruncate();

    // Platform writer will take the ownership of the platform blob and will destroy it on the thread in which we are running.
    PlatformBlob* platformBlob = new PlatformBlob(blob);
    m_platformWriter->write(position, platformBlob);
}

} // namespace WebCore
#endif
