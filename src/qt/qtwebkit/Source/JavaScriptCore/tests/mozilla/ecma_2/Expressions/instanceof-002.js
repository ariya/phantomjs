/**
    File Name:          instanceof-002.js
    Section:
    Description:        Determining Instance Relationships

    This test is the same as js1_3/inherit/proto-002, except that it uses
    the builtin instanceof operator rather than a user-defined function
    called InstanceOf.

    This tests Object Hierarchy and Inheritance, as described in the document
    Object Hierarchy and Inheritance in JavaScript, last modified on 12/18/97
    15:19:34 on http://devedge.netscape.com/.  Current URL:
    http://devedge.netscape.com/docs/manuals/communicator/jsobj/contents.htm

    This tests the syntax ObjectName.prototype = new PrototypeObject using the
    Employee example in the document referenced above.

    Author:             christine@netscape.com
    Date:               12 november 1997
*/
//    onerror = err;

    var SECTION = "instanceof-002";
    var VERSION = "ECMA_2";
    var TITLE   = "Determining Instance Relationships";

    startTest();
    writeHeaderToLog( SECTION + " "+ TITLE);

    var tc = 0;
    var testcases = new Array();

function InstanceOf( object, constructor ) {
    while ( object != null ) {
        if ( object == constructor.prototype ) {
            return true;
        }
        object = object.__proto__;
    }
    return false;
}

function Employee ( name, dept ) {
     this.name = name || "";
     this.dept = dept || "general";
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

var pat = new Engineer()

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__ == Engineer.prototype",
                                    true,
                                    pat.__proto__ == Engineer.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__.__proto__ == WorkerBee.prototype",
                                    true,
                                    pat.__proto__.__proto__ == WorkerBee.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__.__proto__.__proto__ == Employee.prototype",
                                    true,
                                    pat.__proto__.__proto__.__proto__ == Employee.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__.__proto__.__proto__.__proto__ == Object.prototype",
                                    true,
                                    pat.__proto__.__proto__.__proto__.__proto__ == Object.prototype );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat.__proto__.__proto__.__proto__.__proto__.__proto__ == null",
                                    true,
                                    pat.__proto__.__proto__.__proto__.__proto__.__proto__ == null );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat instanceof Engineer",
                                    true,
                                    pat instanceof Engineer );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat instanceof WorkerBee )",
                                    true,
                                     pat instanceof WorkerBee );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat instanceof Employee )",
                                    true,
                                     pat instanceof Employee );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat instanceof Object )",
                                    true,
                                     pat instanceof Object );

    testcases[tc++] = new TestCase( SECTION,
                                    "pat instanceof SalesPerson )",
                                    false,
                                     pat instanceof SalesPerson );
    test();
