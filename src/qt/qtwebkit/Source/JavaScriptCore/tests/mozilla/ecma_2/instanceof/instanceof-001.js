/**
    File Name:          instanceof-1.js
    ECMA Section:
    Description:        instanceof operator

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "";
    var VERSION = "ECMA_2";
    var TITLE   = "instanceof operator";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var b = new Boolean();

    testcases[tc++] = new TestCase( SECTION,
                                    "var b = new Boolean(); b instanceof Boolean",
                                    true,
                                    b instanceof Boolean );

    testcases[tc++] = new TestCase( SECTION,
                                    "b instanceof Object",
                                    true,
                                    b instanceof Object );

    test();
