/**
 *  File Name:          LexicalConventions/regexp-literals-002.js
 *  ECMA Section:       7.8.5
 *  Description:        Based on ECMA 2 Draft 8 October 1999
 *
 *  Author:             christine@netscape.com
 *  Date:               19 February 1999
 */
    var SECTION = "LexicalConventions/regexp-literals-002.js";
    var VERSION = "ECMA_2";
    var TITLE   = "Regular Expression Literals";

    startTest();

    // A regular expression literal represents an object of type RegExp.

    AddTestCase(
        "// A regular expression literal represents an object of type RegExp.",
        "true",
        (/x*/ instanceof RegExp).toString() );

    test();
