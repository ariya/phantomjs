/*
 * Copyright (C) 2012 Igalia S.L.
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

#ifndef WebKitScriptDialogPrivate_h
#define WebKitScriptDialogPrivate_h

#include "WebKitScriptDialog.h"
#include <wtf/text/CString.h>

struct _WebKitScriptDialog {
    _WebKitScriptDialog(unsigned type, const CString& message)
        : type(type)
        , message(message)
        , confirmed(false)
    {
    }

    _WebKitScriptDialog(unsigned type, const CString& message, const CString& defaultText)
        : type(type)
        , message(message)
        , defaultText(defaultText)
        , confirmed(false)
    {
        ASSERT(type == WEBKIT_SCRIPT_DIALOG_PROMPT);
    }

    _WebKitScriptDialog(WebKitScriptDialog* dialog)
        : type(dialog->type)
        , message(dialog->message)
        , defaultText(dialog->defaultText)
        , confirmed(dialog->confirmed)
        , text(dialog->text)
    {
    }

    unsigned type;
    CString message;
    CString defaultText;

    bool confirmed;
    CString text;
};

#endif // WebKitScriptDialogPrivate_h
