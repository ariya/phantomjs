/* ***** BEGIN LICENSE BLOCK *****
* Version: NPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Netscape Public License
* Version 1.1 (the "License"); you may not use this file except in
* compliance with the License. You may obtain a copy of the License at
* http://www.mozilla.org/NPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is JavaScript Engine testing utilities.
*
* The Initial Developer of the Original Code is Netscape Communications Corp.
* Portions created by the Initial Developer are Copyright (C) 2002
* the Initial Developer. All Rights Reserved.
*
* Contributor(s): brendan@mozilla.org, pschwartau@netscape.com
*
* Alternatively, the contents of this file may be used under the terms of
* either the GNU General Public License Version 2 or later (the "GPL"), or
* the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
* in which case the provisions of the GPL or the LGPL are applicable instead
* of those above. If you wish to allow use of your version of this file only
* under the terms of either the GPL or the LGPL, and not to allow others to
* use your version of this file under the terms of the NPL, indicate your
* decision by deleting the provisions above and replace them with the notice
* and other provisions required by the GPL or the LGPL. If you do not delete
* the provisions above, a recipient may use your version of this file under
* the terms of any one of the NPL, the GPL or the LGPL.
*
* ***** END LICENSE BLOCK *****
*
*
* Date:    25 Mar 2002
* SUMMARY: Array.prototype.sort() should not (re-)define .length
* See http://bugzilla.mozilla.org/show_bug.cgi?id=130451
*
* From the ECMA-262 Edition 3 Final spec:
*
* NOTE: The sort function is intentionally generic; it does not require that
* its |this| value be an Array object. Therefore, it can be transferred to
* other kinds of objects for use as a method. Whether the sort function can
* be applied successfully to a host object is implementation-dependent.
*
* The interesting parts of this testcase are the contrasting expectations for
* Brendan's test below, when applied to Array objects vs. non-Array objects.
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 130451;
var summary = 'Array.prototype.sort() should not (re-)define .length';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var arr = [];
var cmp = new Function();


/*
 * First: test Array.prototype.sort() on Array objects
 */
status = inSection(1);
arr = [0,1,2,3];
cmp = function(x,y) {return x-y;};
actual = arr.sort(cmp).length;
expect = 4;
addThis();

status = inSection(2);
arr = [0,1,2,3];
cmp = function(x,y) {return y-x;};
actual = arr.sort(cmp).length;
expect = 4;
addThis();

status = inSection(3);
arr = [0,1,2,3];
cmp = function(x,y) {return x-y;};
arr.length = 1;
actual = arr.sort(cmp).length;
expect = 1;
addThis();

/*
 * This test is by Brendan. Setting arr.length to
 * 2 and then 4 should cause elements to be deleted.
 */
arr = [0,1,2,3];
cmp = function(x,y) {return x-y;};
arr.sort(cmp);

status = inSection(4);
actual = arr.join();
expect = '0,1,2,3';
addThis();

status = inSection(5);
actual = arr.length;
expect = 4;
addThis();

status = inSection(6);
arr.length = 2;
actual = arr.join();
expect = '0,1';
addThis();

status = inSection(7);
arr.length = 4;
actual = arr.join();
expect = '0,1,,';  //<---- see how 2,3 have been lost
addThis();



/*
 * Now test Array.prototype.sort() on non-Array objects
 */
status = inSection(8);
var obj = new Object();
obj.sort = Array.prototype.sort;
obj.length = 4;
obj[0] = 0;
obj[1] = 1;
obj[2] = 2;
obj[3] = 3;
cmp = function(x,y) {return x-y;};
actual = obj.sort(cmp).length;
expect = 4;
addThis();


/*
 * Here again is Brendan's test. Unlike the array case
 * above, the setting of obj.length to 2 and then 4
 * should NOT cause elements to be deleted
 */
obj = new Object();
obj.sort = Array.prototype.sort;
obj.length = 4;
obj[0] = 3;
obj[1] = 2;
obj[2] = 1;
obj[3] = 0;
cmp = function(x,y) {return x-y;};
obj.sort(cmp);  //<---- this is what triggered the buggy behavior below
obj.join = Array.prototype.join;

status = inSection(9);
actual = obj.join();
expect = '0,1,2,3';
addThis();

status = inSection(10);
actual = obj.length;
expect = 4;
addThis();

status = inSection(11);
obj.length = 2;
actual = obj.join();
expect = '0,1';
addThis();

/*
 * Before this bug was fixed, |actual| held the value '0,1,,'
 * as in the Array-object case at top. This bug only occurred
 * if Array.prototype.sort() had been applied to |obj|,
 * as we have done higher up.
 */
status = inSection(12);
obj.length = 4;
actual = obj.join();
expect = '0,1,2,3';
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
  enterFunc('test');
  printBugNumber(bug);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
