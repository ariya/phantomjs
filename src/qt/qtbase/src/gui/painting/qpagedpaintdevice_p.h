/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPAGEDPAINTDEVICE_P_H
#define QPAGEDPAINTDEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpagedpaintdevice.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QPagedPaintDevicePrivate
{
public:
    QPagedPaintDevicePrivate()
        : m_pageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(0, 0, 0, 0)),
          fromPage(0),
          toPage(0),
          pageOrderAscending(true),
          printSelectionOnly(false)
    {
    }

    virtual ~QPagedPaintDevicePrivate()
    {
    }

    // ### Qt6 Remove these and make public class methods virtual
    virtual bool setPageLayout(const QPageLayout &newPageLayout)
    {
        m_pageLayout = newPageLayout;
        return m_pageLayout.isEquivalentTo(newPageLayout);;
    }

    virtual bool setPageSize(const QPageSize &pageSize)
    {
        m_pageLayout.setPageSize(pageSize);
        return m_pageLayout.pageSize().isEquivalentTo(pageSize);
    }

    virtual bool setPageOrientation(QPageLayout::Orientation orientation)
    {
        m_pageLayout.setOrientation(orientation);
        return m_pageLayout.orientation() == orientation;
    }

    virtual bool setPageMargins(const QMarginsF &margins)
    {
        return setPageMargins(margins, m_pageLayout.units());
    }

    virtual bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units)
    {
        m_pageLayout.setUnits(units);
        m_pageLayout.setMargins(margins);
        return m_pageLayout.margins() == margins && m_pageLayout.units() == units;
    }

    virtual QPageLayout pageLayout() const
    {
        return m_pageLayout;
    }

    static inline QPagedPaintDevicePrivate *get(QPagedPaintDevice *pd) { return pd->d; }

    QPageLayout m_pageLayout;

    // These are currently required to keep QPrinter functionality working in QTextDocument::print()
    int fromPage;
    int toPage;
    bool pageOrderAscending;
    bool printSelectionOnly;
};

QT_END_NAMESPACE

#endif
