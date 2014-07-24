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

/**
 * @file    ewk_file_chooser.h
 * @brief   File Chooser API.
 *
 * File Chooser supports APIs for file selection dialog.
 * This support attributes of file chooser regarding:
 * - allowance of multiple files selection
 * - allowance of upload directory
 * - list of accepted mime types
 * - list of accepted file extensions
 * - list of initial selected file names
 * - capture attribute for HTML media capture
 */

#ifndef ewk_file_chooser_h
#define ewk_file_chooser_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Ewk_File_Chooser Ewk_File_Chooser;

/**
 * \enum    _Ewk_File_Chooser_Capture_Type
 * @brief   Types of capture attribute of file chooser to support the HTML media capture.
 */
enum _Ewk_File_Chooser_Capture_Type {
    EWK_FILE_CHOOSER_CAPTURE_TYPE_INVALID,
    EWK_FILE_CHOOSER_CAPTURE_TYPE_FILESYSTEM,
    EWK_FILE_CHOOSER_CAPTURE_TYPE_CAMERA,
    EWK_FILE_CHOOSER_CAPTURE_TYPE_CAMCORDER,
    EWK_FILE_CHOOSER_CAPTURE_TYPE_MICROPHONE
};
typedef enum _Ewk_File_Chooser_Capture_Type Ewk_File_Chooser_Capture_Type;

/**
 * Query if multiple files are supported by file chooser.
 *
 * @param f file chooser object.
 *
 * It returns a boolean value which indicates if multiple file is supported or not.
 *
 * @return @c EINA_TRUE on support multiple files or @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool ewk_file_chooser_allows_multiple_files_get(const Ewk_File_Chooser *f);

/**
 * Query if directory upload is supported by file chooser.
 *
 * @param f file chooser object.
 *
 * It returns a boolean value which indicates if directory upload is supported or not.
 *
 * @return @c EINA_TRUE on support directory upload or @c EINA_FALSE otherwise.
 */
EAPI Eina_Bool ewk_file_chooser_allows_directory_upload_get(const Ewk_File_Chooser *f);

/**
 * Returns the list of accepted mimetypes of the file chooser.
 *
 * @param f file chooser object.
 *
 * @return Eina_List of accepted mimetypes on success, or @c NULL on failure,
 *         the Eina_List and its items should be freed after use. Use eina_stringshare_del()
 *         to free the items.
 */
EAPI Eina_List *ewk_file_chooser_accept_mimetypes_get(const Ewk_File_Chooser *f);

/**
 * Returns the list of accepted file extensions of the file chooser.
 *
 * @param f file chooser object.
 *
 * @return @c Eina_List of accepted file extensions on success, or @c NULL on failure,
 *         the Eina_List and its items should be freed after use. Use eina_stringshare_del()
 *         to free the items.
 */
EAPI Eina_List *ewk_file_chooser_accept_file_extentions_get(const Ewk_File_Chooser *f);

/**
 * Returns the list of selected file names of the file chooser.
 *
 * This list indicates previously selected file names of the file chooser.
 * These can be used by initial values of the file dialog.
 *
 * @param f file chooser object.
 *
 * @return @c Eina_List of selected file names on success, or @c NULL on failure,
 *         the Eina_List and its items should be freed after use. Use eina_stringshare_del()
 *         to free the items.
 */
EAPI Eina_List *ewk_file_chooser_selected_files_get(const Ewk_File_Chooser *f);

/**
 * Returns the capture attribute of the file chooser to support HTML media capture.
 *
 * @see http://www.w3.org/TR/html-media-capture/ for the semantics of the capture attribute.
 *
 * @param f file chooser object.
 *
 * @return @c Ewk_File_Chooser_Capture_Type on supporting HTML media capture or
 *         @c EWK_FILE_CHOOSER_CAPTURE_TYPE_INVALID on failure.
 */
EAPI Ewk_File_Chooser_Capture_Type ewk_file_chooser_capture_get(const Ewk_File_Chooser *f);

#ifdef __cplusplus
}
#endif

#endif // ewk_file_chooser_h
