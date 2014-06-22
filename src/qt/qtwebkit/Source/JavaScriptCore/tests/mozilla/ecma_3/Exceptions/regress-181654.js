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
* Contributor(s): joerg.schaible@gmx.de
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
* Date:    23 Nov 2002
* SUMMARY: Calling toString for an object derived from the Error class
*		   results in an TypeError (Rhino only)
* See http://bugzilla.mozilla.org/show_bug.cgi?id=181654
*/
//-----------------------------------------------------------------------------
var UBound = 0;
var bug = '181654';
var summary = 'Calling toString for an object derived from the Error class should be possible.';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var EMPTY_STRING = '';
var EXPECTED_FORMAT = 0;


// derive MyError from Error
function MyError( msg )
{
	this.message = msg;
}
MyError.prototype = new Error();
MyError.prototype.name = "MyError";


status = inSection(1);
var err1 = new MyError('msg1');
actual = examineThis(err1, 'msg1');
expect = EXPECTED_FORMAT;
addThis();

status = inSection(2);
var err2 = new MyError(err1);
actual = examineThis(err2, err1);
expect = EXPECTED_FORMAT;
addThis();

status = inSection(3);
var err3 = new MyError();
actual = examineThis(err3, EMPTY_STRING);
expect = EXPECTED_FORMAT;
addThis();

status = inSection(4);
var err4 = new MyError(EMPTY_STRING);
actual = examineThis(err4, EMPTY_STRING);
expect = EXPECTED_FORMAT;
addThis();

// now generate an error -
status = inSection(5);
try
{
  throw new MyError("thrown");
}
catch(err5)
{
 actual = examineThis(err5, "thrown");
}
expect = EXPECTED_FORMAT;
addThis();



//-----------------------------------------------------------------------------
test();
//-----------------------------------------------------------------------------



/*
 * Searches err.toString() for err.name + ':' + err.message,
 * with possible whitespace on each side of the colon sign.
 *
 * We allow for no colon in case err.message was not provided by the user.
 * In such a case, SpiderMonkey and Rhino currently set err.message = '',
 * as allowed for by ECMA 15.11.4.3. This makes |pattern| work in this case.
 *
 * If this is ever changed to a non-empty string, e.g. 'undefined',
 * you may have to modify |pattern| to take that into account -
 *
 */
function examineThis(err, msg)
{
  var pattern = err.name + '\\s*:?\\s*' + msg;
  return err.toString().search(RegExp(pattern));
}


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
