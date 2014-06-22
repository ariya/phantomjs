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
    File Name:          10.1.8-2
    ECMA Section:       Arguments Object
    Description:

    When control enters an execution context for declared function code,
    anonymous code, or implementation-supplied code, an arguments object is
    created and initialized as follows:

    The [[Prototype]] of the arguments object is to the original Object
    prototype object, the one that is the initial value of Object.prototype
    (section 15.2.3.1).

    A property is created with name callee and property attributes {DontEnum}.
    The initial value of this property is the function object being executed.
    This allows anonymous functions to be recursive.

    A property is created with name length and property attributes {DontEnum}.
    The initial value of this property is the number of actual parameter values
    supplied by the caller.

    For each non-negative integer, iarg, less than the value of the length
    property, a property is created with name ToString(iarg) and property
    attributes { DontEnum }. The initial value of this property is the value
    of the corresponding actual parameter supplied by the caller. The first
    actual parameter value corresponds to iarg = 0, the second to iarg = 1 and
    so on. In the case when iarg is less than the number of formal parameters
    for the function object, this property shares its value with the
    corresponding property of the activation object. This means that changing
    this property changes the corresponding property of the activation object
    and vice versa. The value sharing mechanism depends on the implementation.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "10.1.8-2";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Arguments Object";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

//  Tests for anonymous functions

    var GetCallee       = new Function( "var c = arguments.callee; return c" );
    var GetArguments    = new Function( "var a = arguments; return a" );
    var GetLength       = new Function( "var l = arguments.length; return l" );

    var ARG_STRING = "value of the argument property";

    testcases[tc++] = new TestCase( SECTION,
                                    "GetCallee()",
                                    GetCallee,
                                    GetCallee() );

    var LIMIT = 100;

    for ( var i = 0, args = "" ; i < LIMIT; i++ ) {
        args += String(i) + ( i+1 < LIMIT ? "," : "" );

    }

    var LENGTH = eval( "GetLength("+ args +")" );

    testcases[tc++] = new TestCase( SECTION,
                                    "GetLength("+args+")",
                                    100,
                                    LENGTH );

    var ARGUMENTS = eval( "GetArguments( " +args+")" );

    for ( var i = 0; i < 100; i++ ) {
        testcases[tc++] = new TestCase( SECTION,
                                        "GetArguments("+args+")["+i+"]",
                                        i,
                                        ARGUMENTS[i] );
    }

    test();

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
