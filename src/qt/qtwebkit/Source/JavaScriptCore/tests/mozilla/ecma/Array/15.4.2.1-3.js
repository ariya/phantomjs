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
    File Name:          15.4.2.1-3.js
    ECMA Section:       15.4.2.1 new Array( item0, item1, ... )
    Description:        This description only applies of the constructor is
                        given two or more arguments.

                        The [[Prototype]] property of the newly constructed
                        object is set to the original Array prototype object,
                        the one that is the initial value of Array.prototype
                        (15.4.3.1).

                        The [[Class]] property of the newly constructed object
                        is set to "Array".

                        The length property of the newly constructed object is
                        set to the number of arguments.

                        The 0 property of the newly constructed object is set
                        to item0... in general, for as many arguments as there
                        are, the k property of the newly constructed object is
                        set to argument k, where the first argument is
                        considered to be argument number 0.

                        This test stresses the number of arguments presented to
                        the Array constructor.  Should support up to Math.pow
                        (2,32) arguments, since that is the maximum length of an
                        ECMAScript array.

                        ***Change TEST_LENGTH to Math.pow(2,32) when larger array
                        lengths are supported.

    Author:             christine@netscape.com
    Date:               7 october 1997
*/
    var SECTION = "15.4.2.1-3";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "The Array Constructor:  new Array( item0, item1, ...)";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = getTestCases();
    test();

function getTestCases() {
    var array = new Array();

    var TEST_STRING = "new Array(";
    var ARGUMENTS = ""
    var TEST_LENGTH = Math.pow(2,10); //Math.pow(2,32);

    for ( var index = 0; index < TEST_LENGTH; index++ ) {
        ARGUMENTS += index;
        ARGUMENTS += (index == (TEST_LENGTH-1) ) ? "" : ",";
    }

    TEST_STRING += ARGUMENTS + ")";

    TEST_ARRAY = eval( TEST_STRING );

    for ( item = 0; item < TEST_LENGTH; item++ ) {
        array[item] = new TestCase( SECTION, "TEST_ARRAY["+item+"]",     item,    TEST_ARRAY[item] );
    }

    array[item++] = new TestCase( SECTION, "new Array( ["+TEST_LENGTH+" arguments] ) +''",  ARGUMENTS,          TEST_ARRAY +"" );
    array[item++] = new TestCase( SECTION, "TEST_ARRAY.toString",                           Array.prototype.toString,   TEST_ARRAY.toString );
    array[item++] = new TestCase( SECTION, "TEST_ARRAY.join",                               Array.prototype.join,       TEST_ARRAY.join );
    array[item++] = new TestCase( SECTION, "TEST_ARRAY.sort",                               Array.prototype.sort,       TEST_ARRAY.sort );
    array[item++] = new TestCase( SECTION, "TEST_ARRAY.reverse",                            Array.prototype.reverse,    TEST_ARRAY.reverse );
    array[item++] = new TestCase( SECTION, "TEST_ARRAY.length",                             TEST_LENGTH,        TEST_ARRAY.length );
    array[item++] = new TestCase( SECTION,
                                    "TEST_ARRAY.toString = Object.prototype.toString; TEST_ARRAY.toString()",
                                    "[object Array]",
                                    eval("TEST_ARRAY.toString = Object.prototype.toString; TEST_ARRAY.toString()") );

    return ( array );
}

function test() {
    for ( tc=0; tc < testcases.length; tc++ ) {
        testcases[tc].passed = writeTestCaseResult(
                            testcases[tc].expect,
                            testcases[tc].actual,
                            testcases[tc].description +" = "+
                            testcases[tc].actual );

        testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
    }
    stopTest();
    return ( testcases );
}
