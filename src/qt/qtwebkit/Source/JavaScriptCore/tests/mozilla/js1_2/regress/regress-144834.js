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
* Contributor(s): bzbarsky@mit.edu, pschwartau@netscape.com
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
* Date:    05 July 2002
* SUMMARY: Testing local var having same name as switch label inside function
* 
* The code below crashed while compiling in JS1.1 or JS1.2
* See http://bugzilla.mozilla.org/show_bug.cgi?id=144834
*
*/
//-----------------------------------------------------------------------------
var bug = 144834;
var summary = 'Local var having same name as switch label inside function';

print(bug);
print(summary);


function RedrawSched()
{
  var MinBound;

  switch (i)
  {
    case MinBound :
  }
}


/*
 * Also try eval scope -
 */
var s = '';
s += 'function RedrawSched()';
s += '{';
s += '  var MinBound;';
s += '';
s += '  switch (i)';
s += '  {';
s += '    case MinBound :';
s += '  }';
s += '}';
eval(s);
