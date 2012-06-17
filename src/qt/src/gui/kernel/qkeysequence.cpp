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

#include "qkeysequence.h"
#include "qkeysequence_p.h"
#include "private/qapplication_p.h"

#ifndef QT_NO_SHORTCUT

#include "qshortcut.h"
#include "qdebug.h"
#ifndef QT_NO_REGEXP
# include "qregexp.h"
#endif
#ifndef QT_NO_DATASTREAM
# include "qdatastream.h"
#endif
#include "qvariant.h"

#ifdef Q_WS_MAC
# include <private/qt_mac_p.h>

#endif

QT_BEGIN_NAMESPACE

#ifdef Q_WS_MAC
static bool qt_sequence_no_mnemonics = true;
struct MacSpecialKey {
    int key;
    ushort macSymbol;
};

static const int NumEntries = 21;
static const MacSpecialKey entries[NumEntries] = {
    { Qt::Key_Escape, 0x238B },
    { Qt::Key_Tab, 0x21E5 },
    { Qt::Key_Backtab, 0x21E4 },
    { Qt::Key_Backspace, 0x232B },
    { Qt::Key_Return, 0x21B5 },
    { Qt::Key_Enter, 0x2324 },
    { Qt::Key_Delete, 0x2326 },
    { Qt::Key_Home, 0x2196 },
    { Qt::Key_End, 0x2198 },
    { Qt::Key_Left, 0x2190 },
    { Qt::Key_Up, 0x2191 },
    { Qt::Key_Right, 0x2192 },
    { Qt::Key_Down, 0x2193 },
    { Qt::Key_PageUp, 0x21DE },
    { Qt::Key_PageDown, 0x21DF },
    { Qt::Key_Shift, kShiftUnicode },
    { Qt::Key_Control, kCommandUnicode },
    { Qt::Key_Meta, kControlUnicode },
    { Qt::Key_Alt, kOptionUnicode },
    { Qt::Key_CapsLock, 0x21EA },
};

static bool operator<(const MacSpecialKey &entry, int key)
{
    return entry.key < key;
}

static bool operator<(int key, const MacSpecialKey &entry)
{
    return key < entry.key;
}

static const MacSpecialKey * const MacSpecialKeyEntriesEnd = entries + NumEntries;

QChar qt_macSymbolForQtKey(int key)
{
    const MacSpecialKey *i = qBinaryFind(entries, MacSpecialKeyEntriesEnd, key);
    if (i == MacSpecialKeyEntriesEnd)
        return QChar();
    ushort macSymbol = i->macSymbol;
    if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)
            && (macSymbol == kControlUnicode || macSymbol == kCommandUnicode)) {
        if (macSymbol == kControlUnicode)
            macSymbol = kCommandUnicode;
        else
            macSymbol = kControlUnicode;
    }

    return QChar(macSymbol);
}

static int qtkeyForMacSymbol(const QChar ch)
{
    const ushort unicode = ch.unicode();
    for (int i = 0; i < NumEntries; ++i) {
        const MacSpecialKey &entry = entries[i];
        if (entry.macSymbol == unicode) {
            int key = entry.key;
            if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)
                    && (unicode == kControlUnicode || unicode == kCommandUnicode)) {
                if (unicode == kControlUnicode)
                    key = Qt::Key_Control;
                else
                    key = Qt::Key_Meta;
            }
            return key;
        }
    }
    return -1;
}

#else
static bool qt_sequence_no_mnemonics = false;
#endif
void Q_GUI_EXPORT qt_set_sequence_auto_mnemonic(bool b) { qt_sequence_no_mnemonics = !b; }

/*!
    \class QKeySequence
    \brief The QKeySequence class encapsulates a key sequence as used
    by shortcuts.

    \ingroup shared


    In its most common form, a key sequence describes a combination of
    keys that must be used together to perform some action. Key sequences
    are used with QAction objects to specify which keyboard shortcuts can
    be used to trigger actions.

    Key sequences can be constructed for use as keyboard shortcuts in
    three different ways:

    \list
    \o For standard shortcuts, a \l{QKeySequence::StandardKey}{standard key}
       can be used to request the platform-specific key sequence associated
       with each shortcut.
    \o For custom shortcuts, human-readable strings such as "Ctrl+X" can
       be used, and these can be translated into the appropriate shortcuts
       for users of different languages. Translations are made in the
       "QShortcut" context.
    \o For hard-coded shortcuts, integer key codes can be specified with
       a combination of values defined by the Qt::Key and Qt::Modifier enum
       values. Each key code consists of a single Qt::Key value and zero or
       more modifiers, such as Qt::SHIFT, Qt::CTRL, Qt::ALT and Qt::META.
    \endlist

    For example, \gui{Ctrl P} might be a sequence used as a shortcut for
    printing a document, and can be specified in any of the following
    ways:

    \snippet doc/src/snippets/code/src_gui_kernel_qkeysequence.cpp 0

    Note that, for letters, the case used in the specification string
    does not matter. In the above examples, the user does not need to
    hold down the \key{Shift} key to activate a shortcut specified
    with "Ctrl+P". However, for other keys, the use of \key{Shift} as
    an unspecified extra modifier key can lead to confusion for users
    of an application whose keyboards have different layouts to those
    used by the developers. See the \l{Keyboard Layout Issues} section
    below for more details.

    It is preferable to use standard shortcuts where possible.
    When creating key sequences for non-standard shortcuts, you should use
    human-readable strings in preference to hard-coded integer values.

    QKeySequence objects can be cast to a QString to obtain a human-readable
    translated version of the sequence. Similarly, the toString() function
    produces human-readable strings for use in menus. On Mac OS X, the
    appropriate symbols are used to describe keyboard shortcuts using special
    keys on the Macintosh keyboard.

    An alternative way to specify hard-coded key codes is to use the Unicode
    code point of the character; for example, 'A' gives the same key sequence
    as Qt::Key_A.

    \bold{Note:} On Mac OS X, references to "Ctrl", Qt::CTRL, Qt::Control
    and Qt::ControlModifier correspond to the \key Command keys on the
    Macintosh keyboard, and references to "Meta", Qt::META, Qt::Meta and
    Qt::MetaModifier correspond to the \key Control keys. Developers on
    Mac OS X can use the same shortcut descriptions across all platforms,
    and their applications will automatically work as expected on Mac OS X.

    \section1 Standard Shortcuts

    QKeySequence defines many \l{QKeySequence::StandardKey} {standard
    keyboard shortcuts} to reduce the amount of effort required when
    setting up actions in a typical application. The table below shows
    some common key sequences that are often used for these standard
    shortcuts by applications on four widely-used platforms.  Note
    that on Mac OS X, the \key Ctrl value corresponds to the \key
    Command keys on the Macintosh keyboard, and the \key Meta value
    corresponds to the \key Control keys.

    \table
    \header \i StandardKey      \i Windows                              \i Mac OS X                 \i KDE          \i GNOME                                 \i S60
    \row    \i HelpContents     \i F1                                   \i Ctrl+?                   \i F1           \i F1                                    \i F2
    \row    \i WhatsThis        \i Shift+F1                             \i Shift+F1                 \i Shift+F1     \i Shift+F1                              \i Shift+F1
    \row    \i Open             \i Ctrl+O                               \i Ctrl+O                   \i Ctrl+O       \i Ctrl+O                                \i (none)
    \row    \i Close            \i Ctrl+F4, Ctrl+W                      \i Ctrl+W, Ctrl+F4          \i Ctrl+W       \i Ctrl+W                                \i (none)
    \row    \i Save             \i Ctrl+S                               \i Ctrl+S                   \i Ctrl+S       \i Ctrl+S                                \i (none)
    \row    \i Quit             \i                                      \i Ctrl+Q                   \i Qtrl+Q       \i Qtrl+Q                                \i (none)
    \row    \i SaveAs           \i                                      \i Ctrl+Shift+S             \i              \i Ctrl+Shift+S                          \i (none)
    \row    \i New              \i Ctrl+N                               \i Ctrl+N                   \i Ctrl+N       \i Ctrl+N                                \i (none)
    \row    \i Delete           \i Del                                  \i Del, Meta+D              \i Del, Ctrl+D  \i Del, Ctrl+D                           \i Del
    \row    \i Cut              \i Ctrl+X, Shift+Del                    \i Ctrl+X                   \i Ctrl+X, F20, Shift+Del \i Ctrl+X, F20, Shift+Del      \i Ctrl+X
    \row    \i Copy             \i Ctrl+C, Ctrl+Ins                     \i Ctrl+C                   \i Ctrl+C, F16, Ctrl+Ins  \i Ctrl+C, F16, Ctrl+Ins       \i Ctrl+C
    \row    \i Paste            \i Ctrl+V, Shift+Ins                    \i Ctrl+V                   \i Ctrl+V, F18, Shift+Ins \i Ctrl+V, F18, Shift+Ins      \i Ctrl+V
    \row    \i Preferences      \i                                      \i Ctrl+,                   \i              \i                                       \i (none)
    \row    \i Undo             \i Ctrl+Z, Alt+Backspace                \i Ctrl+Z                   \i Ctrl+Z, F14  \i Ctrl+Z, F14                           \i Ctrl+Z
    \row    \i Redo             \i Ctrl+Y, Shift+Ctrl+Z, Alt+Shift+Backspace \i Ctrl+Shift+Z        \i Ctrl+Shift+Z \i Ctrl+Shift+Z                     \i (none)
    \row    \i Back             \i Alt+Left, Backspace                  \i Ctrl+[                   \i Alt+Left     \i Alt+Left                              \i (none)
    \row    \i Forward          \i Alt+Right, Shift+Backspace           \i Ctrl+]                   \i Alt+Right    \i Alt+Right                             \i (none)
    \row    \i Refresh          \i F5                                   \i F5                       \i F5           \i Ctrl+R, F5                            \i (none)
    \row    \i ZoomIn           \i Ctrl+Plus                            \i Ctrl+Plus                \i Ctrl+Plus    \i Ctrl+Plus                             \i (none)
    \row    \i ZoomOut          \i Ctrl+Minus                           \i Ctrl+Minus               \i Ctrl+Minus   \i Ctrl+Minus                            \i (none)
    \row    \i Print            \i Ctrl+P                               \i Ctrl+P                   \i Ctrl+P       \i Ctrl+P                                \i (none)
    \row    \i AddTab           \i Ctrl+T                               \i Ctrl+T                   \i Ctrl+Shift+N, Ctrl+T \i Ctrl+T                        \i (none)
    \row    \i NextChild        \i Ctrl+Tab, Forward, Ctrl+F6           \i Ctrl+}, Forward, Ctrl+Tab \i Ctrl+Tab, Forward, Ctrl+Comma \i Ctrl+Tab, Forward   \i (none)
    \row    \i PreviousChild    \i Ctrl+Shift+Tab, Back, Ctrl+Shift+F6  \i Ctrl+{, Back, Ctrl+Shift+Tab \i Ctrl+Shift+Tab, Back, Ctrl+Period \i Ctrl+Shift+Tab, Back \i (none)
    \row    \i Find             \i Ctrl+F                               \i Ctrl+F                   \i Ctrl+F         \i Ctrl+F                              \i (none)
    \row    \i FindNext         \i F3, Ctrl+G                           \i Ctrl+G                   \i F3             \i Ctrl+G, F3                          \i (none)
    \row    \i FindPrevious     \i Shift+F3, Ctrl+Shift+G               \i Ctrl+Shift+G             \i Shift+F3       \i Ctrl+Shift+G, Shift+F3              \i (none)
    \row    \i Replace          \i Ctrl+H                               \i (none)                   \i Ctrl+R         \i Ctrl+H                              \i (none)
    \row    \i SelectAll        \i Ctrl+A                               \i Ctrl+A                   \i Ctrl+A         \i Ctrl+A                              \i (none)
    \row    \i Bold             \i Ctrl+B                               \i Ctrl+B                   \i Ctrl+B         \i Ctrl+B                              \i (none)
    \row    \i Italic           \i Ctrl+I                               \i Ctrl+I                   \i Ctrl+I         \i Ctrl+I                              \i (none)
    \row    \i Underline        \i Ctrl+U                               \i Ctrl+U                   \i Ctrl+U         \i Ctrl+U                              \i (none)
    \row    \i MoveToNextChar       \i Right                            \i Right                    \i Right          \i Right                               \i Right
    \row    \i MoveToPreviousChar   \i Left                             \i Left                     \i Left           \i Left                                \i Left
    \row    \i MoveToNextWord       \i Ctrl+Right                       \i Alt+Right                \i Ctrl+Right     \i Ctrl+Right                          \i Ctrl+Right
    \row    \i MoveToPreviousWord   \i Ctrl+Left                        \i Alt+Left                 \i Ctrl+Left      \i Ctrl+Left                           \i Ctrl+Left
    \row    \i MoveToNextLine       \i Down                             \i Down                     \i Down           \i Down                                \i Down
    \row    \i MoveToPreviousLine   \i Up                               \i Up                       \i Up             \i Up                                  \i Up
    \row    \i MoveToNextPage       \i PgDown                           \i PgDown, Alt+PgDown, Meta+Down, Meta+PgDown\i PgDown \i PgDown                     \i PgDown
    \row    \i MoveToPreviousPage   \i PgUp                             \i PgUp, Alt+PgUp, Meta+Up, Meta+PgUp        \i PgUp   \i PgUp                       \i PgUp
    \row    \i MoveToStartOfLine    \i Home                             \i Ctrl+Left, Meta+Left   \i Home            \i Home                                 \i Home
    \row    \i MoveToEndOfLine      \i End                              \i Ctrl+Right, Meta+Right \i End             \i End                                  \i End
    \row    \i MoveToStartOfBlock   \i (none)                           \i Alt+Up, Meta+A         \i (none)          \i (none)                               \i (none)
    \row    \i MoveToEndOfBlock     \i (none)                           \i Alt+Down, Meta+E       \i (none)          \i (none)                               \i (none)
    \row    \i MoveToStartOfDocument\i Ctrl+Home                        \i Ctrl+Up, Home          \i Ctrl+Home       \i Ctrl+Home                            \i Ctrl+Home
    \row    \i MoveToEndOfDocument  \i Ctrl+End                         \i Ctrl+Down, End         \i Ctrl+End        \i Ctrl+End                             \i Ctrl+End
    \row    \i SelectNextChar       \i Shift+Right                      \i Shift+Right            \i Shift+Right     \i Shift+Right                          \i Shift+Right
    \row    \i SelectPreviousChar   \i Shift+Left                       \i Shift+Left             \i Shift+Left      \i Shift+Left                           \i Shift+Left
    \row    \i SelectNextWord       \i Ctrl+Shift+Right                 \i Alt+Shift+Right        \i Ctrl+Shift+Right \i Ctrl+Shift+Right                    \i Ctrl+Shift+Right
    \row    \i SelectPreviousWord   \i Ctrl+Shift+Left                  \i Alt+Shift+Left         \i Ctrl+Shift+Left \i Ctrl+Shift+Left                      \i Ctrl+Shift+Left
    \row    \i SelectNextLine       \i Shift+Down                       \i Shift+Down             \i Shift+Down     \i Shift+Down                            \i Shift+Down
    \row    \i SelectPreviousLine   \i Shift+Up                         \i Shift+Up               \i Shift+Up       \i Shift+Up                              \i Shift+Up
    \row    \i SelectNextPage       \i Shift+PgDown                     \i Shift+PgDown           \i Shift+PgDown   \i Shift+PgDown                          \i Shift+PgDown
    \row    \i SelectPreviousPage   \i Shift+PgUp                       \i Shift+PgUp             \i Shift+PgUp     \i Shift+PgUp                            \i Shift+PgUp
    \row    \i SelectStartOfLine    \i Shift+Home                       \i Ctrl+Shift+Left        \i Shift+Home     \i Shift+Home                            \i Shift+Home
    \row    \i SelectEndOfLine      \i Shift+End                        \i Ctrl+Shift+Right       \i Shift+End      \i Shift+End                             \i Shift+End
    \row    \i SelectStartOfBlock   \i (none)                           \i Alt+Shift+Up, Meta+Shift+A \i (none)     \i (none)                                \i (none)
    \row    \i SelectEndOfBlock     \i (none)                           \i Alt+Shift+Down, Meta+Shift+E \i (none)   \i (none)                                \i (none)
    \row    \i SelectStartOfDocument\i Ctrl+Shift+Home                  \i Ctrl+Shift+Up, Shift+Home          \i Ctrl+Shift+Home\i Ctrl+Shift+Home           \i Ctrl+Shift+Home
    \row    \i SelectEndOfDocument  \i Ctrl+Shift+End                   \i Ctrl+Shift+Down, Shift+End        \i Ctrl+Shift+End \i Ctrl+Shift+End             \i Ctrl+Shift+End
    \row    \i DeleteStartOfWord    \i Ctrl+Backspace                   \i Alt+Backspace          \i Ctrl+Backspace \i Ctrl+Backspace                        \i (none)
    \row    \i DeleteEndOfWord      \i Ctrl+Del                         \i (none)                 \i Ctrl+Del       \i Ctrl+Del                              \i (none)
    \row    \i DeleteEndOfLine      \i (none)                           \i (none)                 \i Ctrl+K         \i Ctrl+K                                \i (none)
    \row    \i InsertParagraphSeparator     \i Enter                    \i Enter                  \i Enter          \i Enter                                 \i (none)
    \row    \i InsertLineSeparator          \i Shift+Enter              \i Meta+Enter             \i Shift+Enter    \i Shift+Enter                           \i (none)
    \endtable

    Note that, since the key sequences used for the standard shortcuts differ
    between platforms, you still need to test your shortcuts on each platform
    to ensure that you do not unintentionally assign the same key sequence to
    many actions.

    \section1 Keyboard Layout Issues

    Many key sequence specifications are chosen by developers based on the
    layout of certain types of keyboard, rather than choosing keys that
    represent the first letter of an action's name, such as \key{Ctrl S}
    ("Ctrl+S") or \key{Ctrl C} ("Ctrl+C").
    Additionally, because certain symbols can only be entered with the
    help of modifier keys on certain keyboard layouts, key sequences intended
    for use with one keyboard layout may map to a different key, map to no
    keys at all, or require an additional modifier key to be used on
    different keyboard layouts.

    For example, the shortcuts, \key{Ctrl plus} and \key{Ctrl minus}, are often
    used as shortcuts for zoom operations in graphics applications, and these
    may be specified as "Ctrl++" and "Ctrl+-" respectively. However, the way
    these shortcuts are specified and interpreted depends on the keyboard layout.
    Users of Norwegian keyboards will note that the \key{+} and \key{-} keys
    are not adjacent on the keyboard, but will still be able to activate both
    shortcuts without needing to press the \key{Shift} key. However, users
    with British keyboards will need to hold down the \key{Shift} key
    to enter the \key{+} symbol, making the shortcut effectively the same as
    "Ctrl+Shift+=".

    Although some developers might resort to fully specifying all the modifiers
    they use on their keyboards to activate a shortcut, this will also result
    in unexpected behavior for users of different keyboard layouts.

    For example, a developer using a British keyboard may decide to specify
    "Ctrl+Shift+=" as the key sequence in order to create a shortcut that
    coincidentally behaves in the same way as \key{Ctrl plus}. However, the
    \key{=} key needs to be accessed using the \key{Shift} key on Norwegian
    keyboard, making the required shortcut effectively \key{Ctrl Shift Shift =}
    (an impossible key combination).

    As a result, both human-readable strings and hard-coded key codes
    can both be problematic to use when specifying a key sequence that
    can be used on a variety of different keyboard layouts. Only the
    use of \l{QKeySequence::StandardKey} {standard shortcuts}
    guarantees that the user will be able to use the shortcuts that
    the developer intended.

    Despite this, we can address this issue by ensuring that human-readable
    strings are used, making it possible for translations of key sequences to
    be made for users of different languages. This approach will be successful
    for users whose keyboards have the most typical layout for the language
    they are using.

    \section1 GNU Emacs Style Key Sequences

    Key sequences similar to those used in \l{GNU Emacs}, allowing up to four
    key codes, can be created by using the multiple argument constructor,
    or by passing a human-readable string of comma-separated key sequences.

    For example, the key sequence, \key{Ctrl X} followed by \key{Ctrl C}, can
    be specified using either of the following ways:

    \snippet doc/src/snippets/code/src_gui_kernel_qkeysequence.cpp 1

    \warning A QApplication instance must have been constructed before a
             QKeySequence is created; otherwise, your application may crash.

    \sa QShortcut
*/

/*!
    \enum QKeySequence::SequenceMatch

    \value NoMatch The key sequences are different; not even partially
    matching.
    \value PartialMatch The key sequences match partially, but are not
    the same.
    \value ExactMatch The key sequences are the same.
    \omitvalue Identical
*/

/*!
    \enum QKeySequence::SequenceFormat

    \value NativeText The key sequence as a platform specific string.
    This means that it will be shown translated and on the Mac it will
    resemble a key sequence from the menu bar. This enum is best used when you
    want to display the string to the user.

    \value PortableText The key sequence is given in a "portable" format,
    suitable for reading and writing to a file. In many cases, it will look
    similar to the native text on Windows and X11.
*/

static const struct {
    int key;
    const char* name;
} keyname[] = {
    //: This and all following "incomprehensible" strings in QShortcut context
    //: are key names. Please use the localized names appearing on actual
    //: keyboards or whatever is commonly used.
    { Qt::Key_Space,        QT_TRANSLATE_NOOP("QShortcut", "Space") },
    { Qt::Key_Escape,       QT_TRANSLATE_NOOP("QShortcut", "Esc") },
    { Qt::Key_Tab,          QT_TRANSLATE_NOOP("QShortcut", "Tab") },
    { Qt::Key_Backtab,      QT_TRANSLATE_NOOP("QShortcut", "Backtab") },
    { Qt::Key_Backspace,    QT_TRANSLATE_NOOP("QShortcut", "Backspace") },
    { Qt::Key_Return,       QT_TRANSLATE_NOOP("QShortcut", "Return") },
    { Qt::Key_Enter,        QT_TRANSLATE_NOOP("QShortcut", "Enter") },
    { Qt::Key_Insert,       QT_TRANSLATE_NOOP("QShortcut", "Ins") },
    { Qt::Key_Delete,       QT_TRANSLATE_NOOP("QShortcut", "Del") },
    { Qt::Key_Pause,        QT_TRANSLATE_NOOP("QShortcut", "Pause") },
    { Qt::Key_Print,        QT_TRANSLATE_NOOP("QShortcut", "Print") },
    { Qt::Key_SysReq,       QT_TRANSLATE_NOOP("QShortcut", "SysReq") },
    { Qt::Key_Home,         QT_TRANSLATE_NOOP("QShortcut", "Home") },
    { Qt::Key_End,          QT_TRANSLATE_NOOP("QShortcut", "End") },
    { Qt::Key_Left,         QT_TRANSLATE_NOOP("QShortcut", "Left") },
    { Qt::Key_Up,           QT_TRANSLATE_NOOP("QShortcut", "Up") },
    { Qt::Key_Right,        QT_TRANSLATE_NOOP("QShortcut", "Right") },
    { Qt::Key_Down,         QT_TRANSLATE_NOOP("QShortcut", "Down") },
    { Qt::Key_PageUp,       QT_TRANSLATE_NOOP("QShortcut", "PgUp") },
    { Qt::Key_PageDown,     QT_TRANSLATE_NOOP("QShortcut", "PgDown") },
    { Qt::Key_CapsLock,     QT_TRANSLATE_NOOP("QShortcut", "CapsLock") },
    { Qt::Key_NumLock,      QT_TRANSLATE_NOOP("QShortcut", "NumLock") },
    { Qt::Key_ScrollLock,   QT_TRANSLATE_NOOP("QShortcut", "ScrollLock") },
    { Qt::Key_Menu,         QT_TRANSLATE_NOOP("QShortcut", "Menu") },
    { Qt::Key_Help,         QT_TRANSLATE_NOOP("QShortcut", "Help") },

    // Special keys
    // Includes multimedia, launcher, lan keys ( bluetooth, wireless )
    // window navigation
    { Qt::Key_Back,                       QT_TRANSLATE_NOOP("QShortcut", "Back") },
    { Qt::Key_Forward,                    QT_TRANSLATE_NOOP("QShortcut", "Forward") },
    { Qt::Key_Stop,                       QT_TRANSLATE_NOOP("QShortcut", "Stop") },
    { Qt::Key_Refresh,                    QT_TRANSLATE_NOOP("QShortcut", "Refresh") },
    { Qt::Key_VolumeDown,                 QT_TRANSLATE_NOOP("QShortcut", "Volume Down") },
    { Qt::Key_VolumeMute,                 QT_TRANSLATE_NOOP("QShortcut", "Volume Mute") },
    { Qt::Key_VolumeUp,                   QT_TRANSLATE_NOOP("QShortcut", "Volume Up") },
    { Qt::Key_BassBoost,                  QT_TRANSLATE_NOOP("QShortcut", "Bass Boost") },
    { Qt::Key_BassUp,                     QT_TRANSLATE_NOOP("QShortcut", "Bass Up") },
    { Qt::Key_BassDown,                   QT_TRANSLATE_NOOP("QShortcut", "Bass Down") },
    { Qt::Key_TrebleUp,                   QT_TRANSLATE_NOOP("QShortcut", "Treble Up") },
    { Qt::Key_TrebleDown,                 QT_TRANSLATE_NOOP("QShortcut", "Treble Down") },
    { Qt::Key_MediaPlay,                  QT_TRANSLATE_NOOP("QShortcut", "Media Play") },
    { Qt::Key_MediaStop,                  QT_TRANSLATE_NOOP("QShortcut", "Media Stop") },
    { Qt::Key_MediaPrevious,              QT_TRANSLATE_NOOP("QShortcut", "Media Previous") },
    { Qt::Key_MediaNext,                  QT_TRANSLATE_NOOP("QShortcut", "Media Next") },
    { Qt::Key_MediaRecord,                QT_TRANSLATE_NOOP("QShortcut", "Media Record") },
    //: Media player pause button
    { Qt::Key_MediaPause,                 QT_TRANSLATE_NOOP("QShortcut", "Media Pause") },
    //: Media player button to toggle between playing and paused
    { Qt::Key_MediaTogglePlayPause,       QT_TRANSLATE_NOOP("QShortcut", "Toggle Media Play/Pause") },
    { Qt::Key_HomePage,                   QT_TRANSLATE_NOOP("QShortcut", "Home Page") },
    { Qt::Key_Favorites,                  QT_TRANSLATE_NOOP("QShortcut", "Favorites") },
    { Qt::Key_Search,                     QT_TRANSLATE_NOOP("QShortcut", "Search") },
    { Qt::Key_Standby,                    QT_TRANSLATE_NOOP("QShortcut", "Standby") },
    { Qt::Key_OpenUrl,                    QT_TRANSLATE_NOOP("QShortcut", "Open URL") },
    { Qt::Key_LaunchMail,                 QT_TRANSLATE_NOOP("QShortcut", "Launch Mail") },
    { Qt::Key_LaunchMedia,                QT_TRANSLATE_NOOP("QShortcut", "Launch Media") },
    { Qt::Key_Launch0,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (0)") },
    { Qt::Key_Launch1,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (1)") },
    { Qt::Key_Launch2,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (2)") },
    { Qt::Key_Launch3,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (3)") },
    { Qt::Key_Launch4,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (4)") },
    { Qt::Key_Launch5,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (5)") },
    { Qt::Key_Launch6,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (6)") },
    { Qt::Key_Launch7,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (7)") },
    { Qt::Key_Launch8,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (8)") },
    { Qt::Key_Launch9,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (9)") },
    { Qt::Key_LaunchA,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (A)") },
    { Qt::Key_LaunchB,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (B)") },
    { Qt::Key_LaunchC,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (C)") },
    { Qt::Key_LaunchD,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (D)") },
    { Qt::Key_LaunchE,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (E)") },
    { Qt::Key_LaunchF,                    QT_TRANSLATE_NOOP("QShortcut", "Launch (F)") },
    { Qt::Key_MonBrightnessUp,            QT_TRANSLATE_NOOP("QShortcut", "Monitor Brightness Up") },
    { Qt::Key_MonBrightnessDown,          QT_TRANSLATE_NOOP("QShortcut", "Monitor Brightness Down") },
    { Qt::Key_KeyboardLightOnOff,         QT_TRANSLATE_NOOP("QShortcut", "Keyboard Light On/Off") },
    { Qt::Key_KeyboardBrightnessUp,       QT_TRANSLATE_NOOP("QShortcut", "Keyboard Brightness Up") },
    { Qt::Key_KeyboardBrightnessDown,     QT_TRANSLATE_NOOP("QShortcut", "Keyboard Brightness Down") },
    { Qt::Key_PowerOff,                   QT_TRANSLATE_NOOP("QShortcut", "Power Off") },
    { Qt::Key_WakeUp,                     QT_TRANSLATE_NOOP("QShortcut", "Wake Up") },
    { Qt::Key_Eject,                      QT_TRANSLATE_NOOP("QShortcut", "Eject") },
    { Qt::Key_ScreenSaver,                QT_TRANSLATE_NOOP("QShortcut", "Screensaver") },
    { Qt::Key_WWW,                        QT_TRANSLATE_NOOP("QShortcut", "WWW") },
    { Qt::Key_Sleep,                      QT_TRANSLATE_NOOP("QShortcut", "Sleep") },
    { Qt::Key_LightBulb,                  QT_TRANSLATE_NOOP("QShortcut", "LightBulb") },
    { Qt::Key_Shop,                       QT_TRANSLATE_NOOP("QShortcut", "Shop") },
    { Qt::Key_History,                    QT_TRANSLATE_NOOP("QShortcut", "History") },
    { Qt::Key_AddFavorite,                QT_TRANSLATE_NOOP("QShortcut", "Add Favorite") },
    { Qt::Key_HotLinks,                   QT_TRANSLATE_NOOP("QShortcut", "Hot Links") },
    { Qt::Key_BrightnessAdjust,           QT_TRANSLATE_NOOP("QShortcut", "Adjust Brightness") },
    { Qt::Key_Finance,                    QT_TRANSLATE_NOOP("QShortcut", "Finance") },
    { Qt::Key_Community,                  QT_TRANSLATE_NOOP("QShortcut", "Community") },
    { Qt::Key_AudioRewind,                QT_TRANSLATE_NOOP("QShortcut", "Audio Rewind") },
    { Qt::Key_BackForward,                QT_TRANSLATE_NOOP("QShortcut", "Back Forward") },
    { Qt::Key_ApplicationLeft,            QT_TRANSLATE_NOOP("QShortcut", "Application Left") },
    { Qt::Key_ApplicationRight,           QT_TRANSLATE_NOOP("QShortcut", "Application Right") },
    { Qt::Key_Book,                       QT_TRANSLATE_NOOP("QShortcut", "Book") },
    { Qt::Key_CD,                         QT_TRANSLATE_NOOP("QShortcut", "CD") },
    { Qt::Key_Calculator,                 QT_TRANSLATE_NOOP("QShortcut", "Calculator") },
    { Qt::Key_Clear,                      QT_TRANSLATE_NOOP("QShortcut", "Clear") },
    { Qt::Key_ClearGrab,                  QT_TRANSLATE_NOOP("QShortcut", "Clear Grab") },
    { Qt::Key_Close,                      QT_TRANSLATE_NOOP("QShortcut", "Close") },
    { Qt::Key_Copy,                       QT_TRANSLATE_NOOP("QShortcut", "Copy") },
    { Qt::Key_Cut,                        QT_TRANSLATE_NOOP("QShortcut", "Cut") },
    { Qt::Key_Display,                    QT_TRANSLATE_NOOP("QShortcut", "Display") },
    { Qt::Key_DOS,                        QT_TRANSLATE_NOOP("QShortcut", "DOS") },
    { Qt::Key_Documents,                  QT_TRANSLATE_NOOP("QShortcut", "Documents") },
    { Qt::Key_Excel,                      QT_TRANSLATE_NOOP("QShortcut", "Spreadsheet") },
    { Qt::Key_Explorer,                   QT_TRANSLATE_NOOP("QShortcut", "Browser") },
    { Qt::Key_Game,                       QT_TRANSLATE_NOOP("QShortcut", "Game") },
    { Qt::Key_Go,                         QT_TRANSLATE_NOOP("QShortcut", "Go") },
    { Qt::Key_iTouch,                     QT_TRANSLATE_NOOP("QShortcut", "iTouch") },
    { Qt::Key_LogOff,                     QT_TRANSLATE_NOOP("QShortcut", "Logoff") },
    { Qt::Key_Market,                     QT_TRANSLATE_NOOP("QShortcut", "Market") },
    { Qt::Key_Meeting,                    QT_TRANSLATE_NOOP("QShortcut", "Meeting") },
    { Qt::Key_MenuKB,                     QT_TRANSLATE_NOOP("QShortcut", "Keyboard Menu") },
    { Qt::Key_MenuPB,                     QT_TRANSLATE_NOOP("QShortcut", "Menu PB") },
    { Qt::Key_MySites,                    QT_TRANSLATE_NOOP("QShortcut", "My Sites") },
    { Qt::Key_News,                       QT_TRANSLATE_NOOP("QShortcut", "News") },
    { Qt::Key_OfficeHome,                 QT_TRANSLATE_NOOP("QShortcut", "Home Office") },
    { Qt::Key_Option,                     QT_TRANSLATE_NOOP("QShortcut", "Option") },
    { Qt::Key_Paste,                      QT_TRANSLATE_NOOP("QShortcut", "Paste") },
    { Qt::Key_Phone,                      QT_TRANSLATE_NOOP("QShortcut", "Phone") },
    { Qt::Key_Reply,                      QT_TRANSLATE_NOOP("QShortcut", "Reply") },
    { Qt::Key_Reload,                     QT_TRANSLATE_NOOP("QShortcut", "Reload") },
    { Qt::Key_RotateWindows,              QT_TRANSLATE_NOOP("QShortcut", "Rotate Windows") },
    { Qt::Key_RotationPB,                 QT_TRANSLATE_NOOP("QShortcut", "Rotation PB") },
    { Qt::Key_RotationKB,                 QT_TRANSLATE_NOOP("QShortcut", "Rotation KB") },
    { Qt::Key_Save,                       QT_TRANSLATE_NOOP("QShortcut", "Save") },
    { Qt::Key_Send,                       QT_TRANSLATE_NOOP("QShortcut", "Send") },
    { Qt::Key_Spell,                      QT_TRANSLATE_NOOP("QShortcut", "Spellchecker") },
    { Qt::Key_SplitScreen,                QT_TRANSLATE_NOOP("QShortcut", "Split Screen") },
    { Qt::Key_Support,                    QT_TRANSLATE_NOOP("QShortcut", "Support") },
    { Qt::Key_TaskPane,                   QT_TRANSLATE_NOOP("QShortcut", "Task Panel") },
    { Qt::Key_Terminal,                   QT_TRANSLATE_NOOP("QShortcut", "Terminal") },
    { Qt::Key_Tools,                      QT_TRANSLATE_NOOP("QShortcut", "Tools") },
    { Qt::Key_Travel,                     QT_TRANSLATE_NOOP("QShortcut", "Travel") },
    { Qt::Key_Video,                      QT_TRANSLATE_NOOP("QShortcut", "Video") },
    { Qt::Key_Word,                       QT_TRANSLATE_NOOP("QShortcut", "Word Processor") },
    { Qt::Key_Xfer,                       QT_TRANSLATE_NOOP("QShortcut", "XFer") },
    { Qt::Key_ZoomIn,                     QT_TRANSLATE_NOOP("QShortcut", "Zoom In") },
    { Qt::Key_ZoomOut,                    QT_TRANSLATE_NOOP("QShortcut", "Zoom Out") },
    { Qt::Key_Away,                       QT_TRANSLATE_NOOP("QShortcut", "Away") },
    { Qt::Key_Messenger,                  QT_TRANSLATE_NOOP("QShortcut", "Messenger") },
    { Qt::Key_WebCam,                     QT_TRANSLATE_NOOP("QShortcut", "WebCam") },
    { Qt::Key_MailForward,                QT_TRANSLATE_NOOP("QShortcut", "Mail Forward") },
    { Qt::Key_Pictures,                   QT_TRANSLATE_NOOP("QShortcut", "Pictures") },
    { Qt::Key_Music,                      QT_TRANSLATE_NOOP("QShortcut", "Music") },
    { Qt::Key_Battery,                    QT_TRANSLATE_NOOP("QShortcut", "Battery") },
    { Qt::Key_Bluetooth,                  QT_TRANSLATE_NOOP("QShortcut", "Bluetooth") },
    { Qt::Key_WLAN,                       QT_TRANSLATE_NOOP("QShortcut", "Wireless") },
    { Qt::Key_UWB,                        QT_TRANSLATE_NOOP("QShortcut", "Ultra Wide Band") },
    { Qt::Key_AudioForward,               QT_TRANSLATE_NOOP("QShortcut", "Audio Forward") },
    { Qt::Key_AudioRepeat,                QT_TRANSLATE_NOOP("QShortcut", "Audio Repeat") },
    { Qt::Key_AudioRandomPlay,            QT_TRANSLATE_NOOP("QShortcut", "Audio Random Play") },
    { Qt::Key_Subtitle,                   QT_TRANSLATE_NOOP("QShortcut", "Subtitle") },
    { Qt::Key_AudioCycleTrack,            QT_TRANSLATE_NOOP("QShortcut", "Audio Cycle Track") },
    { Qt::Key_Time,                       QT_TRANSLATE_NOOP("QShortcut", "Time") },
    { Qt::Key_Select,                     QT_TRANSLATE_NOOP("QShortcut", "Select") },
    { Qt::Key_View,                       QT_TRANSLATE_NOOP("QShortcut", "View") },
    { Qt::Key_TopMenu,                    QT_TRANSLATE_NOOP("QShortcut", "Top Menu") },
    { Qt::Key_Suspend,                    QT_TRANSLATE_NOOP("QShortcut", "Suspend") },
    { Qt::Key_Hibernate,                  QT_TRANSLATE_NOOP("QShortcut", "Hibernate") },

    // --------------------------------------------------------------
    // More consistent namings
    { Qt::Key_Print,        QT_TRANSLATE_NOOP("QShortcut", "Print Screen") },
    { Qt::Key_PageUp,       QT_TRANSLATE_NOOP("QShortcut", "Page Up") },
    { Qt::Key_PageDown,     QT_TRANSLATE_NOOP("QShortcut", "Page Down") },
    { Qt::Key_CapsLock,     QT_TRANSLATE_NOOP("QShortcut", "Caps Lock") },
    { Qt::Key_NumLock,      QT_TRANSLATE_NOOP("QShortcut", "Num Lock") },
    { Qt::Key_NumLock,      QT_TRANSLATE_NOOP("QShortcut", "Number Lock") },
    { Qt::Key_ScrollLock,   QT_TRANSLATE_NOOP("QShortcut", "Scroll Lock") },
    { Qt::Key_Insert,       QT_TRANSLATE_NOOP("QShortcut", "Insert") },
    { Qt::Key_Delete,       QT_TRANSLATE_NOOP("QShortcut", "Delete") },
    { Qt::Key_Escape,       QT_TRANSLATE_NOOP("QShortcut", "Escape") },
    { Qt::Key_SysReq,       QT_TRANSLATE_NOOP("QShortcut", "System Request") },

    // --------------------------------------------------------------
    // Keypad navigation keys
    { Qt::Key_Select,       QT_TRANSLATE_NOOP("QShortcut", "Select") },
    { Qt::Key_Yes,          QT_TRANSLATE_NOOP("QShortcut", "Yes") },
    { Qt::Key_No,           QT_TRANSLATE_NOOP("QShortcut", "No") },

    // --------------------------------------------------------------
    // Device keys
    { Qt::Key_Context1,         QT_TRANSLATE_NOOP("QShortcut", "Context1") },
    { Qt::Key_Context2,         QT_TRANSLATE_NOOP("QShortcut", "Context2") },
    { Qt::Key_Context3,         QT_TRANSLATE_NOOP("QShortcut", "Context3") },
    { Qt::Key_Context4,         QT_TRANSLATE_NOOP("QShortcut", "Context4") },
    //: Button to start a call (note: a separate button is used to end the call)
    { Qt::Key_Call,             QT_TRANSLATE_NOOP("QShortcut", "Call") },
    //: Button to end a call (note: a separate button is used to start the call)
    { Qt::Key_Hangup,           QT_TRANSLATE_NOOP("QShortcut", "Hangup") },
    //: Button that will hang up if we're in call, or make a call if we're not.
    { Qt::Key_ToggleCallHangup, QT_TRANSLATE_NOOP("QShortcut", "Toggle Call/Hangup") },
    { Qt::Key_Flip,             QT_TRANSLATE_NOOP("QShortcut", "Flip") },
    //: Button to trigger voice dialing
    { Qt::Key_VoiceDial,        QT_TRANSLATE_NOOP("QShortcut", "Voice Dial") },
    //: Button to redial the last number called
    { Qt::Key_LastNumberRedial, QT_TRANSLATE_NOOP("QShortcut", "Last Number Redial") },
    //: Button to trigger the camera shutter (take a picture)
    { Qt::Key_Camera,           QT_TRANSLATE_NOOP("QShortcut", "Camera Shutter") },
    //: Button to focus the camera
    { Qt::Key_CameraFocus,      QT_TRANSLATE_NOOP("QShortcut", "Camera Focus") },

    // --------------------------------------------------------------
    // Japanese keyboard support
    { Qt::Key_Kanji,            QT_TRANSLATE_NOOP("QShortcut", "Kanji") },
    { Qt::Key_Muhenkan,         QT_TRANSLATE_NOOP("QShortcut", "Muhenkan") },
    { Qt::Key_Henkan,           QT_TRANSLATE_NOOP("QShortcut", "Henkan") },
    { Qt::Key_Romaji,           QT_TRANSLATE_NOOP("QShortcut", "Romaji") },
    { Qt::Key_Hiragana,         QT_TRANSLATE_NOOP("QShortcut", "Hiragana") },
    { Qt::Key_Katakana,         QT_TRANSLATE_NOOP("QShortcut", "Katakana") },
    { Qt::Key_Hiragana_Katakana,QT_TRANSLATE_NOOP("QShortcut", "Hiragana Katakana") },
    { Qt::Key_Zenkaku,          QT_TRANSLATE_NOOP("QShortcut", "Zenkaku") },
    { Qt::Key_Hankaku,          QT_TRANSLATE_NOOP("QShortcut", "Hankaku") },
    { Qt::Key_Zenkaku_Hankaku,  QT_TRANSLATE_NOOP("QShortcut", "Zenkaku Hankaku") },
    { Qt::Key_Touroku,          QT_TRANSLATE_NOOP("QShortcut", "Touroku") },
    { Qt::Key_Massyo,           QT_TRANSLATE_NOOP("QShortcut", "Massyo") },
    { Qt::Key_Kana_Lock,        QT_TRANSLATE_NOOP("QShortcut", "Kana Lock") },
    { Qt::Key_Kana_Shift,       QT_TRANSLATE_NOOP("QShortcut", "Kana Shift") },
    { Qt::Key_Eisu_Shift,       QT_TRANSLATE_NOOP("QShortcut", "Eisu Shift") },
    { Qt::Key_Eisu_toggle,      QT_TRANSLATE_NOOP("QShortcut", "Eisu toggle") },
    { Qt::Key_Codeinput,        QT_TRANSLATE_NOOP("QShortcut", "Code input") },
    { Qt::Key_MultipleCandidate,QT_TRANSLATE_NOOP("QShortcut", "Multiple Candidate") },
    { Qt::Key_PreviousCandidate,QT_TRANSLATE_NOOP("QShortcut", "Previous Candidate") },

    // --------------------------------------------------------------
    // Korean keyboard support
    { Qt::Key_Hangul,          QT_TRANSLATE_NOOP("QShortcut", "Hangul") },
    { Qt::Key_Hangul_Start,    QT_TRANSLATE_NOOP("QShortcut", "Hangul Start") },
    { Qt::Key_Hangul_End,      QT_TRANSLATE_NOOP("QShortcut", "Hangul End") },
    { Qt::Key_Hangul_Hanja,    QT_TRANSLATE_NOOP("QShortcut", "Hangul Hanja") },
    { Qt::Key_Hangul_Jamo,     QT_TRANSLATE_NOOP("QShortcut", "Hangul Jamo") },
    { Qt::Key_Hangul_Romaja,   QT_TRANSLATE_NOOP("QShortcut", "Hangul Romaja") },
    { Qt::Key_Hangul_Jeonja,   QT_TRANSLATE_NOOP("QShortcut", "Hangul Jeonja") },
    { Qt::Key_Hangul_Banja,    QT_TRANSLATE_NOOP("QShortcut", "Hangul Banja") },
    { Qt::Key_Hangul_PreHanja, QT_TRANSLATE_NOOP("QShortcut", "Hangul PreHanja") },
    { Qt::Key_Hangul_PostHanja,QT_TRANSLATE_NOOP("QShortcut", "Hangul PostHanja") },
    { Qt::Key_Hangul_Special,  QT_TRANSLATE_NOOP("QShortcut", "Hangul Special") },

    { 0, 0 }
};

//Table of key bindings. It must be sorted on key sequence.
//A priority of 1 indicates that this is the primary key binding when multiple are defined.

const QKeyBinding QKeySequencePrivate::keyBindings[] = {
//   StandardKey                            Priority    Key Sequence                            Platforms
    {QKeySequence::Back,                    0,          Qt::Key_Backspace,                      QApplicationPrivate::KB_Win},
    {QKeySequence::InsertParagraphSeparator,0,          Qt::Key_Return,                         QApplicationPrivate::KB_All},
    {QKeySequence::InsertParagraphSeparator,0,          Qt::Key_Enter,                          QApplicationPrivate::KB_All},
    {QKeySequence::Delete,                  1,          Qt::Key_Delete,                         QApplicationPrivate::KB_All},
    {QKeySequence::MoveToStartOfLine,       0,          Qt::Key_Home,                           QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::MoveToStartOfDocument,   0,          Qt::Key_Home,                           QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToEndOfLine,         0,          Qt::Key_End,                            QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::MoveToEndOfDocument,     0,          Qt::Key_End,                            QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToPreviousChar,      0,          Qt::Key_Left,                           QApplicationPrivate::KB_All},
    {QKeySequence::MoveToPreviousLine,      0,          Qt::Key_Up,                             QApplicationPrivate::KB_All},
    {QKeySequence::MoveToNextChar,          0,          Qt::Key_Right,                          QApplicationPrivate::KB_All},
    {QKeySequence::MoveToNextLine,          0,          Qt::Key_Down,                           QApplicationPrivate::KB_All},
    {QKeySequence::MoveToPreviousPage,      1,          Qt::Key_PageUp,                         QApplicationPrivate::KB_All},
    {QKeySequence::MoveToNextPage,          1,          Qt::Key_PageDown,                       QApplicationPrivate::KB_All},
    {QKeySequence::HelpContents,            0,          Qt::Key_F1,                             QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11},
    {QKeySequence::HelpContents,            0,          Qt::Key_F2,                             QApplicationPrivate::KB_S60},
    {QKeySequence::FindNext,                0,          Qt::Key_F3,                             QApplicationPrivate::KB_X11},
    {QKeySequence::FindNext,                1,          Qt::Key_F3,                             QApplicationPrivate::KB_Win},
    {QKeySequence::Refresh,                 0,          Qt::Key_F5,                             QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11},
    {QKeySequence::Undo,                    0,          Qt::Key_F14,                            QApplicationPrivate::KB_X11}, //Undo on sun keyboards
    {QKeySequence::Copy,                    0,          Qt::Key_F16,                            QApplicationPrivate::KB_X11}, //Copy on sun keyboards
    {QKeySequence::Paste,                   0,          Qt::Key_F18,                            QApplicationPrivate::KB_X11}, //Paste on sun keyboards      
    {QKeySequence::Cut,                     0,          Qt::Key_F20,                            QApplicationPrivate::KB_X11}, //Cut on sun keyboards
    {QKeySequence::PreviousChild,           0,          Qt::Key_Back,                           QApplicationPrivate::KB_All},
    {QKeySequence::NextChild,               0,          Qt::Key_Forward,                        QApplicationPrivate::KB_All}, 
    {QKeySequence::Forward,                 0,          Qt::SHIFT | Qt::Key_Backspace,          QApplicationPrivate::KB_Win},
    {QKeySequence::Delete,                  0,          Qt::SHIFT | Qt::Key_Backspace,          QApplicationPrivate::KB_S60},
    {QKeySequence::InsertLineSeparator,     0,          Qt::SHIFT | Qt::Key_Return,             QApplicationPrivate::KB_All},
    {QKeySequence::InsertLineSeparator,     0,          Qt::SHIFT | Qt::Key_Enter,              QApplicationPrivate::KB_All},
    {QKeySequence::Paste,                   0,          Qt::SHIFT | Qt::Key_Insert,             QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11}, 
    {QKeySequence::Cut,                     0,          Qt::SHIFT | Qt::Key_Delete,             QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11}, //## Check if this should work on mac
    {QKeySequence::SelectStartOfLine,       0,          Qt::SHIFT | Qt::Key_Home,               QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::SelectStartOfDocument,   0,          Qt::SHIFT | Qt::Key_Home,               QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectEndOfLine,         0,          Qt::SHIFT | Qt::Key_End,                QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::SelectEndOfDocument,     0,          Qt::SHIFT | Qt::Key_End,                QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectPreviousChar,      0,          Qt::SHIFT | Qt::Key_Left,               QApplicationPrivate::KB_All},
    {QKeySequence::SelectPreviousLine,      0,          Qt::SHIFT | Qt::Key_Up,                 QApplicationPrivate::KB_All},
    {QKeySequence::SelectNextChar,          0,          Qt::SHIFT | Qt::Key_Right,              QApplicationPrivate::KB_All},
    {QKeySequence::SelectNextLine,          0,          Qt::SHIFT | Qt::Key_Down,               QApplicationPrivate::KB_All},
    {QKeySequence::SelectPreviousPage,      0,          Qt::SHIFT | Qt::Key_PageUp,             QApplicationPrivate::KB_All},
    {QKeySequence::SelectNextPage,          0,          Qt::SHIFT | Qt::Key_PageDown,           QApplicationPrivate::KB_All},
    {QKeySequence::WhatsThis,               1,          Qt::SHIFT | Qt::Key_F1,                 QApplicationPrivate::KB_All},
    {QKeySequence::FindPrevious,            0,          Qt::SHIFT | Qt::Key_F3,                 QApplicationPrivate::KB_X11},
    {QKeySequence::FindPrevious,            1,          Qt::SHIFT | Qt::Key_F3,                 QApplicationPrivate::KB_Win},
    {QKeySequence::ZoomIn,                  1,          Qt::CTRL | Qt::Key_Plus,                QApplicationPrivate::KB_All},
    {QKeySequence::NextChild,               0,          Qt::CTRL | Qt::Key_Comma,               QApplicationPrivate::KB_KDE},
    {QKeySequence::Preferences,             0,          Qt::CTRL | Qt::Key_Comma,               QApplicationPrivate::KB_Mac},
    {QKeySequence::ZoomOut,                 1,          Qt::CTRL | Qt::Key_Minus,               QApplicationPrivate::KB_All},
    {QKeySequence::PreviousChild,           0,          Qt::CTRL | Qt::Key_Period,              QApplicationPrivate::KB_KDE},
    {QKeySequence::HelpContents,            1,          Qt::CTRL | Qt::Key_Question,            QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectAll,               1,          Qt::CTRL | Qt::Key_A,                   QApplicationPrivate::KB_All},
    {QKeySequence::Bold,                    1,          Qt::CTRL | Qt::Key_B,                   QApplicationPrivate::KB_All},
    {QKeySequence::Copy,                    1,          Qt::CTRL | Qt::Key_C,                   QApplicationPrivate::KB_All},
    {QKeySequence::Delete,                  0,          Qt::CTRL | Qt::Key_D,                   QApplicationPrivate::KB_X11}, //emacs (line edit only)
    {QKeySequence::Find,                    0,          Qt::CTRL | Qt::Key_F,                   QApplicationPrivate::KB_All},
    {QKeySequence::FindNext,                1,          Qt::CTRL | Qt::Key_G,                   QApplicationPrivate::KB_Gnome | QApplicationPrivate::KB_Mac},
    {QKeySequence::FindNext,                0,          Qt::CTRL | Qt::Key_G,                   QApplicationPrivate::KB_Win},
    {QKeySequence::Replace,                 0,          Qt::CTRL | Qt::Key_H,                   QApplicationPrivate::KB_Win}, 
    {QKeySequence::Replace,                 0,          Qt::CTRL | Qt::Key_H,                   QApplicationPrivate::KB_Gnome}, 
    {QKeySequence::Italic,                  0,          Qt::CTRL | Qt::Key_I,                   QApplicationPrivate::KB_All}, 
    {QKeySequence::DeleteEndOfLine,         0,          Qt::CTRL | Qt::Key_K,                   QApplicationPrivate::KB_X11}, //emacs (line edit only)
    {QKeySequence::New,                     1,          Qt::CTRL | Qt::Key_N,                   QApplicationPrivate::KB_All},
    {QKeySequence::Open,                    1,          Qt::CTRL | Qt::Key_O,                   QApplicationPrivate::KB_All},
    {QKeySequence::Print,                   1,          Qt::CTRL | Qt::Key_P,                   QApplicationPrivate::KB_All},
    {QKeySequence::Quit,                    0,          Qt::CTRL | Qt::Key_Q,                   QApplicationPrivate::KB_Gnome | QApplicationPrivate::KB_KDE | QApplicationPrivate::KB_Mac},
    {QKeySequence::Refresh,                 1,          Qt::CTRL | Qt::Key_R,                   QApplicationPrivate::KB_Gnome | QApplicationPrivate::KB_Mac},
    {QKeySequence::Replace,                 0,          Qt::CTRL | Qt::Key_R,                   QApplicationPrivate::KB_KDE},
    {QKeySequence::Save,                    1,          Qt::CTRL | Qt::Key_S,                   QApplicationPrivate::KB_All},
    {QKeySequence::AddTab,                  0,          Qt::CTRL | Qt::Key_T,                   QApplicationPrivate::KB_All},
    {QKeySequence::Underline,               1,          Qt::CTRL | Qt::Key_U,                   QApplicationPrivate::KB_All}, 
    {QKeySequence::Paste,                   1,          Qt::CTRL | Qt::Key_V,                   QApplicationPrivate::KB_All},
    {QKeySequence::Close,                   0,          Qt::CTRL | Qt::Key_W,                   QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11},
    {QKeySequence::Close,                   1,          Qt::CTRL | Qt::Key_W,                   QApplicationPrivate::KB_Mac},
    {QKeySequence::Cut,                     1,          Qt::CTRL | Qt::Key_X,                   QApplicationPrivate::KB_All},
    {QKeySequence::Redo,                    1,          Qt::CTRL | Qt::Key_Y,                   QApplicationPrivate::KB_Win | QApplicationPrivate::KB_S60},
    {QKeySequence::Undo,                    1,          Qt::CTRL | Qt::Key_Z,                   QApplicationPrivate::KB_All},
    {QKeySequence::Back,                    1,          Qt::CTRL | Qt::Key_BracketLeft,         QApplicationPrivate::KB_Mac},
    {QKeySequence::Forward,                 1,          Qt::CTRL | Qt::Key_BracketRight,        QApplicationPrivate::KB_Mac},
    {QKeySequence::PreviousChild,           1,          Qt::CTRL | Qt::Key_BraceLeft,           QApplicationPrivate::KB_Mac},
    {QKeySequence::NextChild,               1,          Qt::CTRL | Qt::Key_BraceRight,          QApplicationPrivate::KB_Mac},
    {QKeySequence::NextChild,               1,          Qt::CTRL | Qt::Key_Tab,                 QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11},
    {QKeySequence::NextChild,               0,          Qt::CTRL | Qt::Key_Tab,                 QApplicationPrivate::KB_Mac}, //different priority from above
    {QKeySequence::DeleteStartOfWord,       0,          Qt::CTRL | Qt::Key_Backspace,           QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_Win},
    {QKeySequence::Copy,                    0,          Qt::CTRL | Qt::Key_Insert,              QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_Win}, 
    {QKeySequence::DeleteEndOfWord,         0,          Qt::CTRL | Qt::Key_Delete,              QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_Win},
    {QKeySequence::MoveToStartOfDocument,   0,          Qt::CTRL | Qt::Key_Home,                QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::MoveToEndOfDocument,     0,          Qt::CTRL | Qt::Key_End,                 QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::Back,                    0,          Qt::CTRL | Qt::Key_Left,                QApplicationPrivate::KB_Mac}, 
    {QKeySequence::MoveToPreviousWord,      0,          Qt::CTRL | Qt::Key_Left,                QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::MoveToStartOfLine,       0,          Qt::CTRL | Qt::Key_Left,                QApplicationPrivate::KB_Mac },
    {QKeySequence::MoveToStartOfDocument,   1,          Qt::CTRL | Qt::Key_Up,                  QApplicationPrivate::KB_Mac},
    {QKeySequence::Forward,                 0,          Qt::CTRL | Qt::Key_Right,               QApplicationPrivate::KB_Mac}, 
    {QKeySequence::MoveToEndOfLine,         0,          Qt::CTRL | Qt::Key_Right,               QApplicationPrivate::KB_Mac },
    {QKeySequence::MoveToNextWord,          0,          Qt::CTRL | Qt::Key_Right,               QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::MoveToEndOfDocument,     1,          Qt::CTRL | Qt::Key_Down,                QApplicationPrivate::KB_Mac},
    {QKeySequence::Close,                   1,          Qt::CTRL | Qt::Key_F4,                  QApplicationPrivate::KB_Win},
    {QKeySequence::Close,                   0,          Qt::CTRL | Qt::Key_F4,                  QApplicationPrivate::KB_Mac},
    {QKeySequence::NextChild,               0,          Qt::CTRL | Qt::Key_F6,                  QApplicationPrivate::KB_Win},
    {QKeySequence::FindPrevious,            1,          Qt::CTRL | Qt::SHIFT | Qt::Key_G,       QApplicationPrivate::KB_Gnome | QApplicationPrivate::KB_Mac},
    {QKeySequence::FindPrevious,            0,          Qt::CTRL | Qt::SHIFT | Qt::Key_G,       QApplicationPrivate::KB_Win},
    {QKeySequence::AddTab,                  1,          Qt::CTRL | Qt::SHIFT | Qt::Key_N,       QApplicationPrivate::KB_KDE},
    {QKeySequence::SaveAs,                  0,          Qt::CTRL | Qt::SHIFT | Qt::Key_S,       QApplicationPrivate::KB_Gnome | QApplicationPrivate::KB_Mac},
    {QKeySequence::Redo,                    0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Z,       QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::Redo,                    0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Z,       QApplicationPrivate::KB_Mac},
    {QKeySequence::PreviousChild,           1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab, QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11},
    {QKeySequence::PreviousChild,           0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab, QApplicationPrivate::KB_Mac },//different priority from above 
    {QKeySequence::Paste,                   0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Insert,  QApplicationPrivate::KB_X11},
    {QKeySequence::SelectStartOfDocument,   0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Home,    QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::SelectEndOfDocument,     0,          Qt::CTRL | Qt::SHIFT | Qt::Key_End,     QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::SelectPreviousWord,      0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Left,    QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::SelectStartOfLine,       1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Left,    QApplicationPrivate::KB_Mac },
    {QKeySequence::SelectStartOfDocument,   1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Up,      QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectNextWord,          0,          Qt::CTRL | Qt::SHIFT | Qt::Key_Right,   QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11 | QApplicationPrivate::KB_S60},
    {QKeySequence::SelectEndOfLine,         1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Right,   QApplicationPrivate::KB_Mac },
    {QKeySequence::SelectEndOfDocument,     1,          Qt::CTRL | Qt::SHIFT | Qt::Key_Down,    QApplicationPrivate::KB_Mac},
    {QKeySequence::PreviousChild,           0,          Qt::CTRL | Qt::SHIFT | Qt::Key_F6,      QApplicationPrivate::KB_Win},
    {QKeySequence::Undo,                    0,          Qt::ALT  | Qt::Key_Backspace,           QApplicationPrivate::KB_Win},
    {QKeySequence::DeleteStartOfWord,       0,          Qt::ALT  | Qt::Key_Backspace,           QApplicationPrivate::KB_Mac},
    {QKeySequence::DeleteEndOfWord,         0,          Qt::ALT  | Qt::Key_Delete,              QApplicationPrivate::KB_Mac},    
    {QKeySequence::Back,                    1,          Qt::ALT  | Qt::Key_Left,                QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11},
    {QKeySequence::MoveToPreviousWord,      0,          Qt::ALT  | Qt::Key_Left,                QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToStartOfBlock,      0,          Qt::ALT  | Qt::Key_Up,                  QApplicationPrivate::KB_Mac}, //mac only
    {QKeySequence::MoveToNextWord,          0,          Qt::ALT  | Qt::Key_Right,               QApplicationPrivate::KB_Mac},
    {QKeySequence::Forward,                 1,          Qt::ALT  | Qt::Key_Right,               QApplicationPrivate::KB_Win | QApplicationPrivate::KB_X11},
    {QKeySequence::MoveToEndOfBlock,        0,          Qt::ALT  | Qt::Key_Down,                QApplicationPrivate::KB_Mac}, //mac only
    {QKeySequence::MoveToPreviousPage,      0,          Qt::ALT  | Qt::Key_PageUp,              QApplicationPrivate::KB_Mac },
    {QKeySequence::MoveToNextPage,          0,          Qt::ALT  | Qt::Key_PageDown,            QApplicationPrivate::KB_Mac },
    {QKeySequence::Redo,                    0,          Qt::ALT  | Qt::SHIFT | Qt::Key_Backspace,QApplicationPrivate::KB_Win},
    {QKeySequence::SelectPreviousWord,      0,          Qt::ALT  | Qt::SHIFT | Qt::Key_Left,    QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectStartOfBlock,      0,          Qt::ALT  | Qt::SHIFT | Qt::Key_Up,      QApplicationPrivate::KB_Mac}, //mac only
    {QKeySequence::SelectNextWord,          0,          Qt::ALT  | Qt::SHIFT | Qt::Key_Right,   QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectEndOfBlock,        0,          Qt::ALT  | Qt::SHIFT | Qt::Key_Down,    QApplicationPrivate::KB_Mac}, //mac only
    {QKeySequence::MoveToStartOfBlock,      0,          Qt::META | Qt::Key_A,                   QApplicationPrivate::KB_Mac},
    {QKeySequence::Delete,                  0,          Qt::META | Qt::Key_D,                   QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToEndOfBlock,        0,          Qt::META | Qt::Key_E,                   QApplicationPrivate::KB_Mac},
    {QKeySequence::InsertLineSeparator,     0,          Qt::META | Qt::Key_Return,              QApplicationPrivate::KB_Mac},
    {QKeySequence::InsertLineSeparator,     0,          Qt::META | Qt::Key_Enter,               QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToStartOfLine,       0,          Qt::META | Qt::Key_Left,                QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToPreviousPage,      0,          Qt::META | Qt::Key_Up,                  QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToEndOfLine,         0,          Qt::META | Qt::Key_Right,               QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToNextPage,          0,          Qt::META | Qt::Key_Down,                QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToPreviousPage,      0,          Qt::META | Qt::Key_PageUp,              QApplicationPrivate::KB_Mac},
    {QKeySequence::MoveToNextPage,          0,          Qt::META | Qt::Key_PageDown,            QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectStartOfBlock,      0,          Qt::META | Qt::SHIFT | Qt::Key_A,       QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectEndOfBlock,        0,          Qt::META | Qt::SHIFT | Qt::Key_E,       QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectStartOfLine,       0,          Qt::META | Qt::SHIFT | Qt::Key_Left,    QApplicationPrivate::KB_Mac},
    {QKeySequence::SelectEndOfLine,         0,          Qt::META | Qt::SHIFT | Qt::Key_Right,   QApplicationPrivate::KB_Mac}    
};

const uint QKeySequencePrivate::numberOfKeyBindings = sizeof(QKeySequencePrivate::keyBindings)/(sizeof(QKeyBinding));


/*!
    \enum QKeySequence::StandardKey
    \since 4.2

    This enum represent standard key bindings. They can be used to
    assign platform dependent keyboard shortcuts to a QAction.

    Note that the key bindings are platform dependent. The currently
    bound shortcuts can be queried using keyBindings().

    \value AddTab           Add new tab.
    \value Back             Navigate back.
    \value Bold             Bold text.
    \value Close            Close document/tab.
    \value Copy             Copy.
    \value Cut              Cut.
    \value Delete           Delete.
    \value DeleteEndOfLine          Delete end of line.
    \value DeleteEndOfWord          Delete word from the end of the cursor.
    \value DeleteStartOfWord        Delete the beginning of a word up to the cursor.
    \value Find             Find in document.
    \value FindNext         Find next result.
    \value FindPrevious     Find previous result.
    \value Forward          Navigate forward.
    \value HelpContents     Open help contents.
    \value InsertLineSeparator      Insert a new line.
    \value InsertParagraphSeparator Insert a new paragraph.
    \value Italic           Italic text.
    \value MoveToEndOfBlock         Move cursor to end of block. This shortcut is only used on the OS X.
    \value MoveToEndOfDocument      Move cursor to end of document.
    \value MoveToEndOfLine          Move cursor to end of line.
    \value MoveToNextChar           Move cursor to next character.
    \value MoveToNextLine           Move cursor to next line.
    \value MoveToNextPage           Move cursor to next page.
    \value MoveToNextWord           Move cursor to next word.
    \value MoveToPreviousChar       Move cursor to previous character.
    \value MoveToPreviousLine       Move cursor to previous line.
    \value MoveToPreviousPage       Move cursor to previous page.
    \value MoveToPreviousWord       Move cursor to previous word.
    \value MoveToStartOfBlock       Move cursor to start of a block. This shortcut is only used on OS X.
    \value MoveToStartOfDocument    Move cursor to start of document.
    \value MoveToStartOfLine        Move cursor to start of line.
    \value New              Create new document.
    \value NextChild        Navigate to next tab or child window.
    \value Open             Open document.
    \value Paste            Paste.
    \value Preferences      Open the preferences dialog.
    \value PreviousChild    Navigate to previous tab or child window.
    \value Print            Print document.
    \value Quit             Quit the application.
    \value Redo             Redo.
    \value Refresh          Refresh or reload current document.
    \value Replace          Find and replace.
    \value SaveAs           Save document after prompting the user for a file name.
    \value Save             Save document.
    \value SelectAll        Select all text.
    \value SelectEndOfBlock         Extend selection to the end of a text block. This shortcut is only used on OS X.
    \value SelectEndOfDocument      Extend selection to end of document.
    \value SelectEndOfLine          Extend selection to end of line.
    \value SelectNextChar           Extend selection to next character.
    \value SelectNextLine           Extend selection to next line.
    \value SelectNextPage           Extend selection to next page.
    \value SelectNextWord           Extend selection to next word.
    \value SelectPreviousChar       Extend selection to previous character.
    \value SelectPreviousLine       Extend selection to previous line.
    \value SelectPreviousPage       Extend selection to previous page.
    \value SelectPreviousWord       Extend selection to previous word.
    \value SelectStartOfBlock       Extend selection to the start of a text block. This shortcut is only used on OS X.
    \value SelectStartOfDocument    Extend selection to start of document. 
    \value SelectStartOfLine        Extend selection to start of line.
    \value Underline        Underline text.
    \value Undo             Undo.
    \value UnknownKey       Unbound key.
    \value WhatsThis        Activate whats this.
    \value ZoomIn           Zoom in.
    \value ZoomOut          Zoom out.
*/

/*!
    \since 4.2

    Constructs a QKeySequence object for the given \a key. 
    The result will depend on the currently running platform. 

    The resulting object will be based on the first element in the 
    list of key bindings for the \a key.
*/
QKeySequence::QKeySequence(StandardKey key)
{
    const QList <QKeySequence> bindings = keyBindings(key);
    //pick only the first/primary shortcut from current bindings
    if (bindings.size() > 0) {
        d = bindings.first().d; 
        d->ref.ref();
    }
    else
        d = new QKeySequencePrivate();
}


/*!
    Constructs an empty key sequence.
*/
QKeySequence::QKeySequence()
{
    static QKeySequencePrivate shared_empty;
    d = &shared_empty;
    d->ref.ref();
}

/*!
    Creates a key sequence from the \a key string. For example
    "Ctrl+O" gives CTRL+'O'. The strings "Ctrl",
    "Shift", "Alt" and "Meta" are recognized, as well as their
    translated equivalents in the "QShortcut" context (using
    QObject::tr()).

    Up to four key codes may be entered by separating them with
    commas, e.g. "Alt+X,Ctrl+S,Q".

    \a key should be in NativeText format.

    This constructor is typically used with \link QObject::tr() tr
    \endlink(), so that shortcut keys can be replaced in
    translations:

    \snippet doc/src/snippets/code/src_gui_kernel_qkeysequence.cpp 2

    Note the "File|Open" translator comment. It is by no means
    necessary, but it provides some context for the human translator.
*/
QKeySequence::QKeySequence(const QString &key)
{
    d = new QKeySequencePrivate();
    assign(key);
}

/*!
    \since 4.7
    Creates a key sequence from the \a key string based on \a format.
*/
QKeySequence::QKeySequence(const QString &key, QKeySequence::SequenceFormat format)
{
    d = new QKeySequencePrivate();
    assign(key, format);
}

/*!
    Constructs a key sequence with up to 4 keys \a k1, \a k2,
    \a k3 and \a k4.

    The key codes are listed in Qt::Key and can be combined with
    modifiers (see Qt::Modifier) such as Qt::SHIFT, Qt::CTRL,
    Qt::ALT, or Qt::META.
*/
QKeySequence::QKeySequence(int k1, int k2, int k3, int k4)
{
    d = new QKeySequencePrivate();
    d->key[0] = k1;
    d->key[1] = k2;
    d->key[2] = k3;
    d->key[3] = k4;
}

/*!
    Copy constructor. Makes a copy of \a keysequence.
 */
QKeySequence::QKeySequence(const QKeySequence& keysequence)
    : d(keysequence.d)
{
    d->ref.ref();
}

#ifdef Q_WS_MAC
static inline int maybeSwapShortcut(int shortcut)
{
    if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
        uint oldshortcut = shortcut;
        shortcut &= ~(Qt::CTRL | Qt::META);
        if (oldshortcut & Qt::CTRL)
            shortcut |= Qt::META;
        if (oldshortcut & Qt::META)
            shortcut |= Qt::CTRL;
    }
    return shortcut;
}
#endif

/*!
    \since 4.2

    Returns a list of key bindings for the given \a key.
    The result of calling this function will vary based on the target platform. 
    The first element of the list indicates the primary shortcut for the given platform. 
    If the result contains more than one result, these can
    be considered alternative shortcuts on the same platform for the given \a key.
*/
QList<QKeySequence> QKeySequence::keyBindings(StandardKey key)
{
    uint platform = QApplicationPrivate::currentPlatform();
    QList <QKeySequence> list;
    for (uint i = 0; i < QKeySequencePrivate::numberOfKeyBindings ; ++i) {
        QKeyBinding keyBinding = QKeySequencePrivate::keyBindings[i];
        if (keyBinding.standardKey == key && (keyBinding.platform & platform)) {
            uint shortcut =
#ifdef Q_WS_MAC
                    maybeSwapShortcut(QKeySequencePrivate::keyBindings[i].shortcut);
#else
                    QKeySequencePrivate::keyBindings[i].shortcut;
#endif
            if (keyBinding.priority > 0)
                list.prepend(QKeySequence(shortcut));
            else
                list.append(QKeySequence(shortcut));
        }
    }
    return list;
}

/*!
    Destroys the key sequence.
 */
QKeySequence::~QKeySequence()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \internal
    KeySequences should never be modified, but rather just created.
    Internally though we do need to modify to keep pace in event
    delivery.
*/

void QKeySequence::setKey(int key, int index)
{
    Q_ASSERT_X(index >= 0 && index < 4, "QKeySequence::setKey", "index out of range");
    qAtomicDetach(d);
    d->key[index] = key;
}

/*!
    Returns the number of keys in the key sequence.
    The maximum is 4.
 */
uint QKeySequence::count() const
{
    if (!d->key[0])
        return 0;
    if (!d->key[1])
        return 1;
    if (!d->key[2])
        return 2;
    if (!d->key[3])
        return 3;
    return 4;
}


/*!
    Returns true if the key sequence is empty; otherwise returns
    false.
*/
bool QKeySequence::isEmpty() const
{
    return !d->key[0];
}


/*!
    Returns the shortcut key sequence for the mnemonic in \a text,
    or an empty key sequence if no mnemonics are found.

    For example, mnemonic("E&xit") returns \c{Qt::ALT+Qt::Key_X},
    mnemonic("&Quit") returns \c{ALT+Key_Q}, and mnemonic("Quit")
    returns an empty QKeySequence.

    We provide a \l{accelerators.html}{list of common mnemonics}
    in English. At the time of writing, Microsoft and Open Group do
    not appear to have issued equivalent recommendations for other
    languages.

    \sa qt_set_sequence_auto_mnemonic()
*/
QKeySequence QKeySequence::mnemonic(const QString &text)
{
    QKeySequence ret;

    if(qt_sequence_no_mnemonics)
        return ret;

    bool found = false;
    int p = 0;
    while (p >= 0) {
        p = text.indexOf(QLatin1Char('&'), p) + 1;
        if (p <= 0 || p >= (int)text.length())
            break;
        if (text.at(p) != QLatin1Char('&')) {
            QChar c = text.at(p);
            if (c.isPrint()) {
                if (!found) {
                    c = c.toUpper();
                    ret = QKeySequence(c.unicode() + Qt::ALT);
#ifdef QT_NO_DEBUG
                    return ret;
#else
                    found = true;
                } else {
                    qWarning("QKeySequence::mnemonic: \"%s\" contains multiple occurrences of '&'", qPrintable(text));
#endif
                }
            }
        }
        p++;
    }
    return ret;
}

/*!
    \fn int QKeySequence::assign(const QString &keys)

    Adds the given \a keys to the key sequence. \a keys may
    contain up to four key codes, provided they are separated by a
    comma; for example, "Alt+X,Ctrl+S,Z". The return value is the
    number of key codes added.
    \a keys should be in NativeText format.
*/
int QKeySequence::assign(const QString &ks)
{
    return assign(ks, NativeText);
}

/*!
    \fn int QKeySequence::assign(const QString &keys, QKeySequence::SequenceFormat format)
    \since 4.7

    Adds the given \a keys to the key sequence (based on \a format).
    \a keys may contain up to four key codes, provided they are
    separated by a comma; for example, "Alt+X,Ctrl+S,Z". The return
    value is the number of key codes added.
*/
int QKeySequence::assign(const QString &ks, QKeySequence::SequenceFormat format)
{
    QString keyseq = ks;
    QString part;
    int n = 0;
    int p = 0, diff = 0;

    // Run through the whole string, but stop
    // if we have 4 keys before the end.
    while (keyseq.length() && n < 4) {
        // We MUST use something to separate each sequence, and space
        // does not cut it, since some of the key names have space
        // in them.. (Let's hope no one translate with a comma in it:)
        p = keyseq.indexOf(QLatin1Char(','));
        if (-1 != p) {
            if (p == keyseq.count() - 1) { // Last comma 'Ctrl+,'
                p = -1;
            } else {
                if (QLatin1Char(',') == keyseq.at(p+1)) // e.g. 'Ctrl+,, Shift+,,'
                    p++;
                if (QLatin1Char(' ') == keyseq.at(p+1)) { // Space after comma
                    diff = 1;
                    p++;
                } else {
                    diff = 0;
                }
            }
        }
        part = keyseq.left(-1 == p ? keyseq.length() : p - diff);
        keyseq = keyseq.right(-1 == p ? 0 : keyseq.length() - (p + 1));
        d->key[n] = QKeySequencePrivate::decodeString(part, format);
        ++n;
    }
    return n;
}

struct QModifKeyName {
    QModifKeyName() { }
    QModifKeyName(int q, QChar n) : qt_key(q), name(n) { }
    QModifKeyName(int q, const QString &n) : qt_key(q), name(n) { }
    int qt_key;
    QString name;
};

Q_GLOBAL_STATIC(QList<QModifKeyName>, globalModifs)
Q_GLOBAL_STATIC(QList<QModifKeyName>, globalPortableModifs)

/*!
  Constructs a single key from the string \a str.
*/
int QKeySequence::decodeString(const QString &str)
{
    return QKeySequencePrivate::decodeString(str, NativeText);
}

int QKeySequencePrivate::decodeString(const QString &str, QKeySequence::SequenceFormat format)
{
    int ret = 0;
    QString accel = str.toLower();
    bool nativeText = (format == QKeySequence::NativeText);

    QList<QModifKeyName> *gmodifs;
    if (nativeText) {
        gmodifs = globalModifs();
        if (gmodifs->isEmpty()) {
#ifdef Q_WS_MAC
            const bool dontSwap = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
            if (dontSwap)
                *gmodifs << QModifKeyName(Qt::META, QChar(kCommandUnicode));
            else
                *gmodifs << QModifKeyName(Qt::CTRL, QChar(kCommandUnicode));
            *gmodifs << QModifKeyName(Qt::ALT, QChar(kOptionUnicode));
            if (dontSwap)
                *gmodifs << QModifKeyName(Qt::CTRL, QChar(kControlUnicode));
            else
                *gmodifs << QModifKeyName(Qt::META, QChar(kControlUnicode));
            *gmodifs << QModifKeyName(Qt::SHIFT, QChar(kShiftUnicode));
#endif
            *gmodifs << QModifKeyName(Qt::CTRL, QLatin1String("ctrl+"))
                     << QModifKeyName(Qt::SHIFT, QLatin1String("shift+"))
                     << QModifKeyName(Qt::ALT, QLatin1String("alt+"))
                     << QModifKeyName(Qt::META, QLatin1String("meta+"));
        }
    } else {
        gmodifs = globalPortableModifs();
        if (gmodifs->isEmpty()) {
            *gmodifs << QModifKeyName(Qt::CTRL, QLatin1String("ctrl+"))
                     << QModifKeyName(Qt::SHIFT, QLatin1String("shift+"))
                     << QModifKeyName(Qt::ALT, QLatin1String("alt+"))
                     << QModifKeyName(Qt::META, QLatin1String("meta+"));
        }
    }
    if (!gmodifs) return ret;


    QList<QModifKeyName> modifs;
    if (nativeText) {
        modifs << QModifKeyName(Qt::CTRL, QShortcut::tr("Ctrl").toLower().append(QLatin1Char('+')))
               << QModifKeyName(Qt::SHIFT, QShortcut::tr("Shift").toLower().append(QLatin1Char('+')))
               << QModifKeyName(Qt::ALT, QShortcut::tr("Alt").toLower().append(QLatin1Char('+')))
               << QModifKeyName(Qt::META, QShortcut::tr("Meta").toLower().append(QLatin1Char('+')));
    }
    modifs += *gmodifs; // Test non-translated ones last

    QString sl = accel;
#ifdef Q_WS_MAC
    for (int i = 0; i < modifs.size(); ++i) {
        const QModifKeyName &mkf = modifs.at(i);
        if (sl.contains(mkf.name)) {
            ret |= mkf.qt_key;
            accel.remove(mkf.name);
            sl = accel;
        }
    }
#else
    int i = 0;
    int lastI = 0;
    while ((i = sl.indexOf(QLatin1Char('+'), i + 1)) != -1) {
        const QString sub = sl.mid(lastI, i - lastI + 1);
        // Just shortcut the check here if we only have one character.
        // Rational: A modifier will contain the name AND +, so longer than 1, a length of 1 is just
        // the remaining part of the shortcut (ei. The 'C' in "Ctrl+C"), so no need to check that.
        if (sub.length() > 1) {
            for (int j = 0; j < modifs.size(); ++j) {
                const QModifKeyName &mkf = modifs.at(j);
                if (sub == mkf.name) {
                    ret |= mkf.qt_key;
                    break; // Shortcut, since if we find an other it would/should just be a dup
                }
            }
        }
        lastI = i + 1;
    }
#endif

    int p = accel.lastIndexOf(QLatin1Char('+'), str.length() - 2); // -2 so that Ctrl++ works
    if(p > 0)
        accel = accel.mid(p + 1);

    int fnum = 0;
    if (accel.length() == 1) {
#ifdef Q_WS_MAC
        int qtKey = qtkeyForMacSymbol(accel[0]);
        if (qtKey != -1) {
            ret |= qtKey;
        } else
#endif
        {
            ret |= accel[0].toUpper().unicode();
        }
    } else if (accel[0] == QLatin1Char('f') && (fnum = accel.mid(1).toInt()) && (fnum >= 1) && (fnum <= 35)) {
        ret |= Qt::Key_F1 + fnum - 1;
    } else {
        // For NativeText, check the traslation table first,
        // if we don't find anything then try it out with just the untranlated stuff.
        // PortableText will only try the untranlated table.
        bool found = false;
        for (int tran = 0; tran < 2; ++tran) {
            if (!nativeText)
                ++tran;
            for (int i = 0; keyname[i].name; ++i) {
                QString keyName(tran == 0
                                ? QShortcut::tr(keyname[i].name)
                                : QString::fromLatin1(keyname[i].name));
                if (accel == keyName.toLower()) {
                    ret |= keyname[i].key;
                    found = true;
                    break;
                }
            }
            if (found)
                break;
        }
    }
    return ret;
}

/*!
    Creates a shortcut string for \a key. For example,
    Qt::CTRL+Qt::Key_O gives "Ctrl+O". The strings, "Ctrl", "Shift", etc. are
    translated (using QObject::tr()) in the "QShortcut" context.
 */
QString QKeySequence::encodeString(int key)
{
    return QKeySequencePrivate::encodeString(key, NativeText);
}

static inline void addKey(QString &str, const QString &theKey, QKeySequence::SequenceFormat format)
{
    if (!str.isEmpty())
        str += (format == QKeySequence::NativeText) ? QShortcut::tr("+")
                                                    : QString::fromLatin1("+");
    str += theKey;
}

QString QKeySequencePrivate::encodeString(int key, QKeySequence::SequenceFormat format)
{
    bool nativeText = (format == QKeySequence::NativeText);
    QString s;
#if defined(Q_WS_MAC)
    if (nativeText) {
        // On Mac OS X the order (by default) is Meta, Alt, Shift, Control.
        // If the AA_MacDontSwapCtrlAndMeta is enabled, then the order
        // is Ctrl, Alt, Shift, Meta. The macSymbolForQtKey does this swap
        // for us, which means that we have to adjust our order here.
        // The upshot is a lot more infrastructure to keep the number of
        // if tests down and the code relatively clean.
        static const int ModifierOrder[] = { Qt::META, Qt::ALT, Qt::SHIFT, Qt::CTRL, 0 };
        static const int QtKeyOrder[] = { Qt::Key_Meta, Qt::Key_Alt, Qt::Key_Shift, Qt::Key_Control, 0 };
        static const int DontSwapModifierOrder[] = { Qt::CTRL, Qt::ALT, Qt::SHIFT, Qt::META, 0 };
        static const int DontSwapQtKeyOrder[] = { Qt::Key_Control, Qt::Key_Alt, Qt::Key_Shift, Qt::Key_Meta, 0 };
        const int *modifierOrder;
        const int *qtkeyOrder;
        if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
            modifierOrder = DontSwapModifierOrder;
            qtkeyOrder = DontSwapQtKeyOrder;
        } else {
            modifierOrder = ModifierOrder;
            qtkeyOrder = QtKeyOrder;
        }

        for (int i = 0; modifierOrder[i] != 0; ++i) {
            if (key & modifierOrder[i])
                s += qt_macSymbolForQtKey(qtkeyOrder[i]);
        }
    } else
#endif
    {
        // On other systems the order is Meta, Control, Alt, Shift
        if ((key & Qt::META) == Qt::META)
            s = nativeText ? QShortcut::tr("Meta") : QString::fromLatin1("Meta");
        if ((key & Qt::CTRL) == Qt::CTRL)
            addKey(s, nativeText ? QShortcut::tr("Ctrl") : QString::fromLatin1("Ctrl"), format);
        if ((key & Qt::ALT) == Qt::ALT)
            addKey(s, nativeText ? QShortcut::tr("Alt") : QString::fromLatin1("Alt"), format);
        if ((key & Qt::SHIFT) == Qt::SHIFT)
            addKey(s, nativeText ? QShortcut::tr("Shift") : QString::fromLatin1("Shift"), format);
    }


    key &= ~(Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    QString p;

    if (key && key < Qt::Key_Escape && key != Qt::Key_Space) {
        if (!QChar::requiresSurrogates(key)) {
            p = QChar(ushort(key)).toUpper();
        } else {
            p += QChar(QChar::highSurrogate(key));
            p += QChar(QChar::lowSurrogate(key));
        }
    } else if (key >= Qt::Key_F1 && key <= Qt::Key_F35) {
            p = nativeText ? QShortcut::tr("F%1").arg(key - Qt::Key_F1 + 1)
                           : QString::fromLatin1("F%1").arg(key - Qt::Key_F1 + 1);
    } else if (key) {
        int i=0;
#if defined(Q_WS_MAC)
        if (nativeText) {
            QChar ch = qt_macSymbolForQtKey(key);
            if (!ch.isNull())
                p = ch;
            else
                goto NonSymbol;
        } else
#endif
        {
#ifdef Q_WS_MAC
NonSymbol:
#endif
            while (keyname[i].name) {
                if (key == keyname[i].key) {
                    p = nativeText ? QShortcut::tr(keyname[i].name)
                                   : QString::fromLatin1(keyname[i].name);
                    break;
                }
                ++i;
            }
            // If we can't find the actual translatable keyname,
            // fall back on the unicode representation of it...
            // Or else characters like Qt::Key_aring may not get displayed
            // (Really depends on you locale)
            if (!keyname[i].name) {
                if (!QChar::requiresSurrogates(key)) {
                    p = QChar(ushort(key)).toUpper();
                } else {
                    p += QChar(QChar::highSurrogate(key));
                    p += QChar(QChar::lowSurrogate(key));
                }
            }
        }
    }

#ifdef Q_WS_MAC
    if (nativeText)
        s += p;
    else
#endif
    addKey(s, p, format);
    return s;
}
/*!
    Matches the sequence with \a seq. Returns ExactMatch if
    successful, PartialMatch if \a seq matches incompletely,
    and NoMatch if the sequences have nothing in common.
    Returns NoMatch if \a seq is shorter.
*/
QKeySequence::SequenceMatch QKeySequence::matches(const QKeySequence &seq) const
{
    uint userN = count(),
          seqN = seq.count();

    if (userN > seqN)
        return NoMatch;

    // If equal in length, we have a potential ExactMatch sequence,
    // else we already know it can only be partial.
    SequenceMatch match = (userN == seqN ? ExactMatch : PartialMatch);

    for (uint i = 0; i < userN; ++i) {
        int userKey = (*this)[i],
            sequenceKey = seq[i];
        if (userKey != sequenceKey)
            return NoMatch;
    }
    return match;
}


/*!
    \obsolete

    Use toString() instead. 
    
    Returns the key sequence as a QString. This is equivalent to 
    calling toString(QKeySequence::NativeText). Note that the
    result is not platform independent.
*/
QKeySequence::operator QString() const
{
    return QKeySequence::toString(QKeySequence::NativeText);
}

/*!
   Returns the key sequence as a QVariant
*/
QKeySequence::operator QVariant() const
{
    return QVariant(QVariant::KeySequence, this);
}

/*!
    \obsolete
    For backward compatibility: returns the first keycode
    as integer. If the key sequence is empty, 0 is returned.
 */
QKeySequence::operator int () const
{
    if (1 <= count())
        return d->key[0];
    return 0;
}


/*!
    Returns a reference to the element at position \a index in the key
    sequence. This can only be used to read an element.
 */
int QKeySequence::operator[](uint index) const
{
    Q_ASSERT_X(index < 4, "QKeySequence::operator[]", "index out of range");
    return d->key[index];
}


/*!
    Assignment operator. Assigns the \a other key sequence to this
    object.
 */
QKeySequence &QKeySequence::operator=(const QKeySequence &other)
{
    qAtomicAssign(d, other.d);
    return *this;
}

/*!
    \fn void QKeySequence::swap(QKeySequence &other)
    \since 4.8

    Swaps key sequence \a other with this key sequence. This operation is very
    fast and never fails.
*/

/*!
    \fn bool QKeySequence::operator!=(const QKeySequence &other) const

    Returns true if this key sequence is not equal to the \a other
    key sequence; otherwise returns false.
*/


/*!
    Returns true if this key sequence is equal to the \a other
    key sequence; otherwise returns false.
 */
bool QKeySequence::operator==(const QKeySequence &other) const
{
    return (d->key[0] == other.d->key[0] &&
            d->key[1] == other.d->key[1] &&
            d->key[2] == other.d->key[2] &&
            d->key[3] == other.d->key[3]);
}


/*!
    Provides an arbitrary comparison of this key sequence and
    \a other key sequence. All that is guaranteed is that the
    operator returns false if both key sequences are equal and
    that (ks1 \< ks2) == !( ks2 \< ks1) if the key sequences
    are not equal.

    This function is useful in some circumstances, for example
    if you want to use QKeySequence objects as keys in a QMap.

    \sa operator==() operator!=() operator>() operator<=() operator>=()
*/
bool QKeySequence::operator< (const QKeySequence &other) const
{
    for (int i = 0; i < 4; ++i)
        if (d->key[i] != other.d->key[i])
            return d->key[i] < other.d->key[i];
    return false;
}

/*!
    \fn bool QKeySequence::operator> (const QKeySequence &other) const

    Returns true if this key sequence is larger than the \a other key
    sequence; otherwise returns false.

    \sa operator==() operator!=() operator<() operator<=() operator>=()
*/

/*!
    \fn bool QKeySequence::operator<= (const QKeySequence &other) const

    Returns true if this key sequence is smaller or equal to the
    \a other key sequence; otherwise returns false.

    \sa operator==() operator!=() operator<() operator>() operator>=()
*/

/*!
    \fn bool QKeySequence::operator>= (const QKeySequence &other) const

    Returns true if this key sequence is larger or equal to the
    \a other key sequence; otherwise returns false.

    \sa operator==() operator!=() operator<() operator>() operator<=()
*/

/*!
    \internal
*/
bool QKeySequence::isDetached() const
{
    return d->ref == 1;
}

/*!
    \since 4.1

    Return a string representation of the key sequence,
    based on \a format.

    For example, the value Qt::CTRL+Qt::Key_O results in "Ctrl+O".
    If the key sequence has multiple key codes, each is separated
    by commas in the string returned, such as "Alt+X, Ctrl+Y, Z".
    The strings, "Ctrl", "Shift", etc. are translated using
    QObject::tr() in the "QShortcut" context.

    If the key sequence has no keys, an empty string is returned.

    On Mac OS X, the string returned resembles the sequence that is
    shown in the menu bar.

    \sa fromString()
*/
QString QKeySequence::toString(SequenceFormat format) const
{
    QString finalString;
    // A standard string, with no translation or anything like that. In some ways it will
    // look like our latin case on Windows and X11
    int end = count();
    for (int i = 0; i < end; ++i) {
        finalString += d->encodeString(d->key[i], format);
        finalString += QLatin1String(", ");
    }
    finalString.truncate(finalString.length() - 2);
    return finalString;
}

/*!
    \since 4.1

    Return a QKeySequence from the string \a str based on \a format.

    \sa toString()
*/
QKeySequence QKeySequence::fromString(const QString &str, SequenceFormat format)
{
    return QKeySequence(str, format);
}

/*****************************************************************************
  QKeySequence stream functions
 *****************************************************************************/
#if !defined(QT_NO_DATASTREAM)
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QKeySequence &sequence)
    \relates QKeySequence

    Writes the key \a sequence to the \a stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator<<(QDataStream &s, const QKeySequence &keysequence)
{
    QList<quint32> list;
    list << keysequence.d->key[0];

    if (s.version() >= 5 && keysequence.count() > 1) {
        list << keysequence.d->key[1];
        list << keysequence.d->key[2];
        list << keysequence.d->key[3];
    }
    s << list;
    return s;
}


/*!
    \fn QDataStream &operator>>(QDataStream &stream, QKeySequence &sequence)
    \relates QKeySequence

    Reads a key sequence from the \a stream into the key \a sequence.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator>>(QDataStream &s, QKeySequence &keysequence)
{
	qAtomicDetach(keysequence.d);
    QList<quint32> list;
    s >> list;
    for (int i = 0; i < 4; ++i)
        keysequence.d->key[i] = list.value(i);
    return s;
}

#endif //QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QKeySequence &p)
{
#ifndef Q_BROKEN_DEBUG_STREAM
    dbg.nospace() << "QKeySequence(" << p.toString() << ')';
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QKeySequence to QDebug");
    return dbg;
    Q_UNUSED(p);
#endif
}
#endif

#endif // QT_NO_SHORTCUT


/*!
    \typedef QKeySequence::DataPtr
    \internal
*/

 /*!
    \fn DataPtr &QKeySequence::data_ptr()
    \internal
*/

QT_END_NAMESPACE
