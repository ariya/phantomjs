/*
 * This file generated automatically from xkb.xml by c_client.py.
 * Edit at your peril.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stddef.h>  /* for offsetof() */
#include "xcbext.h"
#include "xkb.h"

#define ALIGNOF(type) offsetof(struct { char dummy; type member; }, member)
#include "xproto.h"

xcb_extension_t xcb_xkb_id = { "XKEYBOARD", 0 };

int qt_xcb_sumof(uint8_t *list, int len)
{
    int i, s = 0;
    for(i=0; i<len; i++) {
        s += *list;
        list++;
    }
    return s;
}

/*****************************************************************************
 **
 ** void xcb_xkb_device_spec_next
 ** 
 ** @param xcb_xkb_device_spec_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_device_spec_next (xcb_xkb_device_spec_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_device_spec_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_device_spec_end
 ** 
 ** @param xcb_xkb_device_spec_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_device_spec_end (xcb_xkb_device_spec_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_led_class_spec_next
 ** 
 ** @param xcb_xkb_led_class_spec_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_led_class_spec_next (xcb_xkb_led_class_spec_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_led_class_spec_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_led_class_spec_end
 ** 
 ** @param xcb_xkb_led_class_spec_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_led_class_spec_end (xcb_xkb_led_class_spec_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_bell_class_spec_next
 ** 
 ** @param xcb_xkb_bell_class_spec_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_bell_class_spec_next (xcb_xkb_bell_class_spec_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_bell_class_spec_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_bell_class_spec_end
 ** 
 ** @param xcb_xkb_bell_class_spec_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_bell_class_spec_end (xcb_xkb_bell_class_spec_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_id_spec_next
 ** 
 ** @param xcb_xkb_id_spec_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_id_spec_next (xcb_xkb_id_spec_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_id_spec_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_id_spec_end
 ** 
 ** @param xcb_xkb_id_spec_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_id_spec_end (xcb_xkb_id_spec_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_indicator_map_next
 ** 
 ** @param xcb_xkb_indicator_map_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_indicator_map_next (xcb_xkb_indicator_map_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_indicator_map_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_indicator_map_end
 ** 
 ** @param xcb_xkb_indicator_map_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_indicator_map_end (xcb_xkb_indicator_map_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_mod_def_next
 ** 
 ** @param xcb_xkb_mod_def_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_mod_def_next (xcb_xkb_mod_def_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_mod_def_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_mod_def_end
 ** 
 ** @param xcb_xkb_mod_def_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_mod_def_end (xcb_xkb_mod_def_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_key_name_next
 ** 
 ** @param xcb_xkb_key_name_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_key_name_next (xcb_xkb_key_name_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_key_name_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_key_name_end
 ** 
 ** @param xcb_xkb_key_name_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_key_name_end (xcb_xkb_key_name_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_key_alias_next
 ** 
 ** @param xcb_xkb_key_alias_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_key_alias_next (xcb_xkb_key_alias_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_key_alias_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_key_alias_end
 ** 
 ** @param xcb_xkb_key_alias_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_key_alias_end (xcb_xkb_key_alias_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

int
xcb_xkb_counted_string_16_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_counted_string_16_t *_aux = (xcb_xkb_counted_string_16_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_counted_string_16_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* string */
    xcb_block_len += _aux->length * sizeof(char);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(char);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* alignment_pad */
    xcb_block_len += (((_aux->length + 5) & (~3)) - (_aux->length + 2)) * sizeof(char);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(char);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** char * xcb_xkb_counted_string_16_string
 ** 
 ** @param const xcb_xkb_counted_string_16_t *R
 ** @returns char *
 **
 *****************************************************************************/
 
char *
xcb_xkb_counted_string_16_string (const xcb_xkb_counted_string_16_t *R  /**< */)
{
    return (char *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_counted_string_16_string_length
 ** 
 ** @param const xcb_xkb_counted_string_16_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_counted_string_16_string_length (const xcb_xkb_counted_string_16_t *R  /**< */)
{
    return R->length;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_counted_string_16_string_end
 ** 
 ** @param const xcb_xkb_counted_string_16_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_counted_string_16_string_end (const xcb_xkb_counted_string_16_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((char *) (R + 1)) + (R->length);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void * xcb_xkb_counted_string_16_alignment_pad
 ** 
 ** @param const xcb_xkb_counted_string_16_t *R
 ** @returns void *
 **
 *****************************************************************************/
 
void *
xcb_xkb_counted_string_16_alignment_pad (const xcb_xkb_counted_string_16_t *R  /**< */)
{
    xcb_generic_iterator_t prev = xcb_xkb_counted_string_16_string_end(R);
    return (void *) ((char *) prev.data + XCB_TYPE_PAD(char, prev.index) + 0);
}


/*****************************************************************************
 **
 ** int xcb_xkb_counted_string_16_alignment_pad_length
 ** 
 ** @param const xcb_xkb_counted_string_16_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_counted_string_16_alignment_pad_length (const xcb_xkb_counted_string_16_t *R  /**< */)
{
    return (((R->length + 5) & (~3)) - (R->length + 2));
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_counted_string_16_alignment_pad_end
 ** 
 ** @param const xcb_xkb_counted_string_16_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_counted_string_16_alignment_pad_end (const xcb_xkb_counted_string_16_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    xcb_generic_iterator_t child = xcb_xkb_counted_string_16_string_end(R);
    i.data = ((char *) child.data) + ((((R->length + 5) & (~3)) - (R->length + 2)));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_counted_string_16_next
 ** 
 ** @param xcb_xkb_counted_string_16_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_counted_string_16_next (xcb_xkb_counted_string_16_iterator_t *i  /**< */)
{
    xcb_xkb_counted_string_16_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_counted_string_16_t *)(((char *)R) + xcb_xkb_counted_string_16_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_counted_string_16_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_counted_string_16_end
 ** 
 ** @param xcb_xkb_counted_string_16_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_counted_string_16_end (xcb_xkb_counted_string_16_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_counted_string_16_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_kt_map_entry_next
 ** 
 ** @param xcb_xkb_kt_map_entry_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_kt_map_entry_next (xcb_xkb_kt_map_entry_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_kt_map_entry_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_kt_map_entry_end
 ** 
 ** @param xcb_xkb_kt_map_entry_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_kt_map_entry_end (xcb_xkb_kt_map_entry_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

int
xcb_xkb_key_type_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_key_type_t *_aux = (xcb_xkb_key_type_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_key_type_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* map */
    xcb_block_len += _aux->nMapEntries * sizeof(xcb_xkb_kt_map_entry_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_kt_map_entry_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* preserve */
    xcb_block_len += (_aux->hasPreserve * _aux->nMapEntries) * sizeof(xcb_xkb_mod_def_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_mod_def_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_kt_map_entry_t * xcb_xkb_key_type_map
 ** 
 ** @param const xcb_xkb_key_type_t *R
 ** @returns xcb_xkb_kt_map_entry_t *
 **
 *****************************************************************************/
 
xcb_xkb_kt_map_entry_t *
xcb_xkb_key_type_map (const xcb_xkb_key_type_t *R  /**< */)
{
    return (xcb_xkb_kt_map_entry_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_key_type_map_length
 ** 
 ** @param const xcb_xkb_key_type_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_key_type_map_length (const xcb_xkb_key_type_t *R  /**< */)
{
    return R->nMapEntries;
}


/*****************************************************************************
 **
 ** xcb_xkb_kt_map_entry_iterator_t xcb_xkb_key_type_map_iterator
 ** 
 ** @param const xcb_xkb_key_type_t *R
 ** @returns xcb_xkb_kt_map_entry_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_kt_map_entry_iterator_t
xcb_xkb_key_type_map_iterator (const xcb_xkb_key_type_t *R  /**< */)
{
    xcb_xkb_kt_map_entry_iterator_t i;
    i.data = (xcb_xkb_kt_map_entry_t *) (R + 1);
    i.rem = R->nMapEntries;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_mod_def_t * xcb_xkb_key_type_preserve
 ** 
 ** @param const xcb_xkb_key_type_t *R
 ** @returns xcb_xkb_mod_def_t *
 **
 *****************************************************************************/
 
xcb_xkb_mod_def_t *
xcb_xkb_key_type_preserve (const xcb_xkb_key_type_t *R  /**< */)
{
    xcb_generic_iterator_t prev = xcb_xkb_kt_map_entry_end(xcb_xkb_key_type_map_iterator(R));
    return (xcb_xkb_mod_def_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_mod_def_t, prev.index) + 0);
}


/*****************************************************************************
 **
 ** int xcb_xkb_key_type_preserve_length
 ** 
 ** @param const xcb_xkb_key_type_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_key_type_preserve_length (const xcb_xkb_key_type_t *R  /**< */)
{
    return (R->hasPreserve * R->nMapEntries);
}


/*****************************************************************************
 **
 ** xcb_xkb_mod_def_iterator_t xcb_xkb_key_type_preserve_iterator
 ** 
 ** @param const xcb_xkb_key_type_t *R
 ** @returns xcb_xkb_mod_def_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_mod_def_iterator_t
xcb_xkb_key_type_preserve_iterator (const xcb_xkb_key_type_t *R  /**< */)
{
    xcb_xkb_mod_def_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_kt_map_entry_end(xcb_xkb_key_type_map_iterator(R));
    i.data = (xcb_xkb_mod_def_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_mod_def_t, prev.index));
    i.rem = (R->hasPreserve * R->nMapEntries);
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_key_type_next
 ** 
 ** @param xcb_xkb_key_type_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_key_type_next (xcb_xkb_key_type_iterator_t *i  /**< */)
{
    xcb_xkb_key_type_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_key_type_t *)(((char *)R) + xcb_xkb_key_type_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_key_type_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_key_type_end
 ** 
 ** @param xcb_xkb_key_type_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_key_type_end (xcb_xkb_key_type_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_key_type_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}

int
xcb_xkb_key_sym_map_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_key_sym_map_t *_aux = (xcb_xkb_key_sym_map_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_key_sym_map_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* syms */
    xcb_block_len += _aux->nSyms * sizeof(xcb_keysym_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_keysym_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_keysym_t * xcb_xkb_key_sym_map_syms
 ** 
 ** @param const xcb_xkb_key_sym_map_t *R
 ** @returns xcb_keysym_t *
 **
 *****************************************************************************/
 
xcb_keysym_t *
xcb_xkb_key_sym_map_syms (const xcb_xkb_key_sym_map_t *R  /**< */)
{
    return (xcb_keysym_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_key_sym_map_syms_length
 ** 
 ** @param const xcb_xkb_key_sym_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_key_sym_map_syms_length (const xcb_xkb_key_sym_map_t *R  /**< */)
{
    return R->nSyms;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_key_sym_map_syms_end
 ** 
 ** @param const xcb_xkb_key_sym_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_key_sym_map_syms_end (const xcb_xkb_key_sym_map_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((xcb_keysym_t *) (R + 1)) + (R->nSyms);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_key_sym_map_next
 ** 
 ** @param xcb_xkb_key_sym_map_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_key_sym_map_next (xcb_xkb_key_sym_map_iterator_t *i  /**< */)
{
    xcb_xkb_key_sym_map_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_key_sym_map_t *)(((char *)R) + xcb_xkb_key_sym_map_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_key_sym_map_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_key_sym_map_end
 ** 
 ** @param xcb_xkb_key_sym_map_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_key_sym_map_end (xcb_xkb_key_sym_map_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_key_sym_map_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_common_behavior_next
 ** 
 ** @param xcb_xkb_common_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_common_behavior_next (xcb_xkb_common_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_common_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_common_behavior_end
 ** 
 ** @param xcb_xkb_common_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_common_behavior_end (xcb_xkb_common_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_default_behavior_next
 ** 
 ** @param xcb_xkb_default_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_default_behavior_next (xcb_xkb_default_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_default_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_default_behavior_end
 ** 
 ** @param xcb_xkb_default_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_default_behavior_end (xcb_xkb_default_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_lock_behavior_next
 ** 
 ** @param xcb_xkb_lock_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_lock_behavior_next (xcb_xkb_lock_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_lock_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_lock_behavior_end
 ** 
 ** @param xcb_xkb_lock_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_lock_behavior_end (xcb_xkb_lock_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_radio_group_behavior_next
 ** 
 ** @param xcb_xkb_radio_group_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_radio_group_behavior_next (xcb_xkb_radio_group_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_radio_group_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_radio_group_behavior_end
 ** 
 ** @param xcb_xkb_radio_group_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_radio_group_behavior_end (xcb_xkb_radio_group_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_overlay_behavior_next
 ** 
 ** @param xcb_xkb_overlay_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_overlay_behavior_next (xcb_xkb_overlay_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_overlay_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_overlay_behavior_end
 ** 
 ** @param xcb_xkb_overlay_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_overlay_behavior_end (xcb_xkb_overlay_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_permament_lock_behavior_next
 ** 
 ** @param xcb_xkb_permament_lock_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_permament_lock_behavior_next (xcb_xkb_permament_lock_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_permament_lock_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_permament_lock_behavior_end
 ** 
 ** @param xcb_xkb_permament_lock_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_permament_lock_behavior_end (xcb_xkb_permament_lock_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_permament_radio_group_behavior_next
 ** 
 ** @param xcb_xkb_permament_radio_group_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_permament_radio_group_behavior_next (xcb_xkb_permament_radio_group_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_permament_radio_group_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_permament_radio_group_behavior_end
 ** 
 ** @param xcb_xkb_permament_radio_group_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_permament_radio_group_behavior_end (xcb_xkb_permament_radio_group_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_permament_overlay_behavior_next
 ** 
 ** @param xcb_xkb_permament_overlay_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_permament_overlay_behavior_next (xcb_xkb_permament_overlay_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_permament_overlay_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_permament_overlay_behavior_end
 ** 
 ** @param xcb_xkb_permament_overlay_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_permament_overlay_behavior_end (xcb_xkb_permament_overlay_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_behavior_next
 ** 
 ** @param xcb_xkb_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_behavior_next (xcb_xkb_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_behavior_end
 ** 
 ** @param xcb_xkb_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_behavior_end (xcb_xkb_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_set_behavior_next
 ** 
 ** @param xcb_xkb_set_behavior_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_set_behavior_next (xcb_xkb_set_behavior_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_set_behavior_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_behavior_end
 ** 
 ** @param xcb_xkb_set_behavior_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_behavior_end (xcb_xkb_set_behavior_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_set_explicit_next
 ** 
 ** @param xcb_xkb_set_explicit_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_set_explicit_next (xcb_xkb_set_explicit_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_set_explicit_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_explicit_end
 ** 
 ** @param xcb_xkb_set_explicit_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_explicit_end (xcb_xkb_set_explicit_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_key_mod_map_next
 ** 
 ** @param xcb_xkb_key_mod_map_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_key_mod_map_next (xcb_xkb_key_mod_map_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_key_mod_map_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_key_mod_map_end
 ** 
 ** @param xcb_xkb_key_mod_map_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_key_mod_map_end (xcb_xkb_key_mod_map_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_key_v_mod_map_next
 ** 
 ** @param xcb_xkb_key_v_mod_map_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_key_v_mod_map_next (xcb_xkb_key_v_mod_map_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_key_v_mod_map_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_key_v_mod_map_end
 ** 
 ** @param xcb_xkb_key_v_mod_map_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_key_v_mod_map_end (xcb_xkb_key_v_mod_map_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_kt_set_map_entry_next
 ** 
 ** @param xcb_xkb_kt_set_map_entry_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_kt_set_map_entry_next (xcb_xkb_kt_set_map_entry_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_kt_set_map_entry_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_kt_set_map_entry_end
 ** 
 ** @param xcb_xkb_kt_set_map_entry_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_kt_set_map_entry_end (xcb_xkb_kt_set_map_entry_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

int
xcb_xkb_set_key_type_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_set_key_type_t *_aux = (xcb_xkb_set_key_type_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_set_key_type_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* entries */
    xcb_block_len += _aux->nMapEntries * sizeof(xcb_xkb_kt_set_map_entry_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_kt_set_map_entry_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* preserve_entries */
    xcb_block_len += (_aux->preserve * _aux->nMapEntries) * sizeof(xcb_xkb_kt_set_map_entry_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_kt_set_map_entry_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_kt_set_map_entry_t * xcb_xkb_set_key_type_entries
 ** 
 ** @param const xcb_xkb_set_key_type_t *R
 ** @returns xcb_xkb_kt_set_map_entry_t *
 **
 *****************************************************************************/
 
xcb_xkb_kt_set_map_entry_t *
xcb_xkb_set_key_type_entries (const xcb_xkb_set_key_type_t *R  /**< */)
{
    return (xcb_xkb_kt_set_map_entry_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_key_type_entries_length
 ** 
 ** @param const xcb_xkb_set_key_type_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_key_type_entries_length (const xcb_xkb_set_key_type_t *R  /**< */)
{
    return R->nMapEntries;
}


/*****************************************************************************
 **
 ** xcb_xkb_kt_set_map_entry_iterator_t xcb_xkb_set_key_type_entries_iterator
 ** 
 ** @param const xcb_xkb_set_key_type_t *R
 ** @returns xcb_xkb_kt_set_map_entry_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_kt_set_map_entry_iterator_t
xcb_xkb_set_key_type_entries_iterator (const xcb_xkb_set_key_type_t *R  /**< */)
{
    xcb_xkb_kt_set_map_entry_iterator_t i;
    i.data = (xcb_xkb_kt_set_map_entry_t *) (R + 1);
    i.rem = R->nMapEntries;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_kt_set_map_entry_t * xcb_xkb_set_key_type_preserve_entries
 ** 
 ** @param const xcb_xkb_set_key_type_t *R
 ** @returns xcb_xkb_kt_set_map_entry_t *
 **
 *****************************************************************************/
 
xcb_xkb_kt_set_map_entry_t *
xcb_xkb_set_key_type_preserve_entries (const xcb_xkb_set_key_type_t *R  /**< */)
{
    xcb_generic_iterator_t prev = xcb_xkb_kt_set_map_entry_end(xcb_xkb_set_key_type_entries_iterator(R));
    return (xcb_xkb_kt_set_map_entry_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_kt_set_map_entry_t, prev.index) + 0);
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_key_type_preserve_entries_length
 ** 
 ** @param const xcb_xkb_set_key_type_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_key_type_preserve_entries_length (const xcb_xkb_set_key_type_t *R  /**< */)
{
    return (R->preserve * R->nMapEntries);
}


/*****************************************************************************
 **
 ** xcb_xkb_kt_set_map_entry_iterator_t xcb_xkb_set_key_type_preserve_entries_iterator
 ** 
 ** @param const xcb_xkb_set_key_type_t *R
 ** @returns xcb_xkb_kt_set_map_entry_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_kt_set_map_entry_iterator_t
xcb_xkb_set_key_type_preserve_entries_iterator (const xcb_xkb_set_key_type_t *R  /**< */)
{
    xcb_xkb_kt_set_map_entry_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_kt_set_map_entry_end(xcb_xkb_set_key_type_entries_iterator(R));
    i.data = (xcb_xkb_kt_set_map_entry_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_kt_set_map_entry_t, prev.index));
    i.rem = (R->preserve * R->nMapEntries);
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_set_key_type_next
 ** 
 ** @param xcb_xkb_set_key_type_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_set_key_type_next (xcb_xkb_set_key_type_iterator_t *i  /**< */)
{
    xcb_xkb_set_key_type_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_set_key_type_t *)(((char *)R) + xcb_xkb_set_key_type_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_set_key_type_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_key_type_end
 ** 
 ** @param xcb_xkb_set_key_type_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_key_type_end (xcb_xkb_set_key_type_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_set_key_type_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_string8_next
 ** 
 ** @param xcb_xkb_string8_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_string8_next (xcb_xkb_string8_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_string8_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_string8_end
 ** 
 ** @param xcb_xkb_string8_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_string8_end (xcb_xkb_string8_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

int
xcb_xkb_outline_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_outline_t *_aux = (xcb_xkb_outline_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_outline_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* points */
    xcb_block_len += _aux->nPoints * sizeof(xcb_point_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_point_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_point_t * xcb_xkb_outline_points
 ** 
 ** @param const xcb_xkb_outline_t *R
 ** @returns xcb_point_t *
 **
 *****************************************************************************/
 
xcb_point_t *
xcb_xkb_outline_points (const xcb_xkb_outline_t *R  /**< */)
{
    return (xcb_point_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_outline_points_length
 ** 
 ** @param const xcb_xkb_outline_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_outline_points_length (const xcb_xkb_outline_t *R  /**< */)
{
    return R->nPoints;
}


/*****************************************************************************
 **
 ** xcb_point_iterator_t xcb_xkb_outline_points_iterator
 ** 
 ** @param const xcb_xkb_outline_t *R
 ** @returns xcb_point_iterator_t
 **
 *****************************************************************************/
 
xcb_point_iterator_t
xcb_xkb_outline_points_iterator (const xcb_xkb_outline_t *R  /**< */)
{
    xcb_point_iterator_t i;
    i.data = (xcb_point_t *) (R + 1);
    i.rem = R->nPoints;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_outline_next
 ** 
 ** @param xcb_xkb_outline_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_outline_next (xcb_xkb_outline_iterator_t *i  /**< */)
{
    xcb_xkb_outline_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_outline_t *)(((char *)R) + xcb_xkb_outline_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_outline_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_outline_end
 ** 
 ** @param xcb_xkb_outline_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_outline_end (xcb_xkb_outline_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_outline_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}

int
xcb_xkb_shape_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_shape_t *_aux = (xcb_xkb_shape_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;

    unsigned int i;
    unsigned int xcb_tmp_len;

    xcb_block_len += sizeof(xcb_xkb_shape_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* outlines */
    for(i=0; i<_aux->nOutlines; i++) {
        xcb_tmp_len = xcb_xkb_outline_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_outline_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** int xcb_xkb_shape_outlines_length
 ** 
 ** @param const xcb_xkb_shape_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_shape_outlines_length (const xcb_xkb_shape_t *R  /**< */)
{
    return R->nOutlines;
}


/*****************************************************************************
 **
 ** xcb_xkb_outline_iterator_t xcb_xkb_shape_outlines_iterator
 ** 
 ** @param const xcb_xkb_shape_t *R
 ** @returns xcb_xkb_outline_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_outline_iterator_t
xcb_xkb_shape_outlines_iterator (const xcb_xkb_shape_t *R  /**< */)
{
    xcb_xkb_outline_iterator_t i;
    i.data = (xcb_xkb_outline_t *) (R + 1);
    i.rem = R->nOutlines;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_shape_next
 ** 
 ** @param xcb_xkb_shape_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_shape_next (xcb_xkb_shape_iterator_t *i  /**< */)
{
    xcb_xkb_shape_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_shape_t *)(((char *)R) + xcb_xkb_shape_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_shape_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_shape_end
 ** 
 ** @param xcb_xkb_shape_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_shape_end (xcb_xkb_shape_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_shape_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_key_next
 ** 
 ** @param xcb_xkb_key_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_key_next (xcb_xkb_key_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_key_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_key_end
 ** 
 ** @param xcb_xkb_key_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_key_end (xcb_xkb_key_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_overlay_key_next
 ** 
 ** @param xcb_xkb_overlay_key_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_overlay_key_next (xcb_xkb_overlay_key_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_overlay_key_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_overlay_key_end
 ** 
 ** @param xcb_xkb_overlay_key_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_overlay_key_end (xcb_xkb_overlay_key_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}

int
xcb_xkb_overlay_row_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_overlay_row_t *_aux = (xcb_xkb_overlay_row_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_overlay_row_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* keys */
    xcb_block_len += _aux->nKeys * sizeof(xcb_xkb_overlay_key_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_overlay_key_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_overlay_key_t * xcb_xkb_overlay_row_keys
 ** 
 ** @param const xcb_xkb_overlay_row_t *R
 ** @returns xcb_xkb_overlay_key_t *
 **
 *****************************************************************************/
 
xcb_xkb_overlay_key_t *
xcb_xkb_overlay_row_keys (const xcb_xkb_overlay_row_t *R  /**< */)
{
    return (xcb_xkb_overlay_key_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_overlay_row_keys_length
 ** 
 ** @param const xcb_xkb_overlay_row_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_overlay_row_keys_length (const xcb_xkb_overlay_row_t *R  /**< */)
{
    return R->nKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_overlay_key_iterator_t xcb_xkb_overlay_row_keys_iterator
 ** 
 ** @param const xcb_xkb_overlay_row_t *R
 ** @returns xcb_xkb_overlay_key_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_overlay_key_iterator_t
xcb_xkb_overlay_row_keys_iterator (const xcb_xkb_overlay_row_t *R  /**< */)
{
    xcb_xkb_overlay_key_iterator_t i;
    i.data = (xcb_xkb_overlay_key_t *) (R + 1);
    i.rem = R->nKeys;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_overlay_row_next
 ** 
 ** @param xcb_xkb_overlay_row_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_overlay_row_next (xcb_xkb_overlay_row_iterator_t *i  /**< */)
{
    xcb_xkb_overlay_row_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_overlay_row_t *)(((char *)R) + xcb_xkb_overlay_row_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_overlay_row_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_overlay_row_end
 ** 
 ** @param xcb_xkb_overlay_row_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_overlay_row_end (xcb_xkb_overlay_row_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_overlay_row_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}

int
xcb_xkb_overlay_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_overlay_t *_aux = (xcb_xkb_overlay_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;

    unsigned int i;
    unsigned int xcb_tmp_len;

    xcb_block_len += sizeof(xcb_xkb_overlay_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* rows */
    for(i=0; i<_aux->nRows; i++) {
        xcb_tmp_len = xcb_xkb_overlay_row_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_overlay_row_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** int xcb_xkb_overlay_rows_length
 ** 
 ** @param const xcb_xkb_overlay_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_overlay_rows_length (const xcb_xkb_overlay_t *R  /**< */)
{
    return R->nRows;
}


/*****************************************************************************
 **
 ** xcb_xkb_overlay_row_iterator_t xcb_xkb_overlay_rows_iterator
 ** 
 ** @param const xcb_xkb_overlay_t *R
 ** @returns xcb_xkb_overlay_row_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_overlay_row_iterator_t
xcb_xkb_overlay_rows_iterator (const xcb_xkb_overlay_t *R  /**< */)
{
    xcb_xkb_overlay_row_iterator_t i;
    i.data = (xcb_xkb_overlay_row_t *) (R + 1);
    i.rem = R->nRows;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_overlay_next
 ** 
 ** @param xcb_xkb_overlay_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_overlay_next (xcb_xkb_overlay_iterator_t *i  /**< */)
{
    xcb_xkb_overlay_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_overlay_t *)(((char *)R) + xcb_xkb_overlay_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_overlay_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_overlay_end
 ** 
 ** @param xcb_xkb_overlay_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_overlay_end (xcb_xkb_overlay_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_overlay_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}

int
xcb_xkb_row_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_row_t *_aux = (xcb_xkb_row_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_row_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* keys */
    xcb_block_len += _aux->nKeys * sizeof(xcb_xkb_key_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_key_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_t * xcb_xkb_row_keys
 ** 
 ** @param const xcb_xkb_row_t *R
 ** @returns xcb_xkb_key_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_t *
xcb_xkb_row_keys (const xcb_xkb_row_t *R  /**< */)
{
    return (xcb_xkb_key_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_row_keys_length
 ** 
 ** @param const xcb_xkb_row_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_row_keys_length (const xcb_xkb_row_t *R  /**< */)
{
    return R->nKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_iterator_t xcb_xkb_row_keys_iterator
 ** 
 ** @param const xcb_xkb_row_t *R
 ** @returns xcb_xkb_key_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_iterator_t
xcb_xkb_row_keys_iterator (const xcb_xkb_row_t *R  /**< */)
{
    xcb_xkb_key_iterator_t i;
    i.data = (xcb_xkb_key_t *) (R + 1);
    i.rem = R->nKeys;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_row_next
 ** 
 ** @param xcb_xkb_row_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_row_next (xcb_xkb_row_iterator_t *i  /**< */)
{
    xcb_xkb_row_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_row_t *)(((char *)R) + xcb_xkb_row_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_row_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_row_end
 ** 
 ** @param xcb_xkb_row_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_row_end (xcb_xkb_row_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_row_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}

int
xcb_xkb_listing_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_listing_t *_aux = (xcb_xkb_listing_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_listing_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* string */
    xcb_block_len += _aux->length * sizeof(xcb_xkb_string8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_string8_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_string8_t * xcb_xkb_listing_string
 ** 
 ** @param const xcb_xkb_listing_t *R
 ** @returns xcb_xkb_string8_t *
 **
 *****************************************************************************/
 
xcb_xkb_string8_t *
xcb_xkb_listing_string (const xcb_xkb_listing_t *R  /**< */)
{
    return (xcb_xkb_string8_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_listing_string_length
 ** 
 ** @param const xcb_xkb_listing_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_listing_string_length (const xcb_xkb_listing_t *R  /**< */)
{
    return R->length;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_listing_string_end
 ** 
 ** @param const xcb_xkb_listing_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_listing_string_end (const xcb_xkb_listing_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((xcb_xkb_string8_t *) (R + 1)) + (R->length);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_listing_next
 ** 
 ** @param xcb_xkb_listing_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_listing_next (xcb_xkb_listing_iterator_t *i  /**< */)
{
    xcb_xkb_listing_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_listing_t *)(((char *)R) + xcb_xkb_listing_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_listing_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_listing_end
 ** 
 ** @param xcb_xkb_listing_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_listing_end (xcb_xkb_listing_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_listing_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}

int
xcb_xkb_device_led_info_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_device_led_info_t *_aux = (xcb_xkb_device_led_info_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_device_led_info_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* names */
    xcb_block_len += xcb_popcount(_aux->namesPresent) * sizeof(uint32_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_atom_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* maps */
    xcb_block_len += xcb_popcount(_aux->mapsPresent) * sizeof(xcb_xkb_indicator_map_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_indicator_map_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_device_led_info_names
 ** 
 ** @param const xcb_xkb_device_led_info_t *R
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_device_led_info_names (const xcb_xkb_device_led_info_t *R  /**< */)
{
    return (xcb_atom_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_device_led_info_names_length
 ** 
 ** @param const xcb_xkb_device_led_info_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_device_led_info_names_length (const xcb_xkb_device_led_info_t *R  /**< */)
{
    return xcb_popcount(R->namesPresent);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_device_led_info_names_end
 ** 
 ** @param const xcb_xkb_device_led_info_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_device_led_info_names_end (const xcb_xkb_device_led_info_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((xcb_atom_t *) (R + 1)) + (xcb_popcount(R->namesPresent));
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_indicator_map_t * xcb_xkb_device_led_info_maps
 ** 
 ** @param const xcb_xkb_device_led_info_t *R
 ** @returns xcb_xkb_indicator_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_indicator_map_t *
xcb_xkb_device_led_info_maps (const xcb_xkb_device_led_info_t *R  /**< */)
{
    xcb_generic_iterator_t prev = xcb_xkb_device_led_info_names_end(R);
    return (xcb_xkb_indicator_map_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_indicator_map_t, prev.index) + 0);
}


/*****************************************************************************
 **
 ** int xcb_xkb_device_led_info_maps_length
 ** 
 ** @param const xcb_xkb_device_led_info_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_device_led_info_maps_length (const xcb_xkb_device_led_info_t *R  /**< */)
{
    return xcb_popcount(R->mapsPresent);
}


/*****************************************************************************
 **
 ** xcb_xkb_indicator_map_iterator_t xcb_xkb_device_led_info_maps_iterator
 ** 
 ** @param const xcb_xkb_device_led_info_t *R
 ** @returns xcb_xkb_indicator_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_indicator_map_iterator_t
xcb_xkb_device_led_info_maps_iterator (const xcb_xkb_device_led_info_t *R  /**< */)
{
    xcb_xkb_indicator_map_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_device_led_info_names_end(R);
    i.data = (xcb_xkb_indicator_map_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_indicator_map_t, prev.index));
    i.rem = xcb_popcount(R->mapsPresent);
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** void xcb_xkb_device_led_info_next
 ** 
 ** @param xcb_xkb_device_led_info_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_device_led_info_next (xcb_xkb_device_led_info_iterator_t *i  /**< */)
{
    xcb_xkb_device_led_info_t *R = i->data;
    xcb_generic_iterator_t child;
    child.data = (xcb_xkb_device_led_info_t *)(((char *)R) + xcb_xkb_device_led_info_sizeof(R));
    i->index = (char *) child.data - (char *) i->data;
    --i->rem;
    i->data = (xcb_xkb_device_led_info_t *) child.data;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_device_led_info_end
 ** 
 ** @param xcb_xkb_device_led_info_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_device_led_info_end (xcb_xkb_device_led_info_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    while(i.rem > 0)
        xcb_xkb_device_led_info_next(&i);
    ret.data = i.data;
    ret.rem = i.rem;
    ret.index = i.index;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_no_action_next
 ** 
 ** @param xcb_xkb_sa_no_action_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_no_action_next (xcb_xkb_sa_no_action_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_no_action_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_no_action_end
 ** 
 ** @param xcb_xkb_sa_no_action_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_no_action_end (xcb_xkb_sa_no_action_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_set_mods_next
 ** 
 ** @param xcb_xkb_sa_set_mods_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_set_mods_next (xcb_xkb_sa_set_mods_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_set_mods_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_set_mods_end
 ** 
 ** @param xcb_xkb_sa_set_mods_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_set_mods_end (xcb_xkb_sa_set_mods_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_latch_mods_next
 ** 
 ** @param xcb_xkb_sa_latch_mods_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_latch_mods_next (xcb_xkb_sa_latch_mods_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_latch_mods_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_latch_mods_end
 ** 
 ** @param xcb_xkb_sa_latch_mods_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_latch_mods_end (xcb_xkb_sa_latch_mods_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_lock_mods_next
 ** 
 ** @param xcb_xkb_sa_lock_mods_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_lock_mods_next (xcb_xkb_sa_lock_mods_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_lock_mods_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_lock_mods_end
 ** 
 ** @param xcb_xkb_sa_lock_mods_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_lock_mods_end (xcb_xkb_sa_lock_mods_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_set_group_next
 ** 
 ** @param xcb_xkb_sa_set_group_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_set_group_next (xcb_xkb_sa_set_group_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_set_group_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_set_group_end
 ** 
 ** @param xcb_xkb_sa_set_group_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_set_group_end (xcb_xkb_sa_set_group_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_latch_group_next
 ** 
 ** @param xcb_xkb_sa_latch_group_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_latch_group_next (xcb_xkb_sa_latch_group_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_latch_group_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_latch_group_end
 ** 
 ** @param xcb_xkb_sa_latch_group_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_latch_group_end (xcb_xkb_sa_latch_group_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_lock_group_next
 ** 
 ** @param xcb_xkb_sa_lock_group_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_lock_group_next (xcb_xkb_sa_lock_group_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_lock_group_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_lock_group_end
 ** 
 ** @param xcb_xkb_sa_lock_group_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_lock_group_end (xcb_xkb_sa_lock_group_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_move_ptr_next
 ** 
 ** @param xcb_xkb_sa_move_ptr_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_move_ptr_next (xcb_xkb_sa_move_ptr_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_move_ptr_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_move_ptr_end
 ** 
 ** @param xcb_xkb_sa_move_ptr_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_move_ptr_end (xcb_xkb_sa_move_ptr_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_ptr_btn_next
 ** 
 ** @param xcb_xkb_sa_ptr_btn_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_ptr_btn_next (xcb_xkb_sa_ptr_btn_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_ptr_btn_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_ptr_btn_end
 ** 
 ** @param xcb_xkb_sa_ptr_btn_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_ptr_btn_end (xcb_xkb_sa_ptr_btn_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_lock_ptr_btn_next
 ** 
 ** @param xcb_xkb_sa_lock_ptr_btn_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_lock_ptr_btn_next (xcb_xkb_sa_lock_ptr_btn_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_lock_ptr_btn_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_lock_ptr_btn_end
 ** 
 ** @param xcb_xkb_sa_lock_ptr_btn_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_lock_ptr_btn_end (xcb_xkb_sa_lock_ptr_btn_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_set_ptr_dflt_next
 ** 
 ** @param xcb_xkb_sa_set_ptr_dflt_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_set_ptr_dflt_next (xcb_xkb_sa_set_ptr_dflt_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_set_ptr_dflt_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_set_ptr_dflt_end
 ** 
 ** @param xcb_xkb_sa_set_ptr_dflt_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_set_ptr_dflt_end (xcb_xkb_sa_set_ptr_dflt_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_iso_lock_next
 ** 
 ** @param xcb_xkb_sa_iso_lock_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_iso_lock_next (xcb_xkb_sa_iso_lock_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_iso_lock_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_iso_lock_end
 ** 
 ** @param xcb_xkb_sa_iso_lock_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_iso_lock_end (xcb_xkb_sa_iso_lock_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_terminate_next
 ** 
 ** @param xcb_xkb_sa_terminate_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_terminate_next (xcb_xkb_sa_terminate_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_terminate_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_terminate_end
 ** 
 ** @param xcb_xkb_sa_terminate_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_terminate_end (xcb_xkb_sa_terminate_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_switch_screen_next
 ** 
 ** @param xcb_xkb_sa_switch_screen_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_switch_screen_next (xcb_xkb_sa_switch_screen_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_switch_screen_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_switch_screen_end
 ** 
 ** @param xcb_xkb_sa_switch_screen_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_switch_screen_end (xcb_xkb_sa_switch_screen_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_set_controls_next
 ** 
 ** @param xcb_xkb_sa_set_controls_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_set_controls_next (xcb_xkb_sa_set_controls_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_set_controls_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_set_controls_end
 ** 
 ** @param xcb_xkb_sa_set_controls_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_set_controls_end (xcb_xkb_sa_set_controls_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_lock_controls_next
 ** 
 ** @param xcb_xkb_sa_lock_controls_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_lock_controls_next (xcb_xkb_sa_lock_controls_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_lock_controls_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_lock_controls_end
 ** 
 ** @param xcb_xkb_sa_lock_controls_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_lock_controls_end (xcb_xkb_sa_lock_controls_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_action_message_next
 ** 
 ** @param xcb_xkb_sa_action_message_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_action_message_next (xcb_xkb_sa_action_message_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_action_message_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_action_message_end
 ** 
 ** @param xcb_xkb_sa_action_message_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_action_message_end (xcb_xkb_sa_action_message_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_redirect_key_next
 ** 
 ** @param xcb_xkb_sa_redirect_key_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_redirect_key_next (xcb_xkb_sa_redirect_key_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_redirect_key_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_redirect_key_end
 ** 
 ** @param xcb_xkb_sa_redirect_key_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_redirect_key_end (xcb_xkb_sa_redirect_key_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_device_btn_next
 ** 
 ** @param xcb_xkb_sa_device_btn_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_device_btn_next (xcb_xkb_sa_device_btn_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_device_btn_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_device_btn_end
 ** 
 ** @param xcb_xkb_sa_device_btn_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_device_btn_end (xcb_xkb_sa_device_btn_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_lock_device_btn_next
 ** 
 ** @param xcb_xkb_sa_lock_device_btn_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_lock_device_btn_next (xcb_xkb_sa_lock_device_btn_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_lock_device_btn_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_lock_device_btn_end
 ** 
 ** @param xcb_xkb_sa_lock_device_btn_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_lock_device_btn_end (xcb_xkb_sa_lock_device_btn_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sa_device_valuator_next
 ** 
 ** @param xcb_xkb_sa_device_valuator_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sa_device_valuator_next (xcb_xkb_sa_device_valuator_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sa_device_valuator_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sa_device_valuator_end
 ** 
 ** @param xcb_xkb_sa_device_valuator_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sa_device_valuator_end (xcb_xkb_sa_device_valuator_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_si_action_next
 ** 
 ** @param xcb_xkb_si_action_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_si_action_next (xcb_xkb_si_action_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_si_action_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_si_action_end
 ** 
 ** @param xcb_xkb_si_action_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_si_action_end (xcb_xkb_si_action_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_sym_interpret_next
 ** 
 ** @param xcb_xkb_sym_interpret_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_sym_interpret_next (xcb_xkb_sym_interpret_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_sym_interpret_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_sym_interpret_end
 ** 
 ** @param xcb_xkb_sym_interpret_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_sym_interpret_end (xcb_xkb_sym_interpret_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** void xcb_xkb_action_next
 ** 
 ** @param xcb_xkb_action_iterator_t *i
 ** @returns void
 **
 *****************************************************************************/
 
void
xcb_xkb_action_next (xcb_xkb_action_iterator_t *i  /**< */)
{
    --i->rem;
    ++i->data;
    i->index += sizeof(xcb_xkb_action_t);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_action_end
 ** 
 ** @param xcb_xkb_action_iterator_t i
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_action_end (xcb_xkb_action_iterator_t i  /**< */)
{
    xcb_generic_iterator_t ret;
    ret.data = i.data + i.rem;
    ret.index = i.index + ((char *) ret.data - (char *) i.data);
    ret.rem = 0;
    return ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_use_extension_cookie_t xcb_xkb_use_extension
 ** 
 ** @param xcb_connection_t *c
 ** @param uint16_t          wantedMajor
 ** @param uint16_t          wantedMinor
 ** @returns xcb_xkb_use_extension_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_use_extension_cookie_t
xcb_xkb_use_extension (xcb_connection_t *c  /**< */,
                       uint16_t          wantedMajor  /**< */,
                       uint16_t          wantedMinor  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_USE_EXTENSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_use_extension_cookie_t xcb_ret;
    xcb_xkb_use_extension_request_t xcb_out;
    
    xcb_out.wantedMajor = wantedMajor;
    xcb_out.wantedMinor = wantedMinor;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_use_extension_cookie_t xcb_xkb_use_extension_unchecked
 ** 
 ** @param xcb_connection_t *c
 ** @param uint16_t          wantedMajor
 ** @param uint16_t          wantedMinor
 ** @returns xcb_xkb_use_extension_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_use_extension_cookie_t
xcb_xkb_use_extension_unchecked (xcb_connection_t *c  /**< */,
                                 uint16_t          wantedMajor  /**< */,
                                 uint16_t          wantedMinor  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_USE_EXTENSION,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_use_extension_cookie_t xcb_ret;
    xcb_xkb_use_extension_request_t xcb_out;
    
    xcb_out.wantedMajor = wantedMajor;
    xcb_out.wantedMinor = wantedMinor;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_use_extension_reply_t * xcb_xkb_use_extension_reply
 ** 
 ** @param xcb_connection_t                *c
 ** @param xcb_xkb_use_extension_cookie_t   cookie
 ** @param xcb_generic_error_t            **e
 ** @returns xcb_xkb_use_extension_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_use_extension_reply_t *
xcb_xkb_use_extension_reply (xcb_connection_t                *c  /**< */,
                             xcb_xkb_use_extension_cookie_t   cookie  /**< */,
                             xcb_generic_error_t            **e  /**< */)
{
    return (xcb_xkb_use_extension_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xkb_select_events_details_serialize (void                                  **_buffer  /**< */,
                                         uint16_t                                affectWhich  /**< */,
                                         uint16_t                                clear  /**< */,
                                         uint16_t                                selectAll  /**< */,
                                         const xcb_xkb_select_events_details_t  *_aux  /**< */)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[23];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectNewKeyboard */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectNewKeyboard;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.newKeyboardDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->newKeyboardDetails;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_STATE_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectState */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectState;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.stateDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->stateDetails;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_CONTROLS_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectCtrls */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectCtrls;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_select_events_details_t.ctrlDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->ctrlDetails;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_INDICATOR_STATE_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectIndicatorState */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectIndicatorState;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_select_events_details_t.indicatorStateDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicatorStateDetails;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_INDICATOR_MAP_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectIndicatorMap */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectIndicatorMap;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_select_events_details_t.indicatorMapDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicatorMapDetails;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_NAMES_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectNames;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.namesDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->namesDetails;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_COMPAT_MAP_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectCompat */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectCompat;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_select_events_details_t.compatDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compatDetails;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_BELL_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectBell */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectBell;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_select_events_details_t.bellDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->bellDetails;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_ACTION_MESSAGE) {
        /* xcb_xkb_select_events_details_t.affectMsgDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectMsgDetails;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_select_events_details_t.msgDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->msgDetails;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_ACCESS_X_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectAccessX */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectAccessX;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.accessXDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->accessXDetails;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_EXTENSION_DEVICE_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectExtDev */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->affectExtDev;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.extdevDetails */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->extdevDetails;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_xkb_select_events_details_unpack (const void                       *_buffer  /**< */,
                                      uint16_t                          affectWhich  /**< */,
                                      uint16_t                          clear  /**< */,
                                      uint16_t                          selectAll  /**< */,
                                      xcb_xkb_select_events_details_t  *_aux  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectNewKeyboard */
        _aux->affectNewKeyboard = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.newKeyboardDetails */
        _aux->newKeyboardDetails = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_STATE_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectState */
        _aux->affectState = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.stateDetails */
        _aux->stateDetails = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_CONTROLS_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectCtrls */
        _aux->affectCtrls = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_select_events_details_t.ctrlDetails */
        _aux->ctrlDetails = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_INDICATOR_STATE_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectIndicatorState */
        _aux->affectIndicatorState = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_select_events_details_t.indicatorStateDetails */
        _aux->indicatorStateDetails = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_INDICATOR_MAP_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectIndicatorMap */
        _aux->affectIndicatorMap = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_select_events_details_t.indicatorMapDetails */
        _aux->indicatorMapDetails = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_NAMES_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectNames */
        _aux->affectNames = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.namesDetails */
        _aux->namesDetails = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_COMPAT_MAP_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectCompat */
        _aux->affectCompat = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_select_events_details_t.compatDetails */
        _aux->compatDetails = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_BELL_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectBell */
        _aux->affectBell = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_select_events_details_t.bellDetails */
        _aux->bellDetails = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_ACTION_MESSAGE) {
        /* xcb_xkb_select_events_details_t.affectMsgDetails */
        _aux->affectMsgDetails = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_select_events_details_t.msgDetails */
        _aux->msgDetails = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_ACCESS_X_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectAccessX */
        _aux->affectAccessX = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.accessXDetails */
        _aux->accessXDetails = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if((affectWhich & ((~clear) & (~selectAll))) & XCB_XKB_EVENT_TYPE_EXTENSION_DEVICE_NOTIFY) {
        /* xcb_xkb_select_events_details_t.affectExtDev */
        _aux->affectExtDev = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_select_events_details_t.extdevDetails */
        _aux->extdevDetails = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

int
xcb_xkb_select_events_details_sizeof (const void  *_buffer  /**< */,
                                      uint16_t     affectWhich  /**< */,
                                      uint16_t     clear  /**< */,
                                      uint16_t     selectAll  /**< */)
{
    xcb_xkb_select_events_details_t _aux;
    return xcb_xkb_select_events_details_unpack(_buffer, affectWhich, clear, selectAll, &_aux);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_select_events_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               affectWhich
 ** @param uint16_t               clear
 ** @param uint16_t               selectAll
 ** @param uint16_t               affectMap
 ** @param uint16_t               map
 ** @param const void            *details
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_select_events_checked (xcb_connection_t      *c  /**< */,
                               xcb_xkb_device_spec_t  deviceSpec  /**< */,
                               uint16_t               affectWhich  /**< */,
                               uint16_t               clear  /**< */,
                               uint16_t               selectAll  /**< */,
                               uint16_t               affectMap  /**< */,
                               uint16_t               map  /**< */,
                               const void            *details  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SELECT_EVENTS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_select_events_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.affectWhich = affectWhich;
    xcb_out.clear = clear;
    xcb_out.selectAll = selectAll;
    xcb_out.affectMap = affectMap;
    xcb_out.map = map;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_select_events_details_t details */
    xcb_parts[4].iov_base = (char *) details;
    xcb_parts[4].iov_len = 
      xcb_xkb_select_events_details_sizeof (details, affectWhich, clear, selectAll);
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_select_events
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               affectWhich
 ** @param uint16_t               clear
 ** @param uint16_t               selectAll
 ** @param uint16_t               affectMap
 ** @param uint16_t               map
 ** @param const void            *details
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_select_events (xcb_connection_t      *c  /**< */,
                       xcb_xkb_device_spec_t  deviceSpec  /**< */,
                       uint16_t               affectWhich  /**< */,
                       uint16_t               clear  /**< */,
                       uint16_t               selectAll  /**< */,
                       uint16_t               affectMap  /**< */,
                       uint16_t               map  /**< */,
                       const void            *details  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SELECT_EVENTS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_select_events_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.affectWhich = affectWhich;
    xcb_out.clear = clear;
    xcb_out.selectAll = selectAll;
    xcb_out.affectMap = affectMap;
    xcb_out.map = map;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_select_events_details_t details */
    xcb_parts[4].iov_base = (char *) details;
    xcb_parts[4].iov_len = 
      xcb_xkb_select_events_details_sizeof (details, affectWhich, clear, selectAll);
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_select_events_aux_checked
 ** 
 ** @param xcb_connection_t                      *c
 ** @param xcb_xkb_device_spec_t                  deviceSpec
 ** @param uint16_t                               affectWhich
 ** @param uint16_t                               clear
 ** @param uint16_t                               selectAll
 ** @param uint16_t                               affectMap
 ** @param uint16_t                               map
 ** @param const xcb_xkb_select_events_details_t *details
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_select_events_aux_checked (xcb_connection_t                      *c  /**< */,
                                   xcb_xkb_device_spec_t                  deviceSpec  /**< */,
                                   uint16_t                               affectWhich  /**< */,
                                   uint16_t                               clear  /**< */,
                                   uint16_t                               selectAll  /**< */,
                                   uint16_t                               affectMap  /**< */,
                                   uint16_t                               map  /**< */,
                                   const xcb_xkb_select_events_details_t *details  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SELECT_EVENTS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_select_events_request_t xcb_out;
    void *xcb_aux0 = 0;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.affectWhich = affectWhich;
    xcb_out.clear = clear;
    xcb_out.selectAll = selectAll;
    xcb_out.affectMap = affectMap;
    xcb_out.map = map;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_select_events_details_t details */
    xcb_parts[4].iov_len = 
      xcb_xkb_select_events_details_serialize (&xcb_aux0, affectWhich, clear, selectAll, details);
    xcb_parts[4].iov_base = xcb_aux0;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    free(xcb_aux0);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_select_events_aux
 ** 
 ** @param xcb_connection_t                      *c
 ** @param xcb_xkb_device_spec_t                  deviceSpec
 ** @param uint16_t                               affectWhich
 ** @param uint16_t                               clear
 ** @param uint16_t                               selectAll
 ** @param uint16_t                               affectMap
 ** @param uint16_t                               map
 ** @param const xcb_xkb_select_events_details_t *details
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_select_events_aux (xcb_connection_t                      *c  /**< */,
                           xcb_xkb_device_spec_t                  deviceSpec  /**< */,
                           uint16_t                               affectWhich  /**< */,
                           uint16_t                               clear  /**< */,
                           uint16_t                               selectAll  /**< */,
                           uint16_t                               affectMap  /**< */,
                           uint16_t                               map  /**< */,
                           const xcb_xkb_select_events_details_t *details  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SELECT_EVENTS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_select_events_request_t xcb_out;
    void *xcb_aux0 = 0;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.affectWhich = affectWhich;
    xcb_out.clear = clear;
    xcb_out.selectAll = selectAll;
    xcb_out.affectMap = affectMap;
    xcb_out.map = map;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_select_events_details_t details */
    xcb_parts[4].iov_len = 
      xcb_xkb_select_events_details_serialize (&xcb_aux0, affectWhich, clear, selectAll, details);
    xcb_parts[4].iov_base = xcb_aux0;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    free(xcb_aux0);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_bell_checked
 ** 
 ** @param xcb_connection_t          *c
 ** @param xcb_xkb_device_spec_t      deviceSpec
 ** @param xcb_xkb_bell_class_spec_t  bellClass
 ** @param xcb_xkb_id_spec_t          bellID
 ** @param int8_t                     percent
 ** @param uint8_t                    forceSound
 ** @param uint8_t                    eventOnly
 ** @param int16_t                    pitch
 ** @param int16_t                    duration
 ** @param xcb_atom_t                 name
 ** @param xcb_window_t               window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_bell_checked (xcb_connection_t          *c  /**< */,
                      xcb_xkb_device_spec_t      deviceSpec  /**< */,
                      xcb_xkb_bell_class_spec_t  bellClass  /**< */,
                      xcb_xkb_id_spec_t          bellID  /**< */,
                      int8_t                     percent  /**< */,
                      uint8_t                    forceSound  /**< */,
                      uint8_t                    eventOnly  /**< */,
                      int16_t                    pitch  /**< */,
                      int16_t                    duration  /**< */,
                      xcb_atom_t                 name  /**< */,
                      xcb_window_t               window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_BELL,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_bell_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.bellClass = bellClass;
    xcb_out.bellID = bellID;
    xcb_out.percent = percent;
    xcb_out.forceSound = forceSound;
    xcb_out.eventOnly = eventOnly;
    xcb_out.pad0 = 0;
    xcb_out.pitch = pitch;
    xcb_out.duration = duration;
    memset(xcb_out.pad1, 0, 2);
    xcb_out.name = name;
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_bell
 ** 
 ** @param xcb_connection_t          *c
 ** @param xcb_xkb_device_spec_t      deviceSpec
 ** @param xcb_xkb_bell_class_spec_t  bellClass
 ** @param xcb_xkb_id_spec_t          bellID
 ** @param int8_t                     percent
 ** @param uint8_t                    forceSound
 ** @param uint8_t                    eventOnly
 ** @param int16_t                    pitch
 ** @param int16_t                    duration
 ** @param xcb_atom_t                 name
 ** @param xcb_window_t               window
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_bell (xcb_connection_t          *c  /**< */,
              xcb_xkb_device_spec_t      deviceSpec  /**< */,
              xcb_xkb_bell_class_spec_t  bellClass  /**< */,
              xcb_xkb_id_spec_t          bellID  /**< */,
              int8_t                     percent  /**< */,
              uint8_t                    forceSound  /**< */,
              uint8_t                    eventOnly  /**< */,
              int16_t                    pitch  /**< */,
              int16_t                    duration  /**< */,
              xcb_atom_t                 name  /**< */,
              xcb_window_t               window  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_BELL,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_bell_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.bellClass = bellClass;
    xcb_out.bellID = bellID;
    xcb_out.percent = percent;
    xcb_out.forceSound = forceSound;
    xcb_out.eventOnly = eventOnly;
    xcb_out.pad0 = 0;
    xcb_out.pitch = pitch;
    xcb_out.duration = duration;
    memset(xcb_out.pad1, 0, 2);
    xcb_out.name = name;
    xcb_out.window = window;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_state_cookie_t xcb_xkb_get_state
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @returns xcb_xkb_get_state_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_state_cookie_t
xcb_xkb_get_state (xcb_connection_t      *c  /**< */,
                   xcb_xkb_device_spec_t  deviceSpec  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_STATE,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_state_cookie_t xcb_ret;
    xcb_xkb_get_state_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_state_cookie_t xcb_xkb_get_state_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @returns xcb_xkb_get_state_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_state_cookie_t
xcb_xkb_get_state_unchecked (xcb_connection_t      *c  /**< */,
                             xcb_xkb_device_spec_t  deviceSpec  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_STATE,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_state_cookie_t xcb_ret;
    xcb_xkb_get_state_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_state_reply_t * xcb_xkb_get_state_reply
 ** 
 ** @param xcb_connection_t            *c
 ** @param xcb_xkb_get_state_cookie_t   cookie
 ** @param xcb_generic_error_t        **e
 ** @returns xcb_xkb_get_state_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_state_reply_t *
xcb_xkb_get_state_reply (xcb_connection_t            *c  /**< */,
                         xcb_xkb_get_state_cookie_t   cookie  /**< */,
                         xcb_generic_error_t        **e  /**< */)
{
    return (xcb_xkb_get_state_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_latch_lock_state_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint8_t                affectModLocks
 ** @param uint8_t                modLocks
 ** @param uint8_t                lockGroup
 ** @param uint8_t                groupLock
 ** @param uint8_t                affectModLatches
 ** @param uint8_t                latchGroup
 ** @param uint16_t               groupLatch
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_latch_lock_state_checked (xcb_connection_t      *c  /**< */,
                                  xcb_xkb_device_spec_t  deviceSpec  /**< */,
                                  uint8_t                affectModLocks  /**< */,
                                  uint8_t                modLocks  /**< */,
                                  uint8_t                lockGroup  /**< */,
                                  uint8_t                groupLock  /**< */,
                                  uint8_t                affectModLatches  /**< */,
                                  uint8_t                latchGroup  /**< */,
                                  uint16_t               groupLatch  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_LATCH_LOCK_STATE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_latch_lock_state_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.affectModLocks = affectModLocks;
    xcb_out.modLocks = modLocks;
    xcb_out.lockGroup = lockGroup;
    xcb_out.groupLock = groupLock;
    xcb_out.affectModLatches = affectModLatches;
    xcb_out.pad0 = 0;
    xcb_out.latchGroup = latchGroup;
    xcb_out.groupLatch = groupLatch;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_latch_lock_state
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint8_t                affectModLocks
 ** @param uint8_t                modLocks
 ** @param uint8_t                lockGroup
 ** @param uint8_t                groupLock
 ** @param uint8_t                affectModLatches
 ** @param uint8_t                latchGroup
 ** @param uint16_t               groupLatch
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_latch_lock_state (xcb_connection_t      *c  /**< */,
                          xcb_xkb_device_spec_t  deviceSpec  /**< */,
                          uint8_t                affectModLocks  /**< */,
                          uint8_t                modLocks  /**< */,
                          uint8_t                lockGroup  /**< */,
                          uint8_t                groupLock  /**< */,
                          uint8_t                affectModLatches  /**< */,
                          uint8_t                latchGroup  /**< */,
                          uint16_t               groupLatch  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_LATCH_LOCK_STATE,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_latch_lock_state_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.affectModLocks = affectModLocks;
    xcb_out.modLocks = modLocks;
    xcb_out.lockGroup = lockGroup;
    xcb_out.groupLock = groupLock;
    xcb_out.affectModLatches = affectModLatches;
    xcb_out.pad0 = 0;
    xcb_out.latchGroup = latchGroup;
    xcb_out.groupLatch = groupLatch;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_controls_cookie_t xcb_xkb_get_controls
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @returns xcb_xkb_get_controls_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_controls_cookie_t
xcb_xkb_get_controls (xcb_connection_t      *c  /**< */,
                      xcb_xkb_device_spec_t  deviceSpec  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_CONTROLS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_controls_cookie_t xcb_ret;
    xcb_xkb_get_controls_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_controls_cookie_t xcb_xkb_get_controls_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @returns xcb_xkb_get_controls_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_controls_cookie_t
xcb_xkb_get_controls_unchecked (xcb_connection_t      *c  /**< */,
                                xcb_xkb_device_spec_t  deviceSpec  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_CONTROLS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_controls_cookie_t xcb_ret;
    xcb_xkb_get_controls_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_controls_reply_t * xcb_xkb_get_controls_reply
 ** 
 ** @param xcb_connection_t               *c
 ** @param xcb_xkb_get_controls_cookie_t   cookie
 ** @param xcb_generic_error_t           **e
 ** @returns xcb_xkb_get_controls_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_controls_reply_t *
xcb_xkb_get_controls_reply (xcb_connection_t               *c  /**< */,
                            xcb_xkb_get_controls_cookie_t   cookie  /**< */,
                            xcb_generic_error_t           **e  /**< */)
{
    return (xcb_xkb_get_controls_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_controls_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint8_t                affectInternalRealMods
 ** @param uint8_t                internalRealMods
 ** @param uint8_t                affectIgnoreLockRealMods
 ** @param uint8_t                ignoreLockRealMods
 ** @param uint16_t               affectInternalVirtualMods
 ** @param uint16_t               internalVirtualMods
 ** @param uint16_t               affectIgnoreLockVirtualMods
 ** @param uint16_t               ignoreLockVirtualMods
 ** @param uint8_t                mouseKeysDfltBtn
 ** @param uint8_t                groupsWrap
 ** @param uint16_t               accessXOptions
 ** @param uint32_t               affectEnabledControls
 ** @param uint32_t               enabledControls
 ** @param uint32_t               changeControls
 ** @param uint16_t               repeatDelay
 ** @param uint16_t               repeatInterval
 ** @param uint16_t               slowKeysDelay
 ** @param uint16_t               debounceDelay
 ** @param uint16_t               mouseKeysDelay
 ** @param uint16_t               mouseKeysInterval
 ** @param uint16_t               mouseKeysTimeToMax
 ** @param uint16_t               mouseKeysMaxSpeed
 ** @param int16_t                mouseKeysCurve
 ** @param uint16_t               accessXTimeout
 ** @param uint32_t               accessXTimeoutMask
 ** @param uint32_t               accessXTimeoutValues
 ** @param uint16_t               accessXTimeoutOptionsMask
 ** @param uint16_t               accessXTimeoutOptionsValues
 ** @param const uint8_t         *perKeyRepeat
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_controls_checked (xcb_connection_t      *c  /**< */,
                              xcb_xkb_device_spec_t  deviceSpec  /**< */,
                              uint8_t                affectInternalRealMods  /**< */,
                              uint8_t                internalRealMods  /**< */,
                              uint8_t                affectIgnoreLockRealMods  /**< */,
                              uint8_t                ignoreLockRealMods  /**< */,
                              uint16_t               affectInternalVirtualMods  /**< */,
                              uint16_t               internalVirtualMods  /**< */,
                              uint16_t               affectIgnoreLockVirtualMods  /**< */,
                              uint16_t               ignoreLockVirtualMods  /**< */,
                              uint8_t                mouseKeysDfltBtn  /**< */,
                              uint8_t                groupsWrap  /**< */,
                              uint16_t               accessXOptions  /**< */,
                              uint32_t               affectEnabledControls  /**< */,
                              uint32_t               enabledControls  /**< */,
                              uint32_t               changeControls  /**< */,
                              uint16_t               repeatDelay  /**< */,
                              uint16_t               repeatInterval  /**< */,
                              uint16_t               slowKeysDelay  /**< */,
                              uint16_t               debounceDelay  /**< */,
                              uint16_t               mouseKeysDelay  /**< */,
                              uint16_t               mouseKeysInterval  /**< */,
                              uint16_t               mouseKeysTimeToMax  /**< */,
                              uint16_t               mouseKeysMaxSpeed  /**< */,
                              int16_t                mouseKeysCurve  /**< */,
                              uint16_t               accessXTimeout  /**< */,
                              uint32_t               accessXTimeoutMask  /**< */,
                              uint32_t               accessXTimeoutValues  /**< */,
                              uint16_t               accessXTimeoutOptionsMask  /**< */,
                              uint16_t               accessXTimeoutOptionsValues  /**< */,
                              const uint8_t         *perKeyRepeat  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_CONTROLS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_controls_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.affectInternalRealMods = affectInternalRealMods;
    xcb_out.internalRealMods = internalRealMods;
    xcb_out.affectIgnoreLockRealMods = affectIgnoreLockRealMods;
    xcb_out.ignoreLockRealMods = ignoreLockRealMods;
    xcb_out.affectInternalVirtualMods = affectInternalVirtualMods;
    xcb_out.internalVirtualMods = internalVirtualMods;
    xcb_out.affectIgnoreLockVirtualMods = affectIgnoreLockVirtualMods;
    xcb_out.ignoreLockVirtualMods = ignoreLockVirtualMods;
    xcb_out.mouseKeysDfltBtn = mouseKeysDfltBtn;
    xcb_out.groupsWrap = groupsWrap;
    xcb_out.accessXOptions = accessXOptions;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.affectEnabledControls = affectEnabledControls;
    xcb_out.enabledControls = enabledControls;
    xcb_out.changeControls = changeControls;
    xcb_out.repeatDelay = repeatDelay;
    xcb_out.repeatInterval = repeatInterval;
    xcb_out.slowKeysDelay = slowKeysDelay;
    xcb_out.debounceDelay = debounceDelay;
    xcb_out.mouseKeysDelay = mouseKeysDelay;
    xcb_out.mouseKeysInterval = mouseKeysInterval;
    xcb_out.mouseKeysTimeToMax = mouseKeysTimeToMax;
    xcb_out.mouseKeysMaxSpeed = mouseKeysMaxSpeed;
    xcb_out.mouseKeysCurve = mouseKeysCurve;
    xcb_out.accessXTimeout = accessXTimeout;
    xcb_out.accessXTimeoutMask = accessXTimeoutMask;
    xcb_out.accessXTimeoutValues = accessXTimeoutValues;
    xcb_out.accessXTimeoutOptionsMask = accessXTimeoutOptionsMask;
    xcb_out.accessXTimeoutOptionsValues = accessXTimeoutOptionsValues;
    memcpy(xcb_out.perKeyRepeat, perKeyRepeat, 32);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_controls
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint8_t                affectInternalRealMods
 ** @param uint8_t                internalRealMods
 ** @param uint8_t                affectIgnoreLockRealMods
 ** @param uint8_t                ignoreLockRealMods
 ** @param uint16_t               affectInternalVirtualMods
 ** @param uint16_t               internalVirtualMods
 ** @param uint16_t               affectIgnoreLockVirtualMods
 ** @param uint16_t               ignoreLockVirtualMods
 ** @param uint8_t                mouseKeysDfltBtn
 ** @param uint8_t                groupsWrap
 ** @param uint16_t               accessXOptions
 ** @param uint32_t               affectEnabledControls
 ** @param uint32_t               enabledControls
 ** @param uint32_t               changeControls
 ** @param uint16_t               repeatDelay
 ** @param uint16_t               repeatInterval
 ** @param uint16_t               slowKeysDelay
 ** @param uint16_t               debounceDelay
 ** @param uint16_t               mouseKeysDelay
 ** @param uint16_t               mouseKeysInterval
 ** @param uint16_t               mouseKeysTimeToMax
 ** @param uint16_t               mouseKeysMaxSpeed
 ** @param int16_t                mouseKeysCurve
 ** @param uint16_t               accessXTimeout
 ** @param uint32_t               accessXTimeoutMask
 ** @param uint32_t               accessXTimeoutValues
 ** @param uint16_t               accessXTimeoutOptionsMask
 ** @param uint16_t               accessXTimeoutOptionsValues
 ** @param const uint8_t         *perKeyRepeat
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_controls (xcb_connection_t      *c  /**< */,
                      xcb_xkb_device_spec_t  deviceSpec  /**< */,
                      uint8_t                affectInternalRealMods  /**< */,
                      uint8_t                internalRealMods  /**< */,
                      uint8_t                affectIgnoreLockRealMods  /**< */,
                      uint8_t                ignoreLockRealMods  /**< */,
                      uint16_t               affectInternalVirtualMods  /**< */,
                      uint16_t               internalVirtualMods  /**< */,
                      uint16_t               affectIgnoreLockVirtualMods  /**< */,
                      uint16_t               ignoreLockVirtualMods  /**< */,
                      uint8_t                mouseKeysDfltBtn  /**< */,
                      uint8_t                groupsWrap  /**< */,
                      uint16_t               accessXOptions  /**< */,
                      uint32_t               affectEnabledControls  /**< */,
                      uint32_t               enabledControls  /**< */,
                      uint32_t               changeControls  /**< */,
                      uint16_t               repeatDelay  /**< */,
                      uint16_t               repeatInterval  /**< */,
                      uint16_t               slowKeysDelay  /**< */,
                      uint16_t               debounceDelay  /**< */,
                      uint16_t               mouseKeysDelay  /**< */,
                      uint16_t               mouseKeysInterval  /**< */,
                      uint16_t               mouseKeysTimeToMax  /**< */,
                      uint16_t               mouseKeysMaxSpeed  /**< */,
                      int16_t                mouseKeysCurve  /**< */,
                      uint16_t               accessXTimeout  /**< */,
                      uint32_t               accessXTimeoutMask  /**< */,
                      uint32_t               accessXTimeoutValues  /**< */,
                      uint16_t               accessXTimeoutOptionsMask  /**< */,
                      uint16_t               accessXTimeoutOptionsValues  /**< */,
                      const uint8_t         *perKeyRepeat  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_CONTROLS,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_controls_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.affectInternalRealMods = affectInternalRealMods;
    xcb_out.internalRealMods = internalRealMods;
    xcb_out.affectIgnoreLockRealMods = affectIgnoreLockRealMods;
    xcb_out.ignoreLockRealMods = ignoreLockRealMods;
    xcb_out.affectInternalVirtualMods = affectInternalVirtualMods;
    xcb_out.internalVirtualMods = internalVirtualMods;
    xcb_out.affectIgnoreLockVirtualMods = affectIgnoreLockVirtualMods;
    xcb_out.ignoreLockVirtualMods = ignoreLockVirtualMods;
    xcb_out.mouseKeysDfltBtn = mouseKeysDfltBtn;
    xcb_out.groupsWrap = groupsWrap;
    xcb_out.accessXOptions = accessXOptions;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.affectEnabledControls = affectEnabledControls;
    xcb_out.enabledControls = enabledControls;
    xcb_out.changeControls = changeControls;
    xcb_out.repeatDelay = repeatDelay;
    xcb_out.repeatInterval = repeatInterval;
    xcb_out.slowKeysDelay = slowKeysDelay;
    xcb_out.debounceDelay = debounceDelay;
    xcb_out.mouseKeysDelay = mouseKeysDelay;
    xcb_out.mouseKeysInterval = mouseKeysInterval;
    xcb_out.mouseKeysTimeToMax = mouseKeysTimeToMax;
    xcb_out.mouseKeysMaxSpeed = mouseKeysMaxSpeed;
    xcb_out.mouseKeysCurve = mouseKeysCurve;
    xcb_out.accessXTimeout = accessXTimeout;
    xcb_out.accessXTimeoutMask = accessXTimeoutMask;
    xcb_out.accessXTimeoutValues = accessXTimeoutValues;
    xcb_out.accessXTimeoutOptionsMask = accessXTimeoutOptionsMask;
    xcb_out.accessXTimeoutOptionsValues = accessXTimeoutOptionsValues;
    memcpy(xcb_out.perKeyRepeat, perKeyRepeat, 32);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_types_rtrn_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_types_rtrn_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                       const xcb_xkb_get_map_map_t *S  /**< */)
{
    return R->nTypes;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_type_iterator_t xcb_xkb_get_map_map_types_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_xkb_key_type_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_type_iterator_t
xcb_xkb_get_map_map_types_rtrn_iterator (const xcb_xkb_get_map_reply_t *R  /**< */,
                                         const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_xkb_key_type_iterator_t i;
    i.data = /* map */ S->types_rtrn;
    i.rem = R->nTypes;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_syms_rtrn_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_syms_rtrn_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                      const xcb_xkb_get_map_map_t *S  /**< */)
{
    return R->nKeySyms;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_sym_map_iterator_t xcb_xkb_get_map_map_syms_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_xkb_key_sym_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_sym_map_iterator_t
xcb_xkb_get_map_map_syms_rtrn_iterator (const xcb_xkb_get_map_reply_t *R  /**< */,
                                        const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_xkb_key_sym_map_iterator_t i;
    i.data = /* map */ S->syms_rtrn;
    i.rem = R->nKeySyms;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_map_map_acts_rtrn_count
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_map_map_acts_rtrn_count (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->acts_rtrn_count;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_acts_rtrn_count_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_acts_rtrn_count_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                            const xcb_xkb_get_map_map_t *S  /**< */)
{
    return R->nKeyActions;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_map_map_acts_rtrn_count_end
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_map_map_acts_rtrn_count_end (const xcb_xkb_get_map_reply_t *R  /**< */,
                                         const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* map */ S->acts_rtrn_count + R->nKeyActions;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_map_map_alignment_pad
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_map_map_alignment_pad (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->alignment_pad;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_alignment_pad_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_alignment_pad_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                          const xcb_xkb_get_map_map_t *S  /**< */)
{
    return (((R->nKeyActions + 3) & (~3)) - R->nKeyActions);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_map_map_alignment_pad_end
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_map_map_alignment_pad_end (const xcb_xkb_get_map_reply_t *R  /**< */,
                                       const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* map */ S->alignment_pad + (((R->nKeyActions + 3) & (~3)) - R->nKeyActions);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_action_t * xcb_xkb_get_map_map_acts_rtrn_acts
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns xcb_xkb_action_t *
 **
 *****************************************************************************/
 
xcb_xkb_action_t *
xcb_xkb_get_map_map_acts_rtrn_acts (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->acts_rtrn_acts;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_acts_rtrn_acts_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_acts_rtrn_acts_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                           const xcb_xkb_get_map_map_t *S  /**< */)
{
    return R->totalActions;
}


/*****************************************************************************
 **
 ** xcb_xkb_action_iterator_t xcb_xkb_get_map_map_acts_rtrn_acts_iterator
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_xkb_action_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_action_iterator_t
xcb_xkb_get_map_map_acts_rtrn_acts_iterator (const xcb_xkb_get_map_reply_t *R  /**< */,
                                             const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_xkb_action_iterator_t i;
    i.data = /* map */ S->acts_rtrn_acts;
    i.rem = R->totalActions;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_behavior_t * xcb_xkb_get_map_map_behaviors_rtrn
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns xcb_xkb_set_behavior_t *
 **
 *****************************************************************************/
 
xcb_xkb_set_behavior_t *
xcb_xkb_get_map_map_behaviors_rtrn (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->behaviors_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_behaviors_rtrn_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_behaviors_rtrn_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                           const xcb_xkb_get_map_map_t *S  /**< */)
{
    return R->totalKeyBehaviors;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_behavior_iterator_t xcb_xkb_get_map_map_behaviors_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_xkb_set_behavior_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_set_behavior_iterator_t
xcb_xkb_get_map_map_behaviors_rtrn_iterator (const xcb_xkb_get_map_reply_t *R  /**< */,
                                             const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_xkb_set_behavior_iterator_t i;
    i.data = /* map */ S->behaviors_rtrn;
    i.rem = R->totalKeyBehaviors;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_map_map_vmods_rtrn
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_map_map_vmods_rtrn (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->vmods_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_vmods_rtrn_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_vmods_rtrn_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                       const xcb_xkb_get_map_map_t *S  /**< */)
{
    return xcb_popcount(R->virtualMods);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_map_map_vmods_rtrn_end
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_map_map_vmods_rtrn_end (const xcb_xkb_get_map_reply_t *R  /**< */,
                                    const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* map */ S->vmods_rtrn + xcb_popcount(R->virtualMods);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_map_map_alignment_pad_2
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_map_map_alignment_pad_2 (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->alignment_pad2;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_alignment_pad_2_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_alignment_pad_2_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                            const xcb_xkb_get_map_map_t *S  /**< */)
{
    return (((xcb_popcount(R->virtualMods) + 3) & (~3)) - xcb_popcount(R->virtualMods));
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_map_map_alignment_pad_2_end
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_map_map_alignment_pad_2_end (const xcb_xkb_get_map_reply_t *R  /**< */,
                                         const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* map */ S->alignment_pad2 + (((xcb_popcount(R->virtualMods) + 3) & (~3)) - xcb_popcount(R->virtualMods));
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_explicit_t * xcb_xkb_get_map_map_explicit_rtrn
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns xcb_xkb_set_explicit_t *
 **
 *****************************************************************************/
 
xcb_xkb_set_explicit_t *
xcb_xkb_get_map_map_explicit_rtrn (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->explicit_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_explicit_rtrn_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_explicit_rtrn_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                          const xcb_xkb_get_map_map_t *S  /**< */)
{
    return R->totalKeyExplicit;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_explicit_iterator_t xcb_xkb_get_map_map_explicit_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_xkb_set_explicit_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_set_explicit_iterator_t
xcb_xkb_get_map_map_explicit_rtrn_iterator (const xcb_xkb_get_map_reply_t *R  /**< */,
                                            const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_xkb_set_explicit_iterator_t i;
    i.data = /* map */ S->explicit_rtrn;
    i.rem = R->totalKeyExplicit;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint16_t * xcb_xkb_get_map_map_alignment_pad_3
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns uint16_t *
 **
 *****************************************************************************/
 
uint16_t *
xcb_xkb_get_map_map_alignment_pad_3 (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->alignment_pad3;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_alignment_pad_3_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_alignment_pad_3_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                            const xcb_xkb_get_map_map_t *S  /**< */)
{
    return (((R->totalKeyExplicit + 1) & (~1)) - R->totalKeyExplicit);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_map_map_alignment_pad_3_end
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_map_map_alignment_pad_3_end (const xcb_xkb_get_map_reply_t *R  /**< */,
                                         const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* map */ S->alignment_pad3 + (((R->totalKeyExplicit + 1) & (~1)) - R->totalKeyExplicit);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_mod_map_t * xcb_xkb_get_map_map_modmap_rtrn
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns xcb_xkb_key_mod_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_mod_map_t *
xcb_xkb_get_map_map_modmap_rtrn (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->modmap_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_modmap_rtrn_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_modmap_rtrn_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                        const xcb_xkb_get_map_map_t *S  /**< */)
{
    return R->totalModMapKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_mod_map_iterator_t xcb_xkb_get_map_map_modmap_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_xkb_key_mod_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_mod_map_iterator_t
xcb_xkb_get_map_map_modmap_rtrn_iterator (const xcb_xkb_get_map_reply_t *R  /**< */,
                                          const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_xkb_key_mod_map_iterator_t i;
    i.data = /* map */ S->modmap_rtrn;
    i.rem = R->totalModMapKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint16_t * xcb_xkb_get_map_map_alignment_pad_4
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns uint16_t *
 **
 *****************************************************************************/
 
uint16_t *
xcb_xkb_get_map_map_alignment_pad_4 (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->alignment_pad4;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_alignment_pad_4_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_alignment_pad_4_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                            const xcb_xkb_get_map_map_t *S  /**< */)
{
    return (((R->totalModMapKeys + 1) & (~1)) - R->totalModMapKeys);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_map_map_alignment_pad_4_end
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_map_map_alignment_pad_4_end (const xcb_xkb_get_map_reply_t *R  /**< */,
                                         const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* map */ S->alignment_pad4 + (((R->totalModMapKeys + 1) & (~1)) - R->totalModMapKeys);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_v_mod_map_t * xcb_xkb_get_map_map_vmodmap_rtrn
 ** 
 ** @param const xcb_xkb_get_map_map_t *S
 ** @returns xcb_xkb_key_v_mod_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_v_mod_map_t *
xcb_xkb_get_map_map_vmodmap_rtrn (const xcb_xkb_get_map_map_t *S  /**< */)
{
    return /* map */ S->vmodmap_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_map_map_vmodmap_rtrn_length
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_map_map_vmodmap_rtrn_length (const xcb_xkb_get_map_reply_t *R  /**< */,
                                         const xcb_xkb_get_map_map_t *S  /**< */)
{
    return R->totalVModMapKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_v_mod_map_iterator_t xcb_xkb_get_map_map_vmodmap_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_map_map_t *R
 ** @returns xcb_xkb_key_v_mod_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_v_mod_map_iterator_t
xcb_xkb_get_map_map_vmodmap_rtrn_iterator (const xcb_xkb_get_map_reply_t *R  /**< */,
                                           const xcb_xkb_get_map_map_t *S  /**< */)
{
    xcb_xkb_key_v_mod_map_iterator_t i;
    i.data = /* map */ S->vmodmap_rtrn;
    i.rem = R->totalVModMapKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}

int
xcb_xkb_get_map_map_serialize (void                        **_buffer  /**< */,
                               uint8_t                       nTypes  /**< */,
                               uint8_t                       nKeySyms  /**< */,
                               uint8_t                       nKeyActions  /**< */,
                               uint16_t                      totalActions  /**< */,
                               uint8_t                       totalKeyBehaviors  /**< */,
                               uint16_t                      virtualMods  /**< */,
                               uint8_t                       totalKeyExplicit  /**< */,
                               uint8_t                       totalModMapKeys  /**< */,
                               uint8_t                       totalVModMapKeys  /**< */,
                               uint16_t                      present  /**< */,
                               const xcb_xkb_get_map_map_t  *_aux  /**< */)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[27];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if(present & XCB_XKB_MAP_PART_KEY_TYPES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* types_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->types_rtrn;
        xcb_parts[xcb_parts_idx].iov_len = 0;
        xcb_tmp = (char *) _aux->types_rtrn;
        for(i=0; i<nTypes; i++) { 
            xcb_block_len = xcb_xkb_key_type_sizeof(xcb_tmp);
            xcb_parts[xcb_parts_idx].iov_len += xcb_block_len;
        }
        xcb_block_len = xcb_parts[xcb_parts_idx].iov_len;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_type_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_SYMS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* syms_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->syms_rtrn;
        xcb_parts[xcb_parts_idx].iov_len = 0;
        xcb_tmp = (char *) _aux->syms_rtrn;
        for(i=0; i<nKeySyms; i++) { 
            xcb_block_len = xcb_xkb_key_sym_map_sizeof(xcb_tmp);
            xcb_parts[xcb_parts_idx].iov_len += xcb_block_len;
        }
        xcb_block_len = xcb_parts[xcb_parts_idx].iov_len;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_sym_map_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_ACTIONS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* acts_rtrn_count */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->acts_rtrn_count;
        xcb_block_len += nKeyActions * sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeyActions * sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->alignment_pad;
        xcb_block_len += (((nKeyActions + 3) & (~3)) - nKeyActions) * sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = (((nKeyActions + 3) & (~3)) - nKeyActions) * sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* acts_rtrn_acts */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->acts_rtrn_acts;
        xcb_block_len += totalActions * sizeof(xcb_xkb_action_t);
        xcb_parts[xcb_parts_idx].iov_len = totalActions * sizeof(xcb_xkb_action_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_action_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_BEHAVIORS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* behaviors_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->behaviors_rtrn;
        xcb_block_len += totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_parts[xcb_parts_idx].iov_len = totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_set_behavior_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MODS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmods_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->vmods_rtrn;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad2 */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->alignment_pad2;
        xcb_block_len += (((xcb_popcount(virtualMods) + 3) & (~3)) - xcb_popcount(virtualMods)) * sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = (((xcb_popcount(virtualMods) + 3) & (~3)) - xcb_popcount(virtualMods)) * sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if(present & XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* explicit_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->explicit_rtrn;
        xcb_block_len += totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_parts[xcb_parts_idx].iov_len = totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_set_explicit_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad3 */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->alignment_pad3;
        xcb_block_len += (((totalKeyExplicit + 1) & (~1)) - totalKeyExplicit) * sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = (((totalKeyExplicit + 1) & (~1)) - totalKeyExplicit) * sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if(present & XCB_XKB_MAP_PART_MODIFIER_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* modmap_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->modmap_rtrn;
        xcb_block_len += totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_parts[xcb_parts_idx].iov_len = totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_mod_map_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad4 */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->alignment_pad4;
        xcb_block_len += (((totalModMapKeys + 1) & (~1)) - totalModMapKeys) * sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = (((totalModMapKeys + 1) & (~1)) - totalModMapKeys) * sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmodmap_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->vmodmap_rtrn;
        xcb_block_len += totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_parts[xcb_parts_idx].iov_len = totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_v_mod_map_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_xkb_get_map_map_unpack (const void             *_buffer  /**< */,
                            uint8_t                 nTypes  /**< */,
                            uint8_t                 nKeySyms  /**< */,
                            uint8_t                 nKeyActions  /**< */,
                            uint16_t                totalActions  /**< */,
                            uint8_t                 totalKeyBehaviors  /**< */,
                            uint16_t                virtualMods  /**< */,
                            uint8_t                 totalKeyExplicit  /**< */,
                            uint8_t                 totalModMapKeys  /**< */,
                            uint8_t                 totalVModMapKeys  /**< */,
                            uint16_t                present  /**< */,
                            xcb_xkb_get_map_map_t  *_aux  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;

    unsigned int i;
    unsigned int xcb_tmp_len;

    if(present & XCB_XKB_MAP_PART_KEY_TYPES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* types_rtrn */
        _aux->types_rtrn = (xcb_xkb_key_type_t *)xcb_tmp;
        for(i=0; i<nTypes; i++) {
            xcb_tmp_len = xcb_xkb_key_type_sizeof(xcb_tmp);
            xcb_block_len += xcb_tmp_len;
            xcb_tmp += xcb_tmp_len;
        }
        xcb_align_to = ALIGNOF(xcb_xkb_key_type_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_SYMS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* syms_rtrn */
        _aux->syms_rtrn = (xcb_xkb_key_sym_map_t *)xcb_tmp;
        for(i=0; i<nKeySyms; i++) {
            xcb_tmp_len = xcb_xkb_key_sym_map_sizeof(xcb_tmp);
            xcb_block_len += xcb_tmp_len;
            xcb_tmp += xcb_tmp_len;
        }
        xcb_align_to = ALIGNOF(xcb_xkb_key_sym_map_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_ACTIONS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* acts_rtrn_count */
        _aux->acts_rtrn_count = (uint8_t *)xcb_tmp;
        xcb_block_len += nKeyActions * sizeof(xcb_keycode_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad */
        _aux->alignment_pad = (uint8_t *)xcb_tmp;
        xcb_block_len += (((nKeyActions + 3) & (~3)) - nKeyActions) * sizeof(xcb_keycode_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* acts_rtrn_acts */
        _aux->acts_rtrn_acts = (xcb_xkb_action_t *)xcb_tmp;
        xcb_block_len += totalActions * sizeof(xcb_xkb_action_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_action_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_BEHAVIORS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* behaviors_rtrn */
        _aux->behaviors_rtrn = (xcb_xkb_set_behavior_t *)xcb_tmp;
        xcb_block_len += totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_set_behavior_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MODS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmods_rtrn */
        _aux->vmods_rtrn = (uint8_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad2 */
        _aux->alignment_pad2 = (uint8_t *)xcb_tmp;
        xcb_block_len += (((xcb_popcount(virtualMods) + 3) & (~3)) - xcb_popcount(virtualMods)) * sizeof(xcb_keycode_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if(present & XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* explicit_rtrn */
        _aux->explicit_rtrn = (xcb_xkb_set_explicit_t *)xcb_tmp;
        xcb_block_len += totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_set_explicit_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad3 */
        _aux->alignment_pad3 = (uint16_t *)xcb_tmp;
        xcb_block_len += (((totalKeyExplicit + 1) & (~1)) - totalKeyExplicit) * sizeof(uint16_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if(present & XCB_XKB_MAP_PART_MODIFIER_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* modmap_rtrn */
        _aux->modmap_rtrn = (xcb_xkb_key_mod_map_t *)xcb_tmp;
        xcb_block_len += totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_mod_map_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad4 */
        _aux->alignment_pad4 = (uint16_t *)xcb_tmp;
        xcb_block_len += (((totalModMapKeys + 1) & (~1)) - totalModMapKeys) * sizeof(uint16_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint16_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmodmap_rtrn */
        _aux->vmodmap_rtrn = (xcb_xkb_key_v_mod_map_t *)xcb_tmp;
        xcb_block_len += totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_v_mod_map_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

int
xcb_xkb_get_map_map_sizeof (const void  *_buffer  /**< */,
                            uint8_t      nTypes  /**< */,
                            uint8_t      nKeySyms  /**< */,
                            uint8_t      nKeyActions  /**< */,
                            uint16_t     totalActions  /**< */,
                            uint8_t      totalKeyBehaviors  /**< */,
                            uint16_t     virtualMods  /**< */,
                            uint8_t      totalKeyExplicit  /**< */,
                            uint8_t      totalModMapKeys  /**< */,
                            uint8_t      totalVModMapKeys  /**< */,
                            uint16_t     present  /**< */)
{
    xcb_xkb_get_map_map_t _aux;
    return xcb_xkb_get_map_map_unpack(_buffer, nTypes, nKeySyms, nKeyActions, totalActions, totalKeyBehaviors, virtualMods, totalKeyExplicit, totalModMapKeys, totalVModMapKeys, present, &_aux);
}


/*****************************************************************************
 **
 ** xcb_xkb_get_map_cookie_t xcb_xkb_get_map
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               full
 ** @param uint16_t               partial
 ** @param uint8_t                firstType
 ** @param uint8_t                nTypes
 ** @param xcb_keycode_t          firstKeySym
 ** @param uint8_t                nKeySyms
 ** @param xcb_keycode_t          firstKeyAction
 ** @param uint8_t                nKeyActions
 ** @param xcb_keycode_t          firstKeyBehavior
 ** @param uint8_t                nKeyBehaviors
 ** @param uint16_t               virtualMods
 ** @param xcb_keycode_t          firstKeyExplicit
 ** @param uint8_t                nKeyExplicit
 ** @param xcb_keycode_t          firstModMapKey
 ** @param uint8_t                nModMapKeys
 ** @param xcb_keycode_t          firstVModMapKey
 ** @param uint8_t                nVModMapKeys
 ** @returns xcb_xkb_get_map_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_map_cookie_t
xcb_xkb_get_map (xcb_connection_t      *c  /**< */,
                 xcb_xkb_device_spec_t  deviceSpec  /**< */,
                 uint16_t               full  /**< */,
                 uint16_t               partial  /**< */,
                 uint8_t                firstType  /**< */,
                 uint8_t                nTypes  /**< */,
                 xcb_keycode_t          firstKeySym  /**< */,
                 uint8_t                nKeySyms  /**< */,
                 xcb_keycode_t          firstKeyAction  /**< */,
                 uint8_t                nKeyActions  /**< */,
                 xcb_keycode_t          firstKeyBehavior  /**< */,
                 uint8_t                nKeyBehaviors  /**< */,
                 uint16_t               virtualMods  /**< */,
                 xcb_keycode_t          firstKeyExplicit  /**< */,
                 uint8_t                nKeyExplicit  /**< */,
                 xcb_keycode_t          firstModMapKey  /**< */,
                 uint8_t                nModMapKeys  /**< */,
                 xcb_keycode_t          firstVModMapKey  /**< */,
                 uint8_t                nVModMapKeys  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_MAP,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_map_cookie_t xcb_ret;
    xcb_xkb_get_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.full = full;
    xcb_out.partial = partial;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKeySym = firstKeySym;
    xcb_out.nKeySyms = nKeySyms;
    xcb_out.firstKeyAction = firstKeyAction;
    xcb_out.nKeyActions = nKeyActions;
    xcb_out.firstKeyBehavior = firstKeyBehavior;
    xcb_out.nKeyBehaviors = nKeyBehaviors;
    xcb_out.virtualMods = virtualMods;
    xcb_out.firstKeyExplicit = firstKeyExplicit;
    xcb_out.nKeyExplicit = nKeyExplicit;
    xcb_out.firstModMapKey = firstModMapKey;
    xcb_out.nModMapKeys = nModMapKeys;
    xcb_out.firstVModMapKey = firstVModMapKey;
    xcb_out.nVModMapKeys = nVModMapKeys;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_map_cookie_t xcb_xkb_get_map_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               full
 ** @param uint16_t               partial
 ** @param uint8_t                firstType
 ** @param uint8_t                nTypes
 ** @param xcb_keycode_t          firstKeySym
 ** @param uint8_t                nKeySyms
 ** @param xcb_keycode_t          firstKeyAction
 ** @param uint8_t                nKeyActions
 ** @param xcb_keycode_t          firstKeyBehavior
 ** @param uint8_t                nKeyBehaviors
 ** @param uint16_t               virtualMods
 ** @param xcb_keycode_t          firstKeyExplicit
 ** @param uint8_t                nKeyExplicit
 ** @param xcb_keycode_t          firstModMapKey
 ** @param uint8_t                nModMapKeys
 ** @param xcb_keycode_t          firstVModMapKey
 ** @param uint8_t                nVModMapKeys
 ** @returns xcb_xkb_get_map_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_map_cookie_t
xcb_xkb_get_map_unchecked (xcb_connection_t      *c  /**< */,
                           xcb_xkb_device_spec_t  deviceSpec  /**< */,
                           uint16_t               full  /**< */,
                           uint16_t               partial  /**< */,
                           uint8_t                firstType  /**< */,
                           uint8_t                nTypes  /**< */,
                           xcb_keycode_t          firstKeySym  /**< */,
                           uint8_t                nKeySyms  /**< */,
                           xcb_keycode_t          firstKeyAction  /**< */,
                           uint8_t                nKeyActions  /**< */,
                           xcb_keycode_t          firstKeyBehavior  /**< */,
                           uint8_t                nKeyBehaviors  /**< */,
                           uint16_t               virtualMods  /**< */,
                           xcb_keycode_t          firstKeyExplicit  /**< */,
                           uint8_t                nKeyExplicit  /**< */,
                           xcb_keycode_t          firstModMapKey  /**< */,
                           uint8_t                nModMapKeys  /**< */,
                           xcb_keycode_t          firstVModMapKey  /**< */,
                           uint8_t                nVModMapKeys  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_MAP,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_map_cookie_t xcb_ret;
    xcb_xkb_get_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.full = full;
    xcb_out.partial = partial;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKeySym = firstKeySym;
    xcb_out.nKeySyms = nKeySyms;
    xcb_out.firstKeyAction = firstKeyAction;
    xcb_out.nKeyActions = nKeyActions;
    xcb_out.firstKeyBehavior = firstKeyBehavior;
    xcb_out.nKeyBehaviors = nKeyBehaviors;
    xcb_out.virtualMods = virtualMods;
    xcb_out.firstKeyExplicit = firstKeyExplicit;
    xcb_out.nKeyExplicit = nKeyExplicit;
    xcb_out.firstModMapKey = firstModMapKey;
    xcb_out.nModMapKeys = nModMapKeys;
    xcb_out.firstVModMapKey = firstVModMapKey;
    xcb_out.nVModMapKeys = nVModMapKeys;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_map_map_t * xcb_xkb_get_map_map
 ** 
 ** @param const xcb_xkb_get_map_reply_t *R
 ** @returns xcb_xkb_get_map_map_t *
 **
 *****************************************************************************/
 
void *
xcb_xkb_get_map_map (const xcb_xkb_get_map_reply_t *R  /**< */)
{
    return (void *) (R + 1);
}


/*****************************************************************************
 **
 ** xcb_xkb_get_map_reply_t * xcb_xkb_get_map_reply
 ** 
 ** @param xcb_connection_t          *c
 ** @param xcb_xkb_get_map_cookie_t   cookie
 ** @param xcb_generic_error_t      **e
 ** @returns xcb_xkb_get_map_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_map_reply_t *
xcb_xkb_get_map_reply (xcb_connection_t          *c  /**< */,
                       xcb_xkb_get_map_cookie_t   cookie  /**< */,
                       xcb_generic_error_t      **e  /**< */)
{
    return (xcb_xkb_get_map_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_types_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_types_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                     const xcb_xkb_set_map_values_t *S  /**< */)
{
    return R->nTypes;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_key_type_iterator_t xcb_xkb_set_map_values_types_iterator
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_xkb_set_key_type_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_set_key_type_iterator_t
xcb_xkb_set_map_values_types_iterator (const xcb_xkb_set_map_request_t *R  /**< */,
                                       const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_xkb_set_key_type_iterator_t i;
    i.data = /* values */ S->types;
    i.rem = R->nTypes;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_syms_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_syms_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                    const xcb_xkb_set_map_values_t *S  /**< */)
{
    return R->nKeySyms;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_sym_map_iterator_t xcb_xkb_set_map_values_syms_iterator
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_xkb_key_sym_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_sym_map_iterator_t
xcb_xkb_set_map_values_syms_iterator (const xcb_xkb_set_map_request_t *R  /**< */,
                                      const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_xkb_key_sym_map_iterator_t i;
    i.data = /* values */ S->syms;
    i.rem = R->nKeySyms;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_set_map_values_actions_count
 ** 
 ** @param const xcb_xkb_set_map_values_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_set_map_values_actions_count (const xcb_xkb_set_map_values_t *S  /**< */)
{
    return /* values */ S->actionsCount;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_actions_count_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_actions_count_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                             const xcb_xkb_set_map_values_t *S  /**< */)
{
    return R->nKeyActions;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_map_values_actions_count_end
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_map_values_actions_count_end (const xcb_xkb_set_map_request_t *R  /**< */,
                                          const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->actionsCount + R->nKeyActions;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_action_t * xcb_xkb_set_map_values_actions
 ** 
 ** @param const xcb_xkb_set_map_values_t *S
 ** @returns xcb_xkb_action_t *
 **
 *****************************************************************************/
 
xcb_xkb_action_t *
xcb_xkb_set_map_values_actions (const xcb_xkb_set_map_values_t *S  /**< */)
{
    return /* values */ S->actions;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_actions_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_actions_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                       const xcb_xkb_set_map_values_t *S  /**< */)
{
    return R->totalActions;
}


/*****************************************************************************
 **
 ** xcb_xkb_action_iterator_t xcb_xkb_set_map_values_actions_iterator
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_xkb_action_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_action_iterator_t
xcb_xkb_set_map_values_actions_iterator (const xcb_xkb_set_map_request_t *R  /**< */,
                                         const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_xkb_action_iterator_t i;
    i.data = /* values */ S->actions;
    i.rem = R->totalActions;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_behavior_t * xcb_xkb_set_map_values_behaviors
 ** 
 ** @param const xcb_xkb_set_map_values_t *S
 ** @returns xcb_xkb_set_behavior_t *
 **
 *****************************************************************************/
 
xcb_xkb_set_behavior_t *
xcb_xkb_set_map_values_behaviors (const xcb_xkb_set_map_values_t *S  /**< */)
{
    return /* values */ S->behaviors;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_behaviors_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_behaviors_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                         const xcb_xkb_set_map_values_t *S  /**< */)
{
    return R->totalKeyBehaviors;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_behavior_iterator_t xcb_xkb_set_map_values_behaviors_iterator
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_xkb_set_behavior_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_set_behavior_iterator_t
xcb_xkb_set_map_values_behaviors_iterator (const xcb_xkb_set_map_request_t *R  /**< */,
                                           const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_xkb_set_behavior_iterator_t i;
    i.data = /* values */ S->behaviors;
    i.rem = R->totalKeyBehaviors;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_set_map_values_vmods
 ** 
 ** @param const xcb_xkb_set_map_values_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_set_map_values_vmods (const xcb_xkb_set_map_values_t *S  /**< */)
{
    return /* values */ S->vmods;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_vmods_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_vmods_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                     const xcb_xkb_set_map_values_t *S  /**< */)
{
    return xcb_popcount(R->virtualMods);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_map_values_vmods_end
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_map_values_vmods_end (const xcb_xkb_set_map_request_t *R  /**< */,
                                  const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->vmods + xcb_popcount(R->virtualMods);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_explicit_t * xcb_xkb_set_map_values_explicit
 ** 
 ** @param const xcb_xkb_set_map_values_t *S
 ** @returns xcb_xkb_set_explicit_t *
 **
 *****************************************************************************/
 
xcb_xkb_set_explicit_t *
xcb_xkb_set_map_values_explicit (const xcb_xkb_set_map_values_t *S  /**< */)
{
    return /* values */ S->explicit;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_explicit_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_explicit_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                        const xcb_xkb_set_map_values_t *S  /**< */)
{
    return R->totalKeyExplicit;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_explicit_iterator_t xcb_xkb_set_map_values_explicit_iterator
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_xkb_set_explicit_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_set_explicit_iterator_t
xcb_xkb_set_map_values_explicit_iterator (const xcb_xkb_set_map_request_t *R  /**< */,
                                          const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_xkb_set_explicit_iterator_t i;
    i.data = /* values */ S->explicit;
    i.rem = R->totalKeyExplicit;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_mod_map_t * xcb_xkb_set_map_values_modmap
 ** 
 ** @param const xcb_xkb_set_map_values_t *S
 ** @returns xcb_xkb_key_mod_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_mod_map_t *
xcb_xkb_set_map_values_modmap (const xcb_xkb_set_map_values_t *S  /**< */)
{
    return /* values */ S->modmap;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_modmap_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_modmap_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                      const xcb_xkb_set_map_values_t *S  /**< */)
{
    return R->totalModMapKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_mod_map_iterator_t xcb_xkb_set_map_values_modmap_iterator
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_xkb_key_mod_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_mod_map_iterator_t
xcb_xkb_set_map_values_modmap_iterator (const xcb_xkb_set_map_request_t *R  /**< */,
                                        const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_xkb_key_mod_map_iterator_t i;
    i.data = /* values */ S->modmap;
    i.rem = R->totalModMapKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_v_mod_map_t * xcb_xkb_set_map_values_vmodmap
 ** 
 ** @param const xcb_xkb_set_map_values_t *S
 ** @returns xcb_xkb_key_v_mod_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_v_mod_map_t *
xcb_xkb_set_map_values_vmodmap (const xcb_xkb_set_map_values_t *S  /**< */)
{
    return /* values */ S->vmodmap;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_map_values_vmodmap_length
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_map_values_vmodmap_length (const xcb_xkb_set_map_request_t *R  /**< */,
                                       const xcb_xkb_set_map_values_t *S  /**< */)
{
    return R->totalVModMapKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_v_mod_map_iterator_t xcb_xkb_set_map_values_vmodmap_iterator
 ** 
 ** @param const xcb_xkb_set_map_values_t *R
 ** @returns xcb_xkb_key_v_mod_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_v_mod_map_iterator_t
xcb_xkb_set_map_values_vmodmap_iterator (const xcb_xkb_set_map_request_t *R  /**< */,
                                         const xcb_xkb_set_map_values_t *S  /**< */)
{
    xcb_xkb_key_v_mod_map_iterator_t i;
    i.data = /* values */ S->vmodmap;
    i.rem = R->totalVModMapKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}

int
xcb_xkb_set_map_values_serialize (void                           **_buffer  /**< */,
                                  uint8_t                          nTypes  /**< */,
                                  uint8_t                          nKeySyms  /**< */,
                                  uint8_t                          nKeyActions  /**< */,
                                  uint16_t                         totalActions  /**< */,
                                  uint8_t                          totalKeyBehaviors  /**< */,
                                  uint16_t                         virtualMods  /**< */,
                                  uint8_t                          totalKeyExplicit  /**< */,
                                  uint8_t                          totalModMapKeys  /**< */,
                                  uint8_t                          totalVModMapKeys  /**< */,
                                  uint16_t                         present  /**< */,
                                  const xcb_xkb_set_map_values_t  *_aux  /**< */)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[19];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if(present & XCB_XKB_MAP_PART_KEY_TYPES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* types */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->types;
        xcb_parts[xcb_parts_idx].iov_len = 0;
        xcb_tmp = (char *) _aux->types;
        for(i=0; i<nTypes; i++) { 
            xcb_block_len = xcb_xkb_set_key_type_sizeof(xcb_tmp);
            xcb_parts[xcb_parts_idx].iov_len += xcb_block_len;
        }
        xcb_block_len = xcb_parts[xcb_parts_idx].iov_len;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_set_key_type_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_SYMS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* syms */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->syms;
        xcb_parts[xcb_parts_idx].iov_len = 0;
        xcb_tmp = (char *) _aux->syms;
        for(i=0; i<nKeySyms; i++) { 
            xcb_block_len = xcb_xkb_key_sym_map_sizeof(xcb_tmp);
            xcb_parts[xcb_parts_idx].iov_len += xcb_block_len;
        }
        xcb_block_len = xcb_parts[xcb_parts_idx].iov_len;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_sym_map_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_ACTIONS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* actionsCount */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->actionsCount;
        xcb_block_len += nKeyActions * sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeyActions * sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* actions */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->actions;
        xcb_block_len += totalActions * sizeof(xcb_xkb_action_t);
        xcb_parts[xcb_parts_idx].iov_len = totalActions * sizeof(xcb_xkb_action_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_action_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_BEHAVIORS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* behaviors */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->behaviors;
        xcb_block_len += totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_parts[xcb_parts_idx].iov_len = totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_set_behavior_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MODS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmods */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->vmods;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if(present & XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* explicit */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->explicit;
        xcb_block_len += totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_parts[xcb_parts_idx].iov_len = totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_set_explicit_t);
    }
    if(present & XCB_XKB_MAP_PART_MODIFIER_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* modmap */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->modmap;
        xcb_block_len += totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_parts[xcb_parts_idx].iov_len = totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_mod_map_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmodmap */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->vmodmap;
        xcb_block_len += totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_parts[xcb_parts_idx].iov_len = totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_v_mod_map_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_xkb_set_map_values_unpack (const void                *_buffer  /**< */,
                               uint8_t                    nTypes  /**< */,
                               uint8_t                    nKeySyms  /**< */,
                               uint8_t                    nKeyActions  /**< */,
                               uint16_t                   totalActions  /**< */,
                               uint8_t                    totalKeyBehaviors  /**< */,
                               uint16_t                   virtualMods  /**< */,
                               uint8_t                    totalKeyExplicit  /**< */,
                               uint8_t                    totalModMapKeys  /**< */,
                               uint8_t                    totalVModMapKeys  /**< */,
                               uint16_t                   present  /**< */,
                               xcb_xkb_set_map_values_t  *_aux  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;

    unsigned int i;
    unsigned int xcb_tmp_len;

    if(present & XCB_XKB_MAP_PART_KEY_TYPES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* types */
        _aux->types = (xcb_xkb_set_key_type_t *)xcb_tmp;
        for(i=0; i<nTypes; i++) {
            xcb_tmp_len = xcb_xkb_set_key_type_sizeof(xcb_tmp);
            xcb_block_len += xcb_tmp_len;
            xcb_tmp += xcb_tmp_len;
        }
        xcb_align_to = ALIGNOF(xcb_xkb_set_key_type_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_SYMS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* syms */
        _aux->syms = (xcb_xkb_key_sym_map_t *)xcb_tmp;
        for(i=0; i<nKeySyms; i++) {
            xcb_tmp_len = xcb_xkb_key_sym_map_sizeof(xcb_tmp);
            xcb_block_len += xcb_tmp_len;
            xcb_tmp += xcb_tmp_len;
        }
        xcb_align_to = ALIGNOF(xcb_xkb_key_sym_map_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_ACTIONS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* actionsCount */
        _aux->actionsCount = (uint8_t *)xcb_tmp;
        xcb_block_len += nKeyActions * sizeof(xcb_keycode_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* actions */
        _aux->actions = (xcb_xkb_action_t *)xcb_tmp;
        xcb_block_len += totalActions * sizeof(xcb_xkb_action_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_action_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_BEHAVIORS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* behaviors */
        _aux->behaviors = (xcb_xkb_set_behavior_t *)xcb_tmp;
        xcb_block_len += totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_set_behavior_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MODS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmods */
        _aux->vmods = (uint8_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if(present & XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* explicit */
        _aux->explicit = (xcb_xkb_set_explicit_t *)xcb_tmp;
        xcb_block_len += totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_set_explicit_t);
    }
    if(present & XCB_XKB_MAP_PART_MODIFIER_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* modmap */
        _aux->modmap = (xcb_xkb_key_mod_map_t *)xcb_tmp;
        xcb_block_len += totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_mod_map_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmodmap */
        _aux->vmodmap = (xcb_xkb_key_v_mod_map_t *)xcb_tmp;
        xcb_block_len += totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_v_mod_map_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

int
xcb_xkb_set_map_values_sizeof (const void  *_buffer  /**< */,
                               uint8_t      nTypes  /**< */,
                               uint8_t      nKeySyms  /**< */,
                               uint8_t      nKeyActions  /**< */,
                               uint16_t     totalActions  /**< */,
                               uint8_t      totalKeyBehaviors  /**< */,
                               uint16_t     virtualMods  /**< */,
                               uint8_t      totalKeyExplicit  /**< */,
                               uint8_t      totalModMapKeys  /**< */,
                               uint8_t      totalVModMapKeys  /**< */,
                               uint16_t     present  /**< */)
{
    xcb_xkb_set_map_values_t _aux;
    return xcb_xkb_set_map_values_unpack(_buffer, nTypes, nKeySyms, nKeyActions, totalActions, totalKeyBehaviors, virtualMods, totalKeyExplicit, totalModMapKeys, totalVModMapKeys, present, &_aux);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_map_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               present
 ** @param uint16_t               flags
 ** @param xcb_keycode_t          minKeyCode
 ** @param xcb_keycode_t          maxKeyCode
 ** @param uint8_t                firstType
 ** @param uint8_t                nTypes
 ** @param xcb_keycode_t          firstKeySym
 ** @param uint8_t                nKeySyms
 ** @param uint16_t               totalSyms
 ** @param xcb_keycode_t          firstKeyAction
 ** @param uint8_t                nKeyActions
 ** @param uint16_t               totalActions
 ** @param xcb_keycode_t          firstKeyBehavior
 ** @param uint8_t                nKeyBehaviors
 ** @param uint8_t                totalKeyBehaviors
 ** @param xcb_keycode_t          firstKeyExplicit
 ** @param uint8_t                nKeyExplicit
 ** @param uint8_t                totalKeyExplicit
 ** @param xcb_keycode_t          firstModMapKey
 ** @param uint8_t                nModMapKeys
 ** @param uint8_t                totalModMapKeys
 ** @param xcb_keycode_t          firstVModMapKey
 ** @param uint8_t                nVModMapKeys
 ** @param uint8_t                totalVModMapKeys
 ** @param uint16_t               virtualMods
 ** @param const void            *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_map_checked (xcb_connection_t      *c  /**< */,
                         xcb_xkb_device_spec_t  deviceSpec  /**< */,
                         uint16_t               present  /**< */,
                         uint16_t               flags  /**< */,
                         xcb_keycode_t          minKeyCode  /**< */,
                         xcb_keycode_t          maxKeyCode  /**< */,
                         uint8_t                firstType  /**< */,
                         uint8_t                nTypes  /**< */,
                         xcb_keycode_t          firstKeySym  /**< */,
                         uint8_t                nKeySyms  /**< */,
                         uint16_t               totalSyms  /**< */,
                         xcb_keycode_t          firstKeyAction  /**< */,
                         uint8_t                nKeyActions  /**< */,
                         uint16_t               totalActions  /**< */,
                         xcb_keycode_t          firstKeyBehavior  /**< */,
                         uint8_t                nKeyBehaviors  /**< */,
                         uint8_t                totalKeyBehaviors  /**< */,
                         xcb_keycode_t          firstKeyExplicit  /**< */,
                         uint8_t                nKeyExplicit  /**< */,
                         uint8_t                totalKeyExplicit  /**< */,
                         xcb_keycode_t          firstModMapKey  /**< */,
                         uint8_t                nModMapKeys  /**< */,
                         uint8_t                totalModMapKeys  /**< */,
                         xcb_keycode_t          firstVModMapKey  /**< */,
                         uint8_t                nVModMapKeys  /**< */,
                         uint8_t                totalVModMapKeys  /**< */,
                         uint16_t               virtualMods  /**< */,
                         const void            *values  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_MAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.present = present;
    xcb_out.flags = flags;
    xcb_out.minKeyCode = minKeyCode;
    xcb_out.maxKeyCode = maxKeyCode;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKeySym = firstKeySym;
    xcb_out.nKeySyms = nKeySyms;
    xcb_out.totalSyms = totalSyms;
    xcb_out.firstKeyAction = firstKeyAction;
    xcb_out.nKeyActions = nKeyActions;
    xcb_out.totalActions = totalActions;
    xcb_out.firstKeyBehavior = firstKeyBehavior;
    xcb_out.nKeyBehaviors = nKeyBehaviors;
    xcb_out.totalKeyBehaviors = totalKeyBehaviors;
    xcb_out.firstKeyExplicit = firstKeyExplicit;
    xcb_out.nKeyExplicit = nKeyExplicit;
    xcb_out.totalKeyExplicit = totalKeyExplicit;
    xcb_out.firstModMapKey = firstModMapKey;
    xcb_out.nModMapKeys = nModMapKeys;
    xcb_out.totalModMapKeys = totalModMapKeys;
    xcb_out.firstVModMapKey = firstVModMapKey;
    xcb_out.nVModMapKeys = nVModMapKeys;
    xcb_out.totalVModMapKeys = totalVModMapKeys;
    xcb_out.virtualMods = virtualMods;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_set_map_values_t values */
    xcb_parts[4].iov_base = (char *) values;
    xcb_parts[4].iov_len = 
      xcb_xkb_set_map_values_sizeof (values, nTypes, nKeySyms, nKeyActions, totalActions, totalKeyBehaviors, virtualMods, totalKeyExplicit, totalModMapKeys, totalVModMapKeys, present);
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_map
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               present
 ** @param uint16_t               flags
 ** @param xcb_keycode_t          minKeyCode
 ** @param xcb_keycode_t          maxKeyCode
 ** @param uint8_t                firstType
 ** @param uint8_t                nTypes
 ** @param xcb_keycode_t          firstKeySym
 ** @param uint8_t                nKeySyms
 ** @param uint16_t               totalSyms
 ** @param xcb_keycode_t          firstKeyAction
 ** @param uint8_t                nKeyActions
 ** @param uint16_t               totalActions
 ** @param xcb_keycode_t          firstKeyBehavior
 ** @param uint8_t                nKeyBehaviors
 ** @param uint8_t                totalKeyBehaviors
 ** @param xcb_keycode_t          firstKeyExplicit
 ** @param uint8_t                nKeyExplicit
 ** @param uint8_t                totalKeyExplicit
 ** @param xcb_keycode_t          firstModMapKey
 ** @param uint8_t                nModMapKeys
 ** @param uint8_t                totalModMapKeys
 ** @param xcb_keycode_t          firstVModMapKey
 ** @param uint8_t                nVModMapKeys
 ** @param uint8_t                totalVModMapKeys
 ** @param uint16_t               virtualMods
 ** @param const void            *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_map (xcb_connection_t      *c  /**< */,
                 xcb_xkb_device_spec_t  deviceSpec  /**< */,
                 uint16_t               present  /**< */,
                 uint16_t               flags  /**< */,
                 xcb_keycode_t          minKeyCode  /**< */,
                 xcb_keycode_t          maxKeyCode  /**< */,
                 uint8_t                firstType  /**< */,
                 uint8_t                nTypes  /**< */,
                 xcb_keycode_t          firstKeySym  /**< */,
                 uint8_t                nKeySyms  /**< */,
                 uint16_t               totalSyms  /**< */,
                 xcb_keycode_t          firstKeyAction  /**< */,
                 uint8_t                nKeyActions  /**< */,
                 uint16_t               totalActions  /**< */,
                 xcb_keycode_t          firstKeyBehavior  /**< */,
                 uint8_t                nKeyBehaviors  /**< */,
                 uint8_t                totalKeyBehaviors  /**< */,
                 xcb_keycode_t          firstKeyExplicit  /**< */,
                 uint8_t                nKeyExplicit  /**< */,
                 uint8_t                totalKeyExplicit  /**< */,
                 xcb_keycode_t          firstModMapKey  /**< */,
                 uint8_t                nModMapKeys  /**< */,
                 uint8_t                totalModMapKeys  /**< */,
                 xcb_keycode_t          firstVModMapKey  /**< */,
                 uint8_t                nVModMapKeys  /**< */,
                 uint8_t                totalVModMapKeys  /**< */,
                 uint16_t               virtualMods  /**< */,
                 const void            *values  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_MAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.present = present;
    xcb_out.flags = flags;
    xcb_out.minKeyCode = minKeyCode;
    xcb_out.maxKeyCode = maxKeyCode;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKeySym = firstKeySym;
    xcb_out.nKeySyms = nKeySyms;
    xcb_out.totalSyms = totalSyms;
    xcb_out.firstKeyAction = firstKeyAction;
    xcb_out.nKeyActions = nKeyActions;
    xcb_out.totalActions = totalActions;
    xcb_out.firstKeyBehavior = firstKeyBehavior;
    xcb_out.nKeyBehaviors = nKeyBehaviors;
    xcb_out.totalKeyBehaviors = totalKeyBehaviors;
    xcb_out.firstKeyExplicit = firstKeyExplicit;
    xcb_out.nKeyExplicit = nKeyExplicit;
    xcb_out.totalKeyExplicit = totalKeyExplicit;
    xcb_out.firstModMapKey = firstModMapKey;
    xcb_out.nModMapKeys = nModMapKeys;
    xcb_out.totalModMapKeys = totalModMapKeys;
    xcb_out.firstVModMapKey = firstVModMapKey;
    xcb_out.nVModMapKeys = nVModMapKeys;
    xcb_out.totalVModMapKeys = totalVModMapKeys;
    xcb_out.virtualMods = virtualMods;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_set_map_values_t values */
    xcb_parts[4].iov_base = (char *) values;
    xcb_parts[4].iov_len = 
      xcb_xkb_set_map_values_sizeof (values, nTypes, nKeySyms, nKeyActions, totalActions, totalKeyBehaviors, virtualMods, totalKeyExplicit, totalModMapKeys, totalVModMapKeys, present);
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_map_aux_checked
 ** 
 ** @param xcb_connection_t               *c
 ** @param xcb_xkb_device_spec_t           deviceSpec
 ** @param uint16_t                        present
 ** @param uint16_t                        flags
 ** @param xcb_keycode_t                   minKeyCode
 ** @param xcb_keycode_t                   maxKeyCode
 ** @param uint8_t                         firstType
 ** @param uint8_t                         nTypes
 ** @param xcb_keycode_t                   firstKeySym
 ** @param uint8_t                         nKeySyms
 ** @param uint16_t                        totalSyms
 ** @param xcb_keycode_t                   firstKeyAction
 ** @param uint8_t                         nKeyActions
 ** @param uint16_t                        totalActions
 ** @param xcb_keycode_t                   firstKeyBehavior
 ** @param uint8_t                         nKeyBehaviors
 ** @param uint8_t                         totalKeyBehaviors
 ** @param xcb_keycode_t                   firstKeyExplicit
 ** @param uint8_t                         nKeyExplicit
 ** @param uint8_t                         totalKeyExplicit
 ** @param xcb_keycode_t                   firstModMapKey
 ** @param uint8_t                         nModMapKeys
 ** @param uint8_t                         totalModMapKeys
 ** @param xcb_keycode_t                   firstVModMapKey
 ** @param uint8_t                         nVModMapKeys
 ** @param uint8_t                         totalVModMapKeys
 ** @param uint16_t                        virtualMods
 ** @param const xcb_xkb_set_map_values_t *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_map_aux_checked (xcb_connection_t               *c  /**< */,
                             xcb_xkb_device_spec_t           deviceSpec  /**< */,
                             uint16_t                        present  /**< */,
                             uint16_t                        flags  /**< */,
                             xcb_keycode_t                   minKeyCode  /**< */,
                             xcb_keycode_t                   maxKeyCode  /**< */,
                             uint8_t                         firstType  /**< */,
                             uint8_t                         nTypes  /**< */,
                             xcb_keycode_t                   firstKeySym  /**< */,
                             uint8_t                         nKeySyms  /**< */,
                             uint16_t                        totalSyms  /**< */,
                             xcb_keycode_t                   firstKeyAction  /**< */,
                             uint8_t                         nKeyActions  /**< */,
                             uint16_t                        totalActions  /**< */,
                             xcb_keycode_t                   firstKeyBehavior  /**< */,
                             uint8_t                         nKeyBehaviors  /**< */,
                             uint8_t                         totalKeyBehaviors  /**< */,
                             xcb_keycode_t                   firstKeyExplicit  /**< */,
                             uint8_t                         nKeyExplicit  /**< */,
                             uint8_t                         totalKeyExplicit  /**< */,
                             xcb_keycode_t                   firstModMapKey  /**< */,
                             uint8_t                         nModMapKeys  /**< */,
                             uint8_t                         totalModMapKeys  /**< */,
                             xcb_keycode_t                   firstVModMapKey  /**< */,
                             uint8_t                         nVModMapKeys  /**< */,
                             uint8_t                         totalVModMapKeys  /**< */,
                             uint16_t                        virtualMods  /**< */,
                             const xcb_xkb_set_map_values_t *values  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_MAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_map_request_t xcb_out;
    void *xcb_aux0 = 0;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.present = present;
    xcb_out.flags = flags;
    xcb_out.minKeyCode = minKeyCode;
    xcb_out.maxKeyCode = maxKeyCode;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKeySym = firstKeySym;
    xcb_out.nKeySyms = nKeySyms;
    xcb_out.totalSyms = totalSyms;
    xcb_out.firstKeyAction = firstKeyAction;
    xcb_out.nKeyActions = nKeyActions;
    xcb_out.totalActions = totalActions;
    xcb_out.firstKeyBehavior = firstKeyBehavior;
    xcb_out.nKeyBehaviors = nKeyBehaviors;
    xcb_out.totalKeyBehaviors = totalKeyBehaviors;
    xcb_out.firstKeyExplicit = firstKeyExplicit;
    xcb_out.nKeyExplicit = nKeyExplicit;
    xcb_out.totalKeyExplicit = totalKeyExplicit;
    xcb_out.firstModMapKey = firstModMapKey;
    xcb_out.nModMapKeys = nModMapKeys;
    xcb_out.totalModMapKeys = totalModMapKeys;
    xcb_out.firstVModMapKey = firstVModMapKey;
    xcb_out.nVModMapKeys = nVModMapKeys;
    xcb_out.totalVModMapKeys = totalVModMapKeys;
    xcb_out.virtualMods = virtualMods;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_set_map_values_t values */
    xcb_parts[4].iov_len = 
      xcb_xkb_set_map_values_serialize (&xcb_aux0, nTypes, nKeySyms, nKeyActions, totalActions, totalKeyBehaviors, virtualMods, totalKeyExplicit, totalModMapKeys, totalVModMapKeys, present, values);
    xcb_parts[4].iov_base = xcb_aux0;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    free(xcb_aux0);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_map_aux
 ** 
 ** @param xcb_connection_t               *c
 ** @param xcb_xkb_device_spec_t           deviceSpec
 ** @param uint16_t                        present
 ** @param uint16_t                        flags
 ** @param xcb_keycode_t                   minKeyCode
 ** @param xcb_keycode_t                   maxKeyCode
 ** @param uint8_t                         firstType
 ** @param uint8_t                         nTypes
 ** @param xcb_keycode_t                   firstKeySym
 ** @param uint8_t                         nKeySyms
 ** @param uint16_t                        totalSyms
 ** @param xcb_keycode_t                   firstKeyAction
 ** @param uint8_t                         nKeyActions
 ** @param uint16_t                        totalActions
 ** @param xcb_keycode_t                   firstKeyBehavior
 ** @param uint8_t                         nKeyBehaviors
 ** @param uint8_t                         totalKeyBehaviors
 ** @param xcb_keycode_t                   firstKeyExplicit
 ** @param uint8_t                         nKeyExplicit
 ** @param uint8_t                         totalKeyExplicit
 ** @param xcb_keycode_t                   firstModMapKey
 ** @param uint8_t                         nModMapKeys
 ** @param uint8_t                         totalModMapKeys
 ** @param xcb_keycode_t                   firstVModMapKey
 ** @param uint8_t                         nVModMapKeys
 ** @param uint8_t                         totalVModMapKeys
 ** @param uint16_t                        virtualMods
 ** @param const xcb_xkb_set_map_values_t *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_map_aux (xcb_connection_t               *c  /**< */,
                     xcb_xkb_device_spec_t           deviceSpec  /**< */,
                     uint16_t                        present  /**< */,
                     uint16_t                        flags  /**< */,
                     xcb_keycode_t                   minKeyCode  /**< */,
                     xcb_keycode_t                   maxKeyCode  /**< */,
                     uint8_t                         firstType  /**< */,
                     uint8_t                         nTypes  /**< */,
                     xcb_keycode_t                   firstKeySym  /**< */,
                     uint8_t                         nKeySyms  /**< */,
                     uint16_t                        totalSyms  /**< */,
                     xcb_keycode_t                   firstKeyAction  /**< */,
                     uint8_t                         nKeyActions  /**< */,
                     uint16_t                        totalActions  /**< */,
                     xcb_keycode_t                   firstKeyBehavior  /**< */,
                     uint8_t                         nKeyBehaviors  /**< */,
                     uint8_t                         totalKeyBehaviors  /**< */,
                     xcb_keycode_t                   firstKeyExplicit  /**< */,
                     uint8_t                         nKeyExplicit  /**< */,
                     uint8_t                         totalKeyExplicit  /**< */,
                     xcb_keycode_t                   firstModMapKey  /**< */,
                     uint8_t                         nModMapKeys  /**< */,
                     uint8_t                         totalModMapKeys  /**< */,
                     xcb_keycode_t                   firstVModMapKey  /**< */,
                     uint8_t                         nVModMapKeys  /**< */,
                     uint8_t                         totalVModMapKeys  /**< */,
                     uint16_t                        virtualMods  /**< */,
                     const xcb_xkb_set_map_values_t *values  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_MAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_map_request_t xcb_out;
    void *xcb_aux0 = 0;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.present = present;
    xcb_out.flags = flags;
    xcb_out.minKeyCode = minKeyCode;
    xcb_out.maxKeyCode = maxKeyCode;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKeySym = firstKeySym;
    xcb_out.nKeySyms = nKeySyms;
    xcb_out.totalSyms = totalSyms;
    xcb_out.firstKeyAction = firstKeyAction;
    xcb_out.nKeyActions = nKeyActions;
    xcb_out.totalActions = totalActions;
    xcb_out.firstKeyBehavior = firstKeyBehavior;
    xcb_out.nKeyBehaviors = nKeyBehaviors;
    xcb_out.totalKeyBehaviors = totalKeyBehaviors;
    xcb_out.firstKeyExplicit = firstKeyExplicit;
    xcb_out.nKeyExplicit = nKeyExplicit;
    xcb_out.totalKeyExplicit = totalKeyExplicit;
    xcb_out.firstModMapKey = firstModMapKey;
    xcb_out.nModMapKeys = nModMapKeys;
    xcb_out.totalModMapKeys = totalModMapKeys;
    xcb_out.firstVModMapKey = firstVModMapKey;
    xcb_out.nVModMapKeys = nVModMapKeys;
    xcb_out.totalVModMapKeys = totalVModMapKeys;
    xcb_out.virtualMods = virtualMods;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_set_map_values_t values */
    xcb_parts[4].iov_len = 
      xcb_xkb_set_map_values_serialize (&xcb_aux0, nTypes, nKeySyms, nKeyActions, totalActions, totalKeyBehaviors, virtualMods, totalKeyExplicit, totalModMapKeys, totalVModMapKeys, present, values);
    xcb_parts[4].iov_base = xcb_aux0;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    free(xcb_aux0);
    return xcb_ret;
}

int
xcb_xkb_get_compat_map_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_get_compat_map_reply_t *_aux = (xcb_xkb_get_compat_map_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_get_compat_map_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* si_rtrn */
    xcb_block_len += _aux->nSIRtrn * sizeof(xcb_xkb_sym_interpret_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_sym_interpret_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* group_rtrn */
    xcb_block_len += xcb_popcount(_aux->groupsRtrn) * sizeof(xcb_xkb_mod_def_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_mod_def_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_compat_map_cookie_t xcb_xkb_get_compat_map
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint8_t                groups
 ** @param uint8_t                getAllSI
 ** @param uint16_t               firstSI
 ** @param uint16_t               nSI
 ** @returns xcb_xkb_get_compat_map_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_compat_map_cookie_t
xcb_xkb_get_compat_map (xcb_connection_t      *c  /**< */,
                        xcb_xkb_device_spec_t  deviceSpec  /**< */,
                        uint8_t                groups  /**< */,
                        uint8_t                getAllSI  /**< */,
                        uint16_t               firstSI  /**< */,
                        uint16_t               nSI  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_COMPAT_MAP,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_compat_map_cookie_t xcb_ret;
    xcb_xkb_get_compat_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.groups = groups;
    xcb_out.getAllSI = getAllSI;
    xcb_out.firstSI = firstSI;
    xcb_out.nSI = nSI;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_compat_map_cookie_t xcb_xkb_get_compat_map_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint8_t                groups
 ** @param uint8_t                getAllSI
 ** @param uint16_t               firstSI
 ** @param uint16_t               nSI
 ** @returns xcb_xkb_get_compat_map_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_compat_map_cookie_t
xcb_xkb_get_compat_map_unchecked (xcb_connection_t      *c  /**< */,
                                  xcb_xkb_device_spec_t  deviceSpec  /**< */,
                                  uint8_t                groups  /**< */,
                                  uint8_t                getAllSI  /**< */,
                                  uint16_t               firstSI  /**< */,
                                  uint16_t               nSI  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_COMPAT_MAP,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_compat_map_cookie_t xcb_ret;
    xcb_xkb_get_compat_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.groups = groups;
    xcb_out.getAllSI = getAllSI;
    xcb_out.firstSI = firstSI;
    xcb_out.nSI = nSI;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_sym_interpret_t * xcb_xkb_get_compat_map_si_rtrn
 ** 
 ** @param const xcb_xkb_get_compat_map_reply_t *R
 ** @returns xcb_xkb_sym_interpret_t *
 **
 *****************************************************************************/
 
xcb_xkb_sym_interpret_t *
xcb_xkb_get_compat_map_si_rtrn (const xcb_xkb_get_compat_map_reply_t *R  /**< */)
{
    return (xcb_xkb_sym_interpret_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_compat_map_si_rtrn_length
 ** 
 ** @param const xcb_xkb_get_compat_map_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_compat_map_si_rtrn_length (const xcb_xkb_get_compat_map_reply_t *R  /**< */)
{
    return R->nSIRtrn;
}


/*****************************************************************************
 **
 ** xcb_xkb_sym_interpret_iterator_t xcb_xkb_get_compat_map_si_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_compat_map_reply_t *R
 ** @returns xcb_xkb_sym_interpret_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_sym_interpret_iterator_t
xcb_xkb_get_compat_map_si_rtrn_iterator (const xcb_xkb_get_compat_map_reply_t *R  /**< */)
{
    xcb_xkb_sym_interpret_iterator_t i;
    i.data = (xcb_xkb_sym_interpret_t *) (R + 1);
    i.rem = R->nSIRtrn;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_mod_def_t * xcb_xkb_get_compat_map_group_rtrn
 ** 
 ** @param const xcb_xkb_get_compat_map_reply_t *R
 ** @returns xcb_xkb_mod_def_t *
 **
 *****************************************************************************/
 
xcb_xkb_mod_def_t *
xcb_xkb_get_compat_map_group_rtrn (const xcb_xkb_get_compat_map_reply_t *R  /**< */)
{
    xcb_generic_iterator_t prev = xcb_xkb_sym_interpret_end(xcb_xkb_get_compat_map_si_rtrn_iterator(R));
    return (xcb_xkb_mod_def_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_mod_def_t, prev.index) + 0);
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_compat_map_group_rtrn_length
 ** 
 ** @param const xcb_xkb_get_compat_map_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_compat_map_group_rtrn_length (const xcb_xkb_get_compat_map_reply_t *R  /**< */)
{
    return xcb_popcount(R->groupsRtrn);
}


/*****************************************************************************
 **
 ** xcb_xkb_mod_def_iterator_t xcb_xkb_get_compat_map_group_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_compat_map_reply_t *R
 ** @returns xcb_xkb_mod_def_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_mod_def_iterator_t
xcb_xkb_get_compat_map_group_rtrn_iterator (const xcb_xkb_get_compat_map_reply_t *R  /**< */)
{
    xcb_xkb_mod_def_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_sym_interpret_end(xcb_xkb_get_compat_map_si_rtrn_iterator(R));
    i.data = (xcb_xkb_mod_def_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_mod_def_t, prev.index));
    i.rem = xcb_popcount(R->groupsRtrn);
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_compat_map_reply_t * xcb_xkb_get_compat_map_reply
 ** 
 ** @param xcb_connection_t                 *c
 ** @param xcb_xkb_get_compat_map_cookie_t   cookie
 ** @param xcb_generic_error_t             **e
 ** @returns xcb_xkb_get_compat_map_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_compat_map_reply_t *
xcb_xkb_get_compat_map_reply (xcb_connection_t                 *c  /**< */,
                              xcb_xkb_get_compat_map_cookie_t   cookie  /**< */,
                              xcb_generic_error_t             **e  /**< */)
{
    return (xcb_xkb_get_compat_map_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xkb_set_compat_map_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_set_compat_map_request_t *_aux = (xcb_xkb_set_compat_map_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_set_compat_map_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* si */
    xcb_block_len += _aux->nSI * sizeof(xcb_xkb_sym_interpret_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_sym_interpret_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* groupMaps */
    xcb_block_len += xcb_popcount(_aux->groups) * sizeof(xcb_xkb_mod_def_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_mod_def_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_compat_map_checked
 ** 
 ** @param xcb_connection_t              *c
 ** @param xcb_xkb_device_spec_t          deviceSpec
 ** @param uint8_t                        recomputeActions
 ** @param uint8_t                        truncateSI
 ** @param uint8_t                        groups
 ** @param uint16_t                       firstSI
 ** @param uint16_t                       nSI
 ** @param const xcb_xkb_sym_interpret_t *si
 ** @param const xcb_xkb_mod_def_t       *groupMaps
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_compat_map_checked (xcb_connection_t              *c  /**< */,
                                xcb_xkb_device_spec_t          deviceSpec  /**< */,
                                uint8_t                        recomputeActions  /**< */,
                                uint8_t                        truncateSI  /**< */,
                                uint8_t                        groups  /**< */,
                                uint16_t                       firstSI  /**< */,
                                uint16_t                       nSI  /**< */,
                                const xcb_xkb_sym_interpret_t *si  /**< */,
                                const xcb_xkb_mod_def_t       *groupMaps  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 6,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_COMPAT_MAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[8];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_compat_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.pad0 = 0;
    xcb_out.recomputeActions = recomputeActions;
    xcb_out.truncateSI = truncateSI;
    xcb_out.groups = groups;
    xcb_out.firstSI = firstSI;
    xcb_out.nSI = nSI;
    memset(xcb_out.pad1, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_sym_interpret_t si */
    xcb_parts[4].iov_base = (char *) si;
    xcb_parts[4].iov_len = nSI * sizeof(xcb_xkb_sym_interpret_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    /* xcb_xkb_mod_def_t groupMaps */
    xcb_parts[6].iov_base = (char *) groupMaps;
    xcb_parts[6].iov_len = xcb_popcount(groups) * sizeof(xcb_xkb_mod_def_t);
    xcb_parts[7].iov_base = 0;
    xcb_parts[7].iov_len = -xcb_parts[6].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_compat_map
 ** 
 ** @param xcb_connection_t              *c
 ** @param xcb_xkb_device_spec_t          deviceSpec
 ** @param uint8_t                        recomputeActions
 ** @param uint8_t                        truncateSI
 ** @param uint8_t                        groups
 ** @param uint16_t                       firstSI
 ** @param uint16_t                       nSI
 ** @param const xcb_xkb_sym_interpret_t *si
 ** @param const xcb_xkb_mod_def_t       *groupMaps
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_compat_map (xcb_connection_t              *c  /**< */,
                        xcb_xkb_device_spec_t          deviceSpec  /**< */,
                        uint8_t                        recomputeActions  /**< */,
                        uint8_t                        truncateSI  /**< */,
                        uint8_t                        groups  /**< */,
                        uint16_t                       firstSI  /**< */,
                        uint16_t                       nSI  /**< */,
                        const xcb_xkb_sym_interpret_t *si  /**< */,
                        const xcb_xkb_mod_def_t       *groupMaps  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 6,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_COMPAT_MAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[8];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_compat_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.pad0 = 0;
    xcb_out.recomputeActions = recomputeActions;
    xcb_out.truncateSI = truncateSI;
    xcb_out.groups = groups;
    xcb_out.firstSI = firstSI;
    xcb_out.nSI = nSI;
    memset(xcb_out.pad1, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_sym_interpret_t si */
    xcb_parts[4].iov_base = (char *) si;
    xcb_parts[4].iov_len = nSI * sizeof(xcb_xkb_sym_interpret_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    /* xcb_xkb_mod_def_t groupMaps */
    xcb_parts[6].iov_base = (char *) groupMaps;
    xcb_parts[6].iov_len = xcb_popcount(groups) * sizeof(xcb_xkb_mod_def_t);
    xcb_parts[7].iov_base = 0;
    xcb_parts[7].iov_len = -xcb_parts[6].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_indicator_state_cookie_t xcb_xkb_get_indicator_state
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @returns xcb_xkb_get_indicator_state_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_indicator_state_cookie_t
xcb_xkb_get_indicator_state (xcb_connection_t      *c  /**< */,
                             xcb_xkb_device_spec_t  deviceSpec  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_INDICATOR_STATE,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_indicator_state_cookie_t xcb_ret;
    xcb_xkb_get_indicator_state_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_indicator_state_cookie_t xcb_xkb_get_indicator_state_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @returns xcb_xkb_get_indicator_state_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_indicator_state_cookie_t
xcb_xkb_get_indicator_state_unchecked (xcb_connection_t      *c  /**< */,
                                       xcb_xkb_device_spec_t  deviceSpec  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_INDICATOR_STATE,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_indicator_state_cookie_t xcb_ret;
    xcb_xkb_get_indicator_state_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_indicator_state_reply_t * xcb_xkb_get_indicator_state_reply
 ** 
 ** @param xcb_connection_t                      *c
 ** @param xcb_xkb_get_indicator_state_cookie_t   cookie
 ** @param xcb_generic_error_t                  **e
 ** @returns xcb_xkb_get_indicator_state_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_indicator_state_reply_t *
xcb_xkb_get_indicator_state_reply (xcb_connection_t                      *c  /**< */,
                                   xcb_xkb_get_indicator_state_cookie_t   cookie  /**< */,
                                   xcb_generic_error_t                  **e  /**< */)
{
    return (xcb_xkb_get_indicator_state_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xkb_get_indicator_map_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_get_indicator_map_reply_t *_aux = (xcb_xkb_get_indicator_map_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_get_indicator_map_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* maps */
    xcb_block_len += xcb_popcount(_aux->which) * sizeof(xcb_xkb_indicator_map_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_indicator_map_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_indicator_map_cookie_t xcb_xkb_get_indicator_map
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint32_t               which
 ** @returns xcb_xkb_get_indicator_map_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_indicator_map_cookie_t
xcb_xkb_get_indicator_map (xcb_connection_t      *c  /**< */,
                           xcb_xkb_device_spec_t  deviceSpec  /**< */,
                           uint32_t               which  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_INDICATOR_MAP,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_indicator_map_cookie_t xcb_ret;
    xcb_xkb_get_indicator_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.which = which;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_indicator_map_cookie_t xcb_xkb_get_indicator_map_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint32_t               which
 ** @returns xcb_xkb_get_indicator_map_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_indicator_map_cookie_t
xcb_xkb_get_indicator_map_unchecked (xcb_connection_t      *c  /**< */,
                                     xcb_xkb_device_spec_t  deviceSpec  /**< */,
                                     uint32_t               which  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_INDICATOR_MAP,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_indicator_map_cookie_t xcb_ret;
    xcb_xkb_get_indicator_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.which = which;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_indicator_map_t * xcb_xkb_get_indicator_map_maps
 ** 
 ** @param const xcb_xkb_get_indicator_map_reply_t *R
 ** @returns xcb_xkb_indicator_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_indicator_map_t *
xcb_xkb_get_indicator_map_maps (const xcb_xkb_get_indicator_map_reply_t *R  /**< */)
{
    return (xcb_xkb_indicator_map_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_indicator_map_maps_length
 ** 
 ** @param const xcb_xkb_get_indicator_map_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_indicator_map_maps_length (const xcb_xkb_get_indicator_map_reply_t *R  /**< */)
{
    return xcb_popcount(R->which);
}


/*****************************************************************************
 **
 ** xcb_xkb_indicator_map_iterator_t xcb_xkb_get_indicator_map_maps_iterator
 ** 
 ** @param const xcb_xkb_get_indicator_map_reply_t *R
 ** @returns xcb_xkb_indicator_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_indicator_map_iterator_t
xcb_xkb_get_indicator_map_maps_iterator (const xcb_xkb_get_indicator_map_reply_t *R  /**< */)
{
    xcb_xkb_indicator_map_iterator_t i;
    i.data = (xcb_xkb_indicator_map_t *) (R + 1);
    i.rem = xcb_popcount(R->which);
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_indicator_map_reply_t * xcb_xkb_get_indicator_map_reply
 ** 
 ** @param xcb_connection_t                    *c
 ** @param xcb_xkb_get_indicator_map_cookie_t   cookie
 ** @param xcb_generic_error_t                **e
 ** @returns xcb_xkb_get_indicator_map_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_indicator_map_reply_t *
xcb_xkb_get_indicator_map_reply (xcb_connection_t                    *c  /**< */,
                                 xcb_xkb_get_indicator_map_cookie_t   cookie  /**< */,
                                 xcb_generic_error_t                **e  /**< */)
{
    return (xcb_xkb_get_indicator_map_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xkb_set_indicator_map_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_set_indicator_map_request_t *_aux = (xcb_xkb_set_indicator_map_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_set_indicator_map_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* maps */
    xcb_block_len += xcb_popcount(_aux->which) * sizeof(xcb_xkb_indicator_map_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_indicator_map_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_indicator_map_checked
 ** 
 ** @param xcb_connection_t              *c
 ** @param xcb_xkb_device_spec_t          deviceSpec
 ** @param uint32_t                       which
 ** @param const xcb_xkb_indicator_map_t *maps
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_indicator_map_checked (xcb_connection_t              *c  /**< */,
                                   xcb_xkb_device_spec_t          deviceSpec  /**< */,
                                   uint32_t                       which  /**< */,
                                   const xcb_xkb_indicator_map_t *maps  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_INDICATOR_MAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_indicator_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.which = which;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_indicator_map_t maps */
    xcb_parts[4].iov_base = (char *) maps;
    xcb_parts[4].iov_len = xcb_popcount(which) * sizeof(xcb_xkb_indicator_map_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_indicator_map
 ** 
 ** @param xcb_connection_t              *c
 ** @param xcb_xkb_device_spec_t          deviceSpec
 ** @param uint32_t                       which
 ** @param const xcb_xkb_indicator_map_t *maps
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_indicator_map (xcb_connection_t              *c  /**< */,
                           xcb_xkb_device_spec_t          deviceSpec  /**< */,
                           uint32_t                       which  /**< */,
                           const xcb_xkb_indicator_map_t *maps  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_INDICATOR_MAP,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[6];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_indicator_map_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.which = which;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_indicator_map_t maps */
    xcb_parts[4].iov_base = (char *) maps;
    xcb_parts[4].iov_len = xcb_popcount(which) * sizeof(xcb_xkb_indicator_map_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_named_indicator_cookie_t xcb_xkb_get_named_indicator
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_xkb_device_spec_t     deviceSpec
 ** @param xcb_xkb_led_class_spec_t  ledClass
 ** @param xcb_xkb_id_spec_t         ledID
 ** @param xcb_atom_t                indicator
 ** @returns xcb_xkb_get_named_indicator_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_named_indicator_cookie_t
xcb_xkb_get_named_indicator (xcb_connection_t         *c  /**< */,
                             xcb_xkb_device_spec_t     deviceSpec  /**< */,
                             xcb_xkb_led_class_spec_t  ledClass  /**< */,
                             xcb_xkb_id_spec_t         ledID  /**< */,
                             xcb_atom_t                indicator  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_NAMED_INDICATOR,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_named_indicator_cookie_t xcb_ret;
    xcb_xkb_get_named_indicator_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.ledClass = ledClass;
    xcb_out.ledID = ledID;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.indicator = indicator;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_named_indicator_cookie_t xcb_xkb_get_named_indicator_unchecked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_xkb_device_spec_t     deviceSpec
 ** @param xcb_xkb_led_class_spec_t  ledClass
 ** @param xcb_xkb_id_spec_t         ledID
 ** @param xcb_atom_t                indicator
 ** @returns xcb_xkb_get_named_indicator_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_named_indicator_cookie_t
xcb_xkb_get_named_indicator_unchecked (xcb_connection_t         *c  /**< */,
                                       xcb_xkb_device_spec_t     deviceSpec  /**< */,
                                       xcb_xkb_led_class_spec_t  ledClass  /**< */,
                                       xcb_xkb_id_spec_t         ledID  /**< */,
                                       xcb_atom_t                indicator  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_NAMED_INDICATOR,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_named_indicator_cookie_t xcb_ret;
    xcb_xkb_get_named_indicator_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.ledClass = ledClass;
    xcb_out.ledID = ledID;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.indicator = indicator;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_named_indicator_reply_t * xcb_xkb_get_named_indicator_reply
 ** 
 ** @param xcb_connection_t                      *c
 ** @param xcb_xkb_get_named_indicator_cookie_t   cookie
 ** @param xcb_generic_error_t                  **e
 ** @returns xcb_xkb_get_named_indicator_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_named_indicator_reply_t *
xcb_xkb_get_named_indicator_reply (xcb_connection_t                      *c  /**< */,
                                   xcb_xkb_get_named_indicator_cookie_t   cookie  /**< */,
                                   xcb_generic_error_t                  **e  /**< */)
{
    return (xcb_xkb_get_named_indicator_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_named_indicator_checked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_xkb_device_spec_t     deviceSpec
 ** @param xcb_xkb_led_class_spec_t  ledClass
 ** @param xcb_xkb_id_spec_t         ledID
 ** @param xcb_atom_t                indicator
 ** @param uint8_t                   setState
 ** @param uint8_t                   on
 ** @param uint8_t                   setMap
 ** @param uint8_t                   createMap
 ** @param uint8_t                   map_flags
 ** @param uint8_t                   map_whichGroups
 ** @param uint8_t                   map_groups
 ** @param uint8_t                   map_whichMods
 ** @param uint8_t                   map_realMods
 ** @param uint16_t                  map_vmods
 ** @param uint32_t                  map_ctrls
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_named_indicator_checked (xcb_connection_t         *c  /**< */,
                                     xcb_xkb_device_spec_t     deviceSpec  /**< */,
                                     xcb_xkb_led_class_spec_t  ledClass  /**< */,
                                     xcb_xkb_id_spec_t         ledID  /**< */,
                                     xcb_atom_t                indicator  /**< */,
                                     uint8_t                   setState  /**< */,
                                     uint8_t                   on  /**< */,
                                     uint8_t                   setMap  /**< */,
                                     uint8_t                   createMap  /**< */,
                                     uint8_t                   map_flags  /**< */,
                                     uint8_t                   map_whichGroups  /**< */,
                                     uint8_t                   map_groups  /**< */,
                                     uint8_t                   map_whichMods  /**< */,
                                     uint8_t                   map_realMods  /**< */,
                                     uint16_t                  map_vmods  /**< */,
                                     uint32_t                  map_ctrls  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_NAMED_INDICATOR,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_named_indicator_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.ledClass = ledClass;
    xcb_out.ledID = ledID;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.indicator = indicator;
    xcb_out.setState = setState;
    xcb_out.on = on;
    xcb_out.setMap = setMap;
    xcb_out.createMap = createMap;
    xcb_out.pad1 = 0;
    xcb_out.map_flags = map_flags;
    xcb_out.map_whichGroups = map_whichGroups;
    xcb_out.map_groups = map_groups;
    xcb_out.map_whichMods = map_whichMods;
    xcb_out.map_realMods = map_realMods;
    xcb_out.map_vmods = map_vmods;
    xcb_out.map_ctrls = map_ctrls;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_named_indicator
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_xkb_device_spec_t     deviceSpec
 ** @param xcb_xkb_led_class_spec_t  ledClass
 ** @param xcb_xkb_id_spec_t         ledID
 ** @param xcb_atom_t                indicator
 ** @param uint8_t                   setState
 ** @param uint8_t                   on
 ** @param uint8_t                   setMap
 ** @param uint8_t                   createMap
 ** @param uint8_t                   map_flags
 ** @param uint8_t                   map_whichGroups
 ** @param uint8_t                   map_groups
 ** @param uint8_t                   map_whichMods
 ** @param uint8_t                   map_realMods
 ** @param uint16_t                  map_vmods
 ** @param uint32_t                  map_ctrls
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_named_indicator (xcb_connection_t         *c  /**< */,
                             xcb_xkb_device_spec_t     deviceSpec  /**< */,
                             xcb_xkb_led_class_spec_t  ledClass  /**< */,
                             xcb_xkb_id_spec_t         ledID  /**< */,
                             xcb_atom_t                indicator  /**< */,
                             uint8_t                   setState  /**< */,
                             uint8_t                   on  /**< */,
                             uint8_t                   setMap  /**< */,
                             uint8_t                   createMap  /**< */,
                             uint8_t                   map_flags  /**< */,
                             uint8_t                   map_whichGroups  /**< */,
                             uint8_t                   map_groups  /**< */,
                             uint8_t                   map_whichMods  /**< */,
                             uint8_t                   map_realMods  /**< */,
                             uint16_t                  map_vmods  /**< */,
                             uint32_t                  map_ctrls  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_NAMED_INDICATOR,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[4];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_named_indicator_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.ledClass = ledClass;
    xcb_out.ledID = ledID;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.indicator = indicator;
    xcb_out.setState = setState;
    xcb_out.on = on;
    xcb_out.setMap = setMap;
    xcb_out.createMap = createMap;
    xcb_out.pad1 = 0;
    xcb_out.map_flags = map_flags;
    xcb_out.map_whichGroups = map_whichGroups;
    xcb_out.map_groups = map_groups;
    xcb_out.map_whichMods = map_whichMods;
    xcb_out.map_realMods = map_realMods;
    xcb_out.map_vmods = map_vmods;
    xcb_out.map_ctrls = map_ctrls;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_names_value_list_type_names
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_names_value_list_type_names (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->typeNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_type_names_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_type_names_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return R->nTypes;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_names_value_list_type_names_end
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_names_value_list_type_names_end (const xcb_xkb_get_names_reply_t *R  /**< */,
                                             const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* valueList */ S->typeNames + R->nTypes;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_names_value_list_n_levels_per_type
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_names_value_list_n_levels_per_type (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->nLevelsPerType;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_n_levels_per_type_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_n_levels_per_type_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                       const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return R->nTypes;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_names_value_list_n_levels_per_type_end
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_names_value_list_n_levels_per_type_end (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                    const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* valueList */ S->nLevelsPerType + R->nTypes;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_names_value_list_alignment_pad
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_names_value_list_alignment_pad (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->alignment_pad;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_alignment_pad_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_alignment_pad_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                   const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return (((R->nTypes + 3) & (~3)) - R->nTypes);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_names_value_list_alignment_pad_end
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_names_value_list_alignment_pad_end (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* valueList */ S->alignment_pad + (((R->nTypes + 3) & (~3)) - R->nTypes);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_names_value_list_kt_level_names
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_names_value_list_kt_level_names (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->ktLevelNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_kt_level_names_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_kt_level_names_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                    const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return qt_xcb_sumof(/* valueList */ S->nLevelsPerType, R->nTypes);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_names_value_list_kt_level_names_end
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_names_value_list_kt_level_names_end (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                 const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* valueList */ S->ktLevelNames + qt_xcb_sumof(/* valueList */ S->nLevelsPerType, R->nTypes);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_names_value_list_indicator_names
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_names_value_list_indicator_names (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->indicatorNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_indicator_names_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_indicator_names_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                     const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return xcb_popcount(R->indicators);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_names_value_list_indicator_names_end
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_names_value_list_indicator_names_end (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                  const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* valueList */ S->indicatorNames + xcb_popcount(R->indicators);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_names_value_list_virtual_mod_names
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_names_value_list_virtual_mod_names (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->virtualModNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_virtual_mod_names_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_virtual_mod_names_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                       const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return xcb_popcount(R->virtualMods);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_names_value_list_virtual_mod_names_end
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_names_value_list_virtual_mod_names_end (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                    const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* valueList */ S->virtualModNames + xcb_popcount(R->virtualMods);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_names_value_list_groups
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_names_value_list_groups (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->groups;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_groups_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_groups_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                            const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return xcb_popcount(R->groupNames);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_names_value_list_groups_end
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_names_value_list_groups_end (const xcb_xkb_get_names_reply_t *R  /**< */,
                                         const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* valueList */ S->groups + xcb_popcount(R->groupNames);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_name_t * xcb_xkb_get_names_value_list_key_names
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns xcb_xkb_key_name_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_name_t *
xcb_xkb_get_names_value_list_key_names (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->keyNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_key_names_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_key_names_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                               const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return R->nKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_name_iterator_t xcb_xkb_get_names_value_list_key_names_iterator
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_xkb_key_name_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_name_iterator_t
xcb_xkb_get_names_value_list_key_names_iterator (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                 const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_xkb_key_name_iterator_t i;
    i.data = /* valueList */ S->keyNames;
    i.rem = R->nKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_alias_t * xcb_xkb_get_names_value_list_key_aliases
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns xcb_xkb_key_alias_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_alias_t *
xcb_xkb_get_names_value_list_key_aliases (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->keyAliases;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_key_aliases_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_key_aliases_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                 const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return R->nKeyAliases;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_alias_iterator_t xcb_xkb_get_names_value_list_key_aliases_iterator
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_xkb_key_alias_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_alias_iterator_t
xcb_xkb_get_names_value_list_key_aliases_iterator (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                   const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_xkb_key_alias_iterator_t i;
    i.data = /* valueList */ S->keyAliases;
    i.rem = R->nKeyAliases;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_names_value_list_radio_group_names
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_names_value_list_radio_group_names (const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return /* valueList */ S->radioGroupNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_names_value_list_radio_group_names_length
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_names_value_list_radio_group_names_length (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                       const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    return R->nRadioGroups;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_names_value_list_radio_group_names_end
 ** 
 ** @param const xcb_xkb_get_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_names_value_list_radio_group_names_end (const xcb_xkb_get_names_reply_t *R  /**< */,
                                                    const xcb_xkb_get_names_value_list_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* valueList */ S->radioGroupNames + R->nRadioGroups;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}

int
xcb_xkb_get_names_value_list_serialize (void                                 **_buffer  /**< */,
                                        uint8_t                                nTypes  /**< */,
                                        uint32_t                               indicators  /**< */,
                                        uint16_t                               virtualMods  /**< */,
                                        uint8_t                                groupNames  /**< */,
                                        uint8_t                                nKeys  /**< */,
                                        uint8_t                                nKeyAliases  /**< */,
                                        uint8_t                                nRadioGroups  /**< */,
                                        uint32_t                               which  /**< */,
                                        const xcb_xkb_get_names_value_list_t  *_aux  /**< */)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[27];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if(which & XCB_XKB_NAME_DETAIL_KEYCODES) {
        /* xcb_xkb_get_names_value_list_t.keycodesName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->keycodesName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GEOMETRY) {
        /* xcb_xkb_get_names_value_list_t.geometryName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometryName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_SYMBOLS) {
        /* xcb_xkb_get_names_value_list_t.symbolsName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->symbolsName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_PHYS_SYMBOLS) {
        /* xcb_xkb_get_names_value_list_t.physSymbolsName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->physSymbolsName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_TYPES) {
        /* xcb_xkb_get_names_value_list_t.typesName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->typesName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_COMPAT) {
        /* xcb_xkb_get_names_value_list_t.compatName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compatName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_TYPE_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* typeNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->typeNames;
        xcb_block_len += nTypes * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = nTypes * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KT_LEVEL_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* nLevelsPerType */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->nLevelsPerType;
        xcb_block_len += nTypes * sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = nTypes * sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->alignment_pad;
        xcb_block_len += (((nTypes + 3) & (~3)) - nTypes) * sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = (((nTypes + 3) & (~3)) - nTypes) * sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* ktLevelNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->ktLevelNames;
        xcb_block_len += qt_xcb_sumof(_aux->nLevelsPerType, nTypes) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = qt_xcb_sumof(_aux->nLevelsPerType, nTypes) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_INDICATOR_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* indicatorNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->indicatorNames;
        xcb_block_len += xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* virtualModNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->virtualModNames;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GROUP_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* groups */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->groups;
        xcb_block_len += xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->keyNames;
        xcb_block_len += nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_name_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_ALIASES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyAliases */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->keyAliases;
        xcb_block_len += nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_alias_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_RG_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* radioGroupNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->radioGroupNames;
        xcb_block_len += nRadioGroups * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = nRadioGroups * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_xkb_get_names_value_list_unpack (const void                      *_buffer  /**< */,
                                     uint8_t                          nTypes  /**< */,
                                     uint32_t                         indicators  /**< */,
                                     uint16_t                         virtualMods  /**< */,
                                     uint8_t                          groupNames  /**< */,
                                     uint8_t                          nKeys  /**< */,
                                     uint8_t                          nKeyAliases  /**< */,
                                     uint8_t                          nRadioGroups  /**< */,
                                     uint32_t                         which  /**< */,
                                     xcb_xkb_get_names_value_list_t  *_aux  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    if(which & XCB_XKB_NAME_DETAIL_KEYCODES) {
        /* xcb_xkb_get_names_value_list_t.keycodesName */
        _aux->keycodesName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GEOMETRY) {
        /* xcb_xkb_get_names_value_list_t.geometryName */
        _aux->geometryName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_SYMBOLS) {
        /* xcb_xkb_get_names_value_list_t.symbolsName */
        _aux->symbolsName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_PHYS_SYMBOLS) {
        /* xcb_xkb_get_names_value_list_t.physSymbolsName */
        _aux->physSymbolsName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_TYPES) {
        /* xcb_xkb_get_names_value_list_t.typesName */
        _aux->typesName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_COMPAT) {
        /* xcb_xkb_get_names_value_list_t.compatName */
        _aux->compatName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_TYPE_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* typeNames */
        _aux->typeNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += nTypes * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KT_LEVEL_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* nLevelsPerType */
        _aux->nLevelsPerType = (uint8_t *)xcb_tmp;
        xcb_block_len += nTypes * sizeof(uint8_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* alignment_pad */
        _aux->alignment_pad = (uint8_t *)xcb_tmp;
        xcb_block_len += (((nTypes + 3) & (~3)) - nTypes) * sizeof(uint8_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* ktLevelNames */
        _aux->ktLevelNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += qt_xcb_sumof(_aux->nLevelsPerType, nTypes) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_INDICATOR_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* indicatorNames */
        _aux->indicatorNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* virtualModNames */
        _aux->virtualModNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GROUP_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* groups */
        _aux->groups = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyNames */
        _aux->keyNames = (xcb_xkb_key_name_t *)xcb_tmp;
        xcb_block_len += nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_name_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_ALIASES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyAliases */
        _aux->keyAliases = (xcb_xkb_key_alias_t *)xcb_tmp;
        xcb_block_len += nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_alias_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_RG_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* radioGroupNames */
        _aux->radioGroupNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += nRadioGroups * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

int
xcb_xkb_get_names_value_list_sizeof (const void  *_buffer  /**< */,
                                     uint8_t      nTypes  /**< */,
                                     uint32_t     indicators  /**< */,
                                     uint16_t     virtualMods  /**< */,
                                     uint8_t      groupNames  /**< */,
                                     uint8_t      nKeys  /**< */,
                                     uint8_t      nKeyAliases  /**< */,
                                     uint8_t      nRadioGroups  /**< */,
                                     uint32_t     which  /**< */)
{
    xcb_xkb_get_names_value_list_t _aux;
    return xcb_xkb_get_names_value_list_unpack(_buffer, nTypes, indicators, virtualMods, groupNames, nKeys, nKeyAliases, nRadioGroups, which, &_aux);
}


/*****************************************************************************
 **
 ** xcb_xkb_get_names_cookie_t xcb_xkb_get_names
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint32_t               which
 ** @returns xcb_xkb_get_names_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_names_cookie_t
xcb_xkb_get_names (xcb_connection_t      *c  /**< */,
                   xcb_xkb_device_spec_t  deviceSpec  /**< */,
                   uint32_t               which  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_NAMES,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_names_cookie_t xcb_ret;
    xcb_xkb_get_names_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.which = which;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_names_cookie_t xcb_xkb_get_names_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint32_t               which
 ** @returns xcb_xkb_get_names_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_names_cookie_t
xcb_xkb_get_names_unchecked (xcb_connection_t      *c  /**< */,
                             xcb_xkb_device_spec_t  deviceSpec  /**< */,
                             uint32_t               which  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_NAMES,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_names_cookie_t xcb_ret;
    xcb_xkb_get_names_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.which = which;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_names_value_list_t * xcb_xkb_get_names_value_list
 ** 
 ** @param const xcb_xkb_get_names_reply_t *R
 ** @returns xcb_xkb_get_names_value_list_t *
 **
 *****************************************************************************/
 
void *
xcb_xkb_get_names_value_list (const xcb_xkb_get_names_reply_t *R  /**< */)
{
    return (void *) (R + 1);
}


/*****************************************************************************
 **
 ** xcb_xkb_get_names_reply_t * xcb_xkb_get_names_reply
 ** 
 ** @param xcb_connection_t            *c
 ** @param xcb_xkb_get_names_cookie_t   cookie
 ** @param xcb_generic_error_t        **e
 ** @returns xcb_xkb_get_names_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_names_reply_t *
xcb_xkb_get_names_reply (xcb_connection_t            *c  /**< */,
                         xcb_xkb_get_names_cookie_t   cookie  /**< */,
                         xcb_generic_error_t        **e  /**< */)
{
    return (xcb_xkb_get_names_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_set_names_values_type_names
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_set_names_values_type_names (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->typeNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_type_names_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_type_names_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                            const xcb_xkb_set_names_values_t *S  /**< */)
{
    return R->nTypes;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_names_values_type_names_end
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_names_values_type_names_end (const xcb_xkb_set_names_request_t *R  /**< */,
                                         const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->typeNames + R->nTypes;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_set_names_values_n_levels_per_type
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_set_names_values_n_levels_per_type (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->nLevelsPerType;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_n_levels_per_type_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_n_levels_per_type_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                                   const xcb_xkb_set_names_values_t *S  /**< */)
{
    return R->nKTLevels;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_names_values_n_levels_per_type_end
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_names_values_n_levels_per_type_end (const xcb_xkb_set_names_request_t *R  /**< */,
                                                const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->nLevelsPerType + R->nKTLevels;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_set_names_values_kt_level_names
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_set_names_values_kt_level_names (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->ktLevelNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_kt_level_names_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_kt_level_names_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                                const xcb_xkb_set_names_values_t *S  /**< */)
{
    return qt_xcb_sumof(/* values */ S->nLevelsPerType, R->nKTLevels);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_names_values_kt_level_names_end
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_names_values_kt_level_names_end (const xcb_xkb_set_names_request_t *R  /**< */,
                                             const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->ktLevelNames + qt_xcb_sumof(/* values */ S->nLevelsPerType, R->nKTLevels);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_set_names_values_indicator_names
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_set_names_values_indicator_names (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->indicatorNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_indicator_names_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_indicator_names_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                                 const xcb_xkb_set_names_values_t *S  /**< */)
{
    return xcb_popcount(R->indicators);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_names_values_indicator_names_end
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_names_values_indicator_names_end (const xcb_xkb_set_names_request_t *R  /**< */,
                                              const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->indicatorNames + xcb_popcount(R->indicators);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_set_names_values_virtual_mod_names
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_set_names_values_virtual_mod_names (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->virtualModNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_virtual_mod_names_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_virtual_mod_names_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                                   const xcb_xkb_set_names_values_t *S  /**< */)
{
    return xcb_popcount(R->virtualMods);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_names_values_virtual_mod_names_end
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_names_values_virtual_mod_names_end (const xcb_xkb_set_names_request_t *R  /**< */,
                                                const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->virtualModNames + xcb_popcount(R->virtualMods);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_set_names_values_groups
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_set_names_values_groups (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->groups;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_groups_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_groups_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                        const xcb_xkb_set_names_values_t *S  /**< */)
{
    return xcb_popcount(R->groupNames);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_names_values_groups_end
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_names_values_groups_end (const xcb_xkb_set_names_request_t *R  /**< */,
                                     const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->groups + xcb_popcount(R->groupNames);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_name_t * xcb_xkb_set_names_values_key_names
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns xcb_xkb_key_name_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_name_t *
xcb_xkb_set_names_values_key_names (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->keyNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_key_names_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_key_names_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                           const xcb_xkb_set_names_values_t *S  /**< */)
{
    return R->nKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_name_iterator_t xcb_xkb_set_names_values_key_names_iterator
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_xkb_key_name_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_name_iterator_t
xcb_xkb_set_names_values_key_names_iterator (const xcb_xkb_set_names_request_t *R  /**< */,
                                             const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_xkb_key_name_iterator_t i;
    i.data = /* values */ S->keyNames;
    i.rem = R->nKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_alias_t * xcb_xkb_set_names_values_key_aliases
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns xcb_xkb_key_alias_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_alias_t *
xcb_xkb_set_names_values_key_aliases (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->keyAliases;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_key_aliases_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_key_aliases_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                             const xcb_xkb_set_names_values_t *S  /**< */)
{
    return R->nKeyAliases;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_alias_iterator_t xcb_xkb_set_names_values_key_aliases_iterator
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_xkb_key_alias_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_alias_iterator_t
xcb_xkb_set_names_values_key_aliases_iterator (const xcb_xkb_set_names_request_t *R  /**< */,
                                               const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_xkb_key_alias_iterator_t i;
    i.data = /* values */ S->keyAliases;
    i.rem = R->nKeyAliases;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_set_names_values_radio_group_names
 ** 
 ** @param const xcb_xkb_set_names_values_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_set_names_values_radio_group_names (const xcb_xkb_set_names_values_t *S  /**< */)
{
    return /* values */ S->radioGroupNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_set_names_values_radio_group_names_length
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_set_names_values_radio_group_names_length (const xcb_xkb_set_names_request_t *R  /**< */,
                                                   const xcb_xkb_set_names_values_t *S  /**< */)
{
    return R->nRadioGroups;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_set_names_values_radio_group_names_end
 ** 
 ** @param const xcb_xkb_set_names_values_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_set_names_values_radio_group_names_end (const xcb_xkb_set_names_request_t *R  /**< */,
                                                const xcb_xkb_set_names_values_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* values */ S->radioGroupNames + R->nRadioGroups;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}

int
xcb_xkb_set_names_values_serialize (void                             **_buffer  /**< */,
                                    uint8_t                            nTypes  /**< */,
                                    uint8_t                            nKTLevels  /**< */,
                                    uint32_t                           indicators  /**< */,
                                    uint16_t                           virtualMods  /**< */,
                                    uint8_t                            groupNames  /**< */,
                                    uint8_t                            nKeys  /**< */,
                                    uint8_t                            nKeyAliases  /**< */,
                                    uint8_t                            nRadioGroups  /**< */,
                                    uint32_t                           which  /**< */,
                                    const xcb_xkb_set_names_values_t  *_aux  /**< */)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[25];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if(which & XCB_XKB_NAME_DETAIL_KEYCODES) {
        /* xcb_xkb_set_names_values_t.keycodesName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->keycodesName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GEOMETRY) {
        /* xcb_xkb_set_names_values_t.geometryName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometryName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_SYMBOLS) {
        /* xcb_xkb_set_names_values_t.symbolsName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->symbolsName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_PHYS_SYMBOLS) {
        /* xcb_xkb_set_names_values_t.physSymbolsName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->physSymbolsName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_TYPES) {
        /* xcb_xkb_set_names_values_t.typesName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->typesName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_COMPAT) {
        /* xcb_xkb_set_names_values_t.compatName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compatName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_TYPE_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* typeNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->typeNames;
        xcb_block_len += nTypes * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = nTypes * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KT_LEVEL_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* nLevelsPerType */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->nLevelsPerType;
        xcb_block_len += nKTLevels * sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = nKTLevels * sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* ktLevelNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->ktLevelNames;
        xcb_block_len += qt_xcb_sumof(_aux->nLevelsPerType, nKTLevels) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = qt_xcb_sumof(_aux->nLevelsPerType, nKTLevels) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_INDICATOR_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* indicatorNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->indicatorNames;
        xcb_block_len += xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* virtualModNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->virtualModNames;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GROUP_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* groups */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->groups;
        xcb_block_len += xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->keyNames;
        xcb_block_len += nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_name_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_ALIASES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyAliases */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->keyAliases;
        xcb_block_len += nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_alias_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_RG_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* radioGroupNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->radioGroupNames;
        xcb_block_len += nRadioGroups * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = nRadioGroups * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_xkb_set_names_values_unpack (const void                  *_buffer  /**< */,
                                 uint8_t                      nTypes  /**< */,
                                 uint8_t                      nKTLevels  /**< */,
                                 uint32_t                     indicators  /**< */,
                                 uint16_t                     virtualMods  /**< */,
                                 uint8_t                      groupNames  /**< */,
                                 uint8_t                      nKeys  /**< */,
                                 uint8_t                      nKeyAliases  /**< */,
                                 uint8_t                      nRadioGroups  /**< */,
                                 uint32_t                     which  /**< */,
                                 xcb_xkb_set_names_values_t  *_aux  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    if(which & XCB_XKB_NAME_DETAIL_KEYCODES) {
        /* xcb_xkb_set_names_values_t.keycodesName */
        _aux->keycodesName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GEOMETRY) {
        /* xcb_xkb_set_names_values_t.geometryName */
        _aux->geometryName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_SYMBOLS) {
        /* xcb_xkb_set_names_values_t.symbolsName */
        _aux->symbolsName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_PHYS_SYMBOLS) {
        /* xcb_xkb_set_names_values_t.physSymbolsName */
        _aux->physSymbolsName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_TYPES) {
        /* xcb_xkb_set_names_values_t.typesName */
        _aux->typesName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_COMPAT) {
        /* xcb_xkb_set_names_values_t.compatName */
        _aux->compatName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_TYPE_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* typeNames */
        _aux->typeNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += nTypes * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KT_LEVEL_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* nLevelsPerType */
        _aux->nLevelsPerType = (uint8_t *)xcb_tmp;
        xcb_block_len += nKTLevels * sizeof(uint8_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* ktLevelNames */
        _aux->ktLevelNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += qt_xcb_sumof(_aux->nLevelsPerType, nKTLevels) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_INDICATOR_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* indicatorNames */
        _aux->indicatorNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* virtualModNames */
        _aux->virtualModNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GROUP_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* groups */
        _aux->groups = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyNames */
        _aux->keyNames = (xcb_xkb_key_name_t *)xcb_tmp;
        xcb_block_len += nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_name_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_ALIASES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyAliases */
        _aux->keyAliases = (xcb_xkb_key_alias_t *)xcb_tmp;
        xcb_block_len += nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_alias_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_RG_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* radioGroupNames */
        _aux->radioGroupNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += nRadioGroups * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

int
xcb_xkb_set_names_values_sizeof (const void  *_buffer  /**< */,
                                 uint8_t      nTypes  /**< */,
                                 uint8_t      nKTLevels  /**< */,
                                 uint32_t     indicators  /**< */,
                                 uint16_t     virtualMods  /**< */,
                                 uint8_t      groupNames  /**< */,
                                 uint8_t      nKeys  /**< */,
                                 uint8_t      nKeyAliases  /**< */,
                                 uint8_t      nRadioGroups  /**< */,
                                 uint32_t     which  /**< */)
{
    xcb_xkb_set_names_values_t _aux;
    return xcb_xkb_set_names_values_unpack(_buffer, nTypes, nKTLevels, indicators, virtualMods, groupNames, nKeys, nKeyAliases, nRadioGroups, which, &_aux);
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_names_checked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               virtualMods
 ** @param uint32_t               which
 ** @param uint8_t                firstType
 ** @param uint8_t                nTypes
 ** @param uint8_t                firstKTLevelt
 ** @param uint8_t                nKTLevels
 ** @param uint32_t               indicators
 ** @param uint8_t                groupNames
 ** @param uint8_t                nRadioGroups
 ** @param xcb_keycode_t          firstKey
 ** @param uint8_t                nKeys
 ** @param uint8_t                nKeyAliases
 ** @param uint16_t               totalKTLevelNames
 ** @param const void            *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_names_checked (xcb_connection_t      *c  /**< */,
                           xcb_xkb_device_spec_t  deviceSpec  /**< */,
                           uint16_t               virtualMods  /**< */,
                           uint32_t               which  /**< */,
                           uint8_t                firstType  /**< */,
                           uint8_t                nTypes  /**< */,
                           uint8_t                firstKTLevelt  /**< */,
                           uint8_t                nKTLevels  /**< */,
                           uint32_t               indicators  /**< */,
                           uint8_t                groupNames  /**< */,
                           uint8_t                nRadioGroups  /**< */,
                           xcb_keycode_t          firstKey  /**< */,
                           uint8_t                nKeys  /**< */,
                           uint8_t                nKeyAliases  /**< */,
                           uint16_t               totalKTLevelNames  /**< */,
                           const void            *values  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_NAMES,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_names_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.virtualMods = virtualMods;
    xcb_out.which = which;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKTLevelt = firstKTLevelt;
    xcb_out.nKTLevels = nKTLevels;
    xcb_out.indicators = indicators;
    xcb_out.groupNames = groupNames;
    xcb_out.nRadioGroups = nRadioGroups;
    xcb_out.firstKey = firstKey;
    xcb_out.nKeys = nKeys;
    xcb_out.nKeyAliases = nKeyAliases;
    xcb_out.pad0 = 0;
    xcb_out.totalKTLevelNames = totalKTLevelNames;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_set_names_values_t values */
    xcb_parts[4].iov_base = (char *) values;
    xcb_parts[4].iov_len = 
      xcb_xkb_set_names_values_sizeof (values, nTypes, nKTLevels, indicators, virtualMods, groupNames, nKeys, nKeyAliases, nRadioGroups, which);
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_names
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               virtualMods
 ** @param uint32_t               which
 ** @param uint8_t                firstType
 ** @param uint8_t                nTypes
 ** @param uint8_t                firstKTLevelt
 ** @param uint8_t                nKTLevels
 ** @param uint32_t               indicators
 ** @param uint8_t                groupNames
 ** @param uint8_t                nRadioGroups
 ** @param xcb_keycode_t          firstKey
 ** @param uint8_t                nKeys
 ** @param uint8_t                nKeyAliases
 ** @param uint16_t               totalKTLevelNames
 ** @param const void            *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_names (xcb_connection_t      *c  /**< */,
                   xcb_xkb_device_spec_t  deviceSpec  /**< */,
                   uint16_t               virtualMods  /**< */,
                   uint32_t               which  /**< */,
                   uint8_t                firstType  /**< */,
                   uint8_t                nTypes  /**< */,
                   uint8_t                firstKTLevelt  /**< */,
                   uint8_t                nKTLevels  /**< */,
                   uint32_t               indicators  /**< */,
                   uint8_t                groupNames  /**< */,
                   uint8_t                nRadioGroups  /**< */,
                   xcb_keycode_t          firstKey  /**< */,
                   uint8_t                nKeys  /**< */,
                   uint8_t                nKeyAliases  /**< */,
                   uint16_t               totalKTLevelNames  /**< */,
                   const void            *values  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_NAMES,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_names_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.virtualMods = virtualMods;
    xcb_out.which = which;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKTLevelt = firstKTLevelt;
    xcb_out.nKTLevels = nKTLevels;
    xcb_out.indicators = indicators;
    xcb_out.groupNames = groupNames;
    xcb_out.nRadioGroups = nRadioGroups;
    xcb_out.firstKey = firstKey;
    xcb_out.nKeys = nKeys;
    xcb_out.nKeyAliases = nKeyAliases;
    xcb_out.pad0 = 0;
    xcb_out.totalKTLevelNames = totalKTLevelNames;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_set_names_values_t values */
    xcb_parts[4].iov_base = (char *) values;
    xcb_parts[4].iov_len = 
      xcb_xkb_set_names_values_sizeof (values, nTypes, nKTLevels, indicators, virtualMods, groupNames, nKeys, nKeyAliases, nRadioGroups, which);
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_names_aux_checked
 ** 
 ** @param xcb_connection_t                 *c
 ** @param xcb_xkb_device_spec_t             deviceSpec
 ** @param uint16_t                          virtualMods
 ** @param uint32_t                          which
 ** @param uint8_t                           firstType
 ** @param uint8_t                           nTypes
 ** @param uint8_t                           firstKTLevelt
 ** @param uint8_t                           nKTLevels
 ** @param uint32_t                          indicators
 ** @param uint8_t                           groupNames
 ** @param uint8_t                           nRadioGroups
 ** @param xcb_keycode_t                     firstKey
 ** @param uint8_t                           nKeys
 ** @param uint8_t                           nKeyAliases
 ** @param uint16_t                          totalKTLevelNames
 ** @param const xcb_xkb_set_names_values_t *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_names_aux_checked (xcb_connection_t                 *c  /**< */,
                               xcb_xkb_device_spec_t             deviceSpec  /**< */,
                               uint16_t                          virtualMods  /**< */,
                               uint32_t                          which  /**< */,
                               uint8_t                           firstType  /**< */,
                               uint8_t                           nTypes  /**< */,
                               uint8_t                           firstKTLevelt  /**< */,
                               uint8_t                           nKTLevels  /**< */,
                               uint32_t                          indicators  /**< */,
                               uint8_t                           groupNames  /**< */,
                               uint8_t                           nRadioGroups  /**< */,
                               xcb_keycode_t                     firstKey  /**< */,
                               uint8_t                           nKeys  /**< */,
                               uint8_t                           nKeyAliases  /**< */,
                               uint16_t                          totalKTLevelNames  /**< */,
                               const xcb_xkb_set_names_values_t *values  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_NAMES,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_names_request_t xcb_out;
    void *xcb_aux0 = 0;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.virtualMods = virtualMods;
    xcb_out.which = which;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKTLevelt = firstKTLevelt;
    xcb_out.nKTLevels = nKTLevels;
    xcb_out.indicators = indicators;
    xcb_out.groupNames = groupNames;
    xcb_out.nRadioGroups = nRadioGroups;
    xcb_out.firstKey = firstKey;
    xcb_out.nKeys = nKeys;
    xcb_out.nKeyAliases = nKeyAliases;
    xcb_out.pad0 = 0;
    xcb_out.totalKTLevelNames = totalKTLevelNames;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_set_names_values_t values */
    xcb_parts[4].iov_len = 
      xcb_xkb_set_names_values_serialize (&xcb_aux0, nTypes, nKTLevels, indicators, virtualMods, groupNames, nKeys, nKeyAliases, nRadioGroups, which, values);
    xcb_parts[4].iov_base = xcb_aux0;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    free(xcb_aux0);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_names_aux
 ** 
 ** @param xcb_connection_t                 *c
 ** @param xcb_xkb_device_spec_t             deviceSpec
 ** @param uint16_t                          virtualMods
 ** @param uint32_t                          which
 ** @param uint8_t                           firstType
 ** @param uint8_t                           nTypes
 ** @param uint8_t                           firstKTLevelt
 ** @param uint8_t                           nKTLevels
 ** @param uint32_t                          indicators
 ** @param uint8_t                           groupNames
 ** @param uint8_t                           nRadioGroups
 ** @param xcb_keycode_t                     firstKey
 ** @param uint8_t                           nKeys
 ** @param uint8_t                           nKeyAliases
 ** @param uint16_t                          totalKTLevelNames
 ** @param const xcb_xkb_set_names_values_t *values
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_names_aux (xcb_connection_t                 *c  /**< */,
                       xcb_xkb_device_spec_t             deviceSpec  /**< */,
                       uint16_t                          virtualMods  /**< */,
                       uint32_t                          which  /**< */,
                       uint8_t                           firstType  /**< */,
                       uint8_t                           nTypes  /**< */,
                       uint8_t                           firstKTLevelt  /**< */,
                       uint8_t                           nKTLevels  /**< */,
                       uint32_t                          indicators  /**< */,
                       uint8_t                           groupNames  /**< */,
                       uint8_t                           nRadioGroups  /**< */,
                       xcb_keycode_t                     firstKey  /**< */,
                       uint8_t                           nKeys  /**< */,
                       uint8_t                           nKeyAliases  /**< */,
                       uint16_t                          totalKTLevelNames  /**< */,
                       const xcb_xkb_set_names_values_t *values  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 3,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_NAMES,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[5];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_names_request_t xcb_out;
    void *xcb_aux0 = 0;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.virtualMods = virtualMods;
    xcb_out.which = which;
    xcb_out.firstType = firstType;
    xcb_out.nTypes = nTypes;
    xcb_out.firstKTLevelt = firstKTLevelt;
    xcb_out.nKTLevels = nKTLevels;
    xcb_out.indicators = indicators;
    xcb_out.groupNames = groupNames;
    xcb_out.nRadioGroups = nRadioGroups;
    xcb_out.firstKey = firstKey;
    xcb_out.nKeys = nKeys;
    xcb_out.nKeyAliases = nKeyAliases;
    xcb_out.pad0 = 0;
    xcb_out.totalKTLevelNames = totalKTLevelNames;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_set_names_values_t values */
    xcb_parts[4].iov_len = 
      xcb_xkb_set_names_values_serialize (&xcb_aux0, nTypes, nKTLevels, indicators, virtualMods, groupNames, nKeys, nKeyAliases, nRadioGroups, which, values);
    xcb_parts[4].iov_base = xcb_aux0;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    free(xcb_aux0);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_per_client_flags_cookie_t xcb_xkb_per_client_flags
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint32_t               change
 ** @param uint32_t               value
 ** @param uint32_t               ctrlsToChange
 ** @param uint32_t               autoCtrls
 ** @param uint32_t               autoCtrlsValues
 ** @returns xcb_xkb_per_client_flags_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_per_client_flags_cookie_t
xcb_xkb_per_client_flags (xcb_connection_t      *c  /**< */,
                          xcb_xkb_device_spec_t  deviceSpec  /**< */,
                          uint32_t               change  /**< */,
                          uint32_t               value  /**< */,
                          uint32_t               ctrlsToChange  /**< */,
                          uint32_t               autoCtrls  /**< */,
                          uint32_t               autoCtrlsValues  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_PER_CLIENT_FLAGS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_per_client_flags_cookie_t xcb_ret;
    xcb_xkb_per_client_flags_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.change = change;
    xcb_out.value = value;
    xcb_out.ctrlsToChange = ctrlsToChange;
    xcb_out.autoCtrls = autoCtrls;
    xcb_out.autoCtrlsValues = autoCtrlsValues;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_per_client_flags_cookie_t xcb_xkb_per_client_flags_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint32_t               change
 ** @param uint32_t               value
 ** @param uint32_t               ctrlsToChange
 ** @param uint32_t               autoCtrls
 ** @param uint32_t               autoCtrlsValues
 ** @returns xcb_xkb_per_client_flags_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_per_client_flags_cookie_t
xcb_xkb_per_client_flags_unchecked (xcb_connection_t      *c  /**< */,
                                    xcb_xkb_device_spec_t  deviceSpec  /**< */,
                                    uint32_t               change  /**< */,
                                    uint32_t               value  /**< */,
                                    uint32_t               ctrlsToChange  /**< */,
                                    uint32_t               autoCtrls  /**< */,
                                    uint32_t               autoCtrlsValues  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_PER_CLIENT_FLAGS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_per_client_flags_cookie_t xcb_ret;
    xcb_xkb_per_client_flags_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.change = change;
    xcb_out.value = value;
    xcb_out.ctrlsToChange = ctrlsToChange;
    xcb_out.autoCtrls = autoCtrls;
    xcb_out.autoCtrlsValues = autoCtrlsValues;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_per_client_flags_reply_t * xcb_xkb_per_client_flags_reply
 ** 
 ** @param xcb_connection_t                   *c
 ** @param xcb_xkb_per_client_flags_cookie_t   cookie
 ** @param xcb_generic_error_t               **e
 ** @returns xcb_xkb_per_client_flags_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_per_client_flags_reply_t *
xcb_xkb_per_client_flags_reply (xcb_connection_t                   *c  /**< */,
                                xcb_xkb_per_client_flags_cookie_t   cookie  /**< */,
                                xcb_generic_error_t               **e  /**< */)
{
    return (xcb_xkb_per_client_flags_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xkb_list_components_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_list_components_reply_t *_aux = (xcb_xkb_list_components_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;

    unsigned int i;
    unsigned int xcb_tmp_len;

    xcb_block_len += sizeof(xcb_xkb_list_components_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* keymaps */
    for(i=0; i<_aux->nKeymaps; i++) {
        xcb_tmp_len = xcb_xkb_listing_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_listing_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* keycodes */
    for(i=0; i<_aux->nKeycodes; i++) {
        xcb_tmp_len = xcb_xkb_listing_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_listing_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* types */
    for(i=0; i<_aux->nTypes; i++) {
        xcb_tmp_len = xcb_xkb_listing_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_listing_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* compatMaps */
    for(i=0; i<_aux->nCompatMaps; i++) {
        xcb_tmp_len = xcb_xkb_listing_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_listing_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* symbols */
    for(i=0; i<_aux->nSymbols; i++) {
        xcb_tmp_len = xcb_xkb_listing_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_listing_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* geometries */
    for(i=0; i<_aux->nGeometries; i++) {
        xcb_tmp_len = xcb_xkb_listing_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_listing_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_list_components_cookie_t xcb_xkb_list_components
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               maxNames
 ** @returns xcb_xkb_list_components_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_list_components_cookie_t
xcb_xkb_list_components (xcb_connection_t      *c  /**< */,
                         xcb_xkb_device_spec_t  deviceSpec  /**< */,
                         uint16_t               maxNames  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_LIST_COMPONENTS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_list_components_cookie_t xcb_ret;
    xcb_xkb_list_components_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.maxNames = maxNames;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_list_components_cookie_t xcb_xkb_list_components_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               maxNames
 ** @returns xcb_xkb_list_components_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_list_components_cookie_t
xcb_xkb_list_components_unchecked (xcb_connection_t      *c  /**< */,
                                   xcb_xkb_device_spec_t  deviceSpec  /**< */,
                                   uint16_t               maxNames  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_LIST_COMPONENTS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_list_components_cookie_t xcb_ret;
    xcb_xkb_list_components_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.maxNames = maxNames;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** int xcb_xkb_list_components_keymaps_length
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_list_components_keymaps_length (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    return R->nKeymaps;
}


/*****************************************************************************
 **
 ** xcb_xkb_listing_iterator_t xcb_xkb_list_components_keymaps_iterator
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns xcb_xkb_listing_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_listing_iterator_t
xcb_xkb_list_components_keymaps_iterator (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    xcb_xkb_listing_iterator_t i;
    i.data = (xcb_xkb_listing_t *) (R + 1);
    i.rem = R->nKeymaps;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_list_components_keycodes_length
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_list_components_keycodes_length (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    return R->nKeycodes;
}


/*****************************************************************************
 **
 ** xcb_xkb_listing_iterator_t xcb_xkb_list_components_keycodes_iterator
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns xcb_xkb_listing_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_listing_iterator_t
xcb_xkb_list_components_keycodes_iterator (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    xcb_xkb_listing_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_listing_end(xcb_xkb_list_components_keymaps_iterator(R));
    i.data = (xcb_xkb_listing_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_listing_t, prev.index));
    i.rem = R->nKeycodes;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_list_components_types_length
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_list_components_types_length (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    return R->nTypes;
}


/*****************************************************************************
 **
 ** xcb_xkb_listing_iterator_t xcb_xkb_list_components_types_iterator
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns xcb_xkb_listing_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_listing_iterator_t
xcb_xkb_list_components_types_iterator (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    xcb_xkb_listing_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_listing_end(xcb_xkb_list_components_keycodes_iterator(R));
    i.data = (xcb_xkb_listing_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_listing_t, prev.index));
    i.rem = R->nTypes;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_list_components_compat_maps_length
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_list_components_compat_maps_length (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    return R->nCompatMaps;
}


/*****************************************************************************
 **
 ** xcb_xkb_listing_iterator_t xcb_xkb_list_components_compat_maps_iterator
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns xcb_xkb_listing_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_listing_iterator_t
xcb_xkb_list_components_compat_maps_iterator (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    xcb_xkb_listing_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_listing_end(xcb_xkb_list_components_types_iterator(R));
    i.data = (xcb_xkb_listing_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_listing_t, prev.index));
    i.rem = R->nCompatMaps;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_list_components_symbols_length
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_list_components_symbols_length (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    return R->nSymbols;
}


/*****************************************************************************
 **
 ** xcb_xkb_listing_iterator_t xcb_xkb_list_components_symbols_iterator
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns xcb_xkb_listing_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_listing_iterator_t
xcb_xkb_list_components_symbols_iterator (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    xcb_xkb_listing_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_listing_end(xcb_xkb_list_components_compat_maps_iterator(R));
    i.data = (xcb_xkb_listing_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_listing_t, prev.index));
    i.rem = R->nSymbols;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_list_components_geometries_length
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_list_components_geometries_length (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    return R->nGeometries;
}


/*****************************************************************************
 **
 ** xcb_xkb_listing_iterator_t xcb_xkb_list_components_geometries_iterator
 ** 
 ** @param const xcb_xkb_list_components_reply_t *R
 ** @returns xcb_xkb_listing_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_listing_iterator_t
xcb_xkb_list_components_geometries_iterator (const xcb_xkb_list_components_reply_t *R  /**< */)
{
    xcb_xkb_listing_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_listing_end(xcb_xkb_list_components_symbols_iterator(R));
    i.data = (xcb_xkb_listing_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_listing_t, prev.index));
    i.rem = R->nGeometries;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_list_components_reply_t * xcb_xkb_list_components_reply
 ** 
 ** @param xcb_connection_t                  *c
 ** @param xcb_xkb_list_components_cookie_t   cookie
 ** @param xcb_generic_error_t              **e
 ** @returns xcb_xkb_list_components_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_list_components_reply_t *
xcb_xkb_list_components_reply (xcb_connection_t                  *c  /**< */,
                               xcb_xkb_list_components_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e  /**< */)
{
    return (xcb_xkb_list_components_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_types_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_types_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                             const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.nTypes;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_type_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_types_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_xkb_key_type_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_type_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_types_rtrn_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                               const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_key_type_iterator_t i;
    i.data = /* replies */ S->types.map.types_rtrn;
    i.rem = /* replies */ S->types.nTypes;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_syms_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_syms_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                            const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.nKeySyms;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_sym_map_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_syms_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_xkb_key_sym_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_sym_map_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_syms_rtrn_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                              const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_key_sym_map_iterator_t i;
    i.data = /* replies */ S->types.map.syms_rtrn;
    i.rem = /* replies */ S->types.nKeySyms;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_count
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_count (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.map.acts_rtrn_count;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_count_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_count_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                  const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.nKeyActions;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_count_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_count_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                               const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->types.map.acts_rtrn_count + /* replies */ S->types.nKeyActions;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_action_t * xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_acts
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_action_t *
 **
 *****************************************************************************/
 
xcb_xkb_action_t *
xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_acts (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.map.acts_rtrn_acts;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_acts_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_acts_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                 const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.totalActions;
}


/*****************************************************************************
 **
 ** xcb_xkb_action_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_acts_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_xkb_action_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_action_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_acts_rtrn_acts_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                   const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_action_iterator_t i;
    i.data = /* replies */ S->types.map.acts_rtrn_acts;
    i.rem = /* replies */ S->types.totalActions;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_behavior_t * xcb_xkb_get_kbd_by_name_replies_types_map_behaviors_rtrn
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_set_behavior_t *
 **
 *****************************************************************************/
 
xcb_xkb_set_behavior_t *
xcb_xkb_get_kbd_by_name_replies_types_map_behaviors_rtrn (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.map.behaviors_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_behaviors_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_behaviors_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                 const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.totalKeyBehaviors;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_behavior_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_behaviors_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_xkb_set_behavior_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_set_behavior_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_behaviors_rtrn_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                   const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_set_behavior_iterator_t i;
    i.data = /* replies */ S->types.map.behaviors_rtrn;
    i.rem = /* replies */ S->types.totalKeyBehaviors;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_kbd_by_name_replies_types_map_vmods_rtrn
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_kbd_by_name_replies_types_map_vmods_rtrn (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.map.vmods_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_vmods_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_vmods_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                             const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return xcb_popcount(/* replies */ S->types.virtualMods);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_vmods_rtrn_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_vmods_rtrn_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                          const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->types.map.vmods_rtrn + xcb_popcount(/* replies */ S->types.virtualMods);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_explicit_t * xcb_xkb_get_kbd_by_name_replies_types_map_explicit_rtrn
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_set_explicit_t *
 **
 *****************************************************************************/
 
xcb_xkb_set_explicit_t *
xcb_xkb_get_kbd_by_name_replies_types_map_explicit_rtrn (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.map.explicit_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_explicit_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_explicit_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.totalKeyExplicit;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_explicit_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_explicit_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_xkb_set_explicit_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_set_explicit_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_explicit_rtrn_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                  const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_set_explicit_iterator_t i;
    i.data = /* replies */ S->types.map.explicit_rtrn;
    i.rem = /* replies */ S->types.totalKeyExplicit;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_mod_map_t * xcb_xkb_get_kbd_by_name_replies_types_map_modmap_rtrn
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_key_mod_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_mod_map_t *
xcb_xkb_get_kbd_by_name_replies_types_map_modmap_rtrn (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.map.modmap_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_modmap_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_modmap_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                              const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.totalModMapKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_mod_map_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_modmap_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_xkb_key_mod_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_mod_map_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_modmap_rtrn_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_key_mod_map_iterator_t i;
    i.data = /* replies */ S->types.map.modmap_rtrn;
    i.rem = /* replies */ S->types.totalModMapKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_v_mod_map_t * xcb_xkb_get_kbd_by_name_replies_types_map_vmodmap_rtrn
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_key_v_mod_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_v_mod_map_t *
xcb_xkb_get_kbd_by_name_replies_types_map_vmodmap_rtrn (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.map.vmodmap_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_types_map_vmodmap_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_types_map_vmodmap_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                               const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->types.totalVModMapKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_v_mod_map_iterator_t xcb_xkb_get_kbd_by_name_replies_types_map_vmodmap_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_types_map_t *R
 ** @returns xcb_xkb_key_v_mod_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_v_mod_map_iterator_t
xcb_xkb_get_kbd_by_name_replies_types_map_vmodmap_rtrn_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                 const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_key_v_mod_map_iterator_t i;
    i.data = /* replies */ S->types.map.vmodmap_rtrn;
    i.rem = /* replies */ S->types.totalVModMapKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}

int
xcb_xkb_get_kbd_by_name_replies_types_map_serialize (void                                              **_buffer  /**< */,
                                                     uint8_t                                             nTypes  /**< */,
                                                     uint8_t                                             nKeySyms  /**< */,
                                                     uint8_t                                             nKeyActions  /**< */,
                                                     uint16_t                                            totalActions  /**< */,
                                                     uint8_t                                             totalKeyBehaviors  /**< */,
                                                     uint16_t                                            virtualMods  /**< */,
                                                     uint8_t                                             totalKeyExplicit  /**< */,
                                                     uint8_t                                             totalModMapKeys  /**< */,
                                                     uint8_t                                             totalVModMapKeys  /**< */,
                                                     uint16_t                                            present  /**< */,
                                                     const xcb_xkb_get_kbd_by_name_replies_types_map_t  *_aux  /**< */)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[19];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if(present & XCB_XKB_MAP_PART_KEY_TYPES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* types_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->types_rtrn;
        xcb_parts[xcb_parts_idx].iov_len = 0;
        xcb_tmp = (char *) _aux->types_rtrn;
        for(i=0; i<nTypes; i++) { 
            xcb_block_len = xcb_xkb_key_type_sizeof(xcb_tmp);
            xcb_parts[xcb_parts_idx].iov_len += xcb_block_len;
        }
        xcb_block_len = xcb_parts[xcb_parts_idx].iov_len;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_type_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_SYMS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* syms_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->syms_rtrn;
        xcb_parts[xcb_parts_idx].iov_len = 0;
        xcb_tmp = (char *) _aux->syms_rtrn;
        for(i=0; i<nKeySyms; i++) { 
            xcb_block_len = xcb_xkb_key_sym_map_sizeof(xcb_tmp);
            xcb_parts[xcb_parts_idx].iov_len += xcb_block_len;
        }
        xcb_block_len = xcb_parts[xcb_parts_idx].iov_len;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_sym_map_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_ACTIONS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* acts_rtrn_count */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->acts_rtrn_count;
        xcb_block_len += nKeyActions * sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeyActions * sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* acts_rtrn_acts */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->acts_rtrn_acts;
        xcb_block_len += totalActions * sizeof(xcb_xkb_action_t);
        xcb_parts[xcb_parts_idx].iov_len = totalActions * sizeof(xcb_xkb_action_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_action_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_BEHAVIORS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* behaviors_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->behaviors_rtrn;
        xcb_block_len += totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_parts[xcb_parts_idx].iov_len = totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_set_behavior_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MODS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmods_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->vmods_rtrn;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if(present & XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* explicit_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->explicit_rtrn;
        xcb_block_len += totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_parts[xcb_parts_idx].iov_len = totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_set_explicit_t);
    }
    if(present & XCB_XKB_MAP_PART_MODIFIER_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* modmap_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->modmap_rtrn;
        xcb_block_len += totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_parts[xcb_parts_idx].iov_len = totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_mod_map_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmodmap_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->vmodmap_rtrn;
        xcb_block_len += totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_parts[xcb_parts_idx].iov_len = totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_v_mod_map_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_xkb_get_kbd_by_name_replies_types_map_unpack (const void                                   *_buffer  /**< */,
                                                  uint8_t                                       nTypes  /**< */,
                                                  uint8_t                                       nKeySyms  /**< */,
                                                  uint8_t                                       nKeyActions  /**< */,
                                                  uint16_t                                      totalActions  /**< */,
                                                  uint8_t                                       totalKeyBehaviors  /**< */,
                                                  uint16_t                                      virtualMods  /**< */,
                                                  uint8_t                                       totalKeyExplicit  /**< */,
                                                  uint8_t                                       totalModMapKeys  /**< */,
                                                  uint8_t                                       totalVModMapKeys  /**< */,
                                                  uint16_t                                      present  /**< */,
                                                  xcb_xkb_get_kbd_by_name_replies_types_map_t  *_aux  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;

    unsigned int i;
    unsigned int xcb_tmp_len;

    if(present & XCB_XKB_MAP_PART_KEY_TYPES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* types_rtrn */
        _aux->types_rtrn = (xcb_xkb_key_type_t *)xcb_tmp;
        for(i=0; i<nTypes; i++) {
            xcb_tmp_len = xcb_xkb_key_type_sizeof(xcb_tmp);
            xcb_block_len += xcb_tmp_len;
            xcb_tmp += xcb_tmp_len;
        }
        xcb_align_to = ALIGNOF(xcb_xkb_key_type_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_SYMS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* syms_rtrn */
        _aux->syms_rtrn = (xcb_xkb_key_sym_map_t *)xcb_tmp;
        for(i=0; i<nKeySyms; i++) {
            xcb_tmp_len = xcb_xkb_key_sym_map_sizeof(xcb_tmp);
            xcb_block_len += xcb_tmp_len;
            xcb_tmp += xcb_tmp_len;
        }
        xcb_align_to = ALIGNOF(xcb_xkb_key_sym_map_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_ACTIONS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* acts_rtrn_count */
        _aux->acts_rtrn_count = (uint8_t *)xcb_tmp;
        xcb_block_len += nKeyActions * sizeof(xcb_keycode_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* acts_rtrn_acts */
        _aux->acts_rtrn_acts = (xcb_xkb_action_t *)xcb_tmp;
        xcb_block_len += totalActions * sizeof(xcb_xkb_action_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_action_t);
    }
    if(present & XCB_XKB_MAP_PART_KEY_BEHAVIORS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* behaviors_rtrn */
        _aux->behaviors_rtrn = (xcb_xkb_set_behavior_t *)xcb_tmp;
        xcb_block_len += totalKeyBehaviors * sizeof(xcb_xkb_set_behavior_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_set_behavior_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MODS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmods_rtrn */
        _aux->vmods_rtrn = (uint8_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_keycode_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
    }
    if(present & XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* explicit_rtrn */
        _aux->explicit_rtrn = (xcb_xkb_set_explicit_t *)xcb_tmp;
        xcb_block_len += totalKeyExplicit * sizeof(xcb_xkb_set_explicit_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_set_explicit_t);
    }
    if(present & XCB_XKB_MAP_PART_MODIFIER_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* modmap_rtrn */
        _aux->modmap_rtrn = (xcb_xkb_key_mod_map_t *)xcb_tmp;
        xcb_block_len += totalModMapKeys * sizeof(xcb_xkb_key_mod_map_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_mod_map_t);
    }
    if(present & XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* vmodmap_rtrn */
        _aux->vmodmap_rtrn = (xcb_xkb_key_v_mod_map_t *)xcb_tmp;
        xcb_block_len += totalVModMapKeys * sizeof(xcb_xkb_key_v_mod_map_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_v_mod_map_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

int
xcb_xkb_get_kbd_by_name_replies_types_map_sizeof (const void  *_buffer  /**< */,
                                                  uint8_t      nTypes  /**< */,
                                                  uint8_t      nKeySyms  /**< */,
                                                  uint8_t      nKeyActions  /**< */,
                                                  uint16_t     totalActions  /**< */,
                                                  uint8_t      totalKeyBehaviors  /**< */,
                                                  uint16_t     virtualMods  /**< */,
                                                  uint8_t      totalKeyExplicit  /**< */,
                                                  uint8_t      totalModMapKeys  /**< */,
                                                  uint8_t      totalVModMapKeys  /**< */,
                                                  uint16_t     present  /**< */)
{
    xcb_xkb_get_kbd_by_name_replies_types_map_t _aux;
    return xcb_xkb_get_kbd_by_name_replies_types_map_unpack(_buffer, nTypes, nKeySyms, nKeyActions, totalActions, totalKeyBehaviors, virtualMods, totalKeyExplicit, totalModMapKeys, totalVModMapKeys, present, &_aux);
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_type_names
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_type_names (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.typeNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_type_names_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_type_names_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                        const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.nTypes;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_type_names_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_type_names_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                     const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.typeNames + /* replies */ S->key_names.nTypes;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** uint8_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_n_levels_per_type
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns uint8_t *
 **
 *****************************************************************************/
 
uint8_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_n_levels_per_type (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.nLevelsPerType;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_n_levels_per_type_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_n_levels_per_type_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                               const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.nKTLevels;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_n_levels_per_type_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_n_levels_per_type_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                            const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.nLevelsPerType + /* replies */ S->key_names.nKTLevels;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_kt_level_names
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_kt_level_names (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.ktLevelNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_kt_level_names_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_kt_level_names_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                            const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return qt_xcb_sumof(/* replies */ S->key_names.valueList.nLevelsPerType, /* replies */ S->key_names.nKTLevels);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_kt_level_names_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_kt_level_names_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                         const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.ktLevelNames + qt_xcb_sumof(/* replies */ S->key_names.valueList.nLevelsPerType, /* replies */ S->key_names.nKTLevels);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_indicator_names
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_indicator_names (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.indicatorNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_indicator_names_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_indicator_names_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                             const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return xcb_popcount(/* replies */ S->key_names.indicators);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_indicator_names_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_indicator_names_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                          const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.indicatorNames + xcb_popcount(/* replies */ S->key_names.indicators);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_virtual_mod_names
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_virtual_mod_names (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.virtualModNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_virtual_mod_names_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_virtual_mod_names_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                               const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return xcb_popcount(/* replies */ S->key_names.virtualMods);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_virtual_mod_names_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_virtual_mod_names_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                            const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.virtualModNames + xcb_popcount(/* replies */ S->key_names.virtualMods);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_groups
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_groups (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.groups;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_groups_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_groups_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                    const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return xcb_popcount(/* replies */ S->key_names.groupNames);
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_groups_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_groups_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                 const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.groups + xcb_popcount(/* replies */ S->key_names.groupNames);
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_name_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_names
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_key_name_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_name_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_names (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.keyNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_names_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_names_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                       const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.nKeys;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_name_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_names_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_xkb_key_name_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_name_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_names_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                         const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_key_name_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.keyNames;
    i.rem = /* replies */ S->key_names.nKeys;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_alias_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_aliases
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_key_alias_t *
 **
 *****************************************************************************/
 
xcb_xkb_key_alias_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_aliases (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.keyAliases;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_aliases_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_aliases_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                         const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.nKeyAliases;
}


/*****************************************************************************
 **
 ** xcb_xkb_key_alias_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_aliases_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_xkb_key_alias_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_key_alias_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_key_aliases_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                           const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_key_alias_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.keyAliases;
    i.rem = /* replies */ S->key_names.nKeyAliases;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_atom_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list_radio_group_names
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_atom_t *
 **
 *****************************************************************************/
 
xcb_atom_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_radio_group_names (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.valueList.radioGroupNames;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_key_names_value_list_radio_group_names_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_radio_group_names_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                               const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->key_names.nRadioGroups;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_kbd_by_name_replies_key_names_value_list_radio_group_names_end
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_radio_group_names_end (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                            const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = /* replies */ S->key_names.valueList.radioGroupNames + /* replies */ S->key_names.nRadioGroups;
    i.rem = 0;
    i.index = (char *) i.data - (char *) S;
    return i;
}

int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_serialize (void                                                         **_buffer  /**< */,
                                                                uint8_t                                                        nTypes  /**< */,
                                                                uint16_t                                                       nKTLevels  /**< */,
                                                                uint32_t                                                       indicators  /**< */,
                                                                uint16_t                                                       virtualMods  /**< */,
                                                                uint8_t                                                        groupNames  /**< */,
                                                                uint8_t                                                        nKeys  /**< */,
                                                                uint8_t                                                        nKeyAliases  /**< */,
                                                                uint8_t                                                        nRadioGroups  /**< */,
                                                                uint32_t                                                       which  /**< */,
                                                                const xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t  *_aux  /**< */)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[25];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if(which & XCB_XKB_NAME_DETAIL_KEYCODES) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.keycodesName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->keycodesName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GEOMETRY) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.geometryName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometryName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_SYMBOLS) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.symbolsName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->symbolsName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_PHYS_SYMBOLS) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.physSymbolsName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->physSymbolsName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_TYPES) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.typesName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->typesName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_COMPAT) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.compatName */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compatName;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_TYPE_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* typeNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->typeNames;
        xcb_block_len += nTypes * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = nTypes * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KT_LEVEL_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* nLevelsPerType */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->nLevelsPerType;
        xcb_block_len += nKTLevels * sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = nKTLevels * sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* ktLevelNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->ktLevelNames;
        xcb_block_len += qt_xcb_sumof(_aux->nLevelsPerType, nKTLevels) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = qt_xcb_sumof(_aux->nLevelsPerType, nKTLevels) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_INDICATOR_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* indicatorNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->indicatorNames;
        xcb_block_len += xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* virtualModNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->virtualModNames;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GROUP_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* groups */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->groups;
        xcb_block_len += xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->keyNames;
        xcb_block_len += nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_name_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_ALIASES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyAliases */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->keyAliases;
        xcb_block_len += nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_parts[xcb_parts_idx].iov_len = nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_key_alias_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_RG_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* radioGroupNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->radioGroupNames;
        xcb_block_len += nRadioGroups * sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = nRadioGroups * sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_unpack (const void                                              *_buffer  /**< */,
                                                             uint8_t                                                  nTypes  /**< */,
                                                             uint16_t                                                 nKTLevels  /**< */,
                                                             uint32_t                                                 indicators  /**< */,
                                                             uint16_t                                                 virtualMods  /**< */,
                                                             uint8_t                                                  groupNames  /**< */,
                                                             uint8_t                                                  nKeys  /**< */,
                                                             uint8_t                                                  nKeyAliases  /**< */,
                                                             uint8_t                                                  nRadioGroups  /**< */,
                                                             uint32_t                                                 which  /**< */,
                                                             xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t  *_aux  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    if(which & XCB_XKB_NAME_DETAIL_KEYCODES) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.keycodesName */
        _aux->keycodesName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GEOMETRY) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.geometryName */
        _aux->geometryName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_SYMBOLS) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.symbolsName */
        _aux->symbolsName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_PHYS_SYMBOLS) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.physSymbolsName */
        _aux->physSymbolsName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_TYPES) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.typesName */
        _aux->typesName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_COMPAT) {
        /* xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t.compatName */
        _aux->compatName = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_TYPE_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* typeNames */
        _aux->typeNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += nTypes * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KT_LEVEL_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* nLevelsPerType */
        _aux->nLevelsPerType = (uint8_t *)xcb_tmp;
        xcb_block_len += nKTLevels * sizeof(uint8_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* ktLevelNames */
        _aux->ktLevelNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += qt_xcb_sumof(_aux->nLevelsPerType, nKTLevels) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_INDICATOR_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* indicatorNames */
        _aux->indicatorNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(indicators) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_VIRTUAL_MOD_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* virtualModNames */
        _aux->virtualModNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(virtualMods) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_GROUP_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* groups */
        _aux->groups = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(groupNames) * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyNames */
        _aux->keyNames = (xcb_xkb_key_name_t *)xcb_tmp;
        xcb_block_len += nKeys * sizeof(xcb_xkb_key_name_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_name_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_KEY_ALIASES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* keyAliases */
        _aux->keyAliases = (xcb_xkb_key_alias_t *)xcb_tmp;
        xcb_block_len += nKeyAliases * sizeof(xcb_xkb_key_alias_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_key_alias_t);
    }
    if(which & XCB_XKB_NAME_DETAIL_RG_NAMES) {
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* radioGroupNames */
        _aux->radioGroupNames = (xcb_atom_t *)xcb_tmp;
        xcb_block_len += nRadioGroups * sizeof(xcb_atom_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_atom_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

int
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_sizeof (const void  *_buffer  /**< */,
                                                             uint8_t      nTypes  /**< */,
                                                             uint16_t     nKTLevels  /**< */,
                                                             uint32_t     indicators  /**< */,
                                                             uint16_t     virtualMods  /**< */,
                                                             uint8_t      groupNames  /**< */,
                                                             uint8_t      nKeys  /**< */,
                                                             uint8_t      nKeyAliases  /**< */,
                                                             uint8_t      nRadioGroups  /**< */,
                                                             uint32_t     which  /**< */)
{
    xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t _aux;
    return xcb_xkb_get_kbd_by_name_replies_key_names_value_list_unpack(_buffer, nTypes, nKTLevels, indicators, virtualMods, groupNames, nKeys, nKeyAliases, nRadioGroups, which, &_aux);
}


/*****************************************************************************
 **
 ** xcb_xkb_get_kbd_by_name_replies_types_map_t * xcb_xkb_get_kbd_by_name_replies_types_map
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns xcb_xkb_get_kbd_by_name_replies_types_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_kbd_by_name_replies_types_map_t *
xcb_xkb_get_kbd_by_name_replies_types_map (const xcb_xkb_get_kbd_by_name_replies_t *R  /**< */)
{
    return (xcb_xkb_get_kbd_by_name_replies_types_map_t *) (R + 1);
}


/*****************************************************************************
 **
 ** xcb_xkb_sym_interpret_t * xcb_xkb_get_kbd_by_name_replies_compat_map_si_rtrn
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_sym_interpret_t *
 **
 *****************************************************************************/
 
xcb_xkb_sym_interpret_t *
xcb_xkb_get_kbd_by_name_replies_compat_map_si_rtrn (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->compat_map.si_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_compat_map_si_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_compat_map_si_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                           const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->compat_map.nSIRtrn;
}


/*****************************************************************************
 **
 ** xcb_xkb_sym_interpret_iterator_t xcb_xkb_get_kbd_by_name_replies_compat_map_si_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns xcb_xkb_sym_interpret_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_sym_interpret_iterator_t
xcb_xkb_get_kbd_by_name_replies_compat_map_si_rtrn_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                             const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_sym_interpret_iterator_t i;
    i.data = /* replies */ S->compat_map.si_rtrn;
    i.rem = /* replies */ S->compat_map.nSIRtrn;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_mod_def_t * xcb_xkb_get_kbd_by_name_replies_compat_map_group_rtrn
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_mod_def_t *
 **
 *****************************************************************************/
 
xcb_xkb_mod_def_t *
xcb_xkb_get_kbd_by_name_replies_compat_map_group_rtrn (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->compat_map.group_rtrn;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_compat_map_group_rtrn_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_compat_map_group_rtrn_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                              const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return xcb_popcount(/* replies */ S->compat_map.groupsRtrn);
}


/*****************************************************************************
 **
 ** xcb_xkb_mod_def_iterator_t xcb_xkb_get_kbd_by_name_replies_compat_map_group_rtrn_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns xcb_xkb_mod_def_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_mod_def_iterator_t
xcb_xkb_get_kbd_by_name_replies_compat_map_group_rtrn_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                                const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_mod_def_iterator_t i;
    i.data = /* replies */ S->compat_map.group_rtrn;
    i.rem = xcb_popcount(/* replies */ S->compat_map.groupsRtrn);
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_indicator_map_t * xcb_xkb_get_kbd_by_name_replies_indicator_maps_maps
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *S
 ** @returns xcb_xkb_indicator_map_t *
 **
 *****************************************************************************/
 
xcb_xkb_indicator_map_t *
xcb_xkb_get_kbd_by_name_replies_indicator_maps_maps (const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->indicator_maps.maps;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_kbd_by_name_replies_indicator_maps_maps_length
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_kbd_by_name_replies_indicator_maps_maps_length (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                            const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    return /* replies */ S->indicator_maps.nIndicators;
}


/*****************************************************************************
 **
 ** xcb_xkb_indicator_map_iterator_t xcb_xkb_get_kbd_by_name_replies_indicator_maps_maps_iterator
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns xcb_xkb_indicator_map_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_indicator_map_iterator_t
xcb_xkb_get_kbd_by_name_replies_indicator_maps_maps_iterator (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */,
                                                              const xcb_xkb_get_kbd_by_name_replies_t *S  /**< */)
{
    xcb_xkb_indicator_map_iterator_t i;
    i.data = /* replies */ S->indicator_maps.maps;
    i.rem = /* replies */ S->indicator_maps.nIndicators;
    i.index = (char *) i.data - (char *) S;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t * xcb_xkb_get_kbd_by_name_replies_key_names_value_list
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *
xcb_xkb_get_kbd_by_name_replies_key_names_value_list (const xcb_xkb_get_kbd_by_name_replies_t *R  /**< */)
{
    return (xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t *) (R + 1);
}


/*****************************************************************************
 **
 ** xcb_xkb_counted_string_16_t * xcb_xkb_get_kbd_by_name_replies_geometry_label_font
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_replies_t *R
 ** @returns xcb_xkb_counted_string_16_t *
 **
 *****************************************************************************/
 
xcb_xkb_counted_string_16_t *
xcb_xkb_get_kbd_by_name_replies_geometry_label_font (const xcb_xkb_get_kbd_by_name_replies_t *R  /**< */)
{
    return (xcb_xkb_counted_string_16_t *) (R + 1);
}

int
xcb_xkb_get_kbd_by_name_replies_serialize (void                                    **_buffer  /**< */,
                                           uint16_t                                  reported  /**< */,
                                           const xcb_xkb_get_kbd_by_name_replies_t  *_aux  /**< */)
{
    char *xcb_out = *_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_align_to = 0;

    unsigned int xcb_pad = 0;
    char xcb_pad0[3] = {0, 0, 0};
    struct iovec xcb_parts[96];
    unsigned int xcb_parts_idx = 0;
    unsigned int xcb_block_len = 0;
    unsigned int i;
    char *xcb_tmp;

    if((reported & XCB_XKB_GBN_DETAIL_TYPES) ||
       (reported & XCB_XKB_GBN_DETAIL_CLIENT_SYMBOLS) ||
       (reported & XCB_XKB_GBN_DETAIL_SERVER_SYMBOLS)) {
        /* xcb_xkb_get_kbd_by_name_replies_t.types.getmap_type */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.getmap_type;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.typeDeviceID */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.typeDeviceID;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.getmap_sequence */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.getmap_sequence;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.getmap_length */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.getmap_length;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.pad0 */
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_block_len += sizeof(uint8_t)*2;
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t)*2;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.typeMinKeyCode */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.typeMinKeyCode;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.typeMaxKeyCode */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.typeMaxKeyCode;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.present */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.present;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstType */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.firstType;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nTypes */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.nTypes;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalTypes */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.totalTypes;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstKeySym */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.firstKeySym;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalSyms */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.totalSyms;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nKeySyms */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.nKeySyms;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstKeyAction */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.firstKeyAction;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalActions */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.totalActions;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nKeyActions */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.nKeyActions;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstKeyBehavior */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.firstKeyBehavior;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nKeyBehaviors */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.nKeyBehaviors;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalKeyBehaviors */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.totalKeyBehaviors;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstKeyExplicit */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.firstKeyExplicit;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nKeyExplicit */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.nKeyExplicit;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalKeyExplicit */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.totalKeyExplicit;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstModMapKey */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.firstModMapKey;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nModMapKeys */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.nModMapKeys;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalModMapKeys */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.totalModMapKeys;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstVModMapKey */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.firstVModMapKey;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nVModMapKeys */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.nVModMapKeys;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalVModMapKeys */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.totalVModMapKeys;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.pad1 */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &xcb_pad;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.virtualMods */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->types.virtualMods;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* map */
        xcb_parts[xcb_parts_idx].iov_base = (char *)0;
        xcb_block_len += xcb_xkb_get_kbd_by_name_replies_types_map_serialize(&xcb_parts[xcb_parts_idx].iov_base, _aux->types.nTypes, _aux->types.nKeySyms, _aux->types.nKeyActions, _aux->types.totalActions, _aux->types.totalKeyBehaviors, _aux->types.virtualMods, _aux->types.totalKeyExplicit, _aux->types.totalModMapKeys, _aux->types.totalVModMapKeys, _aux->types.present, &_aux->types.map);
        xcb_parts[xcb_parts_idx].iov_len = xcb_xkb_get_kbd_by_name_replies_types_map_serialize(&xcb_parts[xcb_parts_idx].iov_base, _aux->types.nTypes, _aux->types.nKeySyms, _aux->types.nKeyActions, _aux->types.totalActions, _aux->types.totalKeyBehaviors, _aux->types.virtualMods, _aux->types.totalKeyExplicit, _aux->types.totalModMapKeys, _aux->types.totalVModMapKeys, _aux->types.present, &_aux->types.map);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_get_kbd_by_name_replies_types_map_t);
    }
    if(reported & XCB_XKB_GBN_DETAIL_COMPAT_MAP) {
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.compatmap_type */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compat_map.compatmap_type;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.compatDeviceID */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compat_map.compatDeviceID;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.compatmap_sequence */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compat_map.compatmap_sequence;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.compatmap_length */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compat_map.compatmap_length;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.groupsRtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compat_map.groupsRtrn;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.pad0 */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &xcb_pad;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.firstSIRtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compat_map.firstSIRtrn;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.nSIRtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compat_map.nSIRtrn;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.nTotalSI */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->compat_map.nTotalSI;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.pad1 */
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_block_len += sizeof(uint8_t)*16;
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t)*16;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* si_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->compat_map.si_rtrn;
        xcb_block_len += _aux->compat_map.nSIRtrn * sizeof(xcb_xkb_sym_interpret_t);
        xcb_parts[xcb_parts_idx].iov_len = _aux->compat_map.nSIRtrn * sizeof(xcb_xkb_sym_interpret_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_sym_interpret_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* group_rtrn */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->compat_map.group_rtrn;
        xcb_block_len += xcb_popcount(_aux->compat_map.groupsRtrn) * sizeof(xcb_xkb_mod_def_t);
        xcb_parts[xcb_parts_idx].iov_len = xcb_popcount(_aux->compat_map.groupsRtrn) * sizeof(xcb_xkb_mod_def_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_mod_def_t);
    }
    if(reported & XCB_XKB_GBN_DETAIL_INDICATOR_MAPS) {
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.indicatormap_type */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicator_maps.indicatormap_type;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.indicatorDeviceID */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicator_maps.indicatorDeviceID;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.indicatormap_sequence */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicator_maps.indicatormap_sequence;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.indicatormap_length */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicator_maps.indicatormap_length;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.which */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicator_maps.which;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.realIndicators */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicator_maps.realIndicators;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.nIndicators */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->indicator_maps.nIndicators;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.pad0 */
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_block_len += sizeof(uint8_t)*15;
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t)*15;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* maps */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->indicator_maps.maps;
        xcb_block_len += _aux->indicator_maps.nIndicators * sizeof(xcb_xkb_indicator_map_t);
        xcb_parts[xcb_parts_idx].iov_len = _aux->indicator_maps.nIndicators * sizeof(xcb_xkb_indicator_map_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_indicator_map_t);
    }
    if((reported & XCB_XKB_GBN_DETAIL_KEY_NAMES) ||
       (reported & XCB_XKB_GBN_DETAIL_OTHER_NAMES)) {
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyname_type */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.keyname_type;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyDeviceID */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.keyDeviceID;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyname_sequence */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.keyname_sequence;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyname_length */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.keyname_length;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.which */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.which;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyMinKeyCode */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.keyMinKeyCode;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyMaxKeyCode */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.keyMaxKeyCode;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nTypes */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.nTypes;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.groupNames */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.groupNames;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.virtualMods */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.virtualMods;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.firstKey */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.firstKey;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_keycode_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nKeys */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.nKeys;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.indicators */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.indicators;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nRadioGroups */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.nRadioGroups;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nKeyAliases */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.nKeyAliases;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nKTLevels */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->key_names.nKTLevels;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.pad0 */
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_block_len += sizeof(uint8_t)*4;
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t)*4;
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* valueList */
        xcb_parts[xcb_parts_idx].iov_base = (char *)0;
        xcb_block_len += xcb_xkb_get_kbd_by_name_replies_key_names_value_list_serialize(&xcb_parts[xcb_parts_idx].iov_base, _aux->key_names.nTypes, _aux->key_names.nKTLevels, _aux->key_names.indicators, _aux->key_names.virtualMods, _aux->key_names.groupNames, _aux->key_names.nKeys, _aux->key_names.nKeyAliases, _aux->key_names.nRadioGroups, _aux->key_names.which, &_aux->key_names.valueList);
        xcb_parts[xcb_parts_idx].iov_len = xcb_xkb_get_kbd_by_name_replies_key_names_value_list_serialize(&xcb_parts[xcb_parts_idx].iov_base, _aux->key_names.nTypes, _aux->key_names.nKTLevels, _aux->key_names.indicators, _aux->key_names.virtualMods, _aux->key_names.groupNames, _aux->key_names.nKeys, _aux->key_names.nKeyAliases, _aux->key_names.nRadioGroups, _aux->key_names.which, &_aux->key_names.valueList);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t);
    }
    if(reported & XCB_XKB_GBN_DETAIL_GEOMETRY) {
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometry_type */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.geometry_type;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometryDeviceID */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.geometryDeviceID;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometry_sequence */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.geometry_sequence;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometry_length */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.geometry_length;
        xcb_block_len += sizeof(uint32_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint32_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.name */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.name;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(xcb_atom_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_atom_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometryFound */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.geometryFound;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.pad0 */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &xcb_pad;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.widthMM */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.widthMM;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.heightMM */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.heightMM;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nProperties */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.nProperties;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nColors */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.nColors;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nShapes */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.nShapes;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nSections */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.nSections;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nDoodads */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.nDoodads;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nKeyAliases */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.nKeyAliases;
        xcb_block_len += sizeof(uint16_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint16_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.baseColorNdx */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.baseColorNdx;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.labelColorNdx */
        xcb_parts[xcb_parts_idx].iov_base = (char *) &_aux->geometry.labelColorNdx;
        xcb_block_len += sizeof(uint8_t);
        xcb_parts[xcb_parts_idx].iov_len = sizeof(uint8_t);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
            xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
            xcb_parts_idx++;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* labelFont */
        xcb_parts[xcb_parts_idx].iov_base = (char *) _aux->geometry.labelFont;
        xcb_block_len += xcb_xkb_counted_string_16_sizeof(_aux->geometry.labelFont);
        xcb_parts[xcb_parts_idx].iov_len = xcb_xkb_counted_string_16_sizeof(_aux->geometry.labelFont);
        xcb_parts_idx++;
        xcb_align_to = ALIGNOF(xcb_xkb_counted_string_16_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_parts[xcb_parts_idx].iov_base = xcb_pad0;
        xcb_parts[xcb_parts_idx].iov_len = xcb_pad;
        xcb_parts_idx++;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    if (NULL == xcb_out) {
        /* allocate memory */
        xcb_out = malloc(xcb_buffer_len);
        *_buffer = xcb_out;
    }

    xcb_tmp = xcb_out;
    for(i=0; i<xcb_parts_idx; i++) {
        if (0 != xcb_parts[i].iov_base && 0 != xcb_parts[i].iov_len)
            memcpy(xcb_tmp, xcb_parts[i].iov_base, xcb_parts[i].iov_len);
        if (0 != xcb_parts[i].iov_len)
            xcb_tmp += xcb_parts[i].iov_len;
    }

    return xcb_buffer_len;
}

int
xcb_xkb_get_kbd_by_name_replies_unpack (const void                         *_buffer  /**< */,
                                        uint16_t                            reported  /**< */,
                                        xcb_xkb_get_kbd_by_name_replies_t  *_aux  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    if((reported & XCB_XKB_GBN_DETAIL_TYPES) ||
       (reported & XCB_XKB_GBN_DETAIL_CLIENT_SYMBOLS) ||
       (reported & XCB_XKB_GBN_DETAIL_SERVER_SYMBOLS)) {
        /* xcb_xkb_get_kbd_by_name_replies_t.types.getmap_type */
        _aux->types.getmap_type = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.typeDeviceID */
        _aux->types.typeDeviceID = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.getmap_sequence */
        _aux->types.getmap_sequence = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.getmap_length */
        _aux->types.getmap_length = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.pad0 */
        _aux->types.pad0[0] = *(uint8_t *)xcb_tmp;
        _aux->types.pad0[1] = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t) * 2;
        xcb_tmp += sizeof(uint8_t) * 2;
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.typeMinKeyCode */
        _aux->types.typeMinKeyCode = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.typeMaxKeyCode */
        _aux->types.typeMaxKeyCode = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.present */
        _aux->types.present = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstType */
        _aux->types.firstType = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nTypes */
        _aux->types.nTypes = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalTypes */
        _aux->types.totalTypes = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstKeySym */
        _aux->types.firstKeySym = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalSyms */
        _aux->types.totalSyms = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nKeySyms */
        _aux->types.nKeySyms = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstKeyAction */
        _aux->types.firstKeyAction = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalActions */
        _aux->types.totalActions = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nKeyActions */
        _aux->types.nKeyActions = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstKeyBehavior */
        _aux->types.firstKeyBehavior = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nKeyBehaviors */
        _aux->types.nKeyBehaviors = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalKeyBehaviors */
        _aux->types.totalKeyBehaviors = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstKeyExplicit */
        _aux->types.firstKeyExplicit = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nKeyExplicit */
        _aux->types.nKeyExplicit = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalKeyExplicit */
        _aux->types.totalKeyExplicit = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstModMapKey */
        _aux->types.firstModMapKey = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nModMapKeys */
        _aux->types.nModMapKeys = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalModMapKeys */
        _aux->types.totalModMapKeys = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.firstVModMapKey */
        _aux->types.firstVModMapKey = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.nVModMapKeys */
        _aux->types.nVModMapKeys = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.totalVModMapKeys */
        _aux->types.totalVModMapKeys = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.pad1 */
        _aux->types.pad1 = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.types.virtualMods */
        _aux->types.virtualMods = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* map */
        xcb_block_len += xcb_xkb_get_kbd_by_name_replies_types_map_unpack(xcb_tmp, _aux->types.nTypes, _aux->types.nKeySyms, _aux->types.nKeyActions, _aux->types.totalActions, _aux->types.totalKeyBehaviors, _aux->types.virtualMods, _aux->types.totalKeyExplicit, _aux->types.totalModMapKeys, _aux->types.totalVModMapKeys, _aux->types.present, &_aux->types.map);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_get_kbd_by_name_replies_types_map_t);
    }
    if(reported & XCB_XKB_GBN_DETAIL_COMPAT_MAP) {
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.compatmap_type */
        _aux->compat_map.compatmap_type = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.compatDeviceID */
        _aux->compat_map.compatDeviceID = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.compatmap_sequence */
        _aux->compat_map.compatmap_sequence = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.compatmap_length */
        _aux->compat_map.compatmap_length = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.groupsRtrn */
        _aux->compat_map.groupsRtrn = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.pad0 */
        _aux->compat_map.pad0 = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.firstSIRtrn */
        _aux->compat_map.firstSIRtrn = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.nSIRtrn */
        _aux->compat_map.nSIRtrn = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.nTotalSI */
        _aux->compat_map.nTotalSI = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.compat_map.pad1 */
        _aux->compat_map.pad1[0] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[1] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[2] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[3] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[4] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[5] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[6] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[7] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[8] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[9] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[10] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[11] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[12] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[13] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[14] = *(uint8_t *)xcb_tmp;
        _aux->compat_map.pad1[15] = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t) * 16;
        xcb_tmp += sizeof(uint8_t) * 16;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* si_rtrn */
        _aux->compat_map.si_rtrn = (xcb_xkb_sym_interpret_t *)xcb_tmp;
        xcb_block_len += _aux->compat_map.nSIRtrn * sizeof(xcb_xkb_sym_interpret_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_sym_interpret_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* group_rtrn */
        _aux->compat_map.group_rtrn = (xcb_xkb_mod_def_t *)xcb_tmp;
        xcb_block_len += xcb_popcount(_aux->compat_map.groupsRtrn) * sizeof(xcb_xkb_mod_def_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_mod_def_t);
    }
    if(reported & XCB_XKB_GBN_DETAIL_INDICATOR_MAPS) {
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.indicatormap_type */
        _aux->indicator_maps.indicatormap_type = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.indicatorDeviceID */
        _aux->indicator_maps.indicatorDeviceID = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.indicatormap_sequence */
        _aux->indicator_maps.indicatormap_sequence = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.indicatormap_length */
        _aux->indicator_maps.indicatormap_length = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.which */
        _aux->indicator_maps.which = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.realIndicators */
        _aux->indicator_maps.realIndicators = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.nIndicators */
        _aux->indicator_maps.nIndicators = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.indicator_maps.pad0 */
        _aux->indicator_maps.pad0[0] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[1] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[2] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[3] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[4] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[5] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[6] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[7] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[8] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[9] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[10] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[11] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[12] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[13] = *(uint8_t *)xcb_tmp;
        _aux->indicator_maps.pad0[14] = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t) * 15;
        xcb_tmp += sizeof(uint8_t) * 15;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* maps */
        _aux->indicator_maps.maps = (xcb_xkb_indicator_map_t *)xcb_tmp;
        xcb_block_len += _aux->indicator_maps.nIndicators * sizeof(xcb_xkb_indicator_map_t);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_indicator_map_t);
    }
    if((reported & XCB_XKB_GBN_DETAIL_KEY_NAMES) ||
       (reported & XCB_XKB_GBN_DETAIL_OTHER_NAMES)) {
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyname_type */
        _aux->key_names.keyname_type = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyDeviceID */
        _aux->key_names.keyDeviceID = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyname_sequence */
        _aux->key_names.keyname_sequence = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyname_length */
        _aux->key_names.keyname_length = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.which */
        _aux->key_names.which = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyMinKeyCode */
        _aux->key_names.keyMinKeyCode = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.keyMaxKeyCode */
        _aux->key_names.keyMaxKeyCode = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nTypes */
        _aux->key_names.nTypes = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.groupNames */
        _aux->key_names.groupNames = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.virtualMods */
        _aux->key_names.virtualMods = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.firstKey */
        _aux->key_names.firstKey = *(xcb_keycode_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_keycode_t);
        xcb_tmp += sizeof(xcb_keycode_t);
        xcb_align_to = ALIGNOF(xcb_keycode_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nKeys */
        _aux->key_names.nKeys = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.indicators */
        _aux->key_names.indicators = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nRadioGroups */
        _aux->key_names.nRadioGroups = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nKeyAliases */
        _aux->key_names.nKeyAliases = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.nKTLevels */
        _aux->key_names.nKTLevels = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.key_names.pad0 */
        _aux->key_names.pad0[0] = *(uint8_t *)xcb_tmp;
        _aux->key_names.pad0[1] = *(uint8_t *)xcb_tmp;
        _aux->key_names.pad0[2] = *(uint8_t *)xcb_tmp;
        _aux->key_names.pad0[3] = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t) * 4;
        xcb_tmp += sizeof(uint8_t) * 4;
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* valueList */
        xcb_block_len += xcb_xkb_get_kbd_by_name_replies_key_names_value_list_unpack(xcb_tmp, _aux->key_names.nTypes, _aux->key_names.nKTLevels, _aux->key_names.indicators, _aux->key_names.virtualMods, _aux->key_names.groupNames, _aux->key_names.nKeys, _aux->key_names.nKeyAliases, _aux->key_names.nRadioGroups, _aux->key_names.which, &_aux->key_names.valueList);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_get_kbd_by_name_replies_key_names_value_list_t);
    }
    if(reported & XCB_XKB_GBN_DETAIL_GEOMETRY) {
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometry_type */
        _aux->geometry.geometry_type = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometryDeviceID */
        _aux->geometry.geometryDeviceID = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometry_sequence */
        _aux->geometry.geometry_sequence = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometry_length */
        _aux->geometry.geometry_length = *(uint32_t *)xcb_tmp;
        xcb_block_len += sizeof(uint32_t);
        xcb_tmp += sizeof(uint32_t);
        xcb_align_to = ALIGNOF(uint32_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.name */
        _aux->geometry.name = *(xcb_atom_t *)xcb_tmp;
        xcb_block_len += sizeof(xcb_atom_t);
        xcb_tmp += sizeof(xcb_atom_t);
        xcb_align_to = ALIGNOF(xcb_atom_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.geometryFound */
        _aux->geometry.geometryFound = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.pad0 */
        _aux->geometry.pad0 = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.widthMM */
        _aux->geometry.widthMM = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.heightMM */
        _aux->geometry.heightMM = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nProperties */
        _aux->geometry.nProperties = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nColors */
        _aux->geometry.nColors = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nShapes */
        _aux->geometry.nShapes = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nSections */
        _aux->geometry.nSections = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nDoodads */
        _aux->geometry.nDoodads = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.nKeyAliases */
        _aux->geometry.nKeyAliases = *(uint16_t *)xcb_tmp;
        xcb_block_len += sizeof(uint16_t);
        xcb_tmp += sizeof(uint16_t);
        xcb_align_to = ALIGNOF(uint16_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.baseColorNdx */
        _aux->geometry.baseColorNdx = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* xcb_xkb_get_kbd_by_name_replies_t.geometry.labelColorNdx */
        _aux->geometry.labelColorNdx = *(uint8_t *)xcb_tmp;
        xcb_block_len += sizeof(uint8_t);
        xcb_tmp += sizeof(uint8_t);
        xcb_align_to = ALIGNOF(uint8_t);
        /* insert padding */
        xcb_pad = -xcb_block_len & (xcb_align_to - 1);
        xcb_buffer_len += xcb_block_len + xcb_pad;
        if (0 != xcb_pad) {
            xcb_tmp += xcb_pad;
            xcb_pad = 0;
        }
        xcb_block_len = 0;
        /* labelFont */
        _aux->geometry.labelFont = (xcb_xkb_counted_string_16_t *)xcb_tmp;
        xcb_block_len += xcb_xkb_counted_string_16_sizeof(xcb_tmp);
        xcb_tmp += xcb_block_len;
        xcb_align_to = ALIGNOF(xcb_xkb_counted_string_16_t);
    }
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}

int
xcb_xkb_get_kbd_by_name_replies_sizeof (const void  *_buffer  /**< */,
                                        uint16_t     reported  /**< */)
{
    xcb_xkb_get_kbd_by_name_replies_t _aux;
    return xcb_xkb_get_kbd_by_name_replies_unpack(_buffer, reported, &_aux);
}


/*****************************************************************************
 **
 ** xcb_xkb_get_kbd_by_name_cookie_t xcb_xkb_get_kbd_by_name
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               need
 ** @param uint16_t               want
 ** @param uint8_t                load
 ** @returns xcb_xkb_get_kbd_by_name_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_kbd_by_name_cookie_t
xcb_xkb_get_kbd_by_name (xcb_connection_t      *c  /**< */,
                         xcb_xkb_device_spec_t  deviceSpec  /**< */,
                         uint16_t               need  /**< */,
                         uint16_t               want  /**< */,
                         uint8_t                load  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_KBD_BY_NAME,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_kbd_by_name_cookie_t xcb_ret;
    xcb_xkb_get_kbd_by_name_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.need = need;
    xcb_out.want = want;
    xcb_out.load = load;
    xcb_out.pad0 = 0;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_kbd_by_name_cookie_t xcb_xkb_get_kbd_by_name_unchecked
 ** 
 ** @param xcb_connection_t      *c
 ** @param xcb_xkb_device_spec_t  deviceSpec
 ** @param uint16_t               need
 ** @param uint16_t               want
 ** @param uint8_t                load
 ** @returns xcb_xkb_get_kbd_by_name_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_kbd_by_name_cookie_t
xcb_xkb_get_kbd_by_name_unchecked (xcb_connection_t      *c  /**< */,
                                   xcb_xkb_device_spec_t  deviceSpec  /**< */,
                                   uint16_t               need  /**< */,
                                   uint16_t               want  /**< */,
                                   uint8_t                load  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_KBD_BY_NAME,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_kbd_by_name_cookie_t xcb_ret;
    xcb_xkb_get_kbd_by_name_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.need = need;
    xcb_out.want = want;
    xcb_out.load = load;
    xcb_out.pad0 = 0;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_kbd_by_name_replies_t * xcb_xkb_get_kbd_by_name_replies
 ** 
 ** @param const xcb_xkb_get_kbd_by_name_reply_t *R
 ** @returns xcb_xkb_get_kbd_by_name_replies_t *
 **
 *****************************************************************************/
 
void *
xcb_xkb_get_kbd_by_name_replies (const xcb_xkb_get_kbd_by_name_reply_t *R  /**< */)
{
    return (void *) (R + 1);
}


/*****************************************************************************
 **
 ** xcb_xkb_get_kbd_by_name_reply_t * xcb_xkb_get_kbd_by_name_reply
 ** 
 ** @param xcb_connection_t                  *c
 ** @param xcb_xkb_get_kbd_by_name_cookie_t   cookie
 ** @param xcb_generic_error_t              **e
 ** @returns xcb_xkb_get_kbd_by_name_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_kbd_by_name_reply_t *
xcb_xkb_get_kbd_by_name_reply (xcb_connection_t                  *c  /**< */,
                               xcb_xkb_get_kbd_by_name_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e  /**< */)
{
    return (xcb_xkb_get_kbd_by_name_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xkb_get_device_info_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_get_device_info_reply_t *_aux = (xcb_xkb_get_device_info_reply_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;

    unsigned int i;
    unsigned int xcb_tmp_len;

    xcb_block_len += sizeof(xcb_xkb_get_device_info_reply_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* name */
    xcb_block_len += _aux->nameLen * sizeof(xcb_xkb_string8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_string8_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* btnActions */
    xcb_block_len += _aux->nBtnsRtrn * sizeof(xcb_xkb_action_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_action_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* leds */
    for(i=0; i<_aux->nDeviceLedFBs; i++) {
        xcb_tmp_len = xcb_xkb_device_led_info_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_device_led_info_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_device_info_cookie_t xcb_xkb_get_device_info
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_xkb_device_spec_t     deviceSpec
 ** @param uint16_t                  wanted
 ** @param uint8_t                   allButtons
 ** @param uint8_t                   firstButton
 ** @param uint8_t                   nButtons
 ** @param xcb_xkb_led_class_spec_t  ledClass
 ** @param xcb_xkb_id_spec_t         ledID
 ** @returns xcb_xkb_get_device_info_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_device_info_cookie_t
xcb_xkb_get_device_info (xcb_connection_t         *c  /**< */,
                         xcb_xkb_device_spec_t     deviceSpec  /**< */,
                         uint16_t                  wanted  /**< */,
                         uint8_t                   allButtons  /**< */,
                         uint8_t                   firstButton  /**< */,
                         uint8_t                   nButtons  /**< */,
                         xcb_xkb_led_class_spec_t  ledClass  /**< */,
                         xcb_xkb_id_spec_t         ledID  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_DEVICE_INFO,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_device_info_cookie_t xcb_ret;
    xcb_xkb_get_device_info_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.wanted = wanted;
    xcb_out.allButtons = allButtons;
    xcb_out.firstButton = firstButton;
    xcb_out.nButtons = nButtons;
    xcb_out.pad0 = 0;
    xcb_out.ledClass = ledClass;
    xcb_out.ledID = ledID;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_device_info_cookie_t xcb_xkb_get_device_info_unchecked
 ** 
 ** @param xcb_connection_t         *c
 ** @param xcb_xkb_device_spec_t     deviceSpec
 ** @param uint16_t                  wanted
 ** @param uint8_t                   allButtons
 ** @param uint8_t                   firstButton
 ** @param uint8_t                   nButtons
 ** @param xcb_xkb_led_class_spec_t  ledClass
 ** @param xcb_xkb_id_spec_t         ledID
 ** @returns xcb_xkb_get_device_info_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_get_device_info_cookie_t
xcb_xkb_get_device_info_unchecked (xcb_connection_t         *c  /**< */,
                                   xcb_xkb_device_spec_t     deviceSpec  /**< */,
                                   uint16_t                  wanted  /**< */,
                                   uint8_t                   allButtons  /**< */,
                                   uint8_t                   firstButton  /**< */,
                                   uint8_t                   nButtons  /**< */,
                                   xcb_xkb_led_class_spec_t  ledClass  /**< */,
                                   xcb_xkb_id_spec_t         ledID  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 2,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_GET_DEVICE_INFO,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[4];
    xcb_xkb_get_device_info_cookie_t xcb_ret;
    xcb_xkb_get_device_info_request_t xcb_out;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.wanted = wanted;
    xcb_out.allButtons = allButtons;
    xcb_out.firstButton = firstButton;
    xcb_out.nButtons = nButtons;
    xcb_out.pad0 = 0;
    xcb_out.ledClass = ledClass;
    xcb_out.ledID = ledID;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_string8_t * xcb_xkb_get_device_info_name
 ** 
 ** @param const xcb_xkb_get_device_info_reply_t *R
 ** @returns xcb_xkb_string8_t *
 **
 *****************************************************************************/
 
xcb_xkb_string8_t *
xcb_xkb_get_device_info_name (const xcb_xkb_get_device_info_reply_t *R  /**< */)
{
    return (xcb_xkb_string8_t *) (R + 1);
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_device_info_name_length
 ** 
 ** @param const xcb_xkb_get_device_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_device_info_name_length (const xcb_xkb_get_device_info_reply_t *R  /**< */)
{
    return R->nameLen;
}


/*****************************************************************************
 **
 ** xcb_generic_iterator_t xcb_xkb_get_device_info_name_end
 ** 
 ** @param const xcb_xkb_get_device_info_reply_t *R
 ** @returns xcb_generic_iterator_t
 **
 *****************************************************************************/
 
xcb_generic_iterator_t
xcb_xkb_get_device_info_name_end (const xcb_xkb_get_device_info_reply_t *R  /**< */)
{
    xcb_generic_iterator_t i;
    i.data = ((xcb_xkb_string8_t *) (R + 1)) + (R->nameLen);
    i.rem = 0;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_action_t * xcb_xkb_get_device_info_btn_actions
 ** 
 ** @param const xcb_xkb_get_device_info_reply_t *R
 ** @returns xcb_xkb_action_t *
 **
 *****************************************************************************/
 
xcb_xkb_action_t *
xcb_xkb_get_device_info_btn_actions (const xcb_xkb_get_device_info_reply_t *R  /**< */)
{
    xcb_generic_iterator_t prev = xcb_xkb_get_device_info_name_end(R);
    return (xcb_xkb_action_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_action_t, prev.index) + 0);
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_device_info_btn_actions_length
 ** 
 ** @param const xcb_xkb_get_device_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_device_info_btn_actions_length (const xcb_xkb_get_device_info_reply_t *R  /**< */)
{
    return R->nBtnsRtrn;
}


/*****************************************************************************
 **
 ** xcb_xkb_action_iterator_t xcb_xkb_get_device_info_btn_actions_iterator
 ** 
 ** @param const xcb_xkb_get_device_info_reply_t *R
 ** @returns xcb_xkb_action_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_action_iterator_t
xcb_xkb_get_device_info_btn_actions_iterator (const xcb_xkb_get_device_info_reply_t *R  /**< */)
{
    xcb_xkb_action_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_get_device_info_name_end(R);
    i.data = (xcb_xkb_action_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_action_t, prev.index));
    i.rem = R->nBtnsRtrn;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** int xcb_xkb_get_device_info_leds_length
 ** 
 ** @param const xcb_xkb_get_device_info_reply_t *R
 ** @returns int
 **
 *****************************************************************************/
 
int
xcb_xkb_get_device_info_leds_length (const xcb_xkb_get_device_info_reply_t *R  /**< */)
{
    return R->nDeviceLedFBs;
}


/*****************************************************************************
 **
 ** xcb_xkb_device_led_info_iterator_t xcb_xkb_get_device_info_leds_iterator
 ** 
 ** @param const xcb_xkb_get_device_info_reply_t *R
 ** @returns xcb_xkb_device_led_info_iterator_t
 **
 *****************************************************************************/
 
xcb_xkb_device_led_info_iterator_t
xcb_xkb_get_device_info_leds_iterator (const xcb_xkb_get_device_info_reply_t *R  /**< */)
{
    xcb_xkb_device_led_info_iterator_t i;
    xcb_generic_iterator_t prev = xcb_xkb_action_end(xcb_xkb_get_device_info_btn_actions_iterator(R));
    i.data = (xcb_xkb_device_led_info_t *) ((char *) prev.data + XCB_TYPE_PAD(xcb_xkb_device_led_info_t, prev.index));
    i.rem = R->nDeviceLedFBs;
    i.index = (char *) i.data - (char *) R;
    return i;
}


/*****************************************************************************
 **
 ** xcb_xkb_get_device_info_reply_t * xcb_xkb_get_device_info_reply
 ** 
 ** @param xcb_connection_t                  *c
 ** @param xcb_xkb_get_device_info_cookie_t   cookie
 ** @param xcb_generic_error_t              **e
 ** @returns xcb_xkb_get_device_info_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_get_device_info_reply_t *
xcb_xkb_get_device_info_reply (xcb_connection_t                  *c  /**< */,
                               xcb_xkb_get_device_info_cookie_t   cookie  /**< */,
                               xcb_generic_error_t              **e  /**< */)
{
    return (xcb_xkb_get_device_info_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

int
xcb_xkb_set_device_info_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_set_device_info_request_t *_aux = (xcb_xkb_set_device_info_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;

    unsigned int i;
    unsigned int xcb_tmp_len;

    xcb_block_len += sizeof(xcb_xkb_set_device_info_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* btnActions */
    xcb_block_len += _aux->nBtns * sizeof(xcb_xkb_action_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_action_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;
    /* leds */
    for(i=0; i<_aux->nDeviceLedFBs; i++) {
        xcb_tmp_len = xcb_xkb_device_led_info_sizeof(xcb_tmp);
        xcb_block_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_align_to = ALIGNOF(xcb_xkb_device_led_info_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_device_info_checked
 ** 
 ** @param xcb_connection_t                *c
 ** @param xcb_xkb_device_spec_t            deviceSpec
 ** @param uint8_t                          firstBtn
 ** @param uint8_t                          nBtns
 ** @param uint16_t                         change
 ** @param uint16_t                         nDeviceLedFBs
 ** @param const xcb_xkb_action_t          *btnActions
 ** @param const xcb_xkb_device_led_info_t *leds
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_device_info_checked (xcb_connection_t                *c  /**< */,
                                 xcb_xkb_device_spec_t            deviceSpec  /**< */,
                                 uint8_t                          firstBtn  /**< */,
                                 uint8_t                          nBtns  /**< */,
                                 uint16_t                         change  /**< */,
                                 uint16_t                         nDeviceLedFBs  /**< */,
                                 const xcb_xkb_action_t          *btnActions  /**< */,
                                 const xcb_xkb_device_led_info_t *leds  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 6,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_DEVICE_INFO,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[8];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_device_info_request_t xcb_out;
    unsigned int i;
    unsigned int xcb_tmp_len;
    char *xcb_tmp;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.firstBtn = firstBtn;
    xcb_out.nBtns = nBtns;
    xcb_out.change = change;
    xcb_out.nDeviceLedFBs = nDeviceLedFBs;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_action_t btnActions */
    xcb_parts[4].iov_base = (char *) btnActions;
    xcb_parts[4].iov_len = nBtns * sizeof(xcb_xkb_action_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    /* xcb_xkb_device_led_info_t leds */
    xcb_parts[6].iov_base = (char *) leds;
    xcb_parts[6].iov_len = 0;
    xcb_tmp = (char *)leds;
    for(i=0; i<nDeviceLedFBs; i++) {
        xcb_tmp_len = xcb_xkb_device_led_info_sizeof(xcb_tmp);
        xcb_parts[6].iov_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_parts[7].iov_base = 0;
    xcb_parts[7].iov_len = -xcb_parts[6].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_void_cookie_t xcb_xkb_set_device_info
 ** 
 ** @param xcb_connection_t                *c
 ** @param xcb_xkb_device_spec_t            deviceSpec
 ** @param uint8_t                          firstBtn
 ** @param uint8_t                          nBtns
 ** @param uint16_t                         change
 ** @param uint16_t                         nDeviceLedFBs
 ** @param const xcb_xkb_action_t          *btnActions
 ** @param const xcb_xkb_device_led_info_t *leds
 ** @returns xcb_void_cookie_t
 **
 *****************************************************************************/
 
xcb_void_cookie_t
xcb_xkb_set_device_info (xcb_connection_t                *c  /**< */,
                         xcb_xkb_device_spec_t            deviceSpec  /**< */,
                         uint8_t                          firstBtn  /**< */,
                         uint8_t                          nBtns  /**< */,
                         uint16_t                         change  /**< */,
                         uint16_t                         nDeviceLedFBs  /**< */,
                         const xcb_xkb_action_t          *btnActions  /**< */,
                         const xcb_xkb_device_led_info_t *leds  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 6,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_DEVICE_INFO,
        /* isvoid */ 1
    };
    
    struct iovec xcb_parts[8];
    xcb_void_cookie_t xcb_ret;
    xcb_xkb_set_device_info_request_t xcb_out;
    unsigned int i;
    unsigned int xcb_tmp_len;
    char *xcb_tmp;
    
    xcb_out.deviceSpec = deviceSpec;
    xcb_out.firstBtn = firstBtn;
    xcb_out.nBtns = nBtns;
    xcb_out.change = change;
    xcb_out.nDeviceLedFBs = nDeviceLedFBs;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_action_t btnActions */
    xcb_parts[4].iov_base = (char *) btnActions;
    xcb_parts[4].iov_len = nBtns * sizeof(xcb_xkb_action_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    /* xcb_xkb_device_led_info_t leds */
    xcb_parts[6].iov_base = (char *) leds;
    xcb_parts[6].iov_len = 0;
    xcb_tmp = (char *)leds;
    for(i=0; i<nDeviceLedFBs; i++) {
        xcb_tmp_len = xcb_xkb_device_led_info_sizeof(xcb_tmp);
        xcb_parts[6].iov_len += xcb_tmp_len;
        xcb_tmp += xcb_tmp_len;
    }
    xcb_parts[7].iov_base = 0;
    xcb_parts[7].iov_len = -xcb_parts[6].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}

int
xcb_xkb_set_debugging_flags_sizeof (const void  *_buffer  /**< */)
{
    char *xcb_tmp = (char *)_buffer;
    const xcb_xkb_set_debugging_flags_request_t *_aux = (xcb_xkb_set_debugging_flags_request_t *)_buffer;
    unsigned int xcb_buffer_len = 0;
    unsigned int xcb_block_len = 0;
    unsigned int xcb_pad = 0;
    unsigned int xcb_align_to = 0;


    xcb_block_len += sizeof(xcb_xkb_set_debugging_flags_request_t);
    xcb_tmp += xcb_block_len;
    xcb_buffer_len += xcb_block_len;
    xcb_block_len = 0;
    /* message */
    xcb_block_len += _aux->msgLength * sizeof(xcb_xkb_string8_t);
    xcb_tmp += xcb_block_len;
    xcb_align_to = ALIGNOF(xcb_xkb_string8_t);
    /* insert padding */
    xcb_pad = -xcb_block_len & (xcb_align_to - 1);
    xcb_buffer_len += xcb_block_len + xcb_pad;
    if (0 != xcb_pad) {
        xcb_tmp += xcb_pad;
        xcb_pad = 0;
    }
    xcb_block_len = 0;

    return xcb_buffer_len;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_debugging_flags_cookie_t xcb_xkb_set_debugging_flags
 ** 
 ** @param xcb_connection_t        *c
 ** @param uint16_t                 msgLength
 ** @param uint32_t                 affectFlags
 ** @param uint32_t                 flags
 ** @param uint32_t                 affectCtrls
 ** @param uint32_t                 ctrls
 ** @param const xcb_xkb_string8_t *message
 ** @returns xcb_xkb_set_debugging_flags_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_set_debugging_flags_cookie_t
xcb_xkb_set_debugging_flags (xcb_connection_t        *c  /**< */,
                             uint16_t                 msgLength  /**< */,
                             uint32_t                 affectFlags  /**< */,
                             uint32_t                 flags  /**< */,
                             uint32_t                 affectCtrls  /**< */,
                             uint32_t                 ctrls  /**< */,
                             const xcb_xkb_string8_t *message  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_DEBUGGING_FLAGS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[6];
    xcb_xkb_set_debugging_flags_cookie_t xcb_ret;
    xcb_xkb_set_debugging_flags_request_t xcb_out;
    
    xcb_out.msgLength = msgLength;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.affectFlags = affectFlags;
    xcb_out.flags = flags;
    xcb_out.affectCtrls = affectCtrls;
    xcb_out.ctrls = ctrls;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_string8_t message */
    xcb_parts[4].iov_base = (char *) message;
    xcb_parts[4].iov_len = msgLength * sizeof(xcb_xkb_string8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, XCB_REQUEST_CHECKED, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_debugging_flags_cookie_t xcb_xkb_set_debugging_flags_unchecked
 ** 
 ** @param xcb_connection_t        *c
 ** @param uint16_t                 msgLength
 ** @param uint32_t                 affectFlags
 ** @param uint32_t                 flags
 ** @param uint32_t                 affectCtrls
 ** @param uint32_t                 ctrls
 ** @param const xcb_xkb_string8_t *message
 ** @returns xcb_xkb_set_debugging_flags_cookie_t
 **
 *****************************************************************************/
 
xcb_xkb_set_debugging_flags_cookie_t
xcb_xkb_set_debugging_flags_unchecked (xcb_connection_t        *c  /**< */,
                                       uint16_t                 msgLength  /**< */,
                                       uint32_t                 affectFlags  /**< */,
                                       uint32_t                 flags  /**< */,
                                       uint32_t                 affectCtrls  /**< */,
                                       uint32_t                 ctrls  /**< */,
                                       const xcb_xkb_string8_t *message  /**< */)
{
    static const xcb_protocol_request_t xcb_req = {
        /* count */ 4,
        /* ext */ &xcb_xkb_id,
        /* opcode */ XCB_XKB_SET_DEBUGGING_FLAGS,
        /* isvoid */ 0
    };
    
    struct iovec xcb_parts[6];
    xcb_xkb_set_debugging_flags_cookie_t xcb_ret;
    xcb_xkb_set_debugging_flags_request_t xcb_out;
    
    xcb_out.msgLength = msgLength;
    memset(xcb_out.pad0, 0, 2);
    xcb_out.affectFlags = affectFlags;
    xcb_out.flags = flags;
    xcb_out.affectCtrls = affectCtrls;
    xcb_out.ctrls = ctrls;
    
    xcb_parts[2].iov_base = (char *) &xcb_out;
    xcb_parts[2].iov_len = sizeof(xcb_out);
    xcb_parts[3].iov_base = 0;
    xcb_parts[3].iov_len = -xcb_parts[2].iov_len & 3;
    /* xcb_xkb_string8_t message */
    xcb_parts[4].iov_base = (char *) message;
    xcb_parts[4].iov_len = msgLength * sizeof(xcb_xkb_string8_t);
    xcb_parts[5].iov_base = 0;
    xcb_parts[5].iov_len = -xcb_parts[4].iov_len & 3;
    
    xcb_ret.sequence = xcb_send_request(c, 0, xcb_parts + 2, &xcb_req);
    return xcb_ret;
}


/*****************************************************************************
 **
 ** xcb_xkb_set_debugging_flags_reply_t * xcb_xkb_set_debugging_flags_reply
 ** 
 ** @param xcb_connection_t                      *c
 ** @param xcb_xkb_set_debugging_flags_cookie_t   cookie
 ** @param xcb_generic_error_t                  **e
 ** @returns xcb_xkb_set_debugging_flags_reply_t *
 **
 *****************************************************************************/
 
xcb_xkb_set_debugging_flags_reply_t *
xcb_xkb_set_debugging_flags_reply (xcb_connection_t                      *c  /**< */,
                                   xcb_xkb_set_debugging_flags_cookie_t   cookie  /**< */,
                                   xcb_generic_error_t                  **e  /**< */)
{
    return (xcb_xkb_set_debugging_flags_reply_t *) xcb_wait_for_reply(c, cookie.sequence, e);
}

