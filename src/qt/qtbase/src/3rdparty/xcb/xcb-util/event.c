/*
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors or
 * their institutions shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization from the authors.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "xcb_event.h"

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#define ssizeof(foo)            (ssize_t)sizeof(foo)
#define countof(foo)            (ssizeof(foo) / ssizeof(foo[0]))

static const char *labelError[] =
{
    "Success",
    "BadRequest",
    "BadValue",
    "BadWindow",
    "BadPixmap",
    "BadAtom",
    "BadCursor",
    "BadFont",
    "BadMatch",
    "BadDrawable",
    "BadAccess",
    "BadAlloc",
    "BadColor",
    "BadGC",
    "BadIDChoice",
    "BadName",
    "BadLength",
    "BadImplementation",
};

static const char *labelRequest[] =
{
    "no request",
    "CreateWindow",
    "ChangeWindowAttributes",
    "GetWindowAttributes",
    "DestroyWindow",
    "DestroySubwindows",
    "ChangeSaveSet",
    "ReparentWindow",
    "MapWindow",
    "MapSubwindows",
    "UnmapWindow",
    "UnmapSubwindows",
    "ConfigureWindow",
    "CirculateWindow",
    "GetGeometry",
    "QueryTree",
    "InternAtom",
    "GetAtomName",
    "ChangeProperty",
    "DeleteProperty",
    "GetProperty",
    "ListProperties",
    "SetSelectionOwner",
    "GetSelectionOwner",
    "ConvertSelection",
    "SendEvent",
    "GrabPointer",
    "UngrabPointer",
    "GrabButton",
    "UngrabButton",
    "ChangeActivePointerGrab",
    "GrabKeyboard",
    "UngrabKeyboard",
    "GrabKey",
    "UngrabKey",
    "AllowEvents",
    "GrabServer",
    "UngrabServer",
    "QueryPointer",
    "GetMotionEvents",
    "TranslateCoords",
    "WarpPointer",
    "SetInputFocus",
    "GetInputFocus",
    "QueryKeymap",
    "OpenFont",
    "CloseFont",
    "QueryFont",
    "QueryTextExtents",
    "ListFonts",
    "ListFontsWithInfo",
    "SetFontPath",
    "GetFontPath",
    "CreatePixmap",
    "FreePixmap",
    "CreateGC",
    "ChangeGC",
    "CopyGC",
    "SetDashes",
    "SetClipRectangles",
    "FreeGC",
    "ClearArea",
    "CopyArea",
    "CopyPlane",
    "PolyPoint",
    "PolyLine",
    "PolySegment",
    "PolyRectangle",
    "PolyArc",
    "FillPoly",
    "PolyFillRectangle",
    "PolyFillArc",
    "PutImage",
    "GetImage",
    "PolyText",
    "PolyText",
    "ImageText",
    "ImageText",
    "CreateColormap",
    "FreeColormap",
    "CopyColormapAndFree",
    "InstallColormap",
    "UninstallColormap",
    "ListInstalledColormaps",
    "AllocColor",
    "AllocNamedColor",
    "AllocColorCells",
    "AllocColorPlanes",
    "FreeColors",
    "StoreColors",
    "StoreNamedColor",
    "QueryColors",
    "LookupColor",
    "CreateCursor",
    "CreateGlyphCursor",
    "FreeCursor",
    "RecolorCursor",
    "QueryBestSize",
    "QueryExtension",
    "ListExtensions",
    "ChangeKeyboardMapping",
    "GetKeyboardMapping",
    "ChangeKeyboardControl",
    "GetKeyboardControl",
    "Bell",
    "ChangePointerControl",
    "GetPointerControl",
    "SetScreenSaver",
    "GetScreenSaver",
    "ChangeHosts",
    "ListHosts",
    "SetAccessControl",
    "SetCloseDownMode",
    "KillClient",
    "RotateProperties",
    "ForceScreenSaver",
    "SetPointerMapping",
    "GetPointerMapping",
    "SetModifierMapping",
    "GetModifierMapping",
    "major 120",
    "major 121",
    "major 122",
    "major 123",
    "major 124",
    "major 125",
    "major 126",
    "NoOperation",
};

static const char *labelEvent[] =
{
    "error",
    "reply",
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify",
    "ClientMessage",
    "MappingNotify",
};

const char *
xcb_event_get_label(uint8_t type)
{
    if(type < countof(labelEvent))
        return labelEvent[type];
    return NULL;
}

const char *
xcb_event_get_error_label(uint8_t type)
{
    if(type < countof(labelError))
        return labelError[type];
    return NULL;
}

const char *
xcb_event_get_request_label(uint8_t type)
{
    if(type < countof(labelRequest))
        return labelRequest[type];
    return NULL;
}
