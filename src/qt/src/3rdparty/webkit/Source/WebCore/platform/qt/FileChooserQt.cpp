/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
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
 *
 */

#include "config.h"
#include "FileChooser.h"

#include "Font.h"
#include "LocalizedStrings.h"
#include <QCoreApplication>
#include <QFontMetrics>

namespace WebCore {

String FileChooser::basenameForWidth(const Font& f, int width) const
{
    if (width <= 0)
        return String();

    String string;
    if (m_filenames.isEmpty())
        string = fileButtonNoFileSelectedLabel();
    else if (m_filenames.size() == 1) {
        String fname = m_filenames[0];
        QFontMetrics fm(f.font());
        string = fm.elidedText(fname, Qt::ElideLeft, width);
    } else {
        int n = m_filenames.size();
        string = QCoreApplication::translate("QWebPage", "%n file(s)",
                                             "number of chosen file",
                                             QCoreApplication::CodecForTr, n);
    }

    return string;
}

}

// vim: ts=4 sw=4 et
