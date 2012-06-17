/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSIZEPOLICY_H
#define QSIZEPOLICY_H

#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QVariant;

class Q_GUI_EXPORT QSizePolicy
{
    Q_GADGET
    Q_ENUMS(Policy)

private:
    enum SizePolicyMasks {
        HSize = 4,
        HMask = 0x0f,
        VMask = HMask << HSize,
        CTShift = 9,
        CTSize = 5,
        CTMask = ((0x1 << CTSize) - 1) << CTShift,
        WFHShift = CTShift + CTSize,
        UnusedShift = WFHShift + 1,
        UnusedSize = 1
    };

public:
    enum PolicyFlag {
        GrowFlag = 1,
        ExpandFlag = 2,
        ShrinkFlag = 4,
        IgnoreFlag = 8
    };

    enum Policy {
        Fixed = 0,
        Minimum = GrowFlag,
        Maximum = ShrinkFlag,
        Preferred = GrowFlag | ShrinkFlag,
        MinimumExpanding = GrowFlag | ExpandFlag,
        Expanding = GrowFlag | ShrinkFlag | ExpandFlag,
        Ignored = ShrinkFlag | GrowFlag | IgnoreFlag
    };

    enum ControlType {
        DefaultType      = 0x00000001,
        ButtonBox        = 0x00000002,
        CheckBox         = 0x00000004,
        ComboBox         = 0x00000008,
        Frame            = 0x00000010,
        GroupBox         = 0x00000020,
        Label            = 0x00000040,
        Line             = 0x00000080,
        LineEdit         = 0x00000100,
        PushButton       = 0x00000200,
        RadioButton      = 0x00000400,
        Slider           = 0x00000800,
        SpinBox          = 0x00001000,
        TabWidget        = 0x00002000,
        ToolButton       = 0x00004000
    };
    Q_DECLARE_FLAGS(ControlTypes, ControlType)

    QSizePolicy() : data(0) { }

    // ### Qt 5: merge these two constructors (with type == DefaultType)
    QSizePolicy(Policy horizontal, Policy vertical)
        : data(horizontal | (vertical << HSize)) { }
    QSizePolicy(Policy horizontal, Policy vertical, ControlType type)
        : data(horizontal | (vertical << HSize)) { setControlType(type); }

    Policy horizontalPolicy() const { return static_cast<Policy>(data & HMask); }
    Policy verticalPolicy() const { return static_cast<Policy>((data & VMask) >> HSize); }
    ControlType controlType() const;

    void setHorizontalPolicy(Policy d) { data = (data & ~HMask) | d; }
    void setVerticalPolicy(Policy d) { data = (data & ~(HMask << HSize)) | (d << HSize); }
    void setControlType(ControlType type);

    Qt::Orientations expandingDirections() const {
        Qt::Orientations result;
        if (verticalPolicy() & ExpandFlag)
            result |= Qt::Vertical;
        if (horizontalPolicy() & ExpandFlag)
            result |= Qt::Horizontal;
        return result;
    }

    void setHeightForWidth(bool b) { data = b ? (data | (1 << 2*HSize)) : (data & ~(1 << 2*HSize));  }
    bool hasHeightForWidth() const { return data & (1 << 2*HSize); }
    void setWidthForHeight(bool b) { data = b ? (data | (1 << (WFHShift))) : (data & ~(1 << (WFHShift)));  }
    bool hasWidthForHeight() const { return data & (1 << (WFHShift)); }

    bool operator==(const QSizePolicy& s) const { return data == s.data; }
    bool operator!=(const QSizePolicy& s) const { return data != s.data; }
    operator QVariant() const; // implemented in qabstractlayout.cpp

    int horizontalStretch() const { return data >> 24; }
    int verticalStretch() const { return (data >> 16) & 0xff; }
    void setHorizontalStretch(uchar stretchFactor) { data = (data&0x00ffffff) | (uint(stretchFactor)<<24); }
    void setVerticalStretch(uchar stretchFactor) { data = (data&0xff00ffff) | (uint(stretchFactor)<<16); }

    void transpose();

#ifdef QT3_SUPPORT
    typedef Policy SizeType;
#ifndef qdoc
    typedef Qt::Orientations ExpandData;
    enum {
        NoDirection = 0,
        Horizontally = 1,
        Vertically = 2,
        BothDirections = Horizontally | Vertically
    };
#else
    enum ExpandData {
        NoDirection = 0x0,
        Horizontally = 0x1,
        Vertically = 0x2,
        BothDirections = 0x3
    };
#endif // qdoc

    inline QT3_SUPPORT bool mayShrinkHorizontally() const
        { return horizontalPolicy() & ShrinkFlag; }
    inline QT3_SUPPORT bool mayShrinkVertically() const { return verticalPolicy() & ShrinkFlag; }
    inline QT3_SUPPORT bool mayGrowHorizontally() const { return horizontalPolicy() & GrowFlag; }
    inline QT3_SUPPORT bool mayGrowVertically() const { return verticalPolicy() & GrowFlag; }
    inline QT3_SUPPORT Qt::Orientations expanding() const { return expandingDirections(); }

    QT3_SUPPORT_CONSTRUCTOR QSizePolicy(Policy hor, Policy ver, bool hfw)
        : data(hor | (ver<<HSize) | (hfw ? (1U<<2*HSize) : 0)) { }

    QT3_SUPPORT_CONSTRUCTOR QSizePolicy(Policy hor, Policy ver, uchar hors, uchar vers, bool hfw = false)
        : data(hor | (ver<<HSize) | (hfw ? (1U<<2*HSize) : 0)) {
        setHorizontalStretch(hors);
        setVerticalStretch(vers);
    }

    inline QT3_SUPPORT Policy horData() const { return static_cast<Policy>(data & HMask); }
    inline QT3_SUPPORT Policy verData() const { return static_cast<Policy>((data & VMask) >> HSize); }
    inline QT3_SUPPORT void setHorData(Policy d) { setHorizontalPolicy(d); }
    inline QT3_SUPPORT void setVerData(Policy d) { setVerticalPolicy(d); }

    inline QT3_SUPPORT uint horStretch() const { return horizontalStretch(); }
    inline QT3_SUPPORT uint verStretch() const { return verticalStretch(); }
    inline QT3_SUPPORT void setHorStretch(uchar sf) { setHorizontalStretch(sf); }
    inline QT3_SUPPORT void setVerStretch(uchar sf) { setVerticalStretch(sf); }
#endif

private:
#ifndef QT_NO_DATASTREAM
    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QSizePolicy &);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QSizePolicy &);
#endif
    QSizePolicy(int i) : data(i) { }

    quint32 data;
/*  Qt5: Use bit flags instead, keep it here for improved readability for now.
    We can maybe change it for Qt4, but we'd have to be careful, since the behaviour
    is implementation defined. It usually varies between little- and big-endian compilers, but
    it might also not vary.
    quint32 horzPolicy : 4;
    quint32 vertPolicy : 4;
    quint32 hfw : 1;
    quint32 ctype : 5;
    quint32 wfh : 1;
    quint32 padding : 1;   // we cannot use the highest bit
    quint32 verStretch : 8;
    quint32 horStretch : 8;
*/

};

Q_DECLARE_OPERATORS_FOR_FLAGS(QSizePolicy::ControlTypes)

#ifndef QT_NO_DATASTREAM
// implemented in qlayout.cpp
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QSizePolicy &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QSizePolicy &);
#endif

inline void QSizePolicy::transpose() {
    Policy hData = horizontalPolicy();
    Policy vData = verticalPolicy();
    uchar hStretch = uchar(horizontalStretch());
    uchar vStretch = uchar(verticalStretch());
    setHorizontalPolicy(vData);
    setVerticalPolicy(hData);
    setHorizontalStretch(vStretch);
    setVerticalStretch(hStretch);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSIZEPOLICY_H
