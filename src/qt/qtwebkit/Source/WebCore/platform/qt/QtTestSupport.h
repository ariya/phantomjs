/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 University of Szeged. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef QtTestSupport_h
#define QtTestSupport_h

#include <QtCore/qglobal.h>

#if defined(BUILDING_WEBKIT)
#define TESTSUPPORT_EXPORT Q_DECL_EXPORT
#else
#define TESTSUPPORT_EXPORT Q_DECL_IMPORT
#endif

// Helpers for test runners (DumpRenderTree and WebKitTestRunner).
// This is living in WebCore for better code sharing, although
// we expose it as (private) API, so it is part of the WebKit layer.

namespace WebKit {

namespace QtTestSupport {

TESTSUPPORT_EXPORT void clearMemoryCaches();
TESTSUPPORT_EXPORT void initializeTestFonts();

}

}

#endif
