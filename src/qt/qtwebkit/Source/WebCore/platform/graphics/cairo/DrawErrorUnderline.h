/*
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2010 Brent Fulgham <bfulgham@webkit.org>
 *
 * Based on Pango sources (see pangocairo-render.c)
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#if USE(CAIRO)

#ifndef DrawErrorUnderline_h
#define DrawErrorUnderline_h

#include <cairo.h>

//
// Draws an error underline that looks like one of:
//
//              H       E                H
//     /\      /\      /\        /\      /\               -
//   A/  \    /  \    /  \     A/  \    /  \              |
//    \   \  /    \  /   /D     \   \  /    \             |
//     \   \/  C   \/   /        \   \/   C  \            | height = heightSquares * square
//      \      /\  F   /          \  F   /\   \           |
//       \    /  \    /            \    /  \   \G         |
//        \  /    \  /              \  /    \  /          |
//         \/      \/                \/      \/           -
//         B                         B
//         |---|
//       unitWidth = (heightSquares - 1) * square
//
// The x, y, width, height passed in give the desired bounding box;
// x/width are adjusted to make the underline a integer number of units
// wide.
//
static inline void drawErrorUnderline(cairo_t* cr, double x, double y, double width, double height)
{
    static const double heightSquares = 2.5;

    double square = height / heightSquares;
    double halfSquare = 0.5 * square;

    double unitWidth = (heightSquares - 1.0) * square;
    int widthUnits = static_cast<int>((width + 0.5 * unitWidth) / unitWidth);

    x += 0.5 * (width - widthUnits * unitWidth);
    width = widthUnits * unitWidth;

    double bottom = y + height;
    double top = y;

    // Bottom of squiggle
    cairo_move_to(cr, x - halfSquare, top + halfSquare); // A

    int i = 0;
    for (i = 0; i < widthUnits; i += 2) {
        double middle = x + (i + 1) * unitWidth;
        double right = x + (i + 2) * unitWidth;

        cairo_line_to(cr, middle, bottom); // B

        if (i + 2 == widthUnits)
            cairo_line_to(cr, right + halfSquare, top + halfSquare); // D
        else if (i + 1 != widthUnits)
            cairo_line_to(cr, right, top + square); // C
    }

    // Top of squiggle
    for (i -= 2; i >= 0; i -= 2) {
        double left = x + i * unitWidth;
        double middle = x + (i + 1) * unitWidth;
        double right = x + (i + 2) * unitWidth;

        if (i + 1 == widthUnits)
            cairo_line_to(cr, middle + halfSquare, bottom - halfSquare); // G
        else {
            if (i + 2 == widthUnits)
                cairo_line_to(cr, right, top); // E

            cairo_line_to(cr, middle, bottom - halfSquare); // F
        }

        cairo_line_to(cr, left, top); // H
    }

    cairo_fill(cr);
}

#endif // DrawErrorUnderline_h

#endif
