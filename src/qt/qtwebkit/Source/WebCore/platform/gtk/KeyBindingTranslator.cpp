/*
 * Copyright (C) 2010, 2011 Igalia S.L.
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

#include "config.h"
#include "KeyBindingTranslator.h"

#include "GtkVersioning.h"
#include <gdk/gdkkeysyms.h>
#include <wtf/HashMap.h>

namespace WebCore {

typedef HashMap<int, const char*> IntConstCharHashMap;

static void backspaceCallback(GtkWidget* widget, KeyBindingTranslator* translator)
{
    g_signal_stop_emission_by_name(widget, "backspace");
    translator->addPendingEditorCommand("DeleteBackward");
}

static void selectAllCallback(GtkWidget* widget, gboolean select, KeyBindingTranslator* translator)
{
    g_signal_stop_emission_by_name(widget, "select-all");
    translator->addPendingEditorCommand(select ? "SelectAll" : "Unselect");
}

static void cutClipboardCallback(GtkWidget* widget, KeyBindingTranslator* translator)
{
    g_signal_stop_emission_by_name(widget, "cut-clipboard");
    translator->addPendingEditorCommand("Cut");
}

static void copyClipboardCallback(GtkWidget* widget, KeyBindingTranslator* translator)
{
    g_signal_stop_emission_by_name(widget, "copy-clipboard");
    translator->addPendingEditorCommand("Copy");
}

static void pasteClipboardCallback(GtkWidget* widget, KeyBindingTranslator* translator)
{
    g_signal_stop_emission_by_name(widget, "paste-clipboard");
    translator->addPendingEditorCommand("Paste");
}

static void toggleOverwriteCallback(GtkWidget* widget, KeyBindingTranslator* translator)
{
    g_signal_stop_emission_by_name(widget, "toggle-overwrite");
    translator->addPendingEditorCommand("OverWrite");
}

// GTK+ will still send these signals to the web view. So we can safely stop signal
// emission without breaking accessibility.
static void popupMenuCallback(GtkWidget* widget, KeyBindingTranslator*)
{
    g_signal_stop_emission_by_name(widget, "popup-menu");
}

static void showHelpCallback(GtkWidget* widget, KeyBindingTranslator*)
{
    g_signal_stop_emission_by_name(widget, "show-help");
}

static const char* const gtkDeleteCommands[][2] = {
    { "DeleteBackward",               "DeleteForward"                        }, // Characters
    { "DeleteWordBackward",           "DeleteWordForward"                    }, // Word ends
    { "DeleteWordBackward",           "DeleteWordForward"                    }, // Words
    { "DeleteToBeginningOfLine",      "DeleteToEndOfLine"                    }, // Lines
    { "DeleteToBeginningOfLine",      "DeleteToEndOfLine"                    }, // Line ends
    { "DeleteToBeginningOfParagraph", "DeleteToEndOfParagraph"               }, // Paragraph ends
    { "DeleteToBeginningOfParagraph", "DeleteToEndOfParagraph"               }, // Paragraphs
    { 0,                              0                                      } // Whitespace (M-\ in Emacs)
};

static void deleteFromCursorCallback(GtkWidget* widget, GtkDeleteType deleteType, gint count, KeyBindingTranslator* translator)
{
    g_signal_stop_emission_by_name(widget, "delete-from-cursor");
    int direction = count > 0 ? 1 : 0;

    // Ensuring that deleteType <= G_N_ELEMENTS here results in a compiler warning
    // that the condition is always true.

    if (deleteType == GTK_DELETE_WORDS) {
        if (!direction) {
            translator->addPendingEditorCommand("MoveWordForward");
            translator->addPendingEditorCommand("MoveWordBackward");
        } else {
            translator->addPendingEditorCommand("MoveWordBackward");
            translator->addPendingEditorCommand("MoveWordForward");
        }
    } else if (deleteType == GTK_DELETE_DISPLAY_LINES) {
        if (!direction)
            translator->addPendingEditorCommand("MoveToBeginningOfLine");
        else
            translator->addPendingEditorCommand("MoveToEndOfLine");
    } else if (deleteType == GTK_DELETE_PARAGRAPHS) {
        if (!direction)
            translator->addPendingEditorCommand("MoveToBeginningOfParagraph");
        else
            translator->addPendingEditorCommand("MoveToEndOfParagraph");
    }

    const char* rawCommand = gtkDeleteCommands[deleteType][direction];
    if (!rawCommand)
      return;

    for (int i = 0; i < abs(count); i++)
        translator->addPendingEditorCommand(rawCommand);
}

static const char* const gtkMoveCommands[][4] = {
    { "MoveBackward",                                   "MoveForward",
      "MoveBackwardAndModifySelection",                 "MoveForwardAndModifySelection"             }, // Forward/backward grapheme
    { "MoveLeft",                                       "MoveRight",
      "MoveBackwardAndModifySelection",                 "MoveForwardAndModifySelection"             }, // Left/right grapheme
    { "MoveWordBackward",                               "MoveWordForward",
      "MoveWordBackwardAndModifySelection",             "MoveWordForwardAndModifySelection"         }, // Forward/backward word
    { "MoveUp",                                         "MoveDown",
      "MoveUpAndModifySelection",                       "MoveDownAndModifySelection"                }, // Up/down line
    { "MoveToBeginningOfLine",                          "MoveToEndOfLine",
      "MoveToBeginningOfLineAndModifySelection",        "MoveToEndOfLineAndModifySelection"         }, // Up/down line ends
    { "MoveParagraphBackward",                          "MoveParagraphForward",
      "MoveParagraphBackwardAndModifySelection",        "MoveParagraphForwardAndModifySelection"    }, // Up/down paragraphs
    { "MoveToBeginningOfParagraph",                     "MoveToEndOfParagraph",
      "MoveToBeginningOfParagraphAndModifySelection",   "MoveToEndOfParagraphAndModifySelection"    }, // Up/down paragraph ends.
    { "MovePageUp",                                     "MovePageDown",
      "MovePageUpAndModifySelection",                   "MovePageDownAndModifySelection"            }, // Up/down page
    { "MoveToBeginningOfDocument",                      "MoveToEndOfDocument",
      "MoveToBeginningOfDocumentAndModifySelection",    "MoveToEndOfDocumentAndModifySelection"     }, // Begin/end of buffer
    { 0,                                                0,
      0,                                                0                                           } // Horizontal page movement
};

static void moveCursorCallback(GtkWidget* widget, GtkMovementStep step, gint count, gboolean extendSelection, KeyBindingTranslator* translator)
{
    g_signal_stop_emission_by_name(widget, "move-cursor");
    int direction = count > 0 ? 1 : 0;
    if (extendSelection)
        direction += 2;

    if (static_cast<unsigned>(step) >= G_N_ELEMENTS(gtkMoveCommands))
        return;

    const char* rawCommand = gtkMoveCommands[step][direction];
    if (!rawCommand)
        return;

    for (int i = 0; i < abs(count); i++)
        translator->addPendingEditorCommand(rawCommand);
}

KeyBindingTranslator::KeyBindingTranslator()
    : m_nativeWidget(gtk_text_view_new())
{
    g_signal_connect(m_nativeWidget.get(), "backspace", G_CALLBACK(backspaceCallback), this);
    g_signal_connect(m_nativeWidget.get(), "cut-clipboard", G_CALLBACK(cutClipboardCallback), this);
    g_signal_connect(m_nativeWidget.get(), "copy-clipboard", G_CALLBACK(copyClipboardCallback), this);
    g_signal_connect(m_nativeWidget.get(), "paste-clipboard", G_CALLBACK(pasteClipboardCallback), this);
    g_signal_connect(m_nativeWidget.get(), "select-all", G_CALLBACK(selectAllCallback), this);
    g_signal_connect(m_nativeWidget.get(), "move-cursor", G_CALLBACK(moveCursorCallback), this);
    g_signal_connect(m_nativeWidget.get(), "delete-from-cursor", G_CALLBACK(deleteFromCursorCallback), this);
    g_signal_connect(m_nativeWidget.get(), "toggle-overwrite", G_CALLBACK(toggleOverwriteCallback), this);
    g_signal_connect(m_nativeWidget.get(), "popup-menu", G_CALLBACK(popupMenuCallback), this);
    g_signal_connect(m_nativeWidget.get(), "show-help", G_CALLBACK(showHelpCallback), this);
}

struct KeyCombinationEntry {
    unsigned gdkKeyCode;
    unsigned state;
    const char* name;
};

static const KeyCombinationEntry keyDownEntries[] = {
    { GDK_b,         GDK_CONTROL_MASK,               "ToggleBold"    },
    { GDK_i,         GDK_CONTROL_MASK,               "ToggleItalic"  },
    { GDK_Escape,    0,                              "Cancel"        },
    { GDK_greater,   GDK_CONTROL_MASK,               "Cancel"        },
};

// These commands are text insertion commands, so should take place
// while handling the KeyPress event.
static const KeyCombinationEntry keyPressEntries[] = {
    { GDK_Tab,       0,                              "InsertTab"     },
    { GDK_Tab,       GDK_SHIFT_MASK,                 "InsertBacktab" },
};

void KeyBindingTranslator::getEditorCommandsForKeyEvent(GdkEventKey* event, EventType type, Vector<WTF::String>& commandList)
{
    m_pendingEditorCommands.clear();

#ifdef GTK_API_VERSION_2
    gtk_bindings_activate_event(GTK_OBJECT(m_nativeWidget.get()), event);
#else
    gtk_bindings_activate_event(G_OBJECT(m_nativeWidget.get()), event);
#endif

    if (!m_pendingEditorCommands.isEmpty()) {
        commandList.appendVector(m_pendingEditorCommands);
        return;
    }

    DEFINE_STATIC_LOCAL(IntConstCharHashMap, keyDownCommandsMap, ());
    DEFINE_STATIC_LOCAL(IntConstCharHashMap, keyPressCommandsMap, ());

    if (keyDownCommandsMap.isEmpty()) {
        for (unsigned i = 0; i < G_N_ELEMENTS(keyDownEntries); i++)
            keyDownCommandsMap.set(keyDownEntries[i].state << 16 | keyDownEntries[i].gdkKeyCode, keyDownEntries[i].name);

        for (unsigned i = 0; i < G_N_ELEMENTS(keyPressEntries); i++)
            keyPressCommandsMap.set(keyPressEntries[i].state << 16 | keyPressEntries[i].gdkKeyCode, keyPressEntries[i].name);
    }

    // Special-case enter keys for we want them to work regardless of modifier.
    if ((event->keyval == GDK_Return || event->keyval == GDK_KP_Enter || event->keyval == GDK_ISO_Enter) && type == KeyPress) {
        commandList.append("InsertNewLine");
        return;
    }

    // For keypress events, we want charCode(), but keyCode() does that.
    int mapKey = event->state << 16 | event->keyval;
    if (mapKey) {
        HashMap<int, const char*>* commandMap = type == KeyDown ?  &keyDownCommandsMap : &keyPressCommandsMap;
        if (const char* commandString = commandMap->get(mapKey)) {
            commandList.append(commandString);
            return;
        }
    }
}

} // namespace WebCore
