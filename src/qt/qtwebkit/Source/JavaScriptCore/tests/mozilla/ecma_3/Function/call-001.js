/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an
* "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either expressed
* or implied. See the License for the specific language governing
* rights and limitations under the License.
*
* The Original Code is mozilla.org code.
*
* The Initial Developer of the Original Code is Netscape
* Communications Corporation.  Portions created by Netscape are
* Copyright (C) 1998 Netscape Communications Corporation.
* All Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com
* Date: 2001-07-13
*
* SUMMARY: Applying Function.prototype.call to the Function object itself
*
*
* ECMA-262 15.3.4.4 Function.prototype.call (thisArg [,arg1 [,arg2,…] ] )
*
* When applied to the Function object itself, thisArg should be ignored.
* As explained by Waldemar (waldemar@netscape.com):
*
* Function.call(obj, "print(this)") is equivalent to invoking
* Function("print(this)") with this set to obj. Now, Function("print(this)")
* is equivalent to new Function("print(this)") (see 15.3.1.1), and the latter
* ignores the this value that you passed it and constructs a function
* (which we'll call F) which will print the value of the this that will be
* passed in when F will be invoked.
*
* With the last set of () you're invoking F(), which means you're calling it
* with no this value. When you don't provide a this value, it defaults to the
* global object.
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = '(none)';
var summary = 'Applying Function.prototype.call to the Function object itself';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var self = this; // capture a reference to the global object
var cnOBJECT_GLOBAL = self.toString();
var cnOBJECT_OBJECT = (new Object).toString();
var cnHello = 'Hello';
var cnRed = 'red';
var objTEST = {color:cnRed};
var f = new Function();
var g = new Function();


f = Function.call(self, 'return cnHello');
g = Function.call(objTEST, 'return cnHello');

status = 'Section A of test';
actual = f();
expect = cnHello;
captureThis();

status = 'Section B of test';
actual = g();
expect = cnHello;
captureThis();


f = Function.call(self, 'return this.toString()');
g = Function.call(objTEST, 'return this.toString()');

status = 'Section C of test';
actual = f();
expect = cnOBJECT_GLOBAL;
captureThis();

status = 'Section D of test';
actual = g();
expect = cnOBJECT_GLOBAL;
captureThis();


f = Function.call(self, 'return this.color');
g = Function.call(objTEST, 'return this.color');

status = 'Section E of test';
actual = f();
expect = undefined;
captureThis();

status = 'Section F of test';
actual = g();
expect = undefined;
captureThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function captureThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber (bug);
  printStatus (summary);

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
