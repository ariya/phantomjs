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
#include "ewk_file_chooser.h"

#include "FileChooser.h"
#include "ewk_file_chooser_private.h"
#include <wtf/text/CString.h>

struct _Ewk_File_Chooser {
    RefPtr<WebCore::FileChooser> fileChooser;
};

Eina_Bool ewk_file_chooser_allows_multiple_files_get(const Ewk_File_Chooser* chooser)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(chooser, false);
    return chooser->fileChooser->settings().allowsMultipleFiles;
}

Eina_Bool ewk_file_chooser_allows_directory_upload_get(const Ewk_File_Chooser* chooser)
{
#if ENABLE(DIRECTORY_UPLOAD)
    EINA_SAFETY_ON_NULL_RETURN_VAL(chooser, false);
    return chooser->fileChooser->settings().allowsDirectoryUpload;
#else
    UNUSED_PARAM(chooser);
    return false;
#endif
}

Eina_List* ewk_file_chooser_accept_mimetypes_get(const Ewk_File_Chooser* chooser)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(chooser, 0);

    Eina_List* mimetypes = 0;
    size_t count = chooser->fileChooser->settings().acceptMIMETypes.size();
    for (size_t i = 0; i < count; ++i)
        mimetypes = eina_list_append(mimetypes, eina_stringshare_add(chooser->fileChooser->settings().acceptMIMETypes[i].utf8().data()));

    return mimetypes;
}

Eina_List* ewk_file_chooser_accept_file_extentions_get(const Ewk_File_Chooser* chooser)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(chooser, 0);

    Eina_List* fileExtentions = 0;
    size_t count = chooser->fileChooser->settings().acceptFileExtensions.size();
    for (size_t i = 0; i < count; ++i)
        fileExtentions = eina_list_append(fileExtentions, eina_stringshare_add(chooser->fileChooser->settings().acceptFileExtensions[i].utf8().data()));

    return fileExtentions;
}

Eina_List* ewk_file_chooser_selected_files_get(const Ewk_File_Chooser* chooser)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(chooser, 0);

    Eina_List* files = 0;
    size_t count = chooser->fileChooser->settings().selectedFiles.size();
    for (size_t i = 0; i < count; ++i)
        files = eina_list_append(files, eina_stringshare_add(chooser->fileChooser->settings().selectedFiles[i].utf8().data()));

    return files;
}

Ewk_File_Chooser_Capture_Type ewk_file_chooser_capture_get(const Ewk_File_Chooser* chooser)
{
#if ENABLE(MEDIA_CAPTURE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(chooser, EWK_FILE_CHOOSER_CAPTURE_TYPE_INVALID);

    String capture = chooser->fileChooser->settings().capture;

    if (capture == "camera")
        return EWK_FILE_CHOOSER_CAPTURE_TYPE_CAMERA;

    if (capture == "camcorder")
        return EWK_FILE_CHOOSER_CAPTURE_TYPE_CAMCORDER;

    if (capture == "microphone")
        return EWK_FILE_CHOOSER_CAPTURE_TYPE_MICROPHONE;

    return EWK_FILE_CHOOSER_CAPTURE_TYPE_FILESYSTEM;
#else
    UNUSED_PARAM(chooser);
    return EWK_FILE_CHOOSER_CAPTURE_TYPE_INVALID;
#endif
}

Ewk_File_Chooser* ewk_file_chooser_new(WebCore::FileChooser* fileChooser)
{
    Ewk_File_Chooser* ewkFileChooser = new Ewk_File_Chooser;
    ewkFileChooser->fileChooser = fileChooser;
    return ewkFileChooser;
}

void ewk_file_chooser_free(Ewk_File_Chooser* chooser)
{
    chooser->fileChooser = 0;
    delete chooser;
}

