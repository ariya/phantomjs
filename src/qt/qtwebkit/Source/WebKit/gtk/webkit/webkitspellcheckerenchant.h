/*
 * Copyright (C) 2011 Igalia S.L.
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
 */

#ifndef webkitspellcheckerenchant_h
#define webkitspellcheckerenchant_h

#if ENABLE(SPELLCHECK)

#include <glib-object.h>
#include <webkit/webkitdefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_SPELL_CHECKER_ENCHANT            (webkit_spell_checker_enchant_get_type())
#define WEBKIT_SPELL_CHECKER_ENCHANT(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_SPELL_CHECKER_ENCHANT, WebKitSpellCheckerEnchant))
#define WEBKIT_SPELL_CHECKER_ENCHANT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  WEBKIT_TYPE_SPELL_CHECKER_ENCHANT, WebKitSpellCheckerEnchantClass))
#define WEBKIT_IS_SPELL_CHECKER_ENCHANT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), WEBKIT_TYPE_SPELL_CHECKER_ENCHANT))
#define WEBKIT_IS_SPELL_CHECKER_ENCHANT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  WEBKIT_TYPE_SPELL_CHECKER_ENCHANT))
#define WEBKIT_SPELL_CHECKER_ENCHANT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  WEBKIT_TYPE_SPELL_CHECKER_ENCHANT, WebKitSpellCheckerEnchantClass))

typedef struct _WebKitSpellCheckerEnchant WebKitSpellCheckerEnchant;
typedef struct _WebKitSpellCheckerEnchantClass WebKitSpellCheckerEnchantClass;
typedef struct _WebKitSpellCheckerEnchantPrivate WebKitSpellCheckerEnchantPrivate;

struct _WebKitSpellCheckerEnchant {
    GObject parent_instance;

    /*< private >*/
    WebKitSpellCheckerEnchantPrivate *priv;
};

struct _WebKitSpellCheckerEnchantClass {
    GObjectClass parent_class;
};

WEBKIT_API GType webkit_spell_checker_enchant_get_type(void);

G_END_DECLS

#endif /* ENABLE(SPELLCHECK) */

#endif
