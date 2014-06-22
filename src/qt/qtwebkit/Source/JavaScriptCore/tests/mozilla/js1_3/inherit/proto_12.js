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
    File Name:          proto_12.js
    Section:
    Description:        new PrototypeObject

    This tests Object Hierarchy and Inheritance, as described in the document
    Object Hierarchy and Inheritance in JavaScript, last modified on 12/18/97
    15:19:34 on http://devedge.netscape.com/.  Current URL:
    http://devedge.netscape.com/docs/manuals/communicator/jsobj/contents.htm

    No Multiple Inheritance

    Author:             christine@netscape.com
    Date:               12 november 1997
*/

    var SECTION = "proto_12";
    var VERSION = "JS1_3";
    var TITLE   = "No Multiple Inheritance";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var testcases = new Array();

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

function Hobbyist( hobby ) {
    this.hobby = hobby || "yodeling";
}

function Engineer ( name, projs, machine, hobby ) {
    this.base1 = WorkerBee;
    this.base1( name, "engineering", projs )

    this.base2 = Hobbyist;
    this.base2( hobby );

    this.projects = projs || new Array();
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
    var idCounter = 1;

    var les = new Engineer( "Morris, Les",  new Array("JavaScript"), "indy" );

    Hobbyist.prototype.equipment = [ "horn", "mountain", "goat" ];

    testcases[tc++] = new TestCase( SECTION,
                                    "les.name",
                                    "Morris, Les",
                                    les.name );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.dept",
                                    "engineering",
                                    les.dept );

    Array.prototype.getClass = Object.prototype.toString;

    testcases[tc++] = new TestCase( SECTION,
                                    "les.projects.getClass()",
                                    "[object Array]",
                                    les.projects.getClass() );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.projects[0]",
                                    "JavaScript",
                                    les.projects[0] );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.machine",
                                    "indy",
                                    les.machine );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.hobby",
                                    "yodeling",
                                    les.hobby );

    testcases[tc++] = new TestCase( SECTION,
                                    "les.equpment",
                                    void 0,
                                    les.equipment );

    test();
