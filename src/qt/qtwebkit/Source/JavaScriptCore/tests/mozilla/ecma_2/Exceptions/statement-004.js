/**
    File Name:          statement-004.js
    Corresponds To:     12.6.3-1.js
    ECMA Section:       12.6.3 The for...in Statement
    Description:
    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "statement-004";
    var VERSION = "JS1_4";
    var TITLE   = "The for..in statment";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();
    var tc = 0;

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        var o = new MyObject();

        eval("for ( \"a\" in o) {\n"
            + "result += this[p];\n"
            + "}");

    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "bad left-hand side expression" +
        " (threw " + exception +")",
        expect,
        result );

    test();


function MyObject() {
    this.value = 2;
    this[0] = 4;
    return this;
}
