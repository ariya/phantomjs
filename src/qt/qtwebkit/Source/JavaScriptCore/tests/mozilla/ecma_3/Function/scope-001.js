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
* Copyright (C) 1998 Netscape Communications Corporation. All
* Rights Reserved.
*
* Contributor(s): pschwartau@netscape.com, rogerl@netscape.com
* Date: 28 May 2001
*
* SUMMARY:  Functions are scoped statically, not dynamically
*
* See ECMA Section 10.1.4 Scope Chain and Identifier Resolution
* (This section defines the scope chain of an execution context)
*
* See ECMA Section 12.10 The with Statement
*
* See ECMA Section 13 Function Definition
* (This section defines the scope chain of a function object as that
*  of the running execution context when the function was declared)
*/
//-------------------------------------------------------------------------------------------------
var UBound = 0;
var bug = '(none)';
var summary = 'Testing that functions are scoped statically, not dynamically';
var self = this;  // capture a reference to the global object
var status = '';
var statusitems = [ ];
var actual = '';
var actualvalues = [ ];
var expect= '';
var expectedvalues = [ ];

/*
 * In this section the expected value is 1, not 2.
 *
 * Why? f captures its scope chain from when it's declared, and imposes that chain
 * when it's executed. In other words, f's scope chain is from when it was compiled.
 * Since f is a top-level function, this is the global object only. Hence 'a' resolves to 1.
 */
status = 'Section A of test';
var a = 1;
function f()
{
  return a;
}
var obj = {a:2};
with (obj)
{
  actual = f();
}
expect = 1;
addThis();


/*
 * In this section the expected value is 2, not 1. That is because here
 * f's associated scope chain now includes 'obj' before the global object.
 */
status = 'Section B of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  function f()
  {
    return a;
  }
  actual = f();
}
// Mozilla result, which contradicts IE and the ECMA spec: expect = 2;
expect = 1;
addThis();


/*
 * Like Section B , except that we call f outside the with block.
 * By the principles explained above, we still expect 2 -
 */
status = 'Section C of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  function f()
  {
    return a;
  }
}
actual = f();
// Mozilla result, which contradicts IE and the ECMA spec: expect = 2;
expect = 1;
addThis();


/*
 * Like Section C, but with one more level of indirection -
 */
status = 'Section D of test';
var a = 1;
var obj = {a:2, obj:{a:3}};
with (obj)
{
  with (obj)
  {
    function f()
    {
      return a;
    }
  }
}
actual = f();
// Mozilla result, which contradicts IE and the ECMA spec: expect = 3;
expect = 1;
addThis();


/*
 * Like Section C, but here we actually delete obj before calling f.
 * We still expect 2 -
 */
status = 'Section E of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  function f()
  {
    return a;
  }
}
delete obj;
actual = f();
// Mozilla result, which contradicts IE and the ECMA spec: expect = 2;
expect = 1;
addThis();


/*
 * Like Section E. Here we redefine obj and call f under with (obj) -
 * We still expect 2 -
 */
status = 'Section F of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  function f()
  {
    return a;
  }
}
delete obj;
var obj = {a:3};
with (obj)
{
  actual = f();
}
// Mozilla result, which contradicts IE and the ECMA spec: expect = 2;  // NOT 3 !!!
expect = 1;
addThis();


/*
 * Explicitly verify that f exists at global level, even though
 * it was defined under the with(obj) block -
 */
status = 'Section G of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  function f()
  {
    return a;
  }
}
actual = String([obj.hasOwnProperty('f'), self.hasOwnProperty('f')]);
expect = String([false, true]);
addThis();


/*
 * Explicitly verify that f exists at global level, even though
 * it was defined under the with(obj) block -
 */
status = 'Section H of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  function f()
  {
    return a;
  }
}
actual = String(['f' in obj, 'f' in self]);
expect = String([false, true]);
addThis();



//-------------------------------------------------------------------------------------------------
test();
//-------------------------------------------------------------------------------------------------


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
  resetTestVars();
}


function resetTestVars()
{
  delete a;
  delete obj;
  delete f;
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
