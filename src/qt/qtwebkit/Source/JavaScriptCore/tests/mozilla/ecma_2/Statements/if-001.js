/**
 *  File Name:          if-001.js
 *  ECMA Section:
 *  Description:        The if statement
 *
 *  Verify that assignment in the if expression is evaluated correctly.
 *  Verifies the fix for bug http://scopus/bugsplat/show_bug.cgi?id=148822.
 *
 *  Author:             christine@netscape.com
 *  Date:               28 August 1998
 */
    var SECTION = "for-001";
    var VERSION = "ECMA_2";
    var TITLE   = "The if  statement";
    var BUGNUMBER="148822";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var a = 0;
    var b = 0;
    var result = "passed";

    if ( a = b ) {
        result = "failed:  a = b should return 0";
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "if ( a = b ), where a and b are both equal to 0",
        "passed",
        result );


    test();

