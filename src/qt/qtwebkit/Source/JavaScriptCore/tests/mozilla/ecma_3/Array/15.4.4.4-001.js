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
* Contributor(s): george@vanous.com, igor@icesoft.no, pschwartau@netscape.com
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
* Date:    19 September 2002
* SUMMARY: Testing Array.prototype.concat()
* See http://bugzilla.mozilla.org/show_bug.cgi?id=169795
*
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 169795;
var summary = 'Testing Array.prototype.concat()';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var x;


status = inSection(1);
x = "Hello";
actual = [].concat(x).toString();
expect = x.toString();
addThis();

status = inSection(2);
x = 999;
actual = [].concat(x).toString();
expect = x.toString();
addThis();

status = inSection(3);
x = /Hello/g;
actual = [].concat(x).toString();
expect = x.toString();
addThis();

status = inSection(4);
x = new Error("Hello");
actual = [].concat(x).toString();
expect = x.toString();
addThis();

status = inSection(5);
x = function() {return "Hello";};
actual = [].concat(x).toString();
expect = x.toString();
addThis();

status = inSection(6);
x = [function() {return "Hello";}];
actual = [].concat(x).toString();
expect = x.toString();
addThis();

status = inSection(7);
x = [1,2,3].concat([4,5,6]);
actual = [].concat(x).toString();
expect = x.toString();
addThis();

status = inSection(8);
x = eval('this');
actual = [].concat(x).toString();
expect = x.toString();
addThis();

/*
 * The next two sections are by igor@icesoft.no; see
 * http://bugzilla.mozilla.org/show_bug.cgi?id=169795#c3
 */
status = inSection(9);
x={length:0};
actual = [].concat(x).toString();
expect = x.toString();
addThis();

status = inSection(10);
x={length:2, 0:0, 1:1};
actual = [].concat(x).toString();
expect = x.toString();
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
