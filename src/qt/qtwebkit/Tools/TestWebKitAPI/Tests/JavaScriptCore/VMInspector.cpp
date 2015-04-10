/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <interpreter/VMInspector.h>
#include <stdarg.h>
#include <stdio.h>

using namespace JSC;

// There's not much we can test for the VMInspector::printf() case except to
// make sure it does not crash. Unfortunately, we also don't want all the
// test strings crowding out stdout. So, we forego the printf tests.
// NOTE that the most interesting part of VMInspector::printf() is the
// formatting functionality, and it is are already being tested by the
// fprintf() and sprintf() cases.


// The VMInspector::fprintf() test works by printing the string to a temp file,
// and then reading the file content back into a buffer, which we, in turn,
// compare against the expected string.

TEST(JSC, VMInspectorFprintf)
{
#if ENABLE(VMINSPECTOR)
    char actual[1024];
    char expected[1024];
    const char* format;
    const char* expectedLiteral;
    FILE* file;
    const char* filename = "/tmp/VMInspectorFprintfTest.txt";
    size_t size;

#define OPEN_FILE(file)                         \
    do {                                        \
        file = fopen(filename, "w");            \
    } while (false)

#define READ_AND_CLOSE_FILE(file, actual)       \
    do {                                        \
        fclose(file);                           \
        file = fopen(filename, "r");            \
        fseek(file, 0, SEEK_END);               \
        size = ftell(file);                     \
        rewind(file);                           \
        fread(actual, 1, size, file);           \
        actual[size] = '\0';                    \
        fclose(file);                           \
    } while (false)

    // Testing standard default format specifiers:
    // Note: should work just like sprintf. So, we just compare against that.
    memset(actual, 'z', sizeof(actual));
    // The compiler warning flags are configured to expect a literal string for
    // ::sprintf below. So, use a #define for this one case to keep the
    // compiler happy.
#undef LITERAL_FORMAT
#define LITERAL_FORMAT "'%%%%'                         ==> '%%'\n"

    OPEN_FILE(file);
    VMInspector::fprintf(file, LITERAL_FORMAT);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, LITERAL_FORMAT);

#undef LITERAL_FORMAT
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%c', 'x'                    ==> '%c'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 'x');
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 'x');
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%*c', 8, 'x'                ==> '%*c'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 8, 'x');
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 8, 'x');
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%s', \"hello world\"          ==> '%s'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, "hello world");
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, "hello world");
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%*s', 8, \"hello world\"      ==> '%*s'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 8, "hello world");
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 8, "hello world");
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%*s', 8, \"hello\"            ==> '%*s'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 8, "hello");
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 8, "hello");
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%d', 987654321              ==> '%d'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 987654321);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 987654321);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%u', 4276543210u            ==> '%u'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 4276543210u);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 4276543210u);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%u', 0xffffffff             ==> '%u'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 0xffffffff);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 0xffffffff);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%x', 0xffffffff             ==> '%x'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 0xffffffff);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 0xffffffff);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%p', (void*)0xabcdbabe      ==> '%p'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, (void*)0xabcdbabe);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, (void*)0xabcdbabe);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%lu', 1234567890987654321ul ==> '%lu'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 1234567890987654321ul);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 1234567890987654321ul);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%f', 1234.567               ==> '%f'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 1234.567);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 1234.567);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%.2f', 1234.567             ==> '%.2f'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 1234.567);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 1234.567);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%10.2f', 1234.567           ==> '%10.2f'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 1234.567);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 1234.567);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%010.2f', 1234.567          ==> '%010.2f'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 1234.567);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 1234.567);
    ASSERT_EQ(strcmp(actual, expected), 0);

    // Bad / weird formats:
    memset(actual, 'z', sizeof(actual));
    format = "'%%5.4', 987654321            ==> '%5.4'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 987654321);
    READ_AND_CLOSE_FILE(file, actual);
    expectedLiteral = "'%5.4', 987654321            ==> 'ERROR @ \"%5.4' \"\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%5.4' '%%d', 987654321, 4    ==> '%5.4' '%d'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 987654321, 4);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 987654321, 4);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%w' '%%d', 987654321, 6      ==> '%w' '%d'\n";
    OPEN_FILE(file);
    VMInspector::fprintf(file, format, 987654321, 6);
    READ_AND_CLOSE_FILE(file, actual);
    ::sprintf(expected, format, 987654321, 6);
    ASSERT_EQ(strcmp(actual, expected), 0);


    // Testing the %b extension:
    memset(actual, 'z', sizeof(actual));
    OPEN_FILE(file);
    VMInspector::fprintf(file, "'%%b', 0          ==> '%b'\n", 0);
    READ_AND_CLOSE_FILE(file, actual);
    ASSERT_EQ(strcmp(actual, "'%b', 0          ==> 'FALSE'\n"), 0);

    memset(actual, 'z', sizeof(actual));
    OPEN_FILE(file);
    VMInspector::fprintf(file, "'%%b', 1          ==> '%b'\n", 1);
    READ_AND_CLOSE_FILE(file, actual);
    ASSERT_EQ(strcmp(actual, "'%b', 1          ==> 'TRUE'\n"), 0);

    memset(actual, 'z', sizeof(actual));
    OPEN_FILE(file);
    VMInspector::fprintf(file, "'%%b', -123456789 ==> '%b'\n", -123456789);
    READ_AND_CLOSE_FILE(file, actual);
    ASSERT_EQ(strcmp(actual, "'%b', -123456789 ==> 'TRUE'\n"), 0);

    memset(actual, 'z', sizeof(actual));
    OPEN_FILE(file);
    VMInspector::fprintf(file, "'%%b', 123456789  ==> '%b'\n", 123456789);
    READ_AND_CLOSE_FILE(file, actual);
    ASSERT_EQ(strcmp(actual, "'%b', 123456789  ==> 'TRUE'\n"), 0);


    // Testing the %J<x> extensions:
    String str1("Test WTF String");
    String str2("");

    memset(actual, 'z', sizeof(actual));
    OPEN_FILE(file);
    VMInspector::fprintf(file, "'%%Js' is %%s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '%Js' is %s\n",
        &str1, str1.isEmpty() ? "EMPTY" : "NOT EMPTY");
    READ_AND_CLOSE_FILE(file, actual);
    expectedLiteral = "'%Js' is %s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> 'Test WTF String' is NOT EMPTY\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

    memset(actual, 'z', sizeof(actual));
    OPEN_FILE(file);
    VMInspector::fprintf(file, "'%%Js' is %%s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '%Js' is %s\n",
        &str2, str2.isEmpty() ? "EMPTY" : "NOT EMPTY");
    READ_AND_CLOSE_FILE(file, actual);
    expectedLiteral = "'%Js' is %s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '' is EMPTY\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

    memset(actual, 'z', sizeof(actual));
    OPEN_FILE(file);
    VMInspector::fprintf(file, "'%%J+s' is %%s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> '%J+s' is %s\n",
        &str1, str1.isEmpty() ? "EMPTY" : "NOT EMPTY");
    READ_AND_CLOSE_FILE(file, actual);
    expectedLiteral = "'%J+s' is %s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> 'WTF::String \"Test WTF String\"' is NOT EMPTY\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

    memset(actual, 'z', sizeof(actual));
    OPEN_FILE(file);
    VMInspector::fprintf(file, "'%%J+s' is %%s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> '%J+s' is %s\n",
        &str2, str2.isEmpty() ? "EMPTY" : "NOT EMPTY");
    READ_AND_CLOSE_FILE(file, actual);
    expectedLiteral = "'%J+s' is %s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> 'WTF::String \"\"' is EMPTY\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

#undef OPEN_FILE
#undef READ_AND_CLOSE_FILE

#endif
}


TEST(JSC, VMInspectorSprintf)
{
#if ENABLE(VMINSPECTOR)
    char actual[1024];
    char expected[1024];
    const char* format;
    const char* expectedLiteral;

    // Testing standard default format specifiers:
    // Note: should work just like sprintf. So, we just compare against that.
    memset(actual, 'z', sizeof(actual));
    // The compiler warning flags are configured to expect a literal string for
    // ::sprintf below. So, use a #define for this one case to keep the
    // compiler happy.
#undef LITERAL_FORMAT
#define LITERAL_FORMAT "'%%%%'                         ==> '%%'\n"
    VMInspector::sprintf(actual, LITERAL_FORMAT);
    ::sprintf(expected, LITERAL_FORMAT);
#undef LITERAL_FORMAT
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%c', 'x'                    ==> '%c'\n";
    VMInspector::sprintf(actual, format, 'x');
    ::sprintf(expected, format, 'x');
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%*c', 8, 'x'                ==> '%*c'\n";
    VMInspector::sprintf(actual, format, 8, 'x');
    ::sprintf(expected, format, 8, 'x');
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%s', \"hello world\"          ==> '%s'\n";
    VMInspector::sprintf(actual, format, "hello world");
    ::sprintf(expected, format, "hello world");
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%*s', 8, \"hello world\"      ==> '%*s'\n";
    VMInspector::sprintf(actual, format, 8, "hello world");
    ::sprintf(expected, format, 8, "hello world");
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%*s', 8, \"hello\"            ==> '%*s'\n";
    VMInspector::sprintf(actual, format, 8, "hello");
    ::sprintf(expected, format, 8, "hello");
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%d', 987654321              ==> '%d'\n";
    VMInspector::sprintf(actual, format, 987654321);
    ::sprintf(expected, format, 987654321);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%u', 4276543210u            ==> '%u'\n";
    VMInspector::sprintf(actual, format, 4276543210u);
    ::sprintf(expected, format, 4276543210u);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%u', 0xffffffff             ==> '%u'\n";
    VMInspector::sprintf(actual, format, 0xffffffff);
    ::sprintf(expected, format, 0xffffffff);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%x', 0xffffffff             ==> '%x'\n";
    VMInspector::sprintf(actual, format, 0xffffffff);
    ::sprintf(expected, format, 0xffffffff);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%p', (void*)0xabcdbabe      ==> '%p'\n";
    VMInspector::sprintf(actual, format, (void*)0xabcdbabe);
    ::sprintf(expected, format, (void*)0xabcdbabe);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%lu', 1234567890987654321ul ==> '%lu'\n";
    VMInspector::sprintf(actual, format, 1234567890987654321ul);
    ::sprintf(expected, format, 1234567890987654321ul);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%f', 1234.567               ==> '%f'\n";
    VMInspector::sprintf(actual, format, 1234.567);
    ::sprintf(expected, format, 1234.567);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%.2f', 1234.567             ==> '%.2f'\n";
    VMInspector::sprintf(actual, format, 1234.567);
    ::sprintf(expected, format, 1234.567);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%10.2f', 1234.567           ==> '%10.2f'\n";
    VMInspector::sprintf(actual, format, 1234.567);
    ::sprintf(expected, format, 1234.567);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%010.2f', 1234.567          ==> '%010.2f'\n";
    VMInspector::sprintf(actual, format, 1234.567);
    ::sprintf(expected, format, 1234.567);
    ASSERT_EQ(strcmp(actual, expected), 0);

    // Bad / weird formats:
    memset(actual, 'z', sizeof(actual));
    format = "'%%5.4', 987654321            ==> '%5.4'\n";
    VMInspector::sprintf(actual, format, 987654321);
    expectedLiteral = "'%5.4', 987654321            ==> 'ERROR @ \"%5.4' \"\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%5.4' '%%d', 987654321, 4    ==> '%5.4' '%d'\n";
    VMInspector::sprintf(actual, format, 987654321, 4);
    ::sprintf(expected, format, 987654321, 4);
    ASSERT_EQ(strcmp(actual, expected), 0);

    memset(actual, 'z', sizeof(actual));
    format = "'%%w' '%%d', 987654321, 6      ==> '%w' '%d'\n";
    VMInspector::sprintf(actual, format, 987654321, 6);
    ::sprintf(expected, format, 987654321, 6);
    ASSERT_EQ(strcmp(actual, expected), 0);


    // Testing the %b extension:
    memset(actual, 'z', sizeof(actual));
    VMInspector::sprintf(actual, "'%%b', 0          ==> '%b'\n", 0);
    ASSERT_EQ(strcmp(actual, "'%b', 0          ==> 'FALSE'\n"), 0);

    memset(actual, 'z', sizeof(actual));
    VMInspector::sprintf(actual, "'%%b', 1          ==> '%b'\n", 1);
    ASSERT_EQ(strcmp(actual, "'%b', 1          ==> 'TRUE'\n"), 0);

    memset(actual, 'z', sizeof(actual));
    VMInspector::sprintf(actual, "'%%b', -123456789 ==> '%b'\n", -123456789);
    ASSERT_EQ(strcmp(actual, "'%b', -123456789 ==> 'TRUE'\n"), 0);

    memset(actual, 'z', sizeof(actual));
    VMInspector::sprintf(actual, "'%%b', 123456789  ==> '%b'\n", 123456789);
    ASSERT_EQ(strcmp(actual, "'%b', 123456789  ==> 'TRUE'\n"), 0);


    // Testing the %J<x> extensions:
    String str1("Test WTF String");
    String str2("");

    memset(actual, 'z', sizeof(actual));
    VMInspector::sprintf(actual, "'%%Js' is %%s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '%Js' is %s\n",
        &str1, str1.isEmpty() ? "EMPTY" : "NOT EMPTY");
    expectedLiteral = "'%Js' is %s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> 'Test WTF String' is NOT EMPTY\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

    memset(actual, 'z', sizeof(actual));
    VMInspector::sprintf(actual, "'%%Js' is %%s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '%Js' is %s\n",
        &str2, str2.isEmpty() ? "EMPTY" : "NOT EMPTY");
    expectedLiteral = "'%Js' is %s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '' is EMPTY\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

    memset(actual, 'z', sizeof(actual));
    VMInspector::sprintf(actual, "'%%J+s' is %%s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> '%J+s' is %s\n",
        &str1, str1.isEmpty() ? "EMPTY" : "NOT EMPTY");
    expectedLiteral = "'%J+s' is %s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> 'WTF::String \"Test WTF String\"' is NOT EMPTY\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);

    memset(actual, 'z', sizeof(actual));
    VMInspector::sprintf(actual, "'%%J+s' is %%s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> '%J+s' is %s\n",
        &str2, str2.isEmpty() ? "EMPTY" : "NOT EMPTY");
    expectedLiteral = "'%J+s' is %s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> 'WTF::String \"\"' is EMPTY\n";
    ASSERT_EQ(strcmp(actual, expectedLiteral), 0);
#endif
}


TEST(JSC, VMInspectorSnprintf)
{
#if ENABLE(VMINSPECTOR)
    char actual[1024];
    char expected[1024];
    const char* format;
    const char* expectedLiteral;

    size_t size = 1;
    while (size <= 100) {

        // Testing standard default format specifiers:
        // Note: should work just like snprintf. So, we just compare against that.
        memset(actual, 'z', sizeof(actual));
        // The compiler warning flags are configured to expect a literal string for
        // ::snprintf below. So, use a #define for this one case to keep the
        // compiler happy.
#undef LITERAL_FORMAT
#define LITERAL_FORMAT "'%%%%'                         ==> '%%'\n"
        VMInspector::snprintf(actual, size, LITERAL_FORMAT);
        ::snprintf(expected, size, LITERAL_FORMAT);
#undef LITERAL_FORMAT
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%c', 'x'                    ==> '%c'\n";
        VMInspector::snprintf(actual, size, format, 'x');
        ::snprintf(expected, size, format, 'x');
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%*c', 8, 'x'                ==> '%*c'\n";
        VMInspector::snprintf(actual, size, format, 8, 'x');
        ::snprintf(expected, size, format, 8, 'x');
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%s', \"hello world\"          ==> '%s'\n";
        VMInspector::snprintf(actual, size, format, "hello world");
        ::snprintf(expected, size, format, "hello world");
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%*s', 8, \"hello world\"      ==> '%*s'\n";
        VMInspector::snprintf(actual, size, format, 8, "hello world");
        ::snprintf(expected, size, format, 8, "hello world");
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%*s', 8, \"hello\"            ==> '%*s'\n";
        VMInspector::snprintf(actual, size, format, 8, "hello");
        ::snprintf(expected, size, format, 8, "hello");
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%d', 987654321              ==> '%d'\n";
        VMInspector::snprintf(actual, size, format, 987654321);
        ::snprintf(expected, size, format, 987654321);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%u', 4276543210u            ==> '%u'\n";
        VMInspector::snprintf(actual, size, format, 4276543210u);
        ::snprintf(expected, size, format, 4276543210u);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%u', 0xffffffff             ==> '%u'\n";
        VMInspector::snprintf(actual, size, format, 0xffffffff);
        ::snprintf(expected, size, format, 0xffffffff);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%x', 0xffffffff             ==> '%x'\n";
        VMInspector::snprintf(actual, size, format, 0xffffffff);
        ::snprintf(expected, size, format, 0xffffffff);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%p', (void*)0xabcdbabe      ==> '%p'\n";
        VMInspector::snprintf(actual, size, format, (void*)0xabcdbabe);
        ::snprintf(expected, size, format, (void*)0xabcdbabe);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%lu', 1234567890987654321ul ==> '%lu'\n";
        VMInspector::snprintf(actual, size, format, 1234567890987654321ul);
        ::snprintf(expected, size, format, 1234567890987654321ul);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%f', 1234.567               ==> '%f'\n";
        VMInspector::snprintf(actual, size, format, 1234.567);
        ::snprintf(expected, size, format, 1234.567);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%.2f', 1234.567             ==> '%.2f'\n";
        VMInspector::snprintf(actual, size, format, 1234.567);
        ::snprintf(expected, size, format, 1234.567);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%10.2f', 1234.567           ==> '%10.2f'\n";
        VMInspector::snprintf(actual, size, format, 1234.567);
        ::snprintf(expected, size, format, 1234.567);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%010.2f', 1234.567          ==> '%010.2f'\n";
        VMInspector::snprintf(actual, size, format, 1234.567);
        ::snprintf(expected, size, format, 1234.567);
        ASSERT_EQ(strcmp(actual, expected), 0);

        // Bad / weird formats:
        memset(actual, 'z', sizeof(actual));
        format = "'%%5.4', 987654321            ==> '%5.4'\n";
        VMInspector::snprintf(actual, size, format, 987654321);
        expectedLiteral = "'%5.4', 987654321            ==> 'ERROR @ \"%5.4' \"\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%5.4' '%%d', 987654321, 4    ==> '%5.4' '%d'\n";
        VMInspector::snprintf(actual, size, format, 987654321, 4);
        ::snprintf(expected, size, format, 987654321, 4);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        format = "'%%w' '%%d', 987654321, 6      ==> '%w' '%d'\n";
        VMInspector::snprintf(actual, size, format, 987654321, 6);
        ::snprintf(expected, size, format, 987654321, 6);
        ASSERT_EQ(strcmp(actual, expected), 0);


        // Testing the %b extension:
        memset(actual, 'z', sizeof(actual));
        VMInspector::snprintf(actual, size, "'%%b', 0          ==> '%b'\n", 0);
        expectedLiteral = "'%b', 0          ==> 'FALSE'\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        VMInspector::snprintf(actual, size, "'%%b', 1          ==> '%b'\n", 1);
        expectedLiteral = "'%b', 1          ==> 'TRUE'\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        VMInspector::snprintf(actual, size, "'%%b', -123456789 ==> '%b'\n", -123456789);
        expectedLiteral = "'%b', -123456789 ==> 'TRUE'\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        VMInspector::snprintf(actual, size, "'%%b', 123456789  ==> '%b'\n", 123456789);
        expectedLiteral = "'%b', 123456789  ==> 'TRUE'\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        // Testing the %J<x> extensions:
        String str1("Test WTF String");
        String str2("");

        memset(actual, 'z', sizeof(actual));
        VMInspector::snprintf(actual, size, "'%%Js' is %%s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '%Js' is %s\n",
            &str1, str1.isEmpty() ? "EMPTY" : "NOT EMPTY");
        expectedLiteral = "'%Js' is %s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> 'Test WTF String' is NOT EMPTY\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        VMInspector::snprintf(actual, size, "'%%Js' is %%s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '%Js' is %s\n",
            &str2, str2.isEmpty() ? "EMPTY" : "NOT EMPTY");
        expectedLiteral = "'%Js' is %s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\"  ==> '' is EMPTY\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        VMInspector::snprintf(actual, size, "'%%J+s' is %%s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> '%J+s' is %s\n",
            &str1, str1.isEmpty() ? "EMPTY" : "NOT EMPTY");
        expectedLiteral = "'%J+s' is %s, &str1, str1.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> 'WTF::String \"Test WTF String\"' is NOT EMPTY\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        memset(actual, 'z', sizeof(actual));
        VMInspector::snprintf(actual, size, "'%%J+s' is %%s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> '%J+s' is %s\n",
            &str2, str2.isEmpty() ? "EMPTY" : "NOT EMPTY");
        expectedLiteral = "'%J+s' is %s, &str2, str2.isEmpty()?\"EMPTY\":\"NOT EMPTY\" ==> 'WTF::String \"\"' is EMPTY\n";
        ::snprintf(expected, size, "%s", expectedLiteral);
        ASSERT_EQ(strcmp(actual, expected), 0);

        // Test lower sizes more densely, and then space out to larger sizes.
        // We're doing this because the lower sizes might be interesting, but
        // for expediency, we don't want to test at this fine grain resolution
        // for all possible sizes. Hence, we accelerate the rate once we're
        // pass the interesting small sizes.
        if (size <= 5)
            size++;
        else
            size += 4;
    }
#endif
}
