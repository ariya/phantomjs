/**
    File Name:          lexical-051.js
    Corresponds to:     7.8.2-3-n.js
    ECMA Section:       7.8.2 Examples of Automatic Semicolon Insertion
    Description:        compare some specific examples of the automatic
                        insertion rules in the EMCA specification.
    Author:             christine@netscape.com
    Date:               15 september 1997
*/

    var SECTION = "lexical-051";
    var VERSION = "JS1_4";
    var TITLE   = "Examples of Automatic Semicolon Insertion";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        eval("for (a; b\n) result += \": got to inner loop\";")
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "for (a; b\n)" +
        " (threw " + exception +")",
        expect,
        result );

    test();



