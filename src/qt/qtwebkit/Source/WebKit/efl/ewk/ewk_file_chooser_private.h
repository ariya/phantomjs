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

#ifndef ewk_file_chooser_private_h
#define ewk_file_chooser_private_h

namespace WebCore {
class FileChooser;
}

Ewk_File_Chooser* ewk_file_chooser_new(WebCore::FileChooser* fileChooser);
void ewk_file_chooser_free(Ewk_File_Chooser* chooser);

#endif // ewk_file_chooser_private_h

