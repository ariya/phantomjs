/*
 * Copyright (C) 2011 University of Szeged. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef WKImageQt_h
#define WKImageQt_h

#include <QImage>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKImage.h>

WK_EXPORT QImage WKImageCreateQImage(WKImageRef image);
WK_EXPORT WKImageRef WKImageCreateFromQImage(const QImage& image);

#endif
