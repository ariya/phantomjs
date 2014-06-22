/**
    File Name:          string-002.js
    Corresponds To:     15.5.4.3-3-n.js
    ECMA Section:       15.5.4.3 String.prototype.valueOf()

    Description:        Returns this string value.

                        The valueOf function is not generic; it generates a
                        runtime error if its this value is not a String object.
                        Therefore it connot be transferred to the other kinds of
                        objects for use as a method.

    Author:             christine@netscape.com
    Date:               1 october 1997
*/
    var SECTION = "string-002";
    var VERSION = "JS1_4";
    var TITLE   = "String.prototype.valueOf";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        var OBJECT =new Object();
        OBJECT.valueOf = String.prototype.valueOf;
        result = OBJECT.valueOf();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "OBJECT = new Object; OBJECT.valueOf = String.prototype.valueOf;"+
        "result = OBJECT.valueOf();" +
        " (threw " + exception +")",
        expect,
        result );

    test();


