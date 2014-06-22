/**
    File Name:          lexical-047.js
    Corresponds To:     7.8.1-7-n.js
    ECMA Section:       7.8.1
    Description:
    Author:             christine@netscape.com
    Date:               15 september 1997
*/

    var SECTION = "lexical-047";
    var VERSION = "JS1_4";
    var TITLE   = "for loops";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        var counter = 0;
        eval("for ( counter = 0\n"
             + "counter <= 1\n"
             + "counter++ )\n"
             + "{\n"
             + "result += \":  got to inner loop\";\n"
             + "}\n");

    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "line breaks within a for expression" +
        " (threw " + exception +")",
        expect,
        result );

    test();


