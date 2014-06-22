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
    File Name:          11.1.1.js
    ECMA Section:       11.1.1 The this keyword
    Description:

    The this keyword evaluates to the this value of the execution context.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
    var SECTION = "11.1.1";
    var VERSION = "ECMA_1";
    startTest();

    writeHeaderToLog( SECTION + " The this keyword");

    var testcases = new Array();
    var item = 0;

    var GLOBAL_OBJECT = this.toString();

    // this in global code and eval(this) in global code should return the global object.

    testcases[item++] = new TestCase(   SECTION,
                                        "Global Code: this.toString()",
                                        GLOBAL_OBJECT,
                                        this.toString() );

    testcases[item++] = new TestCase(   SECTION,
                                        "Global Code:  eval('this.toString()')",
                                        GLOBAL_OBJECT,
                                        eval('this.toString()') );

    // this in anonymous code called as a function should return the global object.

    testcases[item++] = new TestCase(   SECTION,
                                        "Anonymous Code: var MYFUNC = new Function('return this.toString()'); MYFUNC()",
                                        GLOBAL_OBJECT,
                                        eval("var MYFUNC = new Function('return this.toString()'); MYFUNC()") );

    // eval( this ) in anonymous code called as a function should return that function's activation object

    testcases[item++] = new TestCase(   SECTION,
                                        "Anonymous Code: var MYFUNC = new Function('return (eval(\"this.toString()\")'); (MYFUNC()).toString()",
                                        GLOBAL_OBJECT,
                                        eval("var MYFUNC = new Function('return eval(\"this.toString()\")'); (MYFUNC()).toString()") );

    // this and eval( this ) in anonymous code called as a constructor should return the object

    testcases[item++] = new TestCase(   SECTION,
                                        "Anonymous Code: var MYFUNC = new Function('this.THIS = this'); ((new MYFUNC()).THIS).toString()",
                                        "[object Object]",
                                        eval("var MYFUNC = new Function('this.THIS = this'); ((new MYFUNC()).THIS).toString()") );

    testcases[item++] = new TestCase(   SECTION,
                                        "Anonymous Code: var MYFUNC = new Function('this.THIS = this'); var FUN1 = new MYFUNC(); FUN1.THIS == FUN1",
                                        true,
                                        eval("var MYFUNC = new Function('this.THIS = this'); var FUN1 = new MYFUNC(); FUN1.THIS == FUN1") );

    testcases[item++] = new TestCase(   SECTION,
                                        "Anonymous Code: var MYFUNC = new Function('this.THIS = eval(\"this\")'); ((new MYFUNC().THIS).toString()",
                                        "[object Object]",
                                        eval("var MYFUNC = new Function('this.THIS = eval(\"this\")'); ((new MYFUNC()).THIS).toString()") );

    testcases[item++] = new TestCase(   SECTION,
                                        "Anonymous Code: var MYFUNC = new Function('this.THIS = eval(\"this\")'); var FUN1 = new MYFUNC(); FUN1.THIS == FUN1",
                                        true,
                                        eval("var MYFUNC = new Function('this.THIS = eval(\"this\")'); var FUN1 = new MYFUNC(); FUN1.THIS == FUN1") );

    // this and eval(this) in function code called as a function should return the global object.
    testcases[item++] = new TestCase(   SECTION,
                                        "Function Code:  ReturnThis()",
                                        GLOBAL_OBJECT,
                                        ReturnThis() );

    testcases[item++] = new TestCase(   SECTION,
                                        "Function Code:  ReturnEvalThis()",
                                        GLOBAL_OBJECT,
                                        ReturnEvalThis() );

    //  this and eval(this) in function code called as a contructor should return the object.
    testcases[item++] = new TestCase(   SECTION,
                                        "var MYOBJECT = new ReturnThis(); MYOBJECT.toString()",
                                        "[object Object]",
                                        eval("var MYOBJECT = new ReturnThis(); MYOBJECT.toString()") );

    testcases[item++] = new TestCase(   SECTION,
                                        "var MYOBJECT = new ReturnEvalThis(); MYOBJECT.toString()",
                                        "[object Object]",
                                        eval("var MYOBJECT = new ReturnEvalThis(); MYOBJECT.toString()") );



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
function ReturnThis() {
    return this.toString();
}
function ReturnEvalThis() {
    return( eval("this.toString()") );
}