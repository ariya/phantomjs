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
* Contributor(s): zen-parse@gmx.net, pschwartau@netscape.com
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
* Date:    16 July 2002
* SUMMARY: Testing that Array.sort() doesn't crash on very large arrays
* See http://bugzilla.mozilla.org/show_bug.cgi?id=157652
*
* How large can a JavaScript array be?
* ECMA-262 Ed.3 Final, Section 15.4.2.2 : new Array(len)
*
* This states that |len| must be a a uint32 (unsigned 32-bit integer).
* Note the UBound for uint32's is 2^32 -1 = 0xFFFFFFFF = 4,294,967,295.
*
* Check:
*              js> var arr = new Array(0xFFFFFFFF)
*              js> arr.length
*              4294967295
*
*              js> var arr = new Array(0x100000000)
*              RangeError: invalid array length
*
*
* We'll try the largest possible array first, then a couple others.
* We're just testing that we don't crash on Array.sort().
*
* Try to be good about memory by nulling each array variable after it is
* used. This will tell the garbage collector the memory is no longer needed.
*
* As of 2002-08-13, the JS shell runs out of memory no matter what we do,
* when trying to sort such large arrays.
*
* We only want to test that we don't CRASH on the sort. So it will be OK
* if we get the JS "out of memory" error. Note this terminates the test
* with exit code 3. Therefore we put
*
*                       |expectExitCode(3);|
*
* The only problem will arise if the JS shell ever DOES have enough memory
* to do the sort. Then this test will terminate with the normal exit code 0
* and fail.
*
* Right now, I can't see any other way to do this, because "out of memory"
* is not a catchable error: it cannot be trapped with try...catch.
*
*
* FURTHER HEADACHE: Rhino can't seem to handle the largest array: it hangs.
* So we skip this case in Rhino. Here is correspondence with Igor Bukanov.
* He explains that Rhino isn't actually hanging; it's doing the huge sort:
*
* Philip Schwartau wrote:
*
* > Hi,
* >
* > I'm getting a graceful OOM message on trying to sort certain large
* > arrays. But if the array is too big, Rhino simply hangs. Note that ECMA
* > allows array lengths to be anything less than Math.pow(2,32), so the
* > arrays I'm sorting are legal.
* >
* > Note below, I'm getting an instantaneous OOM error on arr.sort() for LEN
* > = Math.pow(2, 30). So shouldn't I also get one for every LEN between
* > that and Math.pow(2, 32)? For some reason, I start to hang with 100% CPU
* > as LEN hits, say, Math.pow(2, 31) and higher. SpiderMonkey gives OOM
* > messages for all of these. Should I file a bug on this?
*
* Igor Bukanov wrote:
*
* This is due to different sorting algorithm Rhino uses when sorting
* arrays with length > Integer.MAX_VALUE. If length can fit Java int,
* Rhino first copies internal spare array to a temporary buffer, and then
* sorts it, otherwise it sorts array directly. In case of very spare
* arrays, that Array(big_number) generates, it is rather inefficient and
* generates OutOfMemory if length fits int. It may be worth in your case
* to optimize sorting to take into account array spareness, but then it
* would be a good idea to file a bug about ineficient sorting of spare
* arrays both in case of Rhino and SpiderMonkey as SM always uses a
* temporary buffer.
*
*/
//-----------------------------------------------------------------------------
var bug = 157652;
var summary = "Testing that Array.sort() doesn't crash on very large arrays";

printBugNumber(bug);
printStatus(summary);

// JSC doesn't run out of memory, so we don't expect an abnormal exit code 
//expectExitCode(3);
var IN_RHINO = inRhino();

if (!IN_RHINO)
{
  var a1=Array(0xFFFFFFFF);
  a1.sort();
  a1 = null;
}

var a2 = Array(0x40000000);
a2.sort();
a2=null;

var a3=Array(0x10000000/4);
a3.sort();
a3=null;
