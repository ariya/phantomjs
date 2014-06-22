/*
 * Copyright (C) 2007, 2008, 2009 Holger Hans Peter Freyther
 * Copyright (C) 2008 Jan Michael C. Alonzo
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2010 Igalia S.L.
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

#ifndef webkitwebpolicydecisionprivate_h
#define webkitwebpolicydecisionprivate_h

#include "webkitwebpolicydecision.h"

extern "C" {

WebKitWebPolicyDecision* webkit_web_policy_decision_new(WebKitWebFrame*, WebCore::FramePolicyFunction);

void webkit_web_policy_decision_cancel(WebKitWebPolicyDecision*);

}

#endif
