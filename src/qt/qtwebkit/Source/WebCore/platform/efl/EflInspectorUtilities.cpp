/*
 * Copyright (C) 2012 Seokju Kwon (seokju.kwon@gmail.com)
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

#if ENABLE(INSPECTOR)
#include "EflInspectorUtilities.h"

#include <unistd.h>
#include <wtf/text/CString.h>

namespace WebCore {

String inspectorResourcePath()
{
    String inspectorResourcePath = String::fromUTF8(WEB_INSPECTOR_INSTALL_DIR);
    if (access(inspectorResourcePath.utf8().data(), R_OK))
        inspectorResourcePath = WEB_INSPECTOR_DIR;

    return inspectorResourcePath;
}

} // namespace WebCore

#endif
