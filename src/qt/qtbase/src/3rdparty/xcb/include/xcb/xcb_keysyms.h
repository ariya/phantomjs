#ifndef __XCB_KEYSYMS_H__
#define __XCB_KEYSYMS_H__

#include <xcb/xcb.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _XCBKeySymbols xcb_key_symbols_t;

xcb_key_symbols_t *xcb_key_symbols_alloc        (xcb_connection_t         *c);

void           xcb_key_symbols_free         (xcb_key_symbols_t         *syms);

xcb_keysym_t      xcb_key_symbols_get_keysym    (xcb_key_symbols_t         *syms,
					  xcb_keycode_t             keycode,
					  int                    col);

/**
 * @brief Get the keycodes attached to a keysyms.
 * There can be several value, so what is returned is an array of keycode
 * terminated by XCB_NO_SYMBOL. You are responsible to free it.
 * Be aware that this function can be slow. It will convert all
 * combinations of all available keycodes to keysyms to find the ones that
 * match.
 * @param syms Key symbols.
 * @param keysym The keysym to look for.
 * @return A XCB_NO_SYMBOL terminated array of keycode, or NULL if nothing is found.
 */
xcb_keycode_t * xcb_key_symbols_get_keycode(xcb_key_symbols_t *syms,
                                            xcb_keysym_t keysym);

xcb_keysym_t      xcb_key_press_lookup_keysym   (xcb_key_symbols_t         *syms,
					  xcb_key_press_event_t      *event,
					  int                    col);

xcb_keysym_t      xcb_key_release_lookup_keysym (xcb_key_symbols_t         *syms,
					  xcb_key_release_event_t    *event,
					  int                    col);

int            xcb_refresh_keyboard_mapping (xcb_key_symbols_t         *syms,
					  xcb_mapping_notify_event_t *event);

/* TODO:  need XLookupString equivalent */

/* Tests for classes of symbols */

int xcb_is_keypad_key        (xcb_keysym_t keysym);

int xcb_is_private_keypad_key (xcb_keysym_t keysym);

int xcb_is_cursor_key        (xcb_keysym_t keysym);

int xcb_is_pf_key            (xcb_keysym_t keysym);

int xcb_is_function_key      (xcb_keysym_t keysym);

int xcb_is_misc_function_key  (xcb_keysym_t keysym);

int xcb_is_modifier_key      (xcb_keysym_t keysym);


#ifdef __cplusplus
}
#endif


#endif /* __XCB_KEYSYMS_H__ */
