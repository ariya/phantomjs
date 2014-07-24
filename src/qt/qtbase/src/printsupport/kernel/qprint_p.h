/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtPrintSupport module of the Qt Toolkit.
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

#ifndef QPRINT_P_H
#define QPRINT_P_H

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

#include <QtPrintSupport/qprinter.h>

#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

#if (defined Q_OS_MAC && !defined Q_OS_IOS) || (defined Q_OS_UNIX && !defined QT_NO_CUPS)
#include <cups/ppd.h>  // Use for type defs only, don't want to actually link in main module
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

// From windgdi.h
#define DMBIN_UPPER           1
#define DMBIN_ONLYONE         1
#define DMBIN_LOWER           2
#define DMBIN_MIDDLE          3
#define DMBIN_MANUAL          4
#define DMBIN_ENVELOPE        5
#define DMBIN_ENVMANUAL       6
#define DMBIN_AUTO            7
#define DMBIN_TRACTOR         8
#define DMBIN_SMALLFMT        9
#define DMBIN_LARGEFMT       10
#define DMBIN_LARGECAPACITY  11
#define DMBIN_CASSETTE       14
#define DMBIN_FORMSOURCE     15
#define DMBIN_USER          256

namespace QPrint {

    // Note: Keep in sync with QPrinter::PrinterState for now
    // Replace later with more detailed status reporting
    enum DeviceState {
        Idle,
        Active,
        Aborted,
        Error
    };

    // Note: Keep in sync with QPrinter::DuplexMode
    enum DuplexMode {
        DuplexNone = 0,
        DuplexAuto,
        DuplexLongSide,
        DuplexShortSide
    };

    enum ColorMode {
        GrayScale,
        Color
    };

    // Note: Keep in sync with QPrinter::PaperSource for now
    // If/when made public, rearrange and rename
    enum InputSlotId {
        Upper,
        Lower,
        Middle,
        Manual,
        Envelope,
        EnvelopeManual,
        Auto,
        Tractor,
        SmallFormat,
        LargeFormat,
        LargeCapacity,
        Cassette,
        FormSource,
        MaxPageSource, // Deprecated, kept for compatibility to QPrinter
        CustomInputSlot,
        LastInputSlot = CustomInputSlot,
        OnlyOne = Upper
    };

    struct InputSlot {
        QByteArray key;
        QString name;
        QPrint::InputSlotId id;
        int windowsId;
    };

    enum OutputBinId {
        AutoOutputBin,
        UpperBin,
        LowerBin,
        RearBin,
        CustomOutputBin,
        LastOutputBin = CustomOutputBin
    };

    struct OutputBin {
        QByteArray key;
        QString name;
        QPrint::OutputBinId id;
    };

};

struct InputSlotMap {
    QPrint::InputSlotId id;
    int windowsId;
    const char *key;
};

// Note: PPD standard does not define a standard set of InputSlot keywords,
// it is a free form text field left to the PPD writer to decide,
// but it does suggest some names for consistency with the Windows enum.
static const InputSlotMap inputSlotMap[] = {
    { QPrint::Upper,           DMBIN_UPPER,          "Upper"          },
    { QPrint::Lower,           DMBIN_LOWER,          "Lower"          },
    { QPrint::Middle,          DMBIN_MIDDLE,         "Middle"         },
    { QPrint::Manual,          DMBIN_MANUAL,         "Manual"         },
    { QPrint::Envelope,        DMBIN_ENVELOPE,       "Envelope"       },
    { QPrint::EnvelopeManual,  DMBIN_ENVMANUAL,      "EnvelopeManual" },
    { QPrint::Auto,            DMBIN_AUTO,           "Auto"           },
    { QPrint::Tractor,         DMBIN_TRACTOR,        "Tractor"        },
    { QPrint::SmallFormat,     DMBIN_SMALLFMT,       "AnySmallFormat" },
    { QPrint::LargeFormat,     DMBIN_LARGEFMT,       "AnyLargeFormat" },
    { QPrint::LargeCapacity,   DMBIN_LARGECAPACITY,  "LargeCapacity"  },
    { QPrint::Cassette,        DMBIN_CASSETTE,       "Cassette"       },
    { QPrint::FormSource,      DMBIN_FORMSOURCE,     "FormSource"     },
    { QPrint::Manual,          DMBIN_MANUAL,         "ManualFeed"     },
    { QPrint::OnlyOne,         DMBIN_ONLYONE,        "OnlyOne"        }, // = QPrint::Upper
    { QPrint::CustomInputSlot, DMBIN_USER,           ""               }  // Must always be last row
};

struct OutputBinMap {
    QPrint::OutputBinId id;
    const char *key;
};

static const OutputBinMap outputBinMap[] = {
    { QPrint::AutoOutputBin,   ""      }, // Not a PPD defined value, internal use only
    { QPrint::UpperBin,        "Upper" },
    { QPrint::LowerBin,        "Lower" },
    { QPrint::RearBin,         "Rear"  },
    { QPrint::CustomOutputBin, ""      }  // Must always be last row
};

// Print utilities shared by print plugins

class QPrintUtils
{

public:

    static QPrint::InputSlotId inputSlotKeyToInputSlotId(const QByteArray &key)
    {
        for (int i = 0; inputSlotMap[i].id != QPrint::CustomInputSlot; ++i) {
            if (inputSlotMap[i].key == key)
                return inputSlotMap[i].id;
        }
        return QPrint::CustomInputSlot;
    }

    static QByteArray inputSlotIdToInputSlotKey(QPrint::InputSlotId id)
    {
        for (int i = 0; inputSlotMap[i].id != QPrint::CustomInputSlot; ++i) {
            if (inputSlotMap[i].id == id)
                return QByteArray(inputSlotMap[i].key);
        }
        return QByteArray();
    }

    static int inputSlotIdToWindowsId(QPrint::InputSlotId id)
    {
        for (int i = 0; inputSlotMap[i].id != QPrint::CustomInputSlot; ++i) {
            if (inputSlotMap[i].id == id)
                return inputSlotMap[i].windowsId;
        }
        return 0;
    }

    static QPrint::OutputBinId outputBinKeyToOutputBinId(const QByteArray &key)
    {
        for (int i = 0; outputBinMap[i].id != QPrint::CustomOutputBin; ++i) {
            if (outputBinMap[i].key == key)
                return outputBinMap[i].id;
        }
        return QPrint::CustomOutputBin;
    }

    static QByteArray outputBinIdToOutputBinKey(QPrint::OutputBinId id)
    {
        for (int i = 0; outputBinMap[i].id != QPrint::CustomOutputBin; ++i) {
            if (outputBinMap[i].id == id)
                return QByteArray(outputBinMap[i].key);
        }
        return QByteArray();
    }

#if (defined Q_OS_MAC && !defined Q_OS_IOS) || (defined Q_OS_UNIX && !defined QT_NO_CUPS)

    // PPD utilities shared by CUPS and Mac plugins requiring CUPS headers
    // May turn into a proper internal QPpd class if enough shared between Mac and CUPS,
    // but where would it live?  Not in base module as don't want to link to CUPS.
    // May have to have two copies in plugins to keep in sync.

    static QPrint::InputSlot ppdChoiceToInputSlot(ppd_choice_t choice)
    {
        QPrint::InputSlot input;
        input.key = choice.choice;
        input.name = QString::fromUtf8(choice.text);
        input.id = inputSlotKeyToInputSlotId(input.key);
        input.windowsId = inputSlotMap[input.id].windowsId;
        return input;
    }

    static QPrint::OutputBin ppdChoiceToOutputBin(ppd_choice_t choice)
    {
        QPrint::OutputBin output;
        output.key = choice.choice;
        output.name = QString::fromUtf8(choice.text);
        output.id = outputBinKeyToOutputBinId(output.key);
        return output;
    }

    static int parsePpdResolution(const QByteArray &value)
    {
        if (value.isEmpty())
            return -1;
        // value can be in form 600dpi or 600x600dpi
        QByteArray result = value.split('x').at(0);
        if (result.endsWith("dpi"))
            result.chop(3);
        return result.toInt();
    }

    static QPrint::DuplexMode ppdChoiceToDuplexMode(const QByteArray &choice)
    {
        if (choice == QByteArrayLiteral("DuplexTumble"))
            return QPrint::DuplexShortSide;
        else if (choice == QByteArrayLiteral("DuplexNoTumble"))
            return QPrint::DuplexLongSide;
        else // None or SimplexTumble or SimplexNoTumble
            return QPrint::DuplexNone;
    }

#endif // Mac and CUPS PPD Utilities

};

#endif // QT_NO_PRINTER

QT_END_NAMESPACE

#endif // QPRINT_P_H
