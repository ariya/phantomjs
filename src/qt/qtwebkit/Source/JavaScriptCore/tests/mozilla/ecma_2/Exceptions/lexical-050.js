/**
    File Name:          lexical-050.js
    Corresponds to:     7.8.2-1-n.js
    ECMA Section:       7.8.2 Examples of Automatic Semicolon Insertion
    Description:        compare some specific examples of the automatic
                        insertion rules in the EMCA specification.
    Author:             christine@netscape.com
    Date:               15 september 1997
*/

    var SECTION = "lexical-050";
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
        eval("{ 1 2 } 3");
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "{ 1 2 } 3" +
        " (threw " + exception +")",
        expect,
        result );

    test();



