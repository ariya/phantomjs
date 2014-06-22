/*
* The contents of this file are subject to the Netscape Public
* License Version 1.1 (the "License"); you may not use this file
* except in compliance with the License. You may obtain a copy of
* the License at http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS
* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
* implied. See the License for the specific language governing
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
* Date: 03 September 2001
*
* SUMMARY: Double quotes should be escaped in uneval(new Error('""'))
* See http://bugzilla.mozilla.org/show_bug.cgi?id=96284
*
* The real point here is this: we should be able to reconstruct an object
* obj from uneval(obj). We'll test this on various types of objects.
*
* Method: define obj2 = eval(uneval(obj1)) and verify that
* obj2.toSource() == obj1.toSource().
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 96284;
var summary = 'Double quotes should be escaped in Error.prototype.toSource()';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var obj1 = {};
var obj2 = {};
var cnTestString = '"This is a \" STUPID \" test string!!!"\\';


// various NativeError objects -
status = inSection(1);
obj1 = Error(cnTestString);
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(2);
obj1 = EvalError(cnTestString);
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(3);
obj1 = RangeError(cnTestString);
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(4);
obj1 = ReferenceError(cnTestString);
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(5);
obj1 = SyntaxError(cnTestString);
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(6);
obj1 = TypeError(cnTestString);
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(7);
obj1 = URIError(cnTestString);
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();


// other types of objects -
status = inSection(8);
obj1 = new String(cnTestString);
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(9);
obj1 = {color:'red', texture:cnTestString, hasOwnProperty:42};
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(10);
obj1 = function(x) {function g(y){return y+1;} return g(x);};
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(11);
obj1 = new Number(eval('6'));
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();

status = inSection(12);
obj1 = /ad;(lf)kj(2309\/\/)\/\//;
obj2 = eval(uneval(obj1));
actual = obj2.toSource();
expect = obj1.toSource();
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------


function addThis()
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
