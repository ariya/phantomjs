/*
    Copyright (C) 2011 ProFUSION embedded systems

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

/**
 * @file    ewk_js.h
 * @brief   Allows to export objects to JavaScript API.
 */

#ifndef ewk_js_h
#define ewk_js_h

#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EWK_JS_CLASS_META_VERSION 0

typedef struct _Ewk_JS_Object Ewk_JS_Object;
typedef struct _Ewk_JS_Class_Meta Ewk_JS_Class_Meta;
typedef struct _Ewk_JS_Variant Ewk_JS_Variant;
typedef struct _Ewk_JS_Method Ewk_JS_Method;
typedef struct _Ewk_JS_Property Ewk_JS_Property;
typedef struct _Ewk_JS_Default Ewk_JS_Default;
typedef Eina_Bool (*Ewk_JS_Set_Cb)(Ewk_JS_Object *obj, const char *name, const Ewk_JS_Variant *value);
typedef Eina_Bool (*Ewk_JS_Get_Cb)(Ewk_JS_Object *obj, const char *name, Ewk_JS_Variant *value);
typedef Eina_Bool (*Ewk_JS_Del_Cb)(Ewk_JS_Object *obj, const char *name);
typedef Ewk_JS_Variant* (*Ewk_JS_Invoke_Cb)(Ewk_JS_Object *obj, Ewk_JS_Variant *args, int argCount);

typedef enum {
    EWK_JS_VARIANT_VOID,
    EWK_JS_VARIANT_NULL,
    EWK_JS_VARIANT_BOOL,
    EWK_JS_VARIANT_INT32,
    EWK_JS_VARIANT_DOUBLE,
    EWK_JS_VARIANT_STRING,
    EWK_JS_VARIANT_OBJECT
} Ewk_JS_Variant_Type;

typedef enum {
    EWK_JS_OBJECT_OBJECT,
    EWK_JS_OBJECT_ARRAY,
    EWK_JS_OBJECT_FUNCTION,
    EWK_JS_OBJECT_INVALID
} Ewk_JS_Object_Type;

struct _Ewk_JS_Variant {
    Ewk_JS_Variant_Type type;
    union {
        Eina_Bool b;
        int i;
        double d;
        const char *s;
        Ewk_JS_Object *o;
    } value;
};

struct _Ewk_JS_Method {
    const char *name;
    Ewk_JS_Invoke_Cb invoke;
};

struct _Ewk_JS_Property {
    const char *name;
    Ewk_JS_Set_Cb set;
    Ewk_JS_Get_Cb get;
    Ewk_JS_Del_Cb del;
    Ewk_JS_Variant value;
};

struct _Ewk_JS_Default {
    Ewk_JS_Set_Cb set;
    Ewk_JS_Get_Cb get;
    Ewk_JS_Del_Cb del;
};

struct _Ewk_JS_Class_Meta {
    unsigned int version; // define
    const Ewk_JS_Method *methods; // null terminated array
    const Ewk_JS_Property *properties; // null terminated array
    Ewk_JS_Default default_prop;
};

/**
 * Gets Eina_Hash with object's properties.
 *
 * @param obj Object whose properties are wanted.
 *
 * @return object's properties.
 */
EAPI Eina_Hash *ewk_js_object_properties_get(const Ewk_JS_Object *obj);

/**
 * Gets name of the object.
 *
 * @param obj Object whose name is wanted.
 *
 * @return name of object.
 */

EAPI const char *ewk_js_object_name_get(const Ewk_JS_Object *obj);

/**
 * Returns the view associated with an Ewk_JS_Object.
 *
 * The returned view is the one passed to ewk_view_js_object_add. Right now,
 * the object is always added to the view's main frame.
 *
 * @param obj The object to be queried.
 *
 * @return The view whose main frame the object has been inserted into, or
 *         @c NULL if the object has not been added to a view yet.
 *
 * @sa ewk_view_js_object_add, ewk_view_frame_main_get
 */
EAPI Evas_Object *ewk_js_object_view_get(const Ewk_JS_Object *obj);

/**
 * Release resources allocated by @a var.
 *
 * @param var @a Ewk_JS_Variant to be release
 */
EAPI void ewk_js_variant_free(Ewk_JS_Variant *var);

/**
 * Release resources allocated by @a var.
 *
 * @param var @a Ewk_JS_Variant to be release
 * @param count @a size of array
 */
EAPI void ewk_js_variant_array_free(Ewk_JS_Variant *var, int count);

/**
 * Create a Ewk_JS_Object to be used in @a ewk_view_js_object_add. The Meta class's
 * methods and properties are not modified but references to it are kept as long
 * as the object created from it lives. All properties created here
 * will be added to the object hash of properties. Properties using default_prop's
 * get/set/del methods should also be added to the objects hash(see:
 * @a ewk_js_object_properties_get). Methods must free the arguments they receive(see:
 * @a ewk_js_variang_array_free).
 *
 *
 * @param cls @a Ewk_JS_Class that describes the object to be created.
 *
 * @return The Ewk_JS_Object created.
 */
EAPI Ewk_JS_Object *ewk_js_object_new(const Ewk_JS_Class_Meta *meta_cls);

/**
 * Release resources allocated by @a obj.
 *
 * @param obj @a Ewk_JS_Object to be released.
 */
EAPI void ewk_js_object_free(Ewk_JS_Object *obj);

/**
 * Calls the function this object represents.
 *
 * @param obj Object that represents function.
 * @param args Arguments to be passed to function.
 * @param arg_count Number of arguments.
 * @param result Return value of the invoked function.
 *
 * @return @c EINA_TRUE if function was executed, @c EINA_FALSE if function was not executed.
 */
EAPI Eina_Bool ewk_js_object_invoke(Ewk_JS_Object *obj, Ewk_JS_Variant *args, int arg_count, Ewk_JS_Variant *result);

/**
 * Returns the type this object represents.
 *
 * @param obj Object
 *
 * @return @c EWK_JS_OBJECT if it is an object, @c EWK_JS_ARRAY if it is an array and
 * @c EWK_JS_FUNCTION if it is a function.
 */
EAPI Ewk_JS_Object_Type ewk_js_object_type_get(Ewk_JS_Object *obj);

/**
 * Sets the type this object represents.
 *
 * @param obj Object
 * @param type Type
 *
 */
EAPI void ewk_js_object_type_set(Ewk_JS_Object *obj, Ewk_JS_Object_Type type);

#ifdef __cplusplus
}
#endif

#endif // ewk_js_h
