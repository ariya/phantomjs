/*
    Copyright (C) 2007 Staikos Computing Services Inc.

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

#ifndef UndoStepQt_h
#define UndoStepQt_h

#include <PlatformExportMacros.h>
#include <UndoStep.h>
#include <qstring.h>
#include <qwebkitglobal.h>
#include <wtf/RefPtr.h>

class WEBKIT_EXPORTDATA UndoStepQt  {
    public:
        ~UndoStepQt();

        void redo();
        void undo();
        QString text() const;

    private:
        UndoStepQt(WTF::RefPtr<WebCore::UndoStep>);

        WTF::RefPtr<WebCore::UndoStep> m_step;
        bool m_first;
        QString m_text;
        friend class QWebPageAdapter;
};

#endif

// vim: ts=4 sw=4 et
