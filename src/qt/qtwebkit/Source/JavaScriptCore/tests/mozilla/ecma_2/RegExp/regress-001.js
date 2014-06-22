/**
 *  File Name:          RegExp/regress-001.js
 *  ECMA Section:       N/A
 *  Description:        Regression test case:
 *  JS regexp anchoring on empty match bug
 *  http://bugzilla.mozilla.org/show_bug.cgi?id=2157
 *
 *  Author:             christine@netscape.com
 *  Date:               19 February 1999
 */
    var SECTION = "RegExp/hex-001.js";
    var VERSION = "ECMA_2";
    var TITLE   = "JS regexp anchoring on empty match bug";
    var BUGNUMBER = "http://bugzilla.mozilla.org/show_bug.cgi?id=2157";

    startTest();

    AddRegExpCases( /a||b/(''),
                    "//a||b/('')",
                    1,
                    [''] );

    test();

function AddRegExpCases( regexp, str_regexp, length, matches_array ) {

    AddTestCase(
        "( " + str_regexp + " ).length",
        regexp.length,
        regexp.length );


    for ( var matches = 0; matches < matches_array.length; matches++ ) {
        AddTestCase(
            "( " + str_regexp + " )[" + matches +"]",
            matches_array[matches],
            regexp[matches] );
    }
}
