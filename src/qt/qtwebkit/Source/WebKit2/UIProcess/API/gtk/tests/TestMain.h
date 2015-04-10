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

#ifndef TestMain_h
#define TestMain_h

#include <cairo.h>
#include <glib-object.h>
#include <wtf/HashSet.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#define MAKE_GLIB_TEST_FIXTURE(ClassName) \
    static void setUp(ClassName* fixture, gconstpointer data) \
    { \
        new (fixture) ClassName; \
    } \
    static void tearDown(ClassName* fixture, gconstpointer data) \
    { \
        fixture->~ClassName(); \
    } \
    static void add(const char* suiteName, const char* testName, void (*testFunc)(ClassName*, const void*)) \
    { \
        GOwnPtr<gchar> testPath(g_strdup_printf("/webkit2/%s/%s", suiteName, testName)); \
        g_test_add(testPath.get(), ClassName, 0, ClassName::setUp, testFunc, ClassName::tearDown); \
    }

#define ASSERT_CMP_CSTRING(s1, cmp, s2) \
    do { CString __s1 = (s1); CString __s2 = (s2); \
        if (g_strcmp0(__s1.data(), __s2.data()) cmp 0) ; else \
            g_assertion_message_cmpstr(G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                #s1 " " #cmp " " #s2, __s1.data(), #cmp, __s2.data()); } while (0)

class Test {
public:
    MAKE_GLIB_TEST_FIXTURE(Test);

    ~Test()
    {
        if (m_watchedObjects.isEmpty())
            return;

        g_print("Leaked objects:");
        HashSet<GObject*>::const_iterator end = m_watchedObjects.end();
        for (HashSet<GObject*>::const_iterator it = m_watchedObjects.begin(); it != end; ++it)
            g_print(" %s(%p)", g_type_name_from_instance(reinterpret_cast<GTypeInstance*>(*it)), *it);
        g_print("\n");

        g_assert(m_watchedObjects.isEmpty());
    }

    static void objectFinalized(Test* test, GObject* finalizedObject)
    {
        test->m_watchedObjects.remove(finalizedObject);
    }

    void assertObjectIsDeletedWhenTestFinishes(GObject* object)
    {
        m_watchedObjects.add(object);
        g_object_weak_ref(object, reinterpret_cast<GWeakNotify>(objectFinalized), this);
    }

    static CString getWebKit1TestResoucesDir()
    {
        GOwnPtr<char> resourcesDir(g_build_filename(WEBKIT_SRC_DIR, "Source", "WebKit", "gtk", "tests", "resources", NULL));
        return resourcesDir.get();
    }

    static CString getResourcesDir()
    {
        GOwnPtr<char> resourcesDir(g_build_filename(WEBKIT_SRC_DIR, "Source", "WebKit2", "UIProcess", "API", "gtk", "tests", "resources", NULL));
        return resourcesDir.get();
    }

    void addLogFatalFlag(unsigned flag)
    {
        unsigned fatalMask = g_log_set_always_fatal(static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK));
        fatalMask |= flag;
        g_log_set_always_fatal(static_cast<GLogLevelFlags>(fatalMask));
    }

    void removeLogFatalFlag(unsigned flag)
    {
        unsigned fatalMask = g_log_set_always_fatal(static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK));
        fatalMask &= ~flag;
        g_log_set_always_fatal(static_cast<GLogLevelFlags>(fatalMask));
    }

    static bool cairoSurfacesEqual(cairo_surface_t* s1, cairo_surface_t* s2)
    {
        return (cairo_image_surface_get_format(s1) == cairo_image_surface_get_format(s2)
            && cairo_image_surface_get_width(s1) == cairo_image_surface_get_width(s2)
            && cairo_image_surface_get_height(s1) == cairo_image_surface_get_height(s2)
            && cairo_image_surface_get_stride(s1) == cairo_image_surface_get_stride(s2)
            && !memcmp(const_cast<const void*>(reinterpret_cast<void*>(cairo_image_surface_get_data(s1))),
                const_cast<const void*>(reinterpret_cast<void*>(cairo_image_surface_get_data(s2))),
                cairo_image_surface_get_height(s1)*cairo_image_surface_get_stride(s1)));
    }

    HashSet<GObject*> m_watchedObjects;
};

#endif // TestMain_h
