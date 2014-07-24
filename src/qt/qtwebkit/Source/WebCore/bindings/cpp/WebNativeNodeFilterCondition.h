/*
 *  Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef WebNativeNodeFilterCondition_h
#define WebNativeNodeFilterCondition_h

#include "NodeFilter.h"
#include "WebDOMNodeFilter.h"

class WebNativeNodeFilterCondition : public WebCore::NodeFilterCondition {
public:
    static PassRefPtr<WebNativeNodeFilterCondition> create(WebUserNodeFilter* filter)
    {
        return adoptRef(new WebNativeNodeFilterCondition(filter));
    }

    virtual ~WebNativeNodeFilterCondition();

    virtual short acceptNode(WebCore::ScriptState*, WebCore::Node*) const;

protected:
    WebNativeNodeFilterCondition(WebUserNodeFilter*);
    WebUserNodeFilter* m_filter;
};

#endif
