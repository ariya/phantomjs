/**
    File Name:          boolean-001.js
    Description:        Corresponds to ecma/Boolean/15.6.4.3-4-n.js

                        15.6.4.3 Boolean.prototype.valueOf()
                        Returns this boolean value.

                        The valueOf function is not generic; it generates
                        a runtime error if its this value is not a Boolean
                        object.  Therefore it cannot be transferred to other
                        kinds of objects for use as a method.

    Author:             christine@netscape.com
    Date:               09 september 1998
*/
    var SECTION = "boolean-002.js";
    var VERSION = "JS1_4";
    var TITLE   = "Boolean.prototype.valueOf()";
    startTest();
    writeHeaderToLog( SECTION +" "+ TITLE );

    var tc = 0;
    var testcases = new Array();

    var exception = "No exception thrown";
    var result = "Failed";

    var VALUE_OF = Boolean.prototype.valueOf;

    try {
        var s = new String("Not a Boolean");
        s.valueOf = VALUE_0F;
        s.valueOf();
    } catch ( e ) {
        result = "Passed!";
        exception = e.toString();
    }

    testcases[tc++] = new TestCase(
        SECTION,
        "Assigning Boolean.prototype.valueOf to a String object "+
        "(threw " +exception +")",
        "Passed!",
        result );

    test();

