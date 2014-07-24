/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Module.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>

namespace WebKit {

bool Module::load()
{
    m_module = adoptPtr(eina_module_new(m_path.utf8().data()));
    if (!m_module || !eina_module_load(m_module.get())) {
        m_module = nullptr;
        return false;
    }

    return true;
}

void Module::unload()
{
    m_module = nullptr;
}

void* Module::platformFunctionPointer(const char* functionName) const
{
    if (m_module)
        return eina_module_symbol_get(m_module.get(), functionName);

    return 0;
}

}
