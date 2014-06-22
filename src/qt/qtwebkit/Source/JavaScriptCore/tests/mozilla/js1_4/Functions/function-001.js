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
 *  File Name:          function-001.js
 *  Description:
 *
 * http://scopus.mcom.com/bugsplat/show_bug.cgi?id=324455
 *
 *  Earlier versions of JavaScript supported access to the arguments property
 *  of the function object. This property held the arguments to the function.
 *  function f() {
 *      return f.arguments[0];    // deprecated
 *  }
 *  var x = f(3);    // x will be 3
 *
 * This feature is not a part of the final ECMA standard. Instead, scripts
 * should simply use just "arguments":
 *
 * function f() {
 *    return arguments[0];    // okay
 * }
 *
 * var x = f(3);    // x will be 3
 *
 * Again, this feature was motivated by performance concerns. Access to the
 * arguments property is not threadsafe, which is of particular concern in
 * server environments. Also, the compiler can generate better code for
 * functions because it can tell when the arguments are being accessed only by
 * name and avoid setting up the arguments object.
 *
 *  Author:             christine@netscape.com
 *  Date:               11 August 1998
 */
    var SECTION = "function-001.js";
    var VERSION = "JS1_4";
    var TITLE   = "Accessing the arguments property of a function object";
    var BUGNUMBER="324455";
    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    testcases[tc++] = new TestCase(
        SECTION,
        "return function.arguments",
        "P",
        TestFunction_2("P", "A","S","S")[0] +"");


    testcases[tc++] = new TestCase(
        SECTION,
        "return arguments",
        "P",
        TestFunction_1( "P", "A", "S", "S" )[0] +"");

    testcases[tc++] = new TestCase(
        SECTION,
        "return arguments when function contains an arguments property",
        "PASS",
        TestFunction_3( "P", "A", "S", "S" ) +"");

    testcases[tc++] = new TestCase(
        SECTION,
        "return function.arguments when function contains an arguments property",
        "PASS",
        TestFunction_4( "P", "A", "S", "S" ) +"");

    test();

    function TestFunction_1( a, b, c, d, e ) {
        return arguments;
    }

    function TestFunction_2( a, b, c, d, e ) {
        return TestFunction_2.arguments;
    }

    function TestFunction_3( a, b, c, d, e ) {
        var arguments = "PASS";
        return arguments;
    }

    function TestFunction_4( a, b, c, d, e ) {
        var arguments = "FAIL";
        return Array.prototype.join.call(TestFunction_4.arguments, "");
    }

