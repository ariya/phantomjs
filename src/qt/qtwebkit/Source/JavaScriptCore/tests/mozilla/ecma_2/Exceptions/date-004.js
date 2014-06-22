/**
    File Name:          date-004.js
    Corresponds To:     15.9.5.4-2-n.js
    ECMA Section:       15.9.5.4-1 Date.prototype.getTime
    Description:

    1.  If the this value is not an object whose [[Class]] property is "Date",
        generate a runtime error.
    2.  Return this time value.
    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "date-004";
    var VERSION = "JS1_4";
    var TITLE   = "Date.prototype.getTime";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

    var result = "Failed";
    var exception = "No exception thrown";
    var expect = "Passed";

    try {
        var MYDATE = new MyDate();
        result = MYDATE.getTime();
    } catch ( e ) {
        result = expect;
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "MYDATE = new MyDate(); MYDATE.getTime()" +
        " (threw " + exception +")",
        expect,
        result );

    test();

function MyDate( value ) {
    this.value = value;
    this.getTime = Date.prototype.getTime;
}
