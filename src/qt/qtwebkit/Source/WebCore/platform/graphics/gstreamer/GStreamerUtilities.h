/*
 *  Copyright (C) 2012 Igalia S.L
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

#define LOG_MEDIA_MESSAGE(...) do { \
    GST_DEBUG(__VA_ARGS__); \
    LOG_VERBOSE(Media, __VA_ARGS__); } while (0)

#define ERROR_MEDIA_MESSAGE(...) do { \
    GST_ERROR(__VA_ARGS__); \
    LOG_VERBOSE(Media, __VA_ARGS__); } while (0)

#define INFO_MEDIA_MESSAGE(...) do { \
    GST_INFO(__VA_ARGS__); \
    LOG_VERBOSE(Media, __VA_ARGS__); } while (0)

namespace WebCore {
bool initializeGStreamer();
}
