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
* Contributor(s): pschwartau@netscape.com
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
* Date:    11 Nov 2002
* SUMMARY: JS shouldn't crash on extraneous args to str.match(), etc.
* See http://bugzilla.mozilla.org/show_bug.cgi?id=179524
*
* Note that when testing str.replace(), we have to be careful if the first
* argument provided to str.replace() is not a regexp object. ECMA-262 says
* it is NOT converted to one, unlike the case for str.match(), str.search().
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=83293#c21. This means
* we have to be careful how we test meta-characters in the first argument
* to str.replace(), if that argument is a string -
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = 179524;
var summary = "Don't crash on extraneous arguments to str.match(), etc.";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


str = 'ABC abc';
var re = /z/ig;

status = inSection(1);
actual = str.match(re);
expect = null;
addThis();

status = inSection(2);
actual = str.match(re, 'i');
expect = null;
addThis();

status = inSection(3);
actual = str.match(re, 'g', '');
expect = null;
addThis();

status = inSection(4);
actual = str.match(re, 'z', new Object(), new Date());
expect = null;
addThis();


/*
 * Now try the same thing with str.search()
 */
status = inSection(5);
actual = str.search(re);
expect = -1;
addThis();

status = inSection(6);
actual = str.search(re, 'i');
expect = -1;
addThis();

status = inSection(7);
actual = str.search(re, 'g', '');
expect = -1;
addThis();

status = inSection(8);
actual = str.search(re, 'z', new Object(), new Date());
expect = -1;
addThis();


/*
 * Now try the same thing with str.replace()
 */
status = inSection(9);
actual = str.replace(re, 'Z');
expect = str;
addThis();

status = inSection(10);
actual = str.replace(re, 'Z', 'i');
expect = str;
addThis();

status = inSection(11);
actual = str.replace(re, 'Z', 'g', '');
expect = str;
addThis();

status = inSection(12);
actual = str.replace(re, 'Z', 'z', new Object(), new Date());
expect = str;
addThis();



/*
 * Now test the case where str.match()'s first argument is not a regexp object.
 * In that case, JS follows ECMA-262 Ed.3 by converting the 1st argument to a
 * regexp object using the argument as a regexp pattern, but then extends ECMA
 * by taking any optional 2nd argument to be a regexp flag string (e.g.'ig').
 *
 * Reference: http://bugzilla.mozilla.org/show_bug.cgi?id=179524#c10
 */
status = inSection(13);
actual = str.match('a').toString();
expect = str.match(/a/).toString();
addThis();

status = inSection(14);
actual = str.match('a', 'i').toString();
expect = str.match(/a/i).toString();
addThis();

status = inSection(15);
actual = str.match('a', 'ig').toString();
expect = str.match(/a/ig).toString();
addThis();

status = inSection(16);
actual = str.match('\\s', 'm').toString();
expect = str.match(/\s/m).toString();
addThis();


/*
 * Now try the previous three cases with extraneous parameters
 */
status = inSection(17);
actual = str.match('a', 'i', 'g').toString();
expect = str.match(/a/i).toString();
addThis();

status = inSection(18);
actual = str.match('a', 'ig', new Object()).toString();
expect = str.match(/a/ig).toString();
addThis();

status = inSection(19);
actual = str.match('\\s', 'm', 999).toString();
expect = str.match(/\s/m).toString();
addThis();


/*
 * Try an invalid second parameter (i.e. an invalid regexp flag)
 */
status = inSection(20);
try
{
  actual = str.match('a', 'z').toString();
  expect = 'SHOULD HAVE FALLEN INTO CATCH-BLOCK!';
  addThis();
}
catch (e)
{
  actual = e instanceof SyntaxError;
  expect = true;
  addThis();
}



/*
 * Now test str.search() where the first argument is not a regexp object.
 * The same considerations as above apply -
 *
 * Reference: http://bugzilla.mozilla.org/show_bug.cgi?id=179524#c16
 */
status = inSection(21);
actual = str.search('a');
expect = str.search(/a/);
addThis();

status = inSection(22);
actual = str.search('a', 'i');
expect = str.search(/a/i);
addThis();

status = inSection(23);
actual = str.search('a', 'ig');
expect = str.search(/a/ig);
addThis();

status = inSection(24);
actual = str.search('\\s', 'm');
expect = str.search(/\s/m);
addThis();


/*
 * Now try the previous three cases with extraneous parameters
 */
status = inSection(25);
actual = str.search('a', 'i', 'g');
expect = str.search(/a/i);
addThis();

status = inSection(26);
actual = str.search('a', 'ig', new Object());
expect = str.search(/a/ig);
addThis();

status = inSection(27);
actual = str.search('\\s', 'm', 999);
expect = str.search(/\s/m);
addThis();


/*
 * Try an invalid second parameter (i.e. an invalid regexp flag)
 */
status = inSection(28);
try
{
  actual = str.search('a', 'z');
  expect = 'SHOULD HAVE FALLEN INTO CATCH-BLOCK!';
  addThis();
}
catch (e)
{
  actual = e instanceof SyntaxError;
  expect = true;
  addThis();
}



/*
 * Now test str.replace() where the first argument is not a regexp object.
 * The same considerations as above apply, EXCEPT for meta-characters.
 * See introduction to testcase above. References:
 *
 * http://bugzilla.mozilla.org/show_bug.cgi?id=179524#c16
 * http://bugzilla.mozilla.org/show_bug.cgi?id=83293#c21
 */
status = inSection(29);
actual = str.replace('a', 'Z');
expect = str.replace(/a/, 'Z');
addThis();

status = inSection(30);
actual = str.replace('a', 'Z', 'i');
expect = str.replace(/a/i, 'Z');
addThis();

status = inSection(31);
actual = str.replace('a', 'Z', 'ig');
expect = str.replace(/a/ig, 'Z');
addThis();

status = inSection(32);
actual = str.replace('\\s', 'Z', 'm'); //<--- NO!!! No meta-characters 1st arg!
actual = str.replace(' ', 'Z', 'm');   //<--- Have to do this instead
expect = str.replace(/\s/m, 'Z');
addThis();


/*
 * Now try the previous three cases with extraneous parameters
 */
status = inSection(33);
actual = str.replace('a', 'Z', 'i', 'g');
expect = str.replace(/a/i, 'Z');
addThis();

status = inSection(34);
actual = str.replace('a', 'Z', 'ig', new Object());
expect = str.replace(/a/ig, 'Z');
addThis();

status = inSection(35);
actual = str.replace('\\s', 'Z', 'm', 999); //<--- NO meta-characters 1st arg!
actual = str.replace(' ', 'Z', 'm', 999);   //<--- Have to do this instead
expect = str.replace(/\s/m, 'Z');
addThis();


/*
 * Try an invalid third parameter (i.e. an invalid regexp flag)
 */
status = inSection(36);
try
{
  actual = str.replace('a', 'Z', 'z');
  expect = 'SHOULD HAVE FALLEN INTO CATCH-BLOCK!';
  addThis();
}
catch (e)
{
  actual = e instanceof SyntaxError;
  expect = true;
  addThis();
}




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
