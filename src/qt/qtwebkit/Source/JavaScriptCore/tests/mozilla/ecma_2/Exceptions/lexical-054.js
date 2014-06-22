/**
    File Name:          lexical-054.js
    Corresponds to:     7.8.2-7-n.js
    ECMA Section:       7.8.2 Examples of Automatic Semicolon Insertion
    Description:        compare some specific examples of the automatic
                        insertion rules in the EMCA specification.
    Author:             christine@netscape.com
    Date:               15 september 1997
*/

    var SECTION = "lexical-054";
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
        a=0;
        b=1;
        c=2;
        d=3;
        eval("if (a > b)\nelse c = d");
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "if (a > b)\nelse c = d" +
        " (threw " + exception +")",
        expect,
        result );

    test();
