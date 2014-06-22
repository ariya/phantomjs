/**
    File Name:          lexical-001.js
    CorrespondsTo:      ecma/LexicalConventions/7.2.js
    ECMA Section:       7.2 Line Terminators
    Description:        - readability
                        - separate tokens
                        - may occur between any two tokens
                        - cannot occur within any token, not even a string
                        - affect the process of automatic semicolon insertion.

                        white space characters are:
                        unicode     name            formal name     string representation
                        \u000A      line feed       <LF>            \n
                        \u000D      carriage return <CR>            \r

                        this test uses onerror to capture line numbers.  because
                        we use on error, we can only have one test case per file.

    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "lexical-001";
    var VERSION = "JS1_4";
    var TITLE   = "Line Terminators";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        result = eval("\r\n\expect");
    } catch ( e ) {
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "OBJECT = new Object; result = new OBJECT()" +
        " (threw " + exception +")",
        expect,
        result );

    test();
