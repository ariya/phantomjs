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
    File Name:          10.1.6
    ECMA Section:       Activation Object
    Description:

    If the function object being invoked has an arguments property, let x be
    the value of that property; the activation object is also given an internal
    property [[OldArguments]] whose initial value is x; otherwise, an arguments
    property is created for the function object but the activation object is
    not given an [[OldArguments]] property. Next, arguments object described
    below (the same one stored in the arguments property of the activation
    object) is used as the new value of the arguments property of the function
    object. This new value is installed even if the arguments property already
    exists and has the ReadOnly attribute (as it will for native Function
    objects). (These actions are taken to provide compatibility with a form of
    program syntax that is now discouraged: to access the arguments object for
    function f within the body of f by using the expression f.arguments.
    The recommended way to access the arguments object for function f within
    the body of f is simply to refer to the variable arguments.)

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "10.1.6";
    var VERSION = "ECMA_1";
    startTest();
    var TITLE   = "Activation Object";

    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

    var arguments = "FAILED!";

    var ARG_STRING = "value of the argument property";

    testcases[tc++] = new TestCase( SECTION,
                                    "(new TestObject(0,1,2,3,4,5)).length",
                                    6,
                                    (new TestObject(0,1,2,3,4,5)).length );

    for ( i = 0; i < 6; i++ ) {

        testcases[tc++] = new TestCase( SECTION,
                                        "(new TestObject(0,1,2,3,4,5))["+i+"]",
                                        i,
                                        (new TestObject(0,1,2,3,4,5))[i]);
    }


    //    The current object already has an arguments property.

    testcases[tc++] = new TestCase( SECTION,
                                    "(new AnotherTestObject(1,2,3)).arguments",
                                    ARG_STRING,
                                    (new AnotherTestObject(1,2,3)).arguments );

    //  The function invoked with [[Call]]

    testcases[tc++] = new TestCase( SECTION,
                                    "TestFunction(1,2,3)",
                                    ARG_STRING,
                                    TestFunction() + '' );


    test();



function Prototype() {
    this.arguments = ARG_STRING;
}
function TestObject() {
    this.__proto__ = new Prototype();
    return arguments;
}
function AnotherTestObject() {
    this.__proto__ = new Prototype();
    return this;
}
function TestFunction() {
    arguments = ARG_STRING;
    return arguments;
}
function AnotherTestFunction() {
    this.__proto__ = new Prototype();
    return this;
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
