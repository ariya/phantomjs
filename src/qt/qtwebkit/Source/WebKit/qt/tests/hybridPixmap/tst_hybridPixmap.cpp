/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
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
 *
 */

#include "../util.h"

#include "widget.h"
#include <QtTest/QtTest>

class tst_hybridPixmap : public QObject {
    Q_OBJECT

public:
    tst_hybridPixmap(QObject* o = 0) : QObject(o) {}

public Q_SLOTS:
    void init()
    {
    }

    void cleanup()
    {
    }

private Q_SLOTS:
    void hybridPixmap()
    {
        Widget widget;
        widget.show();
        widget.start();
        waitForSignal(&widget, SIGNAL(testComplete()));
    }
};

QTEST_MAIN(tst_hybridPixmap)

#include <tst_hybridPixmap.moc>
