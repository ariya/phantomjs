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

#include "config.h"
#include "ewk_js.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "NP_jsobject.h"
#include "Operations.h"
#include "ewk_js_private.h"
#include "ewk_private.h"
#include "npruntime.h"
#include "npruntime_impl.h"
#include <string.h>

#define EINA_MAGIC_CHECK_OR_RETURN(o, ...) \
    if (!EINA_MAGIC_CHECK(o, EWK_JS_OBJECT_MAGIC)) { \
        EINA_MAGIC_FAIL(o, EWK_JS_OBJECT_MAGIC); \
        return __VA_ARGS__; \
    }

struct _Ewk_JS_Class {
    const Ewk_JS_Class_Meta* meta;
    Eina_Hash* methods; // Key=NPIdentifier(name), value=pointer to meta->methods.
    Eina_Hash* properties; // Key=NPIdentifier(name), value=pointer to meta->properties.
    Ewk_JS_Default default_prop;
};

static Eina_Bool ewk_js_npvariant_to_variant(Ewk_JS_Variant* data, const NPVariant* result);

static Eina_Bool ewk_js_variant_to_npvariant(const Ewk_JS_Variant* data, NPVariant* result)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(data, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(result, false);
    const char* string_value;

    switch (data->type) {
    case EWK_JS_VARIANT_VOID:
        VOID_TO_NPVARIANT(*result);
        break;
    case EWK_JS_VARIANT_NULL:
        NULL_TO_NPVARIANT(*result);
        break;
    case EWK_JS_VARIANT_INT32:
        INT32_TO_NPVARIANT(data->value.i, *result);
        break;
    case EWK_JS_VARIANT_DOUBLE:
        DOUBLE_TO_NPVARIANT(data->value.d, *result);
        break;
    case EWK_JS_VARIANT_STRING:
        string_value = eina_stringshare_add(data->value.s);
        if (string_value)
            STRINGZ_TO_NPVARIANT(string_value, *result);
        else
            return false;
        break;
    case EWK_JS_VARIANT_BOOL:
        BOOLEAN_TO_NPVARIANT(data->value.b, *result);
        break;
    case EWK_JS_VARIANT_OBJECT:
        OBJECT_TO_NPVARIANT(reinterpret_cast<NPObject*>(data->value.o), *result);
        break;
    default:
        return false;
    }

    return true;
}

// These methods are used by NPAI, thats the reason to use bool instead of Eina_Bool.
static bool ewk_js_property_has(NPObject* npObject, NPIdentifier name)
{
    Ewk_JS_Object* object = reinterpret_cast<Ewk_JS_Object*>(npObject);

    EINA_SAFETY_ON_NULL_RETURN_VAL(npObject, false);
    EINA_MAGIC_CHECK_OR_RETURN(object, false);

    if (!_NPN_IdentifierIsString(name)) {
        ERR("int NPIdentifier is not supported.");
        return false;
    }

    char* prop_name = _NPN_UTF8FromIdentifier(name);
    bool fail = eina_hash_find(object->properties, prop_name); // FIXME: should search methods too?
    free(prop_name);

    return fail;
}

static bool ewk_js_property_get(NPObject* npObject, NPIdentifier name, NPVariant* result)
{
    Ewk_JS_Object* object = reinterpret_cast<Ewk_JS_Object*>(npObject);
    Ewk_JS_Variant* value;
    Ewk_JS_Property* prop;
    bool fail = false;

    EINA_SAFETY_ON_NULL_RETURN_VAL(npObject, false);
    EINA_MAGIC_CHECK_OR_RETURN(object, false);

    if (!_NPN_IdentifierIsString(name)) {
        ERR("int NPIdentifier is not supported.");
        return false;
    }

    value = static_cast<Ewk_JS_Variant*>(malloc(sizeof(Ewk_JS_Variant)));
    if (!value) {
        ERR("Could not allocate memory for ewk_js_variant");
        return false;
    }

    prop = static_cast<Ewk_JS_Property*>(eina_hash_find(object->cls->properties, name));
    if (prop && prop->get) { // Class has property and property has getter.
        fail = prop->get(object, prop->name, value);
        if (!fail)
            fail = ewk_js_variant_to_npvariant(value, result);
    } else if (object->cls->default_prop.get) { // Default getter exists.
        fail = object->cls->default_prop.get(object, prop->name, value);
        if (!fail)
            fail = ewk_js_variant_to_npvariant(value, result);
    } else { // Fallback to objects hash map.
        char* prop_name = _NPN_UTF8FromIdentifier(name);
        free(value);
        value = static_cast<Ewk_JS_Variant*>(eina_hash_find(object->properties, prop_name));
        free(prop_name);
        if (value)
            return ewk_js_variant_to_npvariant(value, result);
    }

    free(value);
    return fail;
}

static bool ewk_js_property_set(NPObject* npObject, NPIdentifier name, const NPVariant* npValue)
{
    Ewk_JS_Object* object = reinterpret_cast<Ewk_JS_Object*>(npObject);
    Ewk_JS_Variant* value;
    Ewk_JS_Property* prop;
    bool fail = false;

    EINA_SAFETY_ON_NULL_RETURN_VAL(npObject, false);
    EINA_MAGIC_CHECK_OR_RETURN(object, false);

    if (!_NPN_IdentifierIsString(name)) {
        ERR("int NPIdentifier is not supported.");
        return fail;
    }

    value = static_cast<Ewk_JS_Variant*>(malloc(sizeof(Ewk_JS_Variant)));
    if (!value) {
        ERR("Could not allocate memory for ewk_js_variant");
        return false;
    }

    ewk_js_npvariant_to_variant(value, npValue);
    char* prop_name = _NPN_UTF8FromIdentifier(name);
    prop = static_cast<Ewk_JS_Property*>(eina_hash_find(object->cls->properties, prop_name));
    if (prop && prop->set)
        fail = prop->set(object, prop->name, value); // Class has property and property has setter.
    else if (object->cls->default_prop.set)
        fail = object->cls->default_prop.set(object, prop_name, value); // Default getter exists.
    else { // Fallback to objects hash map.
        void* old = eina_hash_set(object->properties, prop_name, value);
        free(old);
        fail = true;
    }

    free(prop_name);
    return fail;
}

static bool ewk_js_property_remove(NPObject* npObject, NPIdentifier name)
{
    Ewk_JS_Object* object = reinterpret_cast<Ewk_JS_Object*>(npObject);
    Ewk_JS_Property* prop;
    bool fail = false;

    EINA_SAFETY_ON_NULL_RETURN_VAL(npObject, false);
    EINA_MAGIC_CHECK_OR_RETURN(object, false);

    if (!_NPN_IdentifierIsString(name)) {
        ERR("int NPIdentifier is not supported.");
        return fail;
    }

    char* prop_name = _NPN_UTF8FromIdentifier(name);
    prop = static_cast<Ewk_JS_Property*>(eina_hash_find(object->cls->properties, prop_name));
    if (prop && prop->del)
        fail = prop->del(object, prop->name); // Class has property and property has getter.
    else if (object->cls->default_prop.del)
        fail = object->cls->default_prop.del(object, prop_name);
    else
        fail = eina_hash_del(object->properties, prop_name, 0);

    free(prop_name);
    return fail;
}

static bool ewk_js_properties_enumerate(NPObject* npObject, NPIdentifier** value, uint32_t* count)
{
    Ewk_JS_Object* object = reinterpret_cast<Ewk_JS_Object*>(npObject);
    Eina_Iterator* it;
    char* key;
    int i = 0;

    EINA_SAFETY_ON_NULL_RETURN_VAL(npObject, false);
    EINA_MAGIC_CHECK_OR_RETURN(object, false);

    *count = eina_hash_population(object->properties);
    *value = static_cast<NPIdentifier*>(malloc(sizeof(NPIdentifier) * *count));
    if (!*value) {
        ERR("Could not allocate memory for NPIdentifier");
        return false;
    }

    it = eina_hash_iterator_key_new(object->properties);
    EINA_ITERATOR_FOREACH(it, key)
        (*value)[i++] = _NPN_GetStringIdentifier(key);

    eina_iterator_free(it);
    return true;
}

static bool ewk_js_method_has(NPObject* npObject, NPIdentifier name)
{
    Ewk_JS_Object* object = reinterpret_cast<Ewk_JS_Object*>(npObject);

    EINA_SAFETY_ON_NULL_RETURN_VAL(npObject, false);
    EINA_MAGIC_CHECK_OR_RETURN(object, false);

    if (!_NPN_IdentifierIsString(name)) {
        ERR("int NPIdentifier is not supported.");
        return false;
    }
    return eina_hash_find(object->cls->methods, name); // Returns pointer if found(true), 0(false) otherwise.
}

static bool ewk_js_method_invoke(NPObject* npObject, NPIdentifier name, const NPVariant* npArgs, uint32_t npArgCount, NPVariant* result)
{
    Ewk_JS_Object* object = reinterpret_cast<Ewk_JS_Object*>(npObject);
    Ewk_JS_Method* method;
    Ewk_JS_Variant* args;
    Ewk_JS_Variant* ret_val;

    EINA_SAFETY_ON_NULL_RETURN_VAL(npObject, false);
    EINA_MAGIC_CHECK_OR_RETURN(object, false);

    if (!_NPN_IdentifierIsString(name)) {
        ERR("int NPIdentifier is not supported.");
        return false;
    }

    method = static_cast<Ewk_JS_Method*>(eina_hash_find(object->cls->methods, name));
    if (!method)
        return false;

    args = static_cast<Ewk_JS_Variant*>(malloc(sizeof(Ewk_JS_Variant)  *npArgCount));
    if (!args) {
        ERR("Could not allocate memory for ewk_js_variant");
        return false;
    }

    for (uint32_t i = 0; i < npArgCount; i++)
        ewk_js_npvariant_to_variant(&args[i], &npArgs[i]);
    ret_val = method->invoke(object, args, npArgCount);
    ewk_js_variant_to_npvariant(ret_val, result);

    ewk_js_variant_free(ret_val);

    return true;
}

static Eina_Bool ewk_js_npobject_property_get(Ewk_JS_Object* jsObject, const char* name, Ewk_JS_Variant* value)
{
    NPIdentifier id = _NPN_GetStringIdentifier(name);
    NPVariant var;
    bool fail = _NPN_GetProperty(0, reinterpret_cast<NPObject*>(jsObject), id, &var);
    if (!fail)
        fail = ewk_js_npvariant_to_variant(value, &var);
    return fail;
}

static Eina_Bool ewk_js_npobject_property_set(Ewk_JS_Object* jsObject, const char* name, const Ewk_JS_Variant* value)
{
    NPIdentifier id = _NPN_GetStringIdentifier(name);
    NPVariant var;
    bool fail = ewk_js_variant_to_npvariant(value, &var);
    if (fail)
        fail = _NPN_SetProperty(0, reinterpret_cast<NPObject*>(jsObject), id, &var);
    return fail;
}

static Eina_Bool ewk_js_npobject_property_del(Ewk_JS_Object* jsObject, const char* name)
{
    NPIdentifier id = _NPN_GetStringIdentifier(name);
    return _NPN_RemoveProperty(0, reinterpret_cast<NPObject*>(jsObject), id);
}

static void ewk_js_property_free(Ewk_JS_Property* prop)
{
    free(const_cast<char*>(prop->name));
    if (prop->value.type == EWK_JS_VARIANT_STRING)
        eina_stringshare_del(prop->value.value.s);
    else if (prop->value.type == EWK_JS_VARIANT_OBJECT)
        ewk_js_object_free(prop->value.value.o);
    free(prop);
}

/**
 * Create a Ewk_JS_Class to be used in @a ewk_js_object_new.
 *
 * @param meta @a Ewk_JS_Class_Meta that describes the class to be created.
 *
 * @return The Ewk_JS_Class created.
 */
Ewk_JS_Class* ewk_js_class_new(const Ewk_JS_Class_Meta* jsMetaClass)
{
    Ewk_JS_Class* cls;

    EINA_SAFETY_ON_NULL_RETURN_VAL(jsMetaClass, 0);

    cls = static_cast<Ewk_JS_Class*>(malloc(sizeof(Ewk_JS_Class)));
    if (!cls) {
        ERR("Could not allocate memory for ewk_js_class");
        return 0;
    }

    cls->meta = jsMetaClass;
    cls->default_prop = cls->meta->default_prop;

    // Don't free methods since they point to meta class methods(will be freed when meta class is freed).
    cls->methods = eina_hash_pointer_new(0);
    for (int i = 0; cls->meta->methods && cls->meta->methods[i].name; i++) {
        NPIdentifier id = _NPN_GetStringIdentifier(cls->meta->methods[i].name);
        eina_hash_add(cls->methods, id, &cls->meta->methods[i]);
    }

    // Don't free properties since they point to cls->meta class properties(will be freed when cls->meta class is freed).
    cls->properties = eina_hash_pointer_new(0);
    for (int i = 0; cls->meta->properties && cls->meta->properties[i].name; i++) {
        NPIdentifier id = _NPN_GetStringIdentifier(cls->meta->properties[i].name);
        eina_hash_add(cls->properties, id, &cls->meta->properties[i]);
    }

    return cls;
}

/**
 * Release resources allocated by @a cls.
 *
 * @param cls @a Ewk_JS_Class to be released.
 */
void ewk_js_class_free(Ewk_JS_Class* jsClass)
{
    EINA_SAFETY_ON_NULL_RETURN(jsClass);
    eina_hash_free(jsClass->methods);
    eina_hash_free(jsClass->properties);
    free(jsClass);
}

static NPClass EWK_NPCLASS = {
    NP_CLASS_STRUCT_VERSION,
    0, // NPAllocateFunctionPtr
    0, // NPDeallocateFunctionPtr
    0, // NPInvalidateFunctionPtr
    ewk_js_method_has, // NPHasMethodFunctionPtr
    ewk_js_method_invoke, // NPInvokeFunctionPtr
    0, // NPInvokeDefaultFunctionPtr
    ewk_js_property_has, // NPHasPropertyFunctionPtr
    ewk_js_property_get, // NPGetPropertyFunctionPtr
    ewk_js_property_set, // NPSetPropertyFunctionPtr
    ewk_js_property_remove, // NPRemovePropertyFunctionPtr
    ewk_js_properties_enumerate, // NPEnumerationFunctionPtr
    0 // NPConstructFunction
};

static Ewk_JS_Object* ewk_js_npobject_to_object(NPObject* npObject)
{
    NPIdentifier* values;
    uint32_t np_props_count;
    Ewk_JS_Class* cls;
    Ewk_JS_Object* object;
    Eina_Iterator* it;
    Ewk_JS_Property* prop;
    JavaScriptObject* jso;

    if (EINA_MAGIC_CHECK(reinterpret_cast<Ewk_JS_Object*>(npObject), EWK_JS_OBJECT_MAGIC))
        return reinterpret_cast<Ewk_JS_Object*>(npObject);

    if (!_NPN_Enumerate(0, npObject, &values, &np_props_count))
        return 0;

    cls = static_cast<Ewk_JS_Class*>(malloc(sizeof(Ewk_JS_Class)));
    if (!cls) {
        ERR("Could not allocate memory for ewk_js_class");
        return 0;
    }

    cls->meta = 0;
    Ewk_JS_Default def = {
        ewk_js_npobject_property_set,
        ewk_js_npobject_property_get,
        ewk_js_npobject_property_del
    };
    cls->default_prop = def;
    cls->methods = eina_hash_pointer_new(0);
    cls->properties = eina_hash_pointer_new(reinterpret_cast<Eina_Free_Cb>(ewk_js_property_free));
    for (uint32_t i = 0; i < np_props_count; i++) {
        if (_NPN_HasProperty(0, npObject, values[i])) {
            NPVariant var;
            Ewk_JS_Property* prop = static_cast<Ewk_JS_Property*>(calloc(sizeof(Ewk_JS_Property), 1));
            if (!prop) {
                ERR("Could not allocate memory for ewk_js_property");
                goto error;
            }

            _NPN_GetProperty(0, npObject, values[i], &var);
            ewk_js_npvariant_to_variant(&(prop->value), &var);
            prop->name = _NPN_UTF8FromIdentifier(values[i]);
            eina_hash_add(cls->properties, values[i], prop);
        }
    }

    // Can't use ewk_js_object_new(cls) because it expects cls->meta to exist.
    object = static_cast<Ewk_JS_Object*>(malloc(sizeof(Ewk_JS_Object)));
    if (!object) {
        ERR("Could not allocate memory for ewk_js_object");
        goto error;
    }

    free(values);
    EINA_MAGIC_SET(object, EWK_JS_OBJECT_MAGIC);
    object->name = 0;
    object->cls = cls;
    object->view = 0;

    jso = reinterpret_cast<JavaScriptObject*>(npObject);
    if (!strcmp("Array", jso->imp->methodTable()->className(jso->imp).ascii().data()))
        object->type = EWK_JS_OBJECT_ARRAY;
    else if (!strcmp("Function", jso->imp->methodTable()->className(jso->imp).ascii().data()))
        object->type = EWK_JS_OBJECT_FUNCTION;
    else
        object->type = EWK_JS_OBJECT_OBJECT;

    if (eina_hash_population(cls->properties) < 25)
        object->properties = eina_hash_string_small_new(0);
    else
        object->properties = eina_hash_string_superfast_new(0);

    it = eina_hash_iterator_data_new(cls->properties);
    EINA_ITERATOR_FOREACH(it, prop) {
        const char* key = prop->name;
        Ewk_JS_Variant* value = &prop->value;
        eina_hash_add(object->properties, key, value);
    }

    eina_iterator_free(it);
    object->base = *reinterpret_cast<JavaScriptObject*>(npObject);

    return object;

error:
    ewk_js_class_free(cls);
    free(values);
    return 0;
}

static Eina_Bool ewk_js_npvariant_to_variant(Ewk_JS_Variant* data, const NPVariant* result)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(data, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(result, false);
    switch (result->type) {
    case NPVariantType_Void:
        data->type = EWK_JS_VARIANT_VOID;
        data->value.o = 0;
        break;
    case NPVariantType_Null:
        data->type = EWK_JS_VARIANT_NULL;
        data->value.o = 0;
        break;
    case NPVariantType_Int32:
        data->type = EWK_JS_VARIANT_INT32;
        data->value.i = NPVARIANT_TO_INT32(*result);
        break;
    case NPVariantType_Double:
        data->type = EWK_JS_VARIANT_DOUBLE;
        data->value.d = NPVARIANT_TO_DOUBLE(*result);
        break;
    case NPVariantType_String:
        data->value.s = eina_stringshare_add_length(NPVARIANT_TO_STRING(*result).UTF8Characters, NPVARIANT_TO_STRING(*result).UTF8Length);
        data->type = EWK_JS_VARIANT_STRING;
        break;
    case NPVariantType_Bool:
        data->type = EWK_JS_VARIANT_BOOL;
        data->value.b = NPVARIANT_TO_BOOLEAN(*result);
        break;
    case NPVariantType_Object:
        data->type = EWK_JS_VARIANT_OBJECT;
        data->value.o = ewk_js_npobject_to_object(NPVARIANT_TO_OBJECT(*result));
        break;
    default:
        return false;
    }

    return true;
}
#endif // ENABLE(NETSCAPE_PLUGIN_API)

Ewk_JS_Object* ewk_js_object_new(const Ewk_JS_Class_Meta* jsMetaClass)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    Ewk_JS_Object* object;

    EINA_SAFETY_ON_NULL_RETURN_VAL(jsMetaClass, 0);

    object = static_cast<Ewk_JS_Object*>(malloc(sizeof(Ewk_JS_Object)));
    if (!object) {
        ERR("Could not allocate memory for ewk_js_object");
        return 0;
    }

    EINA_MAGIC_SET(object, EWK_JS_OBJECT_MAGIC);
    object->cls = ewk_js_class_new(jsMetaClass);
    object->view = 0;
    object->name = 0;
    object->type = EWK_JS_OBJECT_OBJECT;

    if (eina_hash_population(object->cls->properties) < 25)
        object->properties = eina_hash_string_small_new(reinterpret_cast<Eina_Free_Cb>(ewk_js_variant_free));
    else
        object->properties = eina_hash_string_superfast_new(reinterpret_cast<Eina_Free_Cb>(ewk_js_variant_free));

    for (int i = 0; object->cls->meta->properties && object->cls->meta->properties[i].name; i++) {
        Ewk_JS_Property prop = object->cls->meta->properties[i];
        const char* key = object->cls->meta->properties[i].name;
        Ewk_JS_Variant* value = static_cast<Ewk_JS_Variant*>(malloc(sizeof(Ewk_JS_Variant)));
        if (!value) {
            ERR("Could not allocate memory for ewk_js_variant");
            goto error;
        }
        if (prop.get)
            prop.get(object, key, value);
        else {
            value->type = prop.value.type;
            switch (value->type) {
            case EWK_JS_VARIANT_VOID:
            case EWK_JS_VARIANT_NULL:
                value->value.o = 0;
                break;
            case EWK_JS_VARIANT_STRING:
                value->value.s = eina_stringshare_add(prop.value.value.s);
                break;
            case EWK_JS_VARIANT_BOOL:
                value->value.b = prop.value.value.b;
                break;
            case EWK_JS_VARIANT_INT32:
                value->value.i = prop.value.value.i;
                break;
            case EWK_JS_VARIANT_DOUBLE:
                value->value.d = prop.value.value.d;
                break;
            case EWK_JS_VARIANT_OBJECT:
                value->value.o = prop.value.value.o;
                break;
            }
        }
        eina_hash_add(object->properties, key, value);
    }

    object->base.object.referenceCount = 1;
    object->base.object._class = &EWK_NPCLASS;
    return object;

error:
    ewk_js_object_free(object);
    return 0;
#else
    UNUSED_PARAM(jsMetaClass);
    return 0;
#endif
}

void ewk_js_object_free(Ewk_JS_Object* jsObject)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EINA_SAFETY_ON_NULL_RETURN(jsObject);
    EINA_MAGIC_CHECK_OR_RETURN(jsObject);
    Eina_Bool script_obj = !jsObject->cls->meta;

    eina_hash_free(jsObject->properties);
    eina_stringshare_del(jsObject->name);

    ewk_js_class_free(const_cast<Ewk_JS_Class*>(jsObject->cls));

    EINA_MAGIC_SET(jsObject, EINA_MAGIC_NONE);

    if (script_obj)
        free(jsObject);
#else
    UNUSED_PARAM(jsObject);
#endif
}

Evas_Object* ewk_js_object_view_get(const Ewk_JS_Object* jsObject)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(jsObject, 0);
    EINA_MAGIC_CHECK_OR_RETURN(jsObject, 0);
    return jsObject->view;
#else
    UNUSED_PARAM(jsObject);
    return 0;
#endif
}

Eina_Hash* ewk_js_object_properties_get(const Ewk_JS_Object* jsObject)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(jsObject, 0);
    EINA_MAGIC_CHECK_OR_RETURN(jsObject, 0);
    return jsObject->properties;
#else
    UNUSED_PARAM(jsObject);
    return 0;
#endif
}

const char* ewk_js_object_name_get(const Ewk_JS_Object* jsObject)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(jsObject, 0);
    EINA_MAGIC_CHECK_OR_RETURN(jsObject, 0);
    return jsObject->name;
#else
    UNUSED_PARAM(jsObject);
    return 0;
#endif
}

Eina_Bool ewk_js_object_invoke(Ewk_JS_Object* jsObject, Ewk_JS_Variant* args, int argCount, Ewk_JS_Variant* result)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    NPVariant* np_args;
    NPVariant np_result;
    bool fail = false;

    EINA_MAGIC_CHECK_OR_RETURN(jsObject, false);
    if (ewk_js_object_type_get(jsObject) != EWK_JS_OBJECT_FUNCTION)
        return false;
    if (argCount)
        EINA_SAFETY_ON_NULL_RETURN_VAL(args, false);

    np_args = static_cast<NPVariant*>(malloc(sizeof(NPVariant)  *argCount));
    if (!np_args) {
        ERR("Could not allocate memory to method arguments");
        return false;
    }

    for (int i = 0; i < argCount; i++)
        if (!ewk_js_variant_to_npvariant(&args[i], &np_args[i]))
            goto end;

    if (!(fail = _NPN_InvokeDefault(0, reinterpret_cast<NPObject*>(jsObject), np_args, argCount, &np_result)))
        goto end;
    if (result)
        fail = ewk_js_npvariant_to_variant(result, &np_result);

end:
    free(np_args);
    return fail;
#else
    UNUSED_PARAM(jsObject);
    UNUSED_PARAM(args);
    UNUSED_PARAM(argCount);
    UNUSED_PARAM(result);
    return false;
#endif
}

Ewk_JS_Object_Type ewk_js_object_type_get(Ewk_JS_Object* jsObject)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(jsObject, EWK_JS_OBJECT_OBJECT);
    EINA_MAGIC_CHECK_OR_RETURN(jsObject, EWK_JS_OBJECT_OBJECT);

    return jsObject->type;
#else
    UNUSED_PARAM(jsObject);
    return EWK_JS_OBJECT_INVALID;
#endif
}

void ewk_js_object_type_set(Ewk_JS_Object* jsObject, Ewk_JS_Object_Type type)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EINA_SAFETY_ON_NULL_RETURN(jsObject);
    EINA_MAGIC_CHECK_OR_RETURN(jsObject);

    jsObject->type = type;
#else
    UNUSED_PARAM(jsObject);
    UNUSED_PARAM(type);
#endif
}

void ewk_js_variant_free(Ewk_JS_Variant* jsVariant)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EINA_SAFETY_ON_NULL_RETURN(jsVariant);
    if (jsVariant->type == EWK_JS_VARIANT_STRING)
        eina_stringshare_del(jsVariant->value.s);
    else if (jsVariant->type == EWK_JS_VARIANT_OBJECT)
        ewk_js_object_free(jsVariant->value.o);
    free(jsVariant);
#else
    UNUSED_PARAM(jsVariant);
#endif
}

void ewk_js_variant_array_free(Ewk_JS_Variant* jsVariant, int count)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    EINA_SAFETY_ON_NULL_RETURN(jsVariant);
    for (int i = 0; i < count; i++) {
        if (jsVariant[i].type == EWK_JS_VARIANT_STRING)
            eina_stringshare_del(jsVariant[i].value.s);
        else if (jsVariant[i].type == EWK_JS_VARIANT_OBJECT)
            ewk_js_object_free(jsVariant[i].value.o);
    }
    free(jsVariant);
#else
    UNUSED_PARAM(jsVariant);
    UNUSED_PARAM(count);
#endif
}
