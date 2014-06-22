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
* Date:    04 Sep 2002
* SUMMARY: Just seeing that we don't crash when compiling this script -
*
* See http://bugzilla.mozilla.org/show_bug.cgi?id=96526
* See http://bugzilla.mozilla.org/show_bug.cgi?id=133897
*/
//-----------------------------------------------------------------------------
printBugNumber(96526);
printStatus("Just seeing that we don't crash when compiling this script -");


/*
 * This function comes from http://bugzilla.mozilla.org/show_bug.cgi?id=133897
 */
function validId(IDtext)
{
var res = "";

if(IDText.value==""){
                        print("You must enter a valid battery #")
                        return false
}
else if(IDText.value=="x522"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer_e2/energizer2.htm"
                        return true
}
else if(IDText.value=="x91"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer_e2/energizer2.htm"
                        return true
}
else if(IDText.value=="x92"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer_e2/energizer2.htm"
                        return true
}
else if(IDText.value=="x93"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer_e2/energizer2.htm"
                        return true
}
else if(IDText.value=="x95"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer_e2/energizer2.htm"
                        return true
}
else if(IDText.value=="521"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="522"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="528"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="529"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="539"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="e90"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="e91"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="e92"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="e93"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="e95"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_consumeroem.htm"
                        return true
                }
else if(IDText.value=="e96"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer_e2/energizer2.htm"
                        return true
                }
                else if(IDText.value=="en6"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en22"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en90"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en91"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en92"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en93"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en95"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en529"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en539"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="en715"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="edl4a"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="edl4as"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="edl4ac"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
                else if(IDText.value=="edl6a"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_industrial.htm"
                        return true
                }
else if(IDText.value=="3-0316"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }
else if(IDText.value=="3-0316i"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }
else if(IDText.value=="3-00411"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }
else if(IDText.value=="3-0411i"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-312"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }
else if(IDText.value=="3-312i"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-315"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-315i"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-315innc"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-315iwc"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-315wc"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-335"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-335i"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-335wc"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-335nnci"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-350"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-350i"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-3501wc"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-350wc"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-350nnci"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-361"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

else if(IDText.value=="3-361i"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/energizer/alkaline_oem_only.htm"
                        return true
                }

                else if(IDText.value=="a522"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/eveready/value.htm"
                        return true
                }
                else if(IDText.value=="a91"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/eveready/value.htm"
                        return true
                }
                else if(IDText.value=="a92"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/eveready/value.htm"
                        return true
                }
                else if(IDText.value=="a93"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/eveready/value.htm"
                        return true
                }
                else if(IDText.value=="a95"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/alkaline/eveready/value.htm"
                        return true
                }

else if(IDText.value=="510s"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_consumeroem.htm"
                        return true
                }

else if(IDText.value=="1209"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_consumeroem.htm"
                        return true
                }

else if(IDText.value=="1212"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_consumeroem.htm"
                        return true
                }

else if(IDText.value=="1215"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_consumeroem.htm"
                        return true
                }

else if(IDText.value=="1222"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_consumeroem.htm"
                        return true
                }

else if(IDText.value=="1235"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_consumeroem.htm"
                        return true
                }

else if(IDText.value=="1250"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_consumeroem.htm"
                        return true
                }
                else if(IDText.value=="206"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="246"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="266"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="276"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="411"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }


                else if(IDText.value=="412"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="413"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="415"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="416"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="455"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="467"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="489"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="493"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="497"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="504"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="505"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="711"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="732"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="763"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="ev190"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="ev115"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="ev122"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="ev131"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="ev135"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="ev150"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

                else if(IDText.value=="hs14196"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/carbon_zinc/carbon_zinc_industrial.htm"
                        return true
                }

else if(IDText.value=="2l76"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }
else if(IDText.value=="el1cr2"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }
else if(IDText.value=="crv3"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }

else if(IDText.value=="el2cr5"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }

else if(IDText.value=="el123ap"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }

else if(IDText.value=="el223ap"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }

else if(IDText.value=="l91"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }

else if(IDText.value=="l522"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }

else if(IDText.value=="l544"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithium.htm"
                        return true
                }

else if(IDText.value=="cr1025"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr1216"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr1220"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr1225"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr1616"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr1620"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr1632"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr2012"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr2016"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr2025"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr2032"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr2320"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr2430"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }

else if(IDText.value=="cr2450"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/lithium/lithiummin.htm"
                        return true
                }
                else if(IDText.value=="186"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="189"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="191"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="192"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="193"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="a23"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="a27"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="a76"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="a544"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="e11a"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

                else if(IDText.value=="e625g"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/manganese_dioxide/manganese_dioxide.htm"
                        return true
                }

else if(IDText.value=="301"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="303"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="309"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="315"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="317"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="319"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="321"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="329"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }
else if(IDText.value=="333"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }
else if(IDText.value=="335"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="337"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="339"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="341"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="344"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="346"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="350"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="357"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="361"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="362"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="364"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="365"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="366"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="370"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="371"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="373"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="376"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="377"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="379"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="381"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="384"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="386"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="387s"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="389"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="390"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="391"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="392"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="393"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="394"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="395"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="396"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="397"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

else if(IDText.value=="399"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }


else if(IDText.value=="epx76"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/silver/silver_oxide.htm"
                        return true
                }

                else if(IDText.value=="ac5"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/zinc_air/zinc_air.htm"
                        return true
                }

                else if(IDText.value=="ac10/230"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/zinc_air/zinc_air.htm"
                        return true
                }
                else if(IDText.value=="ac10"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/zinc_air/zinc_air.htm"
                        return true
                }
                else if(IDText.value=="ac13"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/zinc_air/zinc_air.htm"
                        return true
                }

                else if(IDText.value=="ac146x"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/zinc_air/zinc_air.htm"
                        return true
                }

                else if(IDText.value=="ac312"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/zinc_air/zinc_air.htm"
                        return true
                }

                else if(IDText.value=="ac675"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/zinc_air/zinc_air.htm"
                        return true
                }

else if(IDText.value=="chm24"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/accessories/rechargeableaccessories_chrger.htm"
                        return true
                }

else if(IDText.value=="chm4aa"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/accessories/rechargeableaccessories_chrger.htm"
                        return true
                }

else if(IDText.value=="chsm"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/accessories/rechargeableaccessories_chrger.htm"
                        return true
                }

else if(IDText.value=="chm4fc"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/accessories/rechargeableaccessories_chrger.htm"
                        return true
                }

else if(IDText.value=="cxl1000"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/accessories/rechargeableaccessories_chrger.htm"
                        return true
                }
                else if(IDText.value=="nh12"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_nimh.htm"
                        return true
                }
                else if(IDText.value=="nh15"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_nimh.htm"
                        return true
                }

                else if(IDText.value=="nh22"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_nimh.htm"
                        return true
                }

                else if(IDText.value=="nh35"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_nimh.htm"
                        return true
                }
                else if(IDText.value=="nh50"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_nimh.htm"
                        return true
                }


else if(IDText.value=="ccm5060"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="ccm5260"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="cm1060h"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="cm1360"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="cm2560"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="cm6136"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="cv3010"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="cv3012"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="cv3112"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="erc510"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc5160"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc520"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc525"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc530"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc545"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc560"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc570"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="erc580"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc590"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc600"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc610"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc620"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="erc630"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

else if(IDText.value=="erc640"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc650"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc660"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc670"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc680"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }
else if(IDText.value=="erc700"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscam.htm"
                        return true
                }

                else if(IDText.value=="cp2360"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }

                else if(IDText.value=="cp3036"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }


                else if(IDText.value=="cp3136"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }

                else if(IDText.value=="cp3336"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }

                else if(IDText.value=="cp5136"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }

                else if(IDText.value=="cp5648"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }

                else if(IDText.value=="cp5748"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }

                else if(IDText.value=="cp8049"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }

                else if(IDText.value=="cp8648"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }


                else if(IDText.value=="cpv5136"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }


                else if(IDText.value=="acp5036"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }


                else if(IDText.value=="acp5136"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }


                else if(IDText.value=="acp7160"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw120"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw210"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw220"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw230"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw240"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw305"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw310"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw320"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw400"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw500"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw510"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw520"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw530"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw600"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw610"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw700"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw720"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
                else if(IDText.value=="erw800"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscell.htm"
                        return true
                }
else if(IDText.value=="erp107"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp110"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp240"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp268"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp275"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp290"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp450"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp506"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp509"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp730"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erp9116"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="p2312"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p2322m"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="p2331"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p3201"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p3301"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p3302"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p3303"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p3306"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p3391"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p5256"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="p7300"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p7301"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="7302"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="7310"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p7320"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="p7330"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="p7340"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p7350"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }

else if(IDText.value=="p7360"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="p7400"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="p7501"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packscord.htm"
                        return true
                }
else if(IDText.value=="erd100"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packsdigicam.htm"
                        return true
                }
else if(IDText.value=="erd110"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packsdigicam.htm"
                        return true
                }
else if(IDText.value=="erd200"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packsdigicam.htm"
                        return true
                }
else if(IDText.value=="erd300"){
                        //Checks for id entry
                        res="../batteryinfo/product_offerings/rechargeable_consumer/rechargeable_consumer_packsdigicam.htm"
                        return true
                }













else if(IDText.value=="164"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="201"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="216"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="226"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="228"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="311"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="314"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }

else if(IDText.value=="313"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="323"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="325"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="333cz"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="343"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="354"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="355"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="387"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="388"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="417"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="420"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="457"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="460"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="477"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="479"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="482"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="484"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="487"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="490"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="491"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="496"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="509"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="510f"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="520"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="523"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="531"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="532"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="537"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="538"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="544"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="560"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="561"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="563"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="564"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="565"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="646"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="703"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="706"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="714"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="715"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="716"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="717"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="724"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="731"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="735"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="736"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="738"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="742"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="744"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="750"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="762s"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="773"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="778"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="781"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="812"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="815"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="835"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="850"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="904"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="912"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="915"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="935"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="950"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1015"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1035"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1050"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1150"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1231"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1461"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1463"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1562"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="1862"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="2356n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="2709n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="2744n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="2745n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="2746n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="2780n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ac41e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cc1096"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ccm1460"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ccm2460"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ccm4060a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ccm4060m"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cdc100"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ch12"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ch15"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ch2aa"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ch22"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ch35"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ch4"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ch50"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cm1060"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cm1560"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cm2360"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cm4160"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cm6036"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cm9072"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cm9172"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp2360"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp3336"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp3536"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp3736"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp5036"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp5160"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp5648"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp5960"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp6072"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp6172"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7049"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7072"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7148"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7149"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7160"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7172"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7248"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7261"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7348"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7548"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7661"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp7960"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp8049"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp8136"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp8160"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp8172"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp8248"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp8661"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp8748"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }

else if(IDText.value=="cp8948"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp8960"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp9061"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp9148"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp9161"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cp9360"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs3336"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs5036"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs5460"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7048"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7072"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7148"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7149"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7160"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7248"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7261"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7348"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7448"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7548"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs7661"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs8136"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs8648"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs9061"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs9148"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cs9161"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cv2012"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cv2096"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cv3010s"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cv3012"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cv3060"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cv3112"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="cv3212"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e1"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e1n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e3"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e4"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e9"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e12"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e12n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e13e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e41e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e42"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e42n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e89"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e115"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e115n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e126"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e132"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e132n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e133"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e133n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e134"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e134n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e135"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e135n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e136"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e137"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e137n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e146x"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e152"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e163"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e164"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e164n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e165"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e169"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e177"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e233"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e233n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e235n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e236n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e286"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e289"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e312e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e340e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e400"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e400n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e401e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e401n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e450"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e502"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e601"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e625"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e630"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e640"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e640n"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e675e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302157"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302250"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302358"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302435"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302462"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302465"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302478"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302642"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302651"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302702"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302904"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302905"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e302908"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e303145"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e303236"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e303314"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e303394"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e303496"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="e303996"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ea6"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ea6f"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ea6ft"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ea6st"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en1a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en132a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en133a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en134a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en135a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en136a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en164a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en165a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en175a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en177a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="en640a"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ep175"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ep401e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ep675e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx1"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx4"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx13"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx14"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx23"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx25"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx27"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx29"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx30"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx625"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx640"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx675"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="epx825"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev6"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev9"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev10s"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev15"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev22"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev31"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev35"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev50"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev90"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="ev90hp"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="fcc2"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs6"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs10s"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs15"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs31"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs35"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs50"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs90"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs95"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs150"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="hs6571"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="if6"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="is6"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="is6t"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="p2321m"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="p2322"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="p2326m"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="p7307"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="p7507"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="qcc4"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="s13e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="s312e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="s41e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="s76e"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="t35"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="t50"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="w353"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/contents/discontinued_battery_index.htm"
                        return true
                }
else if(IDText.value=="3251"){
                        //Checks for id entry
                        res="../datasheets/flashlights/eveready.htm"
                        return true
                }
else if(IDText.value=="4212"){
                        //Checks for id entry
                        res="../datasheets/flashlights/eveready.htm"
                        return true
                }
else if(IDText.value=="4251"){
                        //Checks for id entry
                        res="../datasheets/flashlights/eveready.htm"
                        return true
                }
else if(IDText.value=="5109"){
                        //Checks for id entry
                        res="../datasheets/flashlights/eveready.htm"
                        return true
                }
else if(IDText.value=="2251"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="e220"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="e250"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="e251"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="e251rc210"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="erg2c1"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="glo4aa1"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="rc220"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="rc250"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="x112"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="x215"){
                        //Checks for id entry
                        res="../datasheets/flashlights/home.htm"
                        return true
                }
else if(IDText.value=="4215"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="5215"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="6212"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="bas24a"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="db24a1"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="kcbg"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="kccl"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="kcdl"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="kcl2bu1"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="kcwl"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="ltcr"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="lteb"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="ltpt"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="sl240"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="v220"){
                        //Checks for id entry
                        res="../datasheets/flashlights/novelty.htm"
                        return true
                }
else if(IDText.value=="5100"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="8209"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="8215"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="9450"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="f101"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="f220"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="f420"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="fab4dcm"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="fl450"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="k221"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="k251"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="led4aa1"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="sp220"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="tw420"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="tw450"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="wp220"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="wp250"){
                        //Checks for id entry
                        res="../datasheets/flashlights/outdoor.htm"
                        return true
                }
else if(IDText.value=="cfl420"){
                        //Checks for id entry
                        res="../datasheets/flashlights/premium.htm"
                        return true
                }
else if(IDText.value=="d410"){
                        //Checks for id entry
                        res="../datasheets/flashlights/premium.htm"
                        return true
                }
else if(IDText.value=="d420"){
                        //Checks for id entry
                        res="../datasheets/flashlights/premium.htm"
                        return true
                }
else if(IDText.value=="fn450"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="in215"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="in251"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="in351"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="in421"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="k220"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="k250"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="r215"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="r250"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="r450"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="tuf4d1"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="v109"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="v115"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="v215"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="v250"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }
else if(IDText.value=="val2dl1"){
                        //Checks for id entry
                        res="../datasheets/flashlights/work.htm"
                        return true
                }

else if(IDText.value=="459"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="208ind"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="231ind"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="1151"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="1251"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="1259"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="1351"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="1359"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="3251r"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="3251wh"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="4212wh"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="4250ind"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="5109ind"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="6212wh"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="9101ind"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="e250y"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="e251y"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="in220"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="in253"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="in420"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="in450"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="indwandr"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="indwandy"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="r215ind"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrial.htm"
                        return true
                }
else if(IDText.value=="pr2"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="pr3"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="pr4"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="pr6"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="pr7"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="pr12"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="pr13"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="pr35"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="112"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="222"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="243"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="258"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="407"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="425"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="1156"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="1651"){
                        //Checks for id entry
                        res="../datasheets/flashlights/standard.htm"
                        return true
                }
else if(IDText.value=="kpr102"){
                        //Checks for id entry
                        res="../datasheets/flashlights/krypton.htm"
                        return true
                }
else if(IDText.value=="kpr103"){
                        //Checks for id entry
                        res="../datasheets/flashlights/krypton.htm"
                        return true
                }
else if(IDText.value=="kpr104"){
                        //Checks for id entry
                        res="../datasheets/flashlights/krypton.htm"
                        return true
                }
else if(IDText.value=="kpr113"){
                        //Checks for id entry
                        res="../datasheets/flashlights/krypton.htm"
                        return true
                }
else if(IDText.value=="kpr116"){
                        //Checks for id entry
                        res="../datasheets/flashlights/krypton.htm"
                        return true
                }
else if(IDText.value=="kpr802"){
                        //Checks for id entry
                        res="../datasheets/flashlights/krypton.htm"
                        return true
                }
else if(IDText.value=="skpr823"){
                        //Checks for id entry
                        res="../datasheets/flashlights/krypton.htm"
                        return true
                }
else if(IDText.value=="hpr50"){
                        //Checks for id entry
                        res="../datasheets/flashlights/halogenbulb.htm"
                        return true
                }
else if(IDText.value=="hpr51"){
                        //Checks for id entry
                        res="../datasheets/flashlights/halogenbulb.htm"
                        return true
                }
else if(IDText.value=="hpr52"){
                        //Checks for id entry
                        res="../datasheets/flashlights/halogenbulb.htm"
                        return true
                }
else if(IDText.value=="hpr53"){
                        //Checks for id entry
                        res="../datasheets/flashlights/halogenbulb.htm"
                        return true
                }
else if(IDText.value=="f4t5"){
                        //Checks for id entry
                        res="../datasheets/flashlights/fluorescent.htm"
                        return true
                }
else if(IDText.value=="f6t5"){
                        //Checks for id entry
                        res="../datasheets/flashlights/fluorescent.htm"
                        return true
                }
else if(IDText.value=="t1-1"){
                        //Checks for id entry
                        res="../datasheets/flashlights/highintensity.htm"
                        return true
                }
else if(IDText.value=="t1-2"){
                        //Checks for id entry
                        res="../datasheets/flashlights/highintensity.htm"
                        return true
                }
else if(IDText.value=="t2-2"){
                        //Checks for id entry
                        res="../datasheets/flashlights/halogenxenon.htm"
                        return true
                }
else if(IDText.value=="t2-3"){
                        //Checks for id entry
                        res="../datasheets/flashlights/halogenxenon.htm"
                        return true
                }
else if(IDText.value=="t2-4"){
                        //Checks for id entry
                        res="../datasheets/flashlights/halogenxenon.htm"
                        return true
                }
else if(IDText.value=="tx15-2"){
                        //Checks for id entry
                        res="../datasheets/flashlights/halogenxenon.htm"
                        return true
                }
else if(IDText.value=="4546ib"){
                        //Checks for id entry
                        res="../datasheets/flashlights/industrialbulb.htm"
                        return true
                }
else if(IDText.value=="LED"){
                        //Checks for id entry
                        res="../datasheets/flashlights/led.htm"
                        return true
                }



else if(IDText.value=="108"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="209"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="330"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="330y"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="331"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="331y"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="1251bk"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="2253"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="3233"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="3253"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="3415"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="3452"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="4220"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="4453"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="5154"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="5251"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="7369"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="8115"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="8415"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="b170"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="bkc1"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="d620"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="d820"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="e100"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="e252"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="e350"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="e420"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="em290"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="em420"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="f100"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="f215"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="f250"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="f415"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="h100"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="h250"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="h350"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="in25t"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="kcdb"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="kcsg"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="kctw"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="rc100"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="rc251"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="rc290"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="t430"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="v235"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="x250"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }
else if(IDText.value=="x350"){
                        //Checks for id entry
                        print("You have entered a Discontinued Product Number")
                        res="../datasheets/flashlights/discontinued_flashlight_index.htm"
                        return true
                }



else            {
                        print("You have entered an Invalid Product Number...Please try 'Select Product Group' search.")
                        return false
                }

}
