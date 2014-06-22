/**
    File Name:          date-001.js
    Corresponds To:     15.9.5.2-2.js
    ECMA Section:       15.9.5.2 Date.prototype.toString
    Description:
    This function returns a string value. The contents of the string are
    implementation dependent, but are intended to represent the Date in a
    convenient, human-readable form in the current time zone.

    The toString function is not generic; it generates a runtime error if its
    this value is not a Date object. Therefore it cannot be transferred to
    other kinds of objects for use as a method.


    This verifies that calling toString on an object that is not a string
    generates a runtime error.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "date-001";
    var VERSION = "JS1_4";
    var TITLE   = "Date.prototype.toString";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
    var OBJ = new MyObject( new Date(0) );
        result = OBJ.toString();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "OBJECT = new MyObject( new Date(0)) ; result = OBJ.toString()" +
        " (threw " + exception +")",
        expect,
        result );

    test();

function MyObject( value ) {
    this.value = value;
    this.valueOf = new Function( "return this.value" );
    this.toString = Date.prototype.toString;
    return this;
}
