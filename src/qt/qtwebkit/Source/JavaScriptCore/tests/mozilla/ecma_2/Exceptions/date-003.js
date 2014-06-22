/**
    File Name:          date-003.js
    Corresponds To      15.9.5.3-1.js
    ECMA Section:       15.9.5.3-1 Date.prototype.valueOf
    Description:

    The valueOf function returns a number, which is this time value.

    The valueOf function is not generic; it generates a runtime error if
    its this value is not a Date object.  Therefore it cannot be transferred
    to other kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "date-003";
    var VERSION = "JS1_4";
    var TITLE   = "Date.prototype.valueOf";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        var OBJ = new MyObject( new Date(0) );
        result = OBJ.valueOf();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "OBJ = new MyObject( new Date(0)); OBJ.valueOf()" +
        " (threw " + exception +")",
        expect,
        result );

    test();

function MyObject( value ) {
    this.value = value;
    this.valueOf = Date.prototype.valueOf;
//  The following line causes an infinte loop
//    this.toString = new Function( "return this+\"\";");
    return this;
}
