/**
 *  File Name:          LexicalConventions/regexp-literals-001.js
 *  ECMA Section:       7.8.5
 *  Description:
 *
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "LexicalConventions/regexp-literals-001.js";
    var VERSION = "ECMA_2";
    var TITLE   = "Regular Expression Literals";

    startTest();

    // Regular Expression Literals may not be empty; // should be regarded
    // as a comment, not a RegExp literal.

    s = //;

    "passed";

    AddTestCase(
        "// should be a comment, not a regular expression literal",
        "passed",
        String(s));

    AddTestCase(
        "// typeof object should be type of object declared on following line",
        "passed",
        (typeof s) == "string" ? "passed" : "failed" );

    AddTestCase(
        "// should not return an object of the type RegExp",
        "passed",
        (typeof s == "object") ? "failed" : "passed" );

    test();
