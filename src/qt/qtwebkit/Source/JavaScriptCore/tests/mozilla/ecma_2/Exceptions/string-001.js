/**
    File Name:          string-001.js
    Corresponds To:     15.5.4.2-2-n.js
    ECMA Section:       15.5.4.2 String.prototype.toString()

    Description:        Returns this string value.  Note that, for a String
                        object, the toString() method happens to return the same
                        thing as the valueOf() method.

                        The toString function is not generic; it generates a
                        runtime error if its this value is not a String object.
                        Therefore it connot be transferred to the other kinds of
                        objects for use as a method.

    Author:             christine@netscape.com
    Date:               1 october 1997
*/
    var SECTION = "string-001";
    var VERSION = "JS1_4";
    var TITLE   = "String.prototype.toString";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        OBJECT = new Object();
        OBJECT.toString = String.prototype.toString();
        result = OBJECT.toString();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "OBJECT = new Object; "+
        " OBJECT.toString = String.prototype.toString; OBJECT.toString()" +
        " (threw " + exception +")",
        expect,
        result );

    test();

