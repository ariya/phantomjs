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
    File Name:          proto_2.js
    Section:
    Description:        new PrototypeObject

    This tests Object Hierarchy and Inheritance, as described in the document
    Object Hierarchy and Inheritance in JavaScript, last modified on 12/18/97
    15:19:34 on http://devedge.netscape.com/.  Current URL:
    http://devedge.netscape.com/docs/manuals/communicator/jsobj/contents.htm

    This tests the syntax ObjectName.prototype = new PrototypeObject using the
    Employee example in the document referenced above.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "proto_2";
    var VERSION = "JS1_3";
    var TITLE   = "new PrototypeObject";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

function Employee () {
     this.name = "";
     this.dept = "general";
}
function Manager () {
     this.reports = [];
}
Manager.prototype = new Employee();

function WorkerBee () {
     this.projects = new Array();
}

WorkerBee.prototype = new Employee;

function SalesPerson () {
    this.dept = "sales";
    this.quota = 100;
}
SalesPerson.prototype = new WorkerBee;

function Engineer () {
    this.dept = "engineering";
    this.machine = "";
}
Engineer.prototype = new WorkerBee;


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

    var employee    = new Employee();
    var manager     = new Manager();
    var workerbee   = new WorkerBee();
    var salesperson = new SalesPerson();
    var engineer    = new Engineer();

    testcases[tc++] = new TestCase( SECTION,
                                    "employee.__proto__ == Employee.prototype",
                                    true,
                                    employee.__proto__ == Employee.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "manager.__proto__ == Manager.prototype",
                                    true,
                                    manager.__proto__ == Manager.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "workerbee.__proto__ == WorkerBee.prototype",
                                    true,
                                    workerbee.__proto__ == WorkerBee.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "salesperson.__proto__ == SalesPerson.prototype",
                                    true,
                                    salesperson.__proto__ == SalesPerson.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "engineer.__proto__ == Engineer.prototype",
                                    true,
                                    engineer.__proto__ == Engineer.prototype );

    test();

