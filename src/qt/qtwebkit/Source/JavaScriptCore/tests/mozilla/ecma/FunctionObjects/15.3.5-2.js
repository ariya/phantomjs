/* The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is Mozilla Communicator client code, released March
 * 31, 1998.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation. Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * 
 */
/**
    File Name:          15.3.5-1.js
    ECMA Section:       15.3.5 Properties of Function Instances
                        new Function(p1, p2, ..., pn, body )

    Description:

    15.3.5.1 length

    The value of the length property is usually an integer that indicates
    the "typical" number of arguments expected by the function. However,
    the language permits the function to be invoked with some other number
    of arguments. The behavior of a function when invoked on a number of
    arguments other than the number specified by its length property depends
    on the function.

    15.3.5.2 prototype
    The value of the prototype property is used to initialize the internal [[
    Prototype]] property of a newly created object before the Function object
    is invoked as a constructor for that newly created object.

    15.3.5.3 arguments

    The value of the arguments property is normally null if there is no
    outstanding invocation of the function in progress (that is, the function has been called
    but has not yet returned). When a non-internal Function object (15.3.2.1) is invoked, its
    arguments property is "dynamically bound" to a newly created object that contains the
    arguments on which it was invoked (see 10.1.6 and 10.1.8). Note that the use of this
    property is discouraged; it is provided principally for compatibility with existing old code.

    Author:             christine@netscape.com
    Date:               28 october 1997

*/

    var SECTION = "15.3.5-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Properties of Function Instances";

    writeHeaderToLog( SECTION + " "+TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    var MyObject = new Function( 'a', 'b', 'c', 'this.a = a; this.b = b; this.c = c; this.value = a+b+c; this.valueOf = new Function( "return this.value" )' );

    array[item++] = new TestCase( SECTION, "MyObject.length",                       3,          MyObject.length );
    array[item++] = new TestCase( SECTION, "typeof MyObject.prototype",             "object",   typeof MyObject.prototype );
    array[item++] = new TestCase( SECTION, "typeof MyObject.prototype.constructor", "function", typeof MyObject.prototype.constructor );
    array[item++] = new TestCase( SECTION, "MyObject.arguments",                     null,       MyObject.arguments );

    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+ testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}

