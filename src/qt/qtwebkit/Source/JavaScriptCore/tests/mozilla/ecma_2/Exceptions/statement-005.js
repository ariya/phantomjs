/**
    File Name:          statement-005.js
    Corresponds To:     12.6.3-8-n.js
    ECMA Section:       12.6.3 The for...in Statement
    Description:
    Author:             christine@netscape.com
    Date:               11 september 1997
*/
    var SECTION = "statement-005";
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
        result = 0;

        eval("for (1 in o) {\n"
             + "result += this[p];"
             + "}\n");
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
