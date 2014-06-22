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
    File Name:          proto_11.js
    Section:
    Description:        Global Information in Constructors

    This tests Object Hierarchy and Inheritance, as described in the document
    Object Hierarchy and Inheritance in JavaScript, last modified on 12/18/97
    15:19:34 on http://devedge.netscape.com/.  Current URL:
    http://devedge.netscape.com/docs/manuals/communicator/jsobj/contents.htm

    This tests the syntax ObjectName.prototype = new PrototypeObject using the
    Employee example in the document referenced above.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "proto_11";
    var VERSION = "JS1_3";
    var TITLE   = "Global Information in Constructors";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();
    var idCounter = 1;


function Employee ( name, dept ) {
     this.name = name || "";
     this.dept = dept || "general";
     this.id = idCounter++;
}
function Manager () {
     this.reports = [];
}
Manager.prototype = new Employee();

function WorkerBee ( name, dept, projs ) {
    this.base = Employee;
    this.base( name, dept)
    this.projects = projs || new Array();
}
WorkerBee.prototype = new Employee();

function SalesPerson () {
    this.dept = "sales";
    this.quota = 100;
}
SalesPerson.prototype = new WorkerBee();

function Engineer ( name, projs, machine ) {
    this.base = WorkerBee;
    this.base( name, "engineering", projs )
    this.machine = machine || "";
}
Engineer.prototype = new WorkerBee();

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
    var pat = new Employee( "Toonces, Pat", "Tech Pubs" )
    var terry = new Employee( "O'Sherry Terry", "Marketing" );

    var les = new Engineer( "Morris, Les",  new Array("JavaScript"), "indy" );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.id",
                                    5,
                                    pat.id );

    testcases[tc++] = new TestCase( SECTION,
                                    "terry.id",
                                    6,
                                    terry.id );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.id",
                                    7,
                                    les.id );


    test();

