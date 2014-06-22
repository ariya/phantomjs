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
    File Name:          15.4.2.2-2.js
    ECMA Section:       15.4.2.2 new Array(len)

    Description:        This description only applies of the constructor is
                        given two or more arguments.

                        The [[Prototype]] property of the newly constructed
                        object is set to the original Array prototype object,
                        the one that is the initial value of Array.prototype(0)
                        (15.4.3.1).

                        The [[Class]] property of the newly constructed object
                        is set to "Array".

                        If the argument len is a number, then the length
                        property  of the newly constructed object is set to
                        ToUint32(len).

                        If the argument len is not a number, then the length
                        property of the newly constructed object is set to 1
                        and the 0 property of the newly constructed object is
                        set to len.

                        This file tests length of the newly constructed array
                        when len is not a number.

    Author:             christine@netscape.com
    Date:               7 october 1997
*/
    var SECTION = "15.4.2.2-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Array Constructor:  new Array( len )";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();
    var item = 0;

    array[item++] = new TestCase( SECTION,	"(new Array(new Number(1073741823))).length",   1,      (new Array(new Number(1073741823))).length );
    array[item++] = new TestCase( SECTION,	"(new Array(new Number(0))).length",            1,      (new Array(new Number(0))).length );
    array[item++] = new TestCase( SECTION,	"(new Array(new Number(1000))).length",         1,      (new Array(new Number(1000))).length );
    array[item++] = new TestCase( SECTION,	"(new Array('mozilla, larryzilla, curlyzilla')).length", 1,  (new Array('mozilla, larryzilla, curlyzilla')).length );
    array[item++] = new TestCase( SECTION,	"(new Array(true)).length",                     1,      (new Array(true)).length );
    array[item++] = new TestCase( SECTION,	"(new Array(false)).length",                    1,      (new Array(false)).length);
    array[item++] = new TestCase( SECTION,	"(new Array(new Boolean(true)).length",         1,      (new Array(new Boolean(true))).length );
    array[item++] = new TestCase( SECTION,	"(new Array(new Boolean(false)).length",        1,      (new Array(new Boolean(false))).length );
    return ( array );
}
function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );
        testcases[tc].reason += ( testcases[tc].passed )
                             ? ""
                             : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
