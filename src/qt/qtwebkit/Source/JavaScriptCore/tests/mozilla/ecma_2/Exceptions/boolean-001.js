/**
    File Name:          boolean-001.js
    Description:        Corresponds to ecma/Boolean/15.6.4.2-4-n.js

                        The toString function is not generic; it generates
                        a runtime error if its this value is not a Boolean
                        object.  Therefore it cannot be transferred to other
                        kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               june 27, 1997
*/
    var SECTION = "boolean-001.js";
    var VERSION = "JS1_4";
    var TITLE   = "Boolean.prototype.toString()";
    startTest();
    writeHeaderToLog( SECTION +" "+ TITLE );

    var tc = 0;
    var testcases = new Array();

    var exception = "No exception thrown";
    var result = "Failed";

    var TO_STRING = Boolean.prototype.toString;

    try {
        var s = new String("Not a Boolean");
        s.toString = TO_STRING;
        s.toString();
    } catch ( e ) {
        result = "Passed!";
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "Assigning Boolean.prototype.toString to a String object "+
        "(threw " +exception +")",
        "Passed!",
        result );

    test();
