/*
 * Copyright (C) 2013 Igalia S.L.
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
 */

#include "config.h"

#include "WebGtkExtensionManager.h"
#include <WebKit2/WKBundleInitialize.h>

using namespace WebKit;

#if defined(WIN32) || defined(_WIN32)
extern "C" __declspec(dllexport)
#else
extern "C"
#endif
void WKBundleInitialize(WKBundleRef bundle, WKTypeRef userData)
{
    WebGtkExtensionManager::shared().initialize(bundle, userData);
}
