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
* Portions created by the Initial Developer are Copyright (C) 2001
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
* Date: 26 Nov 2001
* SUMMARY: JS should not crash on this code
* See http://bugzilla.mozilla.org/show_bug.cgi?id=111557
*
*/
//-----------------------------------------------------------------------------
var bug = 111557;
var summary = "Just seeing that we don't crash on this code -";

printBugNumber(bug);
printStatus(summary);


/*
 * NOTE: have defined |top| as |this| to make this a standalone JS shell test.
 * This came from the HTML of a frame, where |top| would have its DOM meaning.
 */
var top = this;

	top.authors = new Array();
	top.titles = new Array();
	var i = 0;
	

	top.authors[i] = "zPROD xA.5375.";
	top.titles[i] =  "NDS Libraries for C";
	i++;

	top.authors[i] = "zFLDR xB.5375.0100.";
	top.titles[i] =  "NDS Backup Services";
	i++;

	top.authors[i] = "zFLDR xC.5375.0100.0001.";
	top.titles[i] =  "Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0001.0001.";
	top.titles[i] =  "NDSBackupServerData";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0001.0002.";
	top.titles[i] =  "NDSFreeNameList";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0001.0003.";
	top.titles[i] =  "NDSGetReplicaPartitionNames";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0001.0004.";
	top.titles[i] =  "NDSIsOnlyServerInTree";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0001.0005.";
	top.titles[i] =  "NDSSYSVolumeRecovery";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0001.0006.";
	top.titles[i] =  "NDSVerifyServerInfo";
	i++;

	top.authors[i] = "zFLDR xC.5375.0100.0002.";
	top.titles[i] =  "Structures";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0002.0001.";
	top.titles[i] =  "NAMEID_TYPE";
	i++;

	top.authors[i] = "zFLDR xC.5375.0100.0003.";
	top.titles[i] =  "Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0003.0001.";
	top.titles[i] =  "NDS Reason Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0100.0003.0002.";
	top.titles[i] =  "NDS Server Flags";
	i++;

	top.authors[i] = "zHTML xC.5375.0100.0004.";
	top.titles[i] =  "Revision History";
	i++;

	top.authors[i] = "zFLDR xB.5375.0300.";
	top.titles[i] =  "NDS Event Services";
	i++;

	top.authors[i] = "zFLDR xC.5375.0300.0001.";
	top.titles[i] =  "Concepts";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0001.0001.";
	top.titles[i] =  "NDS Event Introduction";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0001.0002.";
	top.titles[i] =  "NDS Event Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0001.0003.";
	top.titles[i] =  "NDS Event Priorities";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0001.0004.";
	top.titles[i] =  "NDS Event Data Filtering";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0001.0005.";
	top.titles[i] =  "NDS Event Types";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0001.0006.";
	top.titles[i] =  "Global Network Monitoring";
	i++;

	top.authors[i] = "zFLDR xC.5375.0300.0002.";
	top.titles[i] =  "Tasks";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0002.0001.";
	top.titles[i] =  "Monitoring NDS Events";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0002.0002.";
	top.titles[i] =  "Registering for NDS Events";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0002.0003.";
	top.titles[i] =  "Unregistering for NDS Events";
	i++;

	top.authors[i] = "zFLDR xC.5375.0300.0003.";
	top.titles[i] =  "Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0001.";
	top.titles[i] =  "NWDSEConvertEntryName";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0002.";
	top.titles[i] =  "NWDSEGetLocalAttrID";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0003.";
	top.titles[i] =  "NWDSEGetLocalAttrName";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0004.";
	top.titles[i] =  "NWDSEGetLocalClassID";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0005.";
	top.titles[i] =  "NWDSEGetLocalClassName";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0006.";
	top.titles[i] =  "NWDSEGetLocalEntryID";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0007.";
	top.titles[i] =  "NWDSEGetLocalEntryName";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0008.";
	top.titles[i] =  "NWDSERegisterForEvent";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0009.";
	top.titles[i] =  "NWDSERegisterForEventWithResult";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0003.0010.";
	top.titles[i] =  "NWDSEUnRegisterForEvent";
	i++;

	top.authors[i] = "zFLDR xC.5375.0300.0004.";
	top.titles[i] =  "Structures";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0001.";
	top.titles[i] =  "DSEACL";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0002.";
	top.titles[i] =  "DSEBackLink";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0003.";
	top.titles[i] =  "DSEBinderyObjectInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0004.";
	top.titles[i] =  "DSEBitString";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0005.";
	top.titles[i] =  "DSEChangeConnState";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0006.";
	top.titles[i] =  "DSECIList";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0007.";
	top.titles[i] =  "DSEDebugInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0008.";
	top.titles[i] =  "DSEEmailAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0009.";
	top.titles[i] =  "DSEEntryInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0010.";
	top.titles[i] =  "DSEEntryInfo2";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0011.";
	top.titles[i] =  "DSEEventData";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0012.";
	top.titles[i] =  "DSEFaxNumber";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0013.";
	top.titles[i] =  "DSEHold";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0014.";
	top.titles[i] =  "DSEModuleState";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0015.";
	top.titles[i] =  "DSENetAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0016.";
	top.titles[i] =  "DSEOctetList";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0017.";
	top.titles[i] =  "DSEPath";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0018.";
	top.titles[i] =  "DSEReplicaPointer";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0019.";
	top.titles[i] =  "DSESEVInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0020.";
	top.titles[i] =  "DSETimeStamp";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0021.";
	top.titles[i] =  "DSETraceInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0022.";
	top.titles[i] =  "DSETypedName";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0023.";
	top.titles[i] =  "DSEVALData";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0004.0024.";
	top.titles[i] =  "DSEValueInfo";
	i++;

	top.authors[i] = "zFLDR xC.5375.0300.0005.";
	top.titles[i] =  "Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0005.0001.";
	top.titles[i] =  "Event Priorities";
	i++;

	top.authors[i] = "zHTML xD.5375.0300.0005.0002.";
	top.titles[i] =  "Event Types";
	i++;

	top.authors[i] = "zHTML xC.5375.0300.0006.";
	top.titles[i] =  "Revision History";
	i++;

	top.authors[i] = "zFLDR xB.5375.0600.";
	top.titles[i] =  "NDS Technical Overview";
	i++;

	top.authors[i] = "zFLDR xC.5375.0600.0001.";
	top.titles[i] =  "NDS as the Internet Directory";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0001.0001.";
	top.titles[i] =  "Requirements for Networks and the Internet";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0001.0002.";
	top.titles[i] =  "NDS Compliance to X.500 Standard";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0001.0003.";
	top.titles[i] =  "NDS Compliance with LDAP v3";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0001.0004.";
	top.titles[i] =  "Directory Access Protocols";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0001.0005.";
	top.titles[i] =  "Programming Interfaces for NDS";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0001.0006.";
	top.titles[i] =  "NDS Architecture";
	i++;

	top.authors[i] = "zFLDR xC.5375.0600.0002.";
	top.titles[i] =  "NDS Objects";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0002.0001.";
	top.titles[i] =  "NDS Names";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0002.0002.";
	top.titles[i] =  "Types of Information Stored in NDS";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0002.0003.";
	top.titles[i] =  "Retrieval of Information from NDS";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0002.0004.";
	top.titles[i] =  "Tree Walking";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0002.0005.";
	top.titles[i] =  "NDS Object Management";
	i++;

	top.authors[i] = "zFLDR xC.5375.0600.0003.";
	top.titles[i] =  "NDS Security";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0003.0001.";
	top.titles[i] =  "Authentication";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0003.0002.";
	top.titles[i] =  "Access Control Lists";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0003.0003.";
	top.titles[i] =  "Inheritance";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0003.0004.";
	top.titles[i] =  "NetWare File System";
	i++;

	top.authors[i] = "zFLDR xC.5375.0600.0004.";
	top.titles[i] =  "Partitions and Replicas";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0004.0001.";
	top.titles[i] =  "Partitioning";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0004.0002.";
	top.titles[i] =  "Replication";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0004.0003.";
	top.titles[i] =  "Distributed Reference Management";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0004.0004.";
	top.titles[i] =  "Partition Operations";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0004.0005.";
	top.titles[i] =  "Synchronization";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0004.0006.";
	top.titles[i] =  "Background Processes";
	i++;

	top.authors[i] = "zFLDR xC.5375.0600.0005.";
	top.titles[i] =  "Bindery Services";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0005.0001.";
	top.titles[i] =  "NDS Bindery Context";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0005.0002.";
	top.titles[i] =  "Bindery Context Path";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0005.0003.";
	top.titles[i] =  "Bindery Context Eclipsing";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0005.0004.";
	top.titles[i] =  "NDS Bindery Objects";
	i++;

	top.authors[i] = "zFLDR xC.5375.0600.0006.";
	top.titles[i] =  "NDS Return Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0006.0001.";
	top.titles[i] =  "NDS Return Values from the Operating System";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0006.0002.";
	top.titles[i] =  "NDS Client Return Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0006.0003.";
	top.titles[i] =  "NDS Agent Return Values";
	i++;

	top.authors[i] = "zFLDR xC.5375.0600.0007.";
	top.titles[i] =  "Directory Services Trace Utilities";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0007.0001.";
	top.titles[i] =  "Using the DSTrace NLM";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0007.0002.";
	top.titles[i] =  "Using Basic SET DSTrace Commands";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0007.0003.";
	top.titles[i] =  "Starting Background Processes with SET DSTrace";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0007.0004.";
	top.titles[i] =  "Tuning Background Processes";
	i++;

	top.authors[i] = "zHTML xD.5375.0600.0007.0005.";
	top.titles[i] =  "Enabling DSTrace Messages with SET DSTrace";
	i++;

	top.authors[i] = "zHTML xC.5375.0600.0008.";
	top.titles[i] =  "Revision History";
	i++;

	top.authors[i] = "zFLDR xB.5375.0200.";
	top.titles[i] =  "NDS Core Services";
	i++;

	top.authors[i] = "zFLDR xC.5375.0200.0001.";
	top.titles[i] =  "Programming Concepts";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0001.";
	top.titles[i] =  "Context Handles";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0002.";
	top.titles[i] =  "Buffer Management";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0003.";
	top.titles[i] =  "Read Requests for Object Information";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0004.";
	top.titles[i] =  "Search Requests";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0005.";
	top.titles[i] =  "Developing in a Loosely Consistent Environment";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0006.";
	top.titles[i] =  "Add Object Requests";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0007.";
	top.titles[i] =  "NDS Security and Applications";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0008.";
	top.titles[i] =  "Authentication of Client Applications";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0009.";
	top.titles[i] =  "Multiple Tree Support";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0010.";
	top.titles[i] =  "Effective Rights Function";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0011.";
	top.titles[i] =  "Partition Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0012.";
	top.titles[i] =  "Replica Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0013.";
	top.titles[i] =  "Read Requests for Schema Information";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0001.0014.";
	top.titles[i] =  "Schema Extension Requests";
	i++;

	top.authors[i] = "zFLDR xC.5375.0200.0002.";
	top.titles[i] =  "Tasks";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0002.0001.";
	top.titles[i] =  "Context Handle Tasks";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0002.0002.";
	top.titles[i] =  "Buffer Tasks";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0002.0003.";
	top.titles[i] =  "Authentication and Connection Tasks";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0002.0004.";
	top.titles[i] =  "Object Tasks";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0002.0005.";
	top.titles[i] =  "Partition and Replica Tasks";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0002.0006.";
	top.titles[i] =  "Schema Tasks";
	i++;

	top.authors[i] = "zFLDR xC.5375.0200.0003.";
	top.titles[i] =  "Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0001.";
	top.titles[i] =  "NWDSAbbreviateName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0002.";
	top.titles[i] =  "NWDSAbortPartitionOperation";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0003.";
	top.titles[i] =  "NWDSAddFilterToken";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0004.";
	top.titles[i] =  "NWDSAddObject";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0005.";
	top.titles[i] =  "NWDSAddPartition (obsolete&#45;&#45;&#45;moved from .h file 11/99)";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0006.";
	top.titles[i] =  "NWDSAddReplica";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0007.";
	top.titles[i] =  "NWDSAddSecurityEquiv";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0008.";
	top.titles[i] =  "NWDSAllocBuf";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0009.";
	top.titles[i] =  "NWDSAllocFilter";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0010.";
	top.titles[i] =  "NWDSAuditGetObjectID";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0011.";
	top.titles[i] =  "NWDSAuthenticate";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0012.";
	top.titles[i] =  "NWDSAuthenticateConn";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0013.";
	top.titles[i] =  "NWDSAuthenticateConnEx";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0014.";
	top.titles[i] =  "NWDSBackupObject";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0015.";
	top.titles[i] =  "NWDSBeginClassItem";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0016.";
	top.titles[i] =  "NWDSCanDSAuthenticate";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0017.";
	top.titles[i] =  "NWDSCanonicalizeName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0018.";
	top.titles[i] =  "NWDSChangeObjectPassword";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0019.";
	top.titles[i] =  "NWDSChangeReplicaType";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0020.";
	top.titles[i] =  "NWDSCIStringsMatch";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0021.";
	top.titles[i] =  "NWDSCloseIteration";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0022.";
	top.titles[i] =  "NWDSCompare";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0023.";
	top.titles[i] =  "NWDSComputeAttrValSize";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0024.";
	top.titles[i] =  "NWDSCreateContext (obsolete&#45;&#45;&#45;moved from .h file 6/99)";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0025.";
	top.titles[i] =  "NWDSCreateContextHandle";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0026.";
	top.titles[i] =  "NWDSDefineAttr";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0027.";
	top.titles[i] =  "NWDSDefineClass";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0028.";
	top.titles[i] =  "NWDSDelFilterToken";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0029.";
	top.titles[i] =  "NWDSDuplicateContext (obsolete 03/99)";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0030.";
	top.titles[i] =  "NWDSDuplicateContextHandle";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0031.";
	top.titles[i] =  "NWDSExtSyncList";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0032.";
	top.titles[i] =  "NWDSExtSyncRead";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0033.";
	top.titles[i] =  "NWDSExtSyncSearch";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0034.";
	top.titles[i] =  "NWDSFreeBuf";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0035.";
	top.titles[i] =  "NWDSFreeContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0036.";
	top.titles[i] =  "NWDSFreeFilter";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0037.";
	top.titles[i] =  "NWDSGenerateObjectKeyPair";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0038.";
	top.titles[i] =  "NWDSGetAttrCount";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0039.";
	top.titles[i] =  "NWDSGetAttrDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0040.";
	top.titles[i] =  "NWDSGetAttrName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0041.";
	top.titles[i] =  "NWDSGetAttrVal";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0042.";
	top.titles[i] =  "NWDSGetAttrValFlags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0043.";
	top.titles[i] =  "NWDSGetAttrValModTime";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0044.";
	top.titles[i] =  "NWDSGetBinderyContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0045.";
	top.titles[i] =  "NWDSGetClassDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0046.";
	top.titles[i] =  "NWDSGetClassDefCount";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0047.";
	top.titles[i] =  "NWDSGetClassItem";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0048.";
	top.titles[i] =  "NWDSGetClassItemCount";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0049.";
	top.titles[i] =  "NWDSGetContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0050.";
	top.titles[i] =  "NWDSGetCountByClassAndName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0051.";
	top.titles[i] =  "NWDSGetCurrentUser";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0052.";
	top.titles[i] =  "NWDSGetDefNameContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0053.";
	top.titles[i] =  "NWDSGetDSIInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0054.";
	top.titles[i] =  "NWDSGetDSVerInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0055.";
	top.titles[i] =  "NWDSGetEffectiveRights";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0056.";
	top.titles[i] =  "NWDSGetMonitoredConnRef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0057.";
	top.titles[i] =  "NWDSGetNDSInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0058.";
	top.titles[i] =  "NWDSGetObjectCount";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0059.";
	top.titles[i] =  "NWDSGetObjectHostServerAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0060.";
	top.titles[i] =  "NWDSGetObjectName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0061.";
	top.titles[i] =  "NWDSGetObjectNameAndInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0062.";
	top.titles[i] =  "NWDSGetPartitionExtInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0063.";
	top.titles[i] =  "NWDSGetPartitionExtInfoPtr";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0064.";
	top.titles[i] =  "NWDSGetPartitionInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0065.";
	top.titles[i] =  "NWDSGetPartitionRoot";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0066.";
	top.titles[i] =  "NWDSGetServerAddresses (obsolete 3/98)";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0067.";
	top.titles[i] =  "NWDSGetServerAddresses2";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0068.";
	top.titles[i] =  "NWDSGetServerDN";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0069.";
	top.titles[i] =  "NWDSGetServerName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0070.";
	top.titles[i] =  "NWDSGetSyntaxCount";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0071.";
	top.titles[i] =  "NWDSGetSyntaxDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0072.";
	top.titles[i] =  "NWDSGetSyntaxID";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0073.";
	top.titles[i] =  "NWDSInitBuf";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0074.";
	top.titles[i] =  "NWDSInspectEntry";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0075.";
	top.titles[i] =  "NWDSJoinPartitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0076.";
	top.titles[i] =  "NWDSList";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0077.";
	top.titles[i] =  "NWDSListAttrsEffectiveRights";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0078.";
	top.titles[i] =  "NWDSListByClassAndName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0079.";
	top.titles[i] =  "NWDSListContainableClasses";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0080.";
	top.titles[i] =  "NWDSListContainers";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0081.";
	top.titles[i] =  "NWDSListPartitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0082.";
	top.titles[i] =  "NWDSListPartitionsExtInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0083.";
	top.titles[i] =  "NWDSLogin";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0084.";
	top.titles[i] =  "NWDSLoginAsServer";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0085.";
	top.titles[i] =  "NWDSLogout";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0086.";
	top.titles[i] =  "NWDSMapIDToName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0087.";
	top.titles[i] =  "NWDSMapNameToID";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0088.";
	top.titles[i] =  "NWDSModifyClassDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0089.";
	top.titles[i] =  "NWDSModifyDN";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0090.";
	top.titles[i] =  "NWDSModifyObject";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0091.";
	top.titles[i] =  "NWDSModifyRDN";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0092.";
	top.titles[i] =  "NWDSMoveObject";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0093.";
	top.titles[i] =  "NWDSMutateObject";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0094.";
	top.titles[i] =  "NWDSOpenConnToNDSServer";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0095.";
	top.titles[i] =  "NWDSOpenMonitoredConn";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0096.";
	top.titles[i] =  "NWDSOpenStream";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0097.";
	top.titles[i] =  "NWDSPartitionReceiveAllUpdates";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0098.";
	top.titles[i] =  "NWDSPartitionSendAllUpdates";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0099.";
	top.titles[i] =  "NWDSPutAttrName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0100.";
	top.titles[i] =  "NWDSPutAttrNameAndVal";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0101.";
	top.titles[i] =  "NWDSPutAttrVal";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0102.";
	top.titles[i] =  "NWDSPutChange";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0103.";
	top.titles[i] =  "NWDSPutChangeAndVal";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0104.";
	top.titles[i] =  "NWDSPutClassItem";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0105.";
	top.titles[i] =  "NWDSPutClassName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0106.";
	top.titles[i] =  "NWDSPutFilter";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0107.";
	top.titles[i] =  "NWDSPutSyntaxName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0108.";
	top.titles[i] =  "NWDSRead";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0109.";
	top.titles[i] =  "NWDSReadAttrDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0110.";
	top.titles[i] =  "NWDSReadClassDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0111.";
	top.titles[i] =  "NWDSReadNDSInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0112.";
	top.titles[i] =  "NWDSReadObjectDSIInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0113.";
	top.titles[i] =  "NWDSReadObjectInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0114.";
	top.titles[i] =  "NWDSReadReferences";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0115.";
	top.titles[i] =  "NWDSReadSyntaxDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0116.";
	top.titles[i] =  "NWDSReadSyntaxes";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0117.";
	top.titles[i] =  "NWDSReloadDS";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0118.";
	top.titles[i] =  "NWDSRemoveAllTypes";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0119.";
	top.titles[i] =  "NWDSRemoveAttrDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0120.";
	top.titles[i] =  "NWDSRemoveClassDef";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0121.";
	top.titles[i] =  "NWDSRemoveObject";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0122.";
	top.titles[i] =  "NWDSRemovePartition";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0123.";
	top.titles[i] =  "NWDSRemoveReplica";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0124.";
	top.titles[i] =  "NWDSRemSecurityEquiv";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0125.";
	top.titles[i] =  "NWDSRepairTimeStamps";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0126.";
	top.titles[i] =  "NWDSReplaceAttrNameAbbrev";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0127.";
	top.titles[i] =  "NWDSResolveName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0128.";
	top.titles[i] =  "NWDSRestoreObject";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0129.";
	top.titles[i] =  "NWDSReturnBlockOfAvailableTrees";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0130.";
	top.titles[i] =  "NWDSScanConnsForTrees";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0131.";
	top.titles[i] =  "NWDSScanForAvailableTrees";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0132.";
	top.titles[i] =  "NWDSSearch";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0133.";
	top.titles[i] =  "NWDSSetContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0134.";
	top.titles[i] =  "NWDSSetCurrentUser";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0135.";
	top.titles[i] =  "NWDSSetDefNameContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0136.";
	top.titles[i] =  "NWDSSetMonitoredConnection";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0137.";
	top.titles[i] =  "NWDSSplitPartition";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0138.";
	top.titles[i] =  "NWDSSyncPartition";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0139.";
	top.titles[i] =  "NWDSSyncReplicaToServer";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0140.";
	top.titles[i] =  "NWDSSyncSchema";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0141.";
	top.titles[i] =  "NWDSUnlockConnection";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0142.";
	top.titles[i] =  "NWDSVerifyObjectPassword";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0143.";
	top.titles[i] =  "NWDSWhoAmI";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0144.";
	top.titles[i] =  "NWGetDefaultNameContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0145.";
	top.titles[i] =  "NWGetFileServerUTCTime";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0146.";
	top.titles[i] =  "NWGetNumConnections";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0147.";
	top.titles[i] =  "NWGetNWNetVersion";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0148.";
	top.titles[i] =  "NWGetPreferredConnName";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0149.";
	top.titles[i] =  "NWIsDSAuthenticated";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0150.";
	top.titles[i] =  "NWIsDSServer";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0151.";
	top.titles[i] =  "NWNetInit";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0152.";
	top.titles[i] =  "NWNetTerm";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0153.";
	top.titles[i] =  "NWSetDefaultNameContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0003.0154.";
	top.titles[i] =  "NWSetPreferredDSTree";
	i++;

	top.authors[i] = "zFLDR xC.5375.0200.0004.";
	top.titles[i] =  "Structures";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0001.";
	top.titles[i] =  "Asn1ID_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0002.";
	top.titles[i] =  "Attr_Info_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0003.";
	top.titles[i] =  "Back_Link_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0004.";
	top.titles[i] =  "Bit_String_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0005.";
	top.titles[i] =  "Buf_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0006.";
	top.titles[i] =  "CI_List_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0007.";
	top.titles[i] =  "Class_Info_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0008.";
	top.titles[i] =  "EMail_Address_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0009.";
	top.titles[i] =  "Fax_Number_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0010.";
	top.titles[i] =  "Filter_Cursor_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0011.";
	top.titles[i] =  "Filter_Node_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0012.";
	top.titles[i] =  "Hold_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0013.";
	top.titles[i] =  "NDSOSVersion_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0014.";
	top.titles[i] =  "NDSStatsInfo_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0015.";
	top.titles[i] =  "Net_Address_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0016.";
	top.titles[i] =  "NWDS_TimeStamp_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0017.";
	top.titles[i] =  "Object_ACL_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0018.";
	top.titles[i] =  "Object_Info_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0019.";
	top.titles[i] =  "Octet_List_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0020.";
	top.titles[i] =  "Octet_String_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0021.";
	top.titles[i] =  "Path_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0022.";
	top.titles[i] =  "Replica_Pointer_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0023.";
	top.titles[i] =  "Syntax_Info_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0024.";
	top.titles[i] =  "TimeStamp_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0025.";
	top.titles[i] =  "Typed_Name_T";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0004.0026.";
	top.titles[i] =  "Unknown_Attr_T";
	i++;

	top.authors[i] = "zFLDR xC.5375.0200.0005.";
	top.titles[i] =  "Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0001.";
	top.titles[i] =  "Attribute Constraint Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0002.";
	top.titles[i] =  "Attribute Value Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0003.";
	top.titles[i] =  "Buffer Operation Types and Related Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0004.";
	top.titles[i] =  "Class Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0005.";
	top.titles[i] =  "Change Types for Modifying Objects";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0006.";
	top.titles[i] =  "Context Keys and Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0007.";
	top.titles[i] =  "Default Context Key Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0008.";
	top.titles[i] =  "DCK_FLAGS Bit Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0009.";
	top.titles[i] =  "DCK_NAME_FORM Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0010.";
	top.titles[i] =  "DCK_CONFIDENCE Bit Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0011.";
	top.titles[i] =  "DCK_DSI_FLAGS Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0012.";
	top.titles[i] =  "DSI_ENTRY_FLAGS Values";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0013.";
	top.titles[i] =  "Filter Tokens";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0014.";
	top.titles[i] =  "Information Types for Attribute Definitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0015.";
	top.titles[i] =  "Information Types for Class Definitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0016.";
	top.titles[i] =  "Information Types for Search and Read";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0017.";
	top.titles[i] =  "Name Space Types";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0018.";
	top.titles[i] =  "NDS Access Control Rights";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0019.";
	top.titles[i] =  "NDS Ping Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0020.";
	top.titles[i] =  "DSP Replica Information Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0021.";
	top.titles[i] =  "Network Address Types";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0022.";
	top.titles[i] =  "Scope Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0023.";
	top.titles[i] =  "Replica Types";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0024.";
	top.titles[i] =  "Replica States";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0025.";
	top.titles[i] =  "Syntax Matching Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0005.0026.";
	top.titles[i] =  "Syntax IDs";
	i++;

	top.authors[i] = "zFLDR xC.5375.0200.0006.";
	top.titles[i] =  "NDS Example Code";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0006.0001.";
	top.titles[i] =  "Context Handle";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0006.0002.";
	top.titles[i] =  "Object and Attribute";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0006.0003.";
	top.titles[i] =  "Browsing and Searching";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0006.0004.";
	top.titles[i] =  "Batch Modification of Objects and Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0200.0006.0005.";
	top.titles[i] =  "Schema";
	i++;

	top.authors[i] = "zHTML xC.5375.0200.0007.";
	top.titles[i] =  "Revision History";
	i++;

	top.authors[i] = "zFLDR xB.5375.0500.";
	top.titles[i] =  "NDS Schema Reference";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0001.";
	top.titles[i] =  "Schema Concepts";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0001.";
	top.titles[i] =  "Schema Structure";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0002.";
	top.titles[i] =  "Schema Components";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0003.";
	top.titles[i] =  "Object Classes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0004.";
	top.titles[i] =  "Naming Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0005.";
	top.titles[i] =  "Containment Classes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0006.";
	top.titles[i] =  "Super Classes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0007.";
	top.titles[i] =  "Object Class Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0008.";
	top.titles[i] =  "Mandatory and Optional Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0009.";
	top.titles[i] =  "Default ACL Templates";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0010.";
	top.titles[i] =  "Auxiliary Classes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0011.";
	top.titles[i] =  "Attribute Type Definitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0012.";
	top.titles[i] =  "Attribute Syntax Definitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0001.0013.";
	top.titles[i] =  "Schema Extensions";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0002.";
	top.titles[i] =  "Base Object Class Definitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0001.";
	top.titles[i] =  "AFP Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0002.";
	top.titles[i] =  "Alias";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0003.";
	top.titles[i] =  "applicationEntity";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0004.";
	top.titles[i] =  "applicationProcess";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0005.";
	top.titles[i] =  "Audit:File Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0006.";
	top.titles[i] =  "Bindery Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0007.";
	top.titles[i] =  "Bindery Queue";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0008.";
	top.titles[i] =  "certificationAuthority";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0009.";
	top.titles[i] =  "CommExec";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0010.";
	top.titles[i] =  "Computer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0011.";
	top.titles[i] =  "Country";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0012.";
	top.titles[i] =  "cRLDistributionPoint";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0013.";
	top.titles[i] =  "dcObject";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0014.";
	top.titles[i] =  "Device";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0015.";
	top.titles[i] =  "Directory Map";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0016.";
	top.titles[i] =  "domain";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0017.";
	top.titles[i] =  "dSA";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0018.";
	top.titles[i] =  "External Entity";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0019.";
	top.titles[i] =  "Group";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0020.";
	top.titles[i] =  "LDAP Group";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0021.";
	top.titles[i] =  "LDAP Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0022.";
	top.titles[i] =  "List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0023.";
	top.titles[i] =  "Locality";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0024.";
	top.titles[i] =  "MASV:Security Policy";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0025.";
	top.titles[i] =  "Message Routing Group";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0026.";
	top.titles[i] =  "Messaging Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0027.";
	top.titles[i] =  "NCP Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0028.";
	top.titles[i] =  "ndsLoginProperties";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0029.";
	top.titles[i] =  "NDSPKI:Certificate Authority";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0030.";
	top.titles[i] =  "NDSPKI:Key Material";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0031.";
	top.titles[i] =  "NDSPKI:SD Key Access Partition";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0032.";
	top.titles[i] =  "NDSPKI:SD Key List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0033.";
	top.titles[i] =  "NDSPKI:Trusted Root";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0034.";
	top.titles[i] =  "NDSPKI:Trusted Root Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0035.";
	top.titles[i] =  "NSCP:groupOfCertificates";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0036.";
	top.titles[i] =  "NSCP:mailGroup1";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0037.";
	top.titles[i] =  "NSCP:mailRecipient";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0038.";
	top.titles[i] =  "NSCP:NetscapeMailServer5";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0039.";
	top.titles[i] =  "NSCP:NetscapeServer5";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0040.";
	top.titles[i] =  "NSCP:nginfo3";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0041.";
	top.titles[i] =  "NSCP:nsLicenseUser";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0042.";
	top.titles[i] =  "Organization";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0043.";
	top.titles[i] =  "Organizational Person";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0044.";
	top.titles[i] =  "Organizational Role";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0045.";
	top.titles[i] =  "Organizational Unit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0046.";
	top.titles[i] =  "Partition";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0047.";
	top.titles[i] =  "Person";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0048.";
	top.titles[i] =  "pkiCA";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0049.";
	top.titles[i] =  "pkiUser";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0050.";
	top.titles[i] =  "Print Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0051.";
	top.titles[i] =  "Printer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0052.";
	top.titles[i] =  "Profile";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0053.";
	top.titles[i] =  "Queue";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0054.";
	top.titles[i] =  "Resource";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0055.";
	top.titles[i] =  "SAS:Security";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0056.";
	top.titles[i] =  "SAS:Service";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0057.";
	top.titles[i] =  "Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0058.";
	top.titles[i] =  "strongAuthenticationUser";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0059.";
	top.titles[i] =  "Template";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0060.";
	top.titles[i] =  "Top";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0061.";
	top.titles[i] =  "Tree Root";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0062.";
	top.titles[i] =  "Unknown";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0063.";
	top.titles[i] =  "User";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0064.";
	top.titles[i] =  "userSecurityInformation";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0065.";
	top.titles[i] =  "Volume";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0002.0066.";
	top.titles[i] =  "WANMAN:LAN Area";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0003.";
	top.titles[i] =  "Novell Object Class Extensions";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0001.";
	top.titles[i] =  "Entrust:CRLDistributionPoint";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0002.";
	top.titles[i] =  "inetOrgPerson";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0003.";
	top.titles[i] =  "NDPS Broker";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0004.";
	top.titles[i] =  "NDPS Manager";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0005.";
	top.titles[i] =  "NDPS Printer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0006.";
	top.titles[i] =  "NDSCat:Catalog";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0007.";
	top.titles[i] =  "NDSCat:Master Catalog";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0008.";
	top.titles[i] =  "NDSCat:Slave Catalog";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0009.";
	top.titles[i] =  "NetSvc";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0010.";
	top.titles[i] =  "NLS:License Certificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0011.";
	top.titles[i] =  "NLS:License Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0012.";
	top.titles[i] =  "NLS:Product Container";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0013.";
	top.titles[i] =  "NSCP:groupOfUniqueNames5";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0014.";
	top.titles[i] =  "NSCP:mailGroup5";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0015.";
	top.titles[i] =  "NSCP:Nginfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0016.";
	top.titles[i] =  "NSCP:Nginfo2";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0017.";
	top.titles[i] =  "residentialPerson";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0018.";
	top.titles[i] =  "SLP Scope Unit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0019.";
	top.titles[i] =  "SLP Directory Agent";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0020.";
	top.titles[i] =  "SLP Service";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0003.0021.";
	top.titles[i] =  "SMS SMDR Class";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0004.";
	top.titles[i] =  "Graphical View of Object Class Inheritance";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0001.";
	top.titles[i] =  "Alias and Bindery Object Classes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0002.";
	top.titles[i] =  "Tree Root, domain, and Unknown";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0003.";
	top.titles[i] =  "Computer, Country, Device, and Printer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0004.";
	top.titles[i] =  "List and Locality";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0005.";
	top.titles[i] =  "Organizational Role and Partition";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0006.";
	top.titles[i] =  "ndsLoginProperties, Organization, and Organizational Unit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0007.";
	top.titles[i] =  "ndsLoginProperties, Person, Organizational Person, and User";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0008.";
	top.titles[i] =  "Directory Map, Profile, Queues, Resource, and Volume";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0009.";
	top.titles[i] =  "Servers (AFP, Messaging, NCP, Print) and CommExec";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0004.0010.";
	top.titles[i] =  "External Entity, Group, and Message Routing Group";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0005.";
	top.titles[i] =  "Base Attribute Definitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0001.";
	top.titles[i] =  "Aliased Object Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0002.";
	top.titles[i] =  "Account Balance";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0003.";
	top.titles[i] =  "ACL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0004.";
	top.titles[i] =  "Allow Unlimited Credit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0005.";
	top.titles[i] =  "associatedName";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0006.";
	top.titles[i] =  "attributeCertificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0007.";
	top.titles[i] =  "Audit:A Encryption Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0008.";
	top.titles[i] =  "Audit:B Encryption Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0009.";
	top.titles[i] =  "Audit:Contents";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0010.";
	top.titles[i] =  "Audit:Current Encryption Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0011.";
	top.titles[i] =  "Audit:File Link";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0012.";
	top.titles[i] =  "Audit:Link List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0013.";
	top.titles[i] =  "Audit:Path";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0014.";
	top.titles[i] =  "Audit:Policy";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0015.";
	top.titles[i] =  "Audit:Type";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0016.";
	top.titles[i] =  "authorityRevocationList";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0017.";
	top.titles[i] =  "Authority Revocation";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0018.";
	top.titles[i] =  "AuxClass Object Class Backup";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0019.";
	top.titles[i] =  "Auxiliary Class Flag";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0020.";
	top.titles[i] =  "Back Link";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0021.";
	top.titles[i] =  "Bindery Object Restriction";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0022.";
	top.titles[i] =  "Bindery Property";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0023.";
	top.titles[i] =  "Bindery Restriction Level";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0024.";
	top.titles[i] =  "Bindery Type";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0025.";
	top.titles[i] =  "businessCategory";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0026.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0027.";
	top.titles[i] =  "cACertificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0028.";
	top.titles[i] =  "CA Private Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0029.";
	top.titles[i] =  "CA Public Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0030.";
	top.titles[i] =  "Cartridge";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0031.";
	top.titles[i] =  "certificateRevocationList";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0032.";
	top.titles[i] =  "Certificate Revocation";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0033.";
	top.titles[i] =  "Certificate Validity Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0034.";
	top.titles[i] =  "crossCertificatePair";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0035.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0036.";
	top.titles[i] =  "Convergence";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0037.";
	top.titles[i] =  "Cross Certificate Pair";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0038.";
	top.titles[i] =  "dc";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0039.";
	top.titles[i] =  "Default Queue";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0040.";
	top.titles[i] =  "deltaRevocationList";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0041.";
	top.titles[i] =  "departmentNumber";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0042.";
	top.titles[i] =  "Description";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0043.";
	top.titles[i] =  "destinationIndicator";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0044.";
	top.titles[i] =  "Detect Intruder";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0045.";
	top.titles[i] =  "Device";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0046.";
	top.titles[i] =  "dmdName";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0047.";
	top.titles[i] =  "dn";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0048.";
	top.titles[i] =  "dnQualifier";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0049.";
	top.titles[i] =  "DS Revision";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0050.";
	top.titles[i] =  "EMail Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0051.";
	top.titles[i] =  "employeeType";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0052.";
	top.titles[i] =  "enhancedSearchGuide";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0053.";
	top.titles[i] =  "Equivalent To Me";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0054.";
	top.titles[i] =  "External Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0055.";
	top.titles[i] =  "External Synchronizer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0056.";
	top.titles[i] =  "Facsimile Telephone Number";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0057.";
	top.titles[i] =  "Full Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0058.";
	top.titles[i] =  "Generational Qualifier";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0059.";
	top.titles[i] =  "generationQualifier";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0060.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0061.";
	top.titles[i] =  "Given Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0062.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0063.";
	top.titles[i] =  "GUID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0064.";
	top.titles[i] =  "High Convergence Sync Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0065.";
	top.titles[i] =  "Higher Privileges";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0066.";
	top.titles[i] =  "Home Directory";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0067.";
	top.titles[i] =  "Home Directory Rights";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0068.";
	top.titles[i] =  "homePhone";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0069.";
	top.titles[i] =  "homePostalAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0070.";
	top.titles[i] =  "houseIdentifier";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0071.";
	top.titles[i] =  "Host Device";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0072.";
	top.titles[i] =  "Host Resource Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0073.";
	top.titles[i] =  "Host Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0074.";
	top.titles[i] =  "Inherited ACL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0075.";
	top.titles[i] =  "Initials";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0076.";
	top.titles[i] =  "internationaliSDNNumber";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0077.";
	top.titles[i] =  "Internet EMail Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0078.";
	top.titles[i] =  "Intruder Attempt Reset Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0079.";
	top.titles[i] =  "Intruder Lockout Reset Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0080.";
	top.titles[i] =  "knowledgeInformation";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0081.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0082.";
	top.titles[i] =  "Language";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0083.";
	top.titles[i] =  "Last Login Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0084.";
	top.titles[i] =  "Last Referenced Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0085.";
	top.titles[i] =  "LDAP ACL v11";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0086.";
	top.titles[i] =  "LDAP Allow Clear Text Password";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0087.";
	top.titles[i] =  "LDAP Anonymous Identity";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0088.";
	top.titles[i] =  "LDAP Attribute Map v11";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0089.";
	top.titles[i] =  "LDAP Backup Log Filename";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0090.";
	top.titles[i] =  "LDAP Class Map v11";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0091.";
	top.titles[i] =  "LDAP Enable SSL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0092.";
	top.titles[i] =  "LDAP Enable TCP";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0093.";
	top.titles[i] =  "LDAP Enable UDP";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0094.";
	top.titles[i] =  "LDAP Group";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0095.";
	top.titles[i] =  "LDAP Host Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0096.";
	top.titles[i] =  "LDAP Log Filename";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0097.";
	top.titles[i] =  "LDAP Log Level";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0098.";
	top.titles[i] =  "LDAP Log Size Limit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0099.";
	top.titles[i] =  "LDAP Referral";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0100.";
	top.titles[i] =  "LDAP Screen Level";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0101.";
	top.titles[i] =  "LDAP Search Size Limit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0102.";
	top.titles[i] =  "LDAP Search Time Limit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0103.";
	top.titles[i] =  "LDAP Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0104.";
	top.titles[i] =  "LDAP Server Bind Limit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0105.";
	top.titles[i] =  "LDAP Server Idle Timeout";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0106.";
	top.titles[i] =  "LDAP Server List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0107.";
	top.titles[i] =  "LDAP SSL Port";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0108.";
	top.titles[i] =  "LDAP Suffix";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0109.";
	top.titles[i] =  "LDAP TCP Port";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0110.";
	top.titles[i] =  "LDAP UDP Port";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0111.";
	top.titles[i] =  "LDAP:bindCatalog";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0112.";
	top.titles[i] =  "LDAP:bindCatalogUsage";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0113.";
	top.titles[i] =  "LDAP:keyMaterialName";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0114.";
	top.titles[i] =  "LDAP:otherReferralUsage";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0115.";
	top.titles[i] =  "LDAP:searchCatalog";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0116.";
	top.titles[i] =  "LDAP:searchCatalogUsage";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0117.";
	top.titles[i] =  "LDAP:searchReferralUsage";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0118.";
	top.titles[i] =  "Locked By Intruder";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0119.";
	top.titles[i] =  "Lockout After Detection";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0120.";
	top.titles[i] =  "Login Allowed Time Map";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0121.";
	top.titles[i] =  "Login Disabled";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0122.";
	top.titles[i] =  "Login Expiration Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0123.";
	top.titles[i] =  "Login Grace Limit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0124.";
	top.titles[i] =  "Login Grace Remaining";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0125.";
	top.titles[i] =  "Login Intruder Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0126.";
	top.titles[i] =  "Login Intruder Attempts";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0127.";
	top.titles[i] =  "Login Intruder Limit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0128.";
	top.titles[i] =  "Login Intruder Reset Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0129.";
	top.titles[i] =  "Login Maximum Simultaneous";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0130.";
	top.titles[i] =  "Login Script";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0131.";
	top.titles[i] =  "Login Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0132.";
	top.titles[i] =  "Low Convergence Reset Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0133.";
	top.titles[i] =  "Low Convergence Sync Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0134.";
	top.titles[i] =  "Mailbox ID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0135.";
	top.titles[i] =  "Mailbox Location";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0136.";
	top.titles[i] =  "manager";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0137.";
	top.titles[i] =  "masvAuthorizedRange";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0138.";
	top.titles[i] =  "masvDefaultRange";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0139.";
	top.titles[i] =  "masvDomainPolicy";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0140.";
	top.titles[i] =  "masvLabel";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0141.";
	top.titles[i] =  "masvProposedLabel";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0142.";
	top.titles[i] =  "Member";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0143.";
	top.titles[i] =  "Members Of Template";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0144.";
	top.titles[i] =  "Memory";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0145.";
	top.titles[i] =  "Message Routing Group";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0146.";
	top.titles[i] =  "Message Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0147.";
	top.titles[i] =  "Messaging Database Location";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0148.";
	top.titles[i] =  "Messaging Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0149.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0150.";
	top.titles[i] =  "Minimum Account Balance";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0151.";
	top.titles[i] =  "mobile";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0152.";
	top.titles[i] =  "NDSPKI:Certificate Chain";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0153.";
	top.titles[i] =  "NDSPKI:Given Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0154.";
	top.titles[i] =  "NDSPKI:Key File";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0155.";
	top.titles[i] =  "NDSPKI:Key Material DN";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0156.";
	top.titles[i] =  "NDSPKI:Keystore";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0157.";
	top.titles[i] =  "NDSPKI:Not After";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0158.";
	top.titles[i] =  "NDSPKI:Not Before";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0159.";
	top.titles[i] =  "NDSPKI:Parent CA";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0160.";
	top.titles[i] =  "NDSPKI:Parent CA DN";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0161.";
	top.titles[i] =  "NDSPKI:Private Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0162.";
	top.titles[i] =  "NDSPKI:Public Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0163.";
	top.titles[i] =  "NDSPKI:Public Key Certificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0164.";
	top.titles[i] =  "NDSPKI:SD Key Cert";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0165.";
	top.titles[i] =  "NDSPKI:SD Key ID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0166.";
	top.titles[i] =  "NDSPKI:SD Key Server DN";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0167.";
	top.titles[i] =  "NDSPKI:SD Key Struct";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0168.";
	top.titles[i] =  "NDSPKI:Subject Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0169.";
	top.titles[i] =  "NDSPKI:Tree CA DN";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0170.";
	top.titles[i] =  "NDSPKI:Trusted Root Certificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0171.";
	top.titles[i] =  "NDSPKI:userCertificateInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0172.";
	top.titles[i] =  "Network Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0173.";
	top.titles[i] =  "Network Address Restriction";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0174.";
	top.titles[i] =  "New Object's DS Rights";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0175.";
	top.titles[i] =  "New Object's FS Rights";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0176.";
	top.titles[i] =  "New Object's Self Rights";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0177.";
	top.titles[i] =  "NNS Domain";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0178.";
	top.titles[i] =  "Notify";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0179.";
	top.titles[i] =  "NSCP:administratorContactInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0180.";
	top.titles[i] =  "NSCP:adminURL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0181.";
	top.titles[i] =  "NSCP:AmailAccessDomain";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0182.";
	top.titles[i] =  "NSCP:AmailAlternateAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0183.";
	top.titles[i] =  "NSCP:AmailAutoReplyMode";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0184.";
	top.titles[i] =  "NSCP:AmailAutoReplyText";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0185.";
	top.titles[i] =  "NSCP:AmailDeliveryOption";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0186.";
	top.titles[i] =  "NSCP:AmailForwardingAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0187.";
	top.titles[i] =  "NSCP:AmailHost";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0188.";
	top.titles[i] =  "NSCP:AmailMessageStore";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0189.";
	top.titles[i] =  "NSCP:AmailProgramDeliveryInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0190.";
	top.titles[i] =  "NSCP:AmailQuota";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0191.";
	top.titles[i] =  "NSCP:AnsLicenseEndTime";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0192.";
	top.titles[i] =  "NSCP:AnsLicensedFor";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0193.";
	top.titles[i] =  "NSCP:AnsLicenseStartTime";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0194.";
	top.titles[i] =  "NSCP:employeeNumber";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0195.";
	top.titles[i] =  "NSCP:installationTimeStamp";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0196.";
	top.titles[i] =  "NSCP:mailRoutingAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0197.";
	top.titles[i] =  "NSCP:memberCertificateDesc";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0198.";
	top.titles[i] =  "NSCP:mgrpRFC822mailmember";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0199.";
	top.titles[i] =  "NSCP:ngcomponentCIS";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0200.";
	top.titles[i] =  "NSCP:nsaclrole";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0201.";
	top.titles[i] =  "NSCP:nscreator";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0202.";
	top.titles[i] =  "NSCP:nsflags";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0203.";
	top.titles[i] =  "NSCP:nsnewsACL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0204.";
	top.titles[i] =  "NSCP:nsprettyname";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0205.";
	top.titles[i] =  "NSCP:serverHostName";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0206.";
	top.titles[i] =  "NSCP:serverProductName";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0207.";
	top.titles[i] =  "NSCP:serverRoot";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0208.";
	top.titles[i] =  "NSCP:serverVersionNumber";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0209.";
	top.titles[i] =  "NSCP:subtreeACI";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0210.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0211.";
	top.titles[i] =  "Obituary";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0212.";
	top.titles[i] =  "Obituary Notify";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0213.";
	top.titles[i] =  "Object Class";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0214.";
	top.titles[i] =  "Operator";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0215.";
	top.titles[i] =  "Other GUID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0216.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0217.";
	top.titles[i] =  "Owner";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0218.";
	top.titles[i] =  "Page Description Language";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0219.";
	top.titles[i] =  "pager";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0220.";
	top.titles[i] =  "Partition Control";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0221.";
	top.titles[i] =  "Partition Creation Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0222.";
	top.titles[i] =  "Partition Status";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0223.";
	top.titles[i] =  "Password Allow Change";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0224.";
	top.titles[i] =  "Password Expiration Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0225.";
	top.titles[i] =  "Password Expiration Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0226.";
	top.titles[i] =  "Password Management";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0227.";
	top.titles[i] =  "Password Minimum Length";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0228.";
	top.titles[i] =  "Password Required";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0229.";
	top.titles[i] =  "Password Unique Required";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0230.";
	top.titles[i] =  "Passwords Used";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0231.";
	top.titles[i] =  "Path";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0232.";
	top.titles[i] =  "Permanent Config Parms";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0233.";
	top.titles[i] =  "Physical Delivery Office Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0234.";
	top.titles[i] =  "Postal Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0235.";
	top.titles[i] =  "Postal Code";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0236.";
	top.titles[i] =  "Postal Office Box";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0237.";
	top.titles[i] =  "Postmaster";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0238.";
	top.titles[i] =  "preferredDeliveryMethod";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0239.";
	top.titles[i] =  "presentationAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0240.";
	top.titles[i] =  "Print Job Configuration";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0241.";
	top.titles[i] =  "Print Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0242.";
	top.titles[i] =  "Printer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0243.";
	top.titles[i] =  "Printer Configuration";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0244.";
	top.titles[i] =  "Printer Control";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0245.";
	top.titles[i] =  "Private Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0246.";
	top.titles[i] =  "Profile";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0247.";
	top.titles[i] =  "Profile Membership";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0248.";
	top.titles[i] =  "protocolInformation";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0249.";
	top.titles[i] =  "Public Key";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0250.";
	top.titles[i] =  "Purge Vector";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0251.";
	top.titles[i] =  "Queue";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0252.";
	top.titles[i] =  "Queue Directory";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0253.";
	top.titles[i] =  "Received Up To";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0254.";
	top.titles[i] =  "Reference";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0255.";
	top.titles[i] =  "registeredAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0256.";
	top.titles[i] =  "Replica";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0257.";
	top.titles[i] =  "Replica Up To";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0258.";
	top.titles[i] =  "Resource";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0259.";
	top.titles[i] =  "Revision";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0260.";
	top.titles[i] =  "Role Occupant";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0261.";
	top.titles[i] =  "roomNumber";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0262.";
	top.titles[i] =  "Run Setup Script";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0263.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0264.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0265.";
	top.titles[i] =  "SAP Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0266.";
	top.titles[i] =  "SAS:Security DN";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0267.";
	top.titles[i] =  "SAS:Service DN";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0268.";
	top.titles[i] =  "searchGuide";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0269.";
	top.titles[i] =  "searchSizeLimit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0270.";
	top.titles[i] =  "searchTimeLimit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0271.";
	top.titles[i] =  "Security Equals";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0272.";
	top.titles[i] =  "Security Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0273.";
	top.titles[i] =  "See Also";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0274.";
	top.titles[i] =  "Serial Number";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0275.";
	top.titles[i] =  "Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0276.";
	top.titles[i] =  "Server Holds";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0277.";
	top.titles[i] =  "Set Password After Create";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0278.";
	top.titles[i] =  "Setup Script";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0279.";
	top.titles[i] =  "Status";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0280.";
	top.titles[i] =  "supportedAlgorithms";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0281.";
	top.titles[i] =  "supportedApplicationContext";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0282.";
	top.titles[i] =  "Supported Connections";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0283.";
	top.titles[i] =  "Supported Gateway";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0284.";
	top.titles[i] =  "Supported Services";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0285.";
	top.titles[i] =  "Supported Typefaces";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0286.";
	top.titles[i] =  "Surname";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0287.";
	top.titles[i] =  "Synchronization Tolerance";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0288.";
	top.titles[i] =  "Synchronized Up To";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0289.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0290.";
	top.titles[i] =  "Telephone Number";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0291.";
	top.titles[i] =  "telexNumber";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0292.";
	top.titles[i] =  "telexTerminalIdentifier";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0293.";
	top.titles[i] =  "Timezone";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0294.";
	top.titles[i] =  "Title";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0295.";
	top.titles[i] =  "Transitive Vector";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0296.";
	top.titles[i] =  "Trustees Of New Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0297.";
	top.titles[i] =  "Type Creator Map";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0298.";
	top.titles[i] =  "";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0299.";
	top.titles[i] =  "uniqueID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0300.";
	top.titles[i] =  "Unknown";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0301.";
	top.titles[i] =  "Unknown Auxiliary Class";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0302.";
	top.titles[i] =  "Unknown Base Class";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0303.";
	top.titles[i] =  "Used By";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0304.";
	top.titles[i] =  "User";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0305.";
	top.titles[i] =  "userCertificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0306.";
	top.titles[i] =  "userPassword";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0307.";
	top.titles[i] =  "Uses";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0308.";
	top.titles[i] =  "Version";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0309.";
	top.titles[i] =  "Volume";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0310.";
	top.titles[i] =  "Volume Space Restrictions";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0311.";
	top.titles[i] =  "WANMAN:Cost";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0312.";
	top.titles[i] =  "WANMAN:Default Cost";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0313.";
	top.titles[i] =  "WANMAN:LAN Area Membership";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0314.";
	top.titles[i] =  "WANMAN:WAN Policy";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0315.";
	top.titles[i] =  "x121Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0005.0316.";
	top.titles[i] =  "x500UniqueIdentifier";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0006.";
	top.titles[i] =  "Novell Attribute Extensions";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0001.";
	top.titles[i] =  "audio";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0002.";
	top.titles[i] =  "carLicense";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0003.";
	top.titles[i] =  "Client Install Candidate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0004.";
	top.titles[i] =  "Color Supported";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0005.";
	top.titles[i] =  "Database Dir Path";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0006.";
	top.titles[i] =  "Database Volume Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0007.";
	top.titles[i] =  "Datapool Location";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0008.";
	top.titles[i] =  "Datapool Locations";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0009.";
	top.titles[i] =  "Delivery Methods Installed";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0010.";
	top.titles[i] =  "displayName";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0011.";
	top.titles[i] =  "Employee ID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0012.";
	top.titles[i] =  "Entrust:AttributeCertificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0013.";
	top.titles[i] =  "Entrust:User";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0014.";
	top.titles[i] =  "GW API Gateway Directory Path";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0015.";
	top.titles[i] =  "GW API Gateway Directory Volume";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0016.";
	top.titles[i] =  "IPP URI";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0017.";
	top.titles[i] =  "IPP URI Security Scheme";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0018.";
	top.titles[i] =  "jpegPhoto";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0019.";
	top.titles[i] =  "labeledUri";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0020.";
	top.titles[i] =  "LDAP Class Map";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0021.";
	top.titles[i] =  "ldapPhoto";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0022.";
	top.titles[i] =  "LDAPUserCertificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0023.";
	top.titles[i] =  "LDAP:ARL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0024.";
	top.titles[i] =  "LDAP:caCertificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0025.";
	top.titles[i] =  "LDAP:CRL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0026.";
	top.titles[i] =  "LDAP:crossCertificatePair";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0027.";
	top.titles[i] =  "MASV:Authorized Range";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0028.";
	top.titles[i] =  "MASV:Default Range";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0029.";
	top.titles[i] =  "MASV:Domain Policy";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0030.";
	top.titles[i] =  "MASV:Label";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0031.";
	top.titles[i] =  "MASV:Proposed Label";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0032.";
	top.titles[i] =  "Maximum Speed";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0033.";
	top.titles[i] =  "Maximum Speed Units";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0034.";
	top.titles[i] =  "MHS Send Directory Path";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0035.";
	top.titles[i] =  "MHS Send Directory Volume";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0036.";
	top.titles[i] =  "NDPS Accountant Role";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0037.";
	top.titles[i] =  "NDPS Control Flags";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0038.";
	top.titles[i] =  "NDPS Database Saved Timestamp";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0039.";
	top.titles[i] =  "NDPS Database Saved Data Image";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0040.";
	top.titles[i] =  "NDPS Database Saved Index Image";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0041.";
	top.titles[i] =  "NDPS Default Printer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0042.";
	top.titles[i] =  "NDPS Default Public Printer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0043.";
	top.titles[i] =  "NDPS Job Configuration";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0044.";
	top.titles[i] =  "NDPS Manager Status";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0045.";
	top.titles[i] =  "NDPS Operator Role";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0046.";
	top.titles[i] =  "NDPS Printer Install List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0047.";
	top.titles[i] =  "NDPS Printer Install Timestamp";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0048.";
	top.titles[i] =  "NDPS Printer Queue List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0049.";
	top.titles[i] =  "NDPS Printer Siblings";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0050.";
	top.titles[i] =  "NDPS Public Printer Install List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0051.";
	top.titles[i] =  "NDPS Replace All Client Printers";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0052.";
	top.titles[i] =  "NDPS SMTP Server";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0053.";
	top.titles[i] =  "NDPS User Role";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0054.";
	top.titles[i] =  "NDSCat:Actual All Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0055.";
	top.titles[i] =  "NDSCat:Actual Attribute Count";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0056.";
	top.titles[i] =  "NDSCat:Actual Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0057.";
	top.titles[i] =  "NDSCat:Actual Base Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0058.";
	top.titles[i] =  "NDSCat:Actual Catalog Size";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0059.";
	top.titles[i] =  "NDSCat:Actual End Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0060.";
	top.titles[i] =  "NDSCat:Actual Filter";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0061.";
	top.titles[i] =  "NDSCat:Actual Object Count";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0062.";
	top.titles[i] =  "NDSCat:Actual Return Code";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0063.";
	top.titles[i] =  "NDSCat:Actual Scope";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0064.";
	top.titles[i] =  "NDSCat:Actual Search Aliases";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0065.";
	top.titles[i] =  "NDSCat:Actual Start Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0066.";
	top.titles[i] =  "NDSCat:Actual Value Count";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0067.";
	top.titles[i] =  "NDSCat:All Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0068.";
	top.titles[i] =  "NDSCat:AttrDefTbl";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0069.";
	top.titles[i] =  "NDSCat:Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0070.";
	top.titles[i] =  "NDSCat:Auto Dredge";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0071.";
	top.titles[i] =  "NDSCat:Base Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0072.";
	top.titles[i] =  "NDSCat:CatalogDB";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0073.";
	top.titles[i] =  "NDSCat:Catalog List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0074.";
	top.titles[i] =  "NDSCat:Dredge Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0075.";
	top.titles[i] =  "NDSCat:Filter";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0076.";
	top.titles[i] =  "NDSCat:IndexDefTbl";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0077.";
	top.titles[i] =  "NDSCat:Indexes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0078.";
	top.titles[i] =  "NDSCat:Label";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0079.";
	top.titles[i] =  "NDSCat:Log";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0080.";
	top.titles[i] =  "NDSCat:Master Catalog";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0081.";
	top.titles[i] =  "NDSCat:Max Log Size";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0082.";
	top.titles[i] =  "NDSCat:Max Retries";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0083.";
	top.titles[i] =  "NDSCat:Max Threads";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0084.";
	top.titles[i] =  "NDSCat:Retry Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0085.";
	top.titles[i] =  "NDSCat:Scope";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0086.";
	top.titles[i] =  "NDSCat:Search Aliases";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0087.";
	top.titles[i] =  "NDSCat:Slave Catalog List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0088.";
	top.titles[i] =  "NDSCat:Start Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0089.";
	top.titles[i] =  "NDSCat:Synch Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0090.";
	top.titles[i] =  "NLS:Common Certificate";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0091.";
	top.titles[i] =  "NLS:Current Installed";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0092.";
	top.titles[i] =  "NLS:Current Peak Installed";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0093.";
	top.titles[i] =  "NLS:Current Peak Used";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0094.";
	top.titles[i] =  "NLS:Current Used";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0095.";
	top.titles[i] =  "NLS:Hourly Data Size";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0096.";
	top.titles[i] =  "NLS:License Database";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0097.";
	top.titles[i] =  "NLS:License ID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0098.";
	top.titles[i] =  "NLS:License Service Provider";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0099.";
	top.titles[i] =  "NLS:LSP Revision";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0100.";
	top.titles[i] =  "NLS:Owner";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0101.";
	top.titles[i] =  "NLS:Peak Installed Data";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0102.";
	top.titles[i] =  "NLS:Peak Used Data";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0103.";
	top.titles[i] =  "NLS:Product";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0104.";
	top.titles[i] =  "NLS:Publisher";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0105.";
	top.titles[i] =  "NLS:Revision";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0106.";
	top.titles[i] =  "NLS:Search Type";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0107.";
	top.titles[i] =  "NLS:Summary Update Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0108.";
	top.titles[i] =  "NLS:Summary Version";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0109.";
	top.titles[i] =  "NLS:Transaction Database";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0110.";
	top.titles[i] =  "NLS:Transaction Log Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0111.";
	top.titles[i] =  "NLS:Transaction Log Size";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0112.";
	top.titles[i] =  "NLS:Version";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0113.";
	top.titles[i] =  "Notification Consumers";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0114.";
	top.titles[i] =  "Notification Profile";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0115.";
	top.titles[i] =  "Notification Service Enabled";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0116.";
	top.titles[i] =  "Notification Srvc Net Addr";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0117.";
	top.titles[i] =  "Notification Srvc Net Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0118.";
	top.titles[i] =  "NRD:Registry Data";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0119.";
	top.titles[i] =  "NRD:Registry Index";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0120.";
	top.titles[i] =  "NSCP:mailAccessDomain";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0121.";
	top.titles[i] =  "NSCP:mailAlternateAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0122.";
	top.titles[i] =  "NSCP:mailAutoReplyMode";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0123.";
	top.titles[i] =  "NSCP:mailAutoReplyText";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0124.";
	top.titles[i] =  "NSCP:mailDeliveryOption";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0125.";
	top.titles[i] =  "NSCP:mailForwardingAddress";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0126.";
	top.titles[i] =  "NSCP:mailHost";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0127.";
	top.titles[i] =  "NSCP:mailMessageStore";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0128.";
	top.titles[i] =  "NSCP:mailProgramDeliveryInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0129.";
	top.titles[i] =  "NSCP:mailQuota";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0130.";
	top.titles[i] =  "NSCP:ngComponent";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0131.";
	top.titles[i] =  "NSCP:nsLicenseEndTime";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0132.";
	top.titles[i] =  "NSCP:nsLicensedFor";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0133.";
	top.titles[i] =  "NSCP:nsLicenseStartTime";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0134.";
	top.titles[i] =  "Page Description Languages";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0135.";
	top.titles[i] =  "preferredLanguage";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0136.";
	top.titles[i] =  "Primary Notification Service";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0137.";
	top.titles[i] =  "Primary Resource Service";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0138.";
	top.titles[i] =  "Printer Agent Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0139.";
	top.titles[i] =  "Printer Manufacturer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0140.";
	top.titles[i] =  "Printer Mechanism Types";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0141.";
	top.titles[i] =  "Printer Model";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0142.";
	top.titles[i] =  "Printer Status";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0143.";
	top.titles[i] =  "Printer to PA ID Mappings";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0144.";
	top.titles[i] =  "PSM Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0145.";
	top.titles[i] =  "Registry Advertising Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0146.";
	top.titles[i] =  "Registry Service Enabled";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0147.";
	top.titles[i] =  "Registry Srvc Net Addr";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0148.";
	top.titles[i] =  "Registry Srvc Net Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0149.";
	top.titles[i] =  "Resolution";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0150.";
	top.titles[i] =  "Resource Mgmt Srvc Net Addr";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0151.";
	top.titles[i] =  "Resource Mgmt Srvc Net Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0152.";
	top.titles[i] =  "Resource Mgmt Service Enabled";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0153.";
	top.titles[i] =  "Resource Mgr Database Path";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0154.";
	top.titles[i] =  "Resource Mgr Database Volume";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0155.";
	top.titles[i] =  "secretary";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0156.";
	top.titles[i] =  "Sides Supported";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0157.";
	top.titles[i] =  "SLP Attribute";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0158.";
	top.titles[i] =  "SLP Cache Limit";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0159.";
	top.titles[i] =  "SLP DA Back Link";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0160.";
	top.titles[i] =  "SLP Directory Agent DN";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0161.";
	top.titles[i] =  "SLP Language";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0162.";
	top.titles[i] =  "SLP Lifetime";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0163.";
	top.titles[i] =  "SLP Scope Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0164.";
	top.titles[i] =  "SLP Scope Unit DN";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0165.";
	top.titles[i] =  "SLP Start Purge Hour";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0166.";
	top.titles[i] =  "SLP Status";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0167.";
	top.titles[i] =  "SLP SU Back Link";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0168.";
	top.titles[i] =  "SLP SU Type";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0169.";
	top.titles[i] =  "SLP Type";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0170.";
	top.titles[i] =  "SLP URL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0171.";
	top.titles[i] =  "SMS Protocol Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0172.";
	top.titles[i] =  "SMS Registered Service";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0173.";
	top.titles[i] =  "SU";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0174.";
	top.titles[i] =  "SvcInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0175.";
	top.titles[i] =  "SvcType";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0176.";
	top.titles[i] =  "SvcTypeID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0006.0177.";
	top.titles[i] =  "userSMIMECertificate";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0007.";
	top.titles[i] =  "LDAP Operational Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0001.";
	top.titles[i] =  "createTimeStamp";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0002.";
	top.titles[i] =  "creatorsName";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0003.";
	top.titles[i] =  "entryFlags";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0004.";
	top.titles[i] =  "federationBoundary";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0005.";
	top.titles[i] =  "localEntryID";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0006.";
	top.titles[i] =  "modifiersName";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0007.";
	top.titles[i] =  "modifyTimeStamp";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0008.";
	top.titles[i] =  "structuralObjectClass";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0009.";
	top.titles[i] =  "subordinateCount";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0007.0010.";
	top.titles[i] =  "subschemaSubentry";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0008.";
	top.titles[i] =  "Attribute Syntax Definitions";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0001.";
	top.titles[i] =  "Back Link";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0002.";
	top.titles[i] =  "Boolean";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0003.";
	top.titles[i] =  "Case Exact String";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0004.";
	top.titles[i] =  "Case Ignore List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0005.";
	top.titles[i] =  "Case Ignore String";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0006.";
	top.titles[i] =  "Class Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0007.";
	top.titles[i] =  "Counter";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0008.";
	top.titles[i] =  "Distinguished Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0009.";
	top.titles[i] =  "EMail Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0010.";
	top.titles[i] =  "Facsimile Telephone Number";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0011.";
	top.titles[i] =  "Hold";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0012.";
	top.titles[i] =  "Integer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0013.";
	top.titles[i] =  "Interval";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0014.";
	top.titles[i] =  "Net Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0015.";
	top.titles[i] =  "Numeric String";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0016.";
	top.titles[i] =  "Object ACL";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0017.";
	top.titles[i] =  "Octet List";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0018.";
	top.titles[i] =  "Octet String";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0019.";
	top.titles[i] =  "Path";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0020.";
	top.titles[i] =  "Postal Address";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0021.";
	top.titles[i] =  "Printable String";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0022.";
	top.titles[i] =  "Replica Pointer";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0023.";
	top.titles[i] =  "Stream";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0024.";
	top.titles[i] =  "Telephone Number";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0025.";
	top.titles[i] =  "Time";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0026.";
	top.titles[i] =  "Timestamp";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0027.";
	top.titles[i] =  "Typed Name";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0008.0028.";
	top.titles[i] =  "Unknown";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0009.";
	top.titles[i] =  "Index of Classes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0001.";
	top.titles[i] =  "A through B";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0002.";
	top.titles[i] =  "C through D";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0003.";
	top.titles[i] =  "E through K";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0004.";
	top.titles[i] =  "L through M";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0005.";
	top.titles[i] =  "N";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0006.";
	top.titles[i] =  "O";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0007.";
	top.titles[i] =  "P through R";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0008.";
	top.titles[i] =  "S";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0009.0009.";
	top.titles[i] =  "T through Z";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0010.";
	top.titles[i] =  "Index of Attributes";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0001.";
	top.titles[i] =  "A";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0002.";
	top.titles[i] =  "B";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0003.";
	top.titles[i] =  "C";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0004.";
	top.titles[i] =  "D";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0005.";
	top.titles[i] =  "E";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0006.";
	top.titles[i] =  "F through G";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0007.";
	top.titles[i] =  "H";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0008.";
	top.titles[i] =  "I through K";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0009.";
	top.titles[i] =  "L";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0010.";
	top.titles[i] =  "M";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0011.";
	top.titles[i] =  "N";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0012.";
	top.titles[i] =  "O";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0013.";
	top.titles[i] =  "P";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0014.";
	top.titles[i] =  "Q";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0015.";
	top.titles[i] =  "R";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0016.";
	top.titles[i] =  "S";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0017.";
	top.titles[i] =  "T";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0018.";
	top.titles[i] =  "U";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0010.0019.";
	top.titles[i] =  "V through Z";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0011.";
	top.titles[i] =  "Index of ASN.1 IDs";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0011.0001.";
	top.titles[i] =  "0";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0011.0002.";
	top.titles[i] =  "1";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0011.0003.";
	top.titles[i] =  "2 through 2.4";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0011.0004.";
	top.titles[i] =  "2.5 through 2.9";
	i++;

	top.authors[i] = "zFLDR xC.5375.0500.0012.";
	top.titles[i] =  "Index of LDAP Names";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0001.";
	top.titles[i] =  "A through B";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0002.";
	top.titles[i] =  "C";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0003.";
	top.titles[i] =  "D";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0004.";
	top.titles[i] =  "E through F";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0005.";
	top.titles[i] =  "G";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0006.";
	top.titles[i] =  "H";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0007.";
	top.titles[i] =  "I through K";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0008.";
	top.titles[i] =  "L";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0009.";
	top.titles[i] =  "M";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0010.";
	top.titles[i] =  "N";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0011.";
	top.titles[i] =  "O";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0012.";
	top.titles[i] =  "P";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0013.";
	top.titles[i] =  "Q through R";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0014.";
	top.titles[i] =  "S";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0015.";
	top.titles[i] =  "T";
	i++;

	top.authors[i] = "zHTML xD.5375.0500.0012.0016.";
	top.titles[i] =  "U through Z";
	i++;

	top.authors[i] = "zHTML xC.5375.0500.0013.";
	top.titles[i] =  "Revision History";
	i++;

	top.authors[i] = "zFLDR xB.5375.0400.";
	top.titles[i] =  "NDS Iterator Services";
	i++;

	top.authors[i] = "zFLDR xC.5375.0400.0001.";
	top.titles[i] =  "Concepts";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0001.0001.";
	top.titles[i] =  "Iterator Objects";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0001.0002.";
	top.titles[i] =  "Creation of an Iterator Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0001.0003.";
	top.titles[i] =  "Iterator Indexes";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0001.0004.";
	top.titles[i] =  "Positions of an Iterator Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0001.0005.";
	top.titles[i] =  "Current Position Movement with Retrieval Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0001.0006.";
	top.titles[i] =  "Retrieval of Data";
	i++;

	top.authors[i] = "zFLDR xC.5375.0400.0002.";
	top.titles[i] =  "Tasks";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0002.0001.";
	top.titles[i] =  "Creating a Search Iterator Object";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0002.0002.";
	top.titles[i] =  "Retrieving and Unpacking Object and Attribute Name Data";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0002.0003.";
	top.titles[i] =  "Retrieving and Unpacking Object, Attribute, and Value Data";
	i++;

	top.authors[i] = "zFLDR xC.5375.0400.0003.";
	top.titles[i] =  "Functions";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0001.";
	top.titles[i] =  "NWDSItrAtEOF";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0002.";
	top.titles[i] =  "NWDSItrAtFirst";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0003.";
	top.titles[i] =  "NWDSItrClone";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0004.";
	top.titles[i] =  "NWDSItrCount";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0005.";
	top.titles[i] =  "NWDSItrCreateList";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0006.";
	top.titles[i] =  "NWDSItrCreateSearch";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0007.";
	top.titles[i] =  "NWDSItrDestroy";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0008.";
	top.titles[i] =  "NWDSItrGetCurrent";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0009.";
	top.titles[i] =  "NWDSItrGetInfo";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0010.";
	top.titles[i] =  "NWDSItrGetNext";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0011.";
	top.titles[i] =  "NWDSItrGetPosition";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0012.";
	top.titles[i] =  "NWDSItrGetPrev";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0013.";
	top.titles[i] =  "NWDSItrSetPosition";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0014.";
	top.titles[i] =  "NWDSItrSetPositionFromIterator";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0015.";
	top.titles[i] =  "NWDSItrSkip";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0003.0016.";
	top.titles[i] =  "NWDSItrTypeDown";
	i++;

	top.authors[i] = "zFLDR xC.5375.0400.0004.";
	top.titles[i] =  "NDS Iterator Example Code";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0001.";
	top.titles[i] =  "Cloning an Iterator Object: Example";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0002.";
	top.titles[i] =  "Counting with NDS Iterators: Example";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0003.";
	top.titles[i] =  "Creating and Using a List Iterator: Example";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0004.";
	top.titles[i] =  "Creating a Search Iterator and Displaying the Results: Example";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0005.";
	top.titles[i] =  "Getting Iterator Information: Example";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0006.";
	top.titles[i] =  "Getting and Setting the Iterator's Position: Example";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0007.";
	top.titles[i] =  "Listing in Reverse Order: Example";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0008.";
	top.titles[i] =  "Positioning the Iterator with Typedown: Example";
	i++;

	top.authors[i] = "zHTML xD.5375.0400.0004.0009.";
	top.titles[i] =  "Skipping Objects in the List: Example";
	i++;

	top.authors[i] = "zHTML xC.5375.0400.0005.";
	top.titles[i] =  "Revision History";
	i++;


// Morten's JavaScript Tree Menu
// written by Morten Wang <morten@treemenu.com> (c) 1998-2000
// This is version 2.2.6, dated 2000-03-30

// The script is freely distributable
// It may be used (and modified) as you wish, but retain this message
// For more information about the menu visit its home page
// http://www.treemenu.com/

/******************************************************************************
* Define the MenuItem object.                                                 *
******************************************************************************/

function MTMenuItem(text, url, target,nsearchID, icon) {
  this.text = text;
  this.url = url ? url : "";
  this.target =  target ? target : "";
  this.icon = icon ? icon : "";
  this.nsearchID = nsearchID;
  
  this.number = MTMSubNumber++;
  this.parent      = null;
  this.submenu     = null;
  this.expanded    = false;
  this.selected    = false;
  this.childSelected = false;
  
  this.MTMakeSubmenu = MTMakeSubmenu;

}

function MTMakeSubmenu(menu) {
  this.submenu = menu;
  for (var i = 0; i < menu.items.length; i++)
  {
    menu.items[i].parent = this;
  }
}



function getChildrenChecked(item, selected)
{
  if (item.submenu != null)
   {
     for (var x = 0; x < item.submenu.items.length; x++)
	 {
	    item.submenu.items[x].selected = selected;
		item.submenu.items[x].childSelected = false;
		MarkChildren(item.submenu.items[x],selected);
	 }
   }
}

/******************************************************************************
* Define the Menu object.                                                     *
******************************************************************************/

function MTMenu() {
  this.items   = new Array();
  this.MTMAddItem = MTMAddItem;
}

function MTMAddItem(item) {
  this.items[this.items.length] = item;
}

/******************************************************************************
* Define the icon list, addIcon function and MTMIcon item.                    *
******************************************************************************/

function IconList() {
  this.items = new Array();
  this.addIcon = addIcon;
}

function addIcon(item) {
  this.items[this.items.length] = item;
}

function MTMIcon(iconfile, match, type) {
  this.file = iconfile;
  this.match = match;
  this.type = type;
}

/******************************************************************************
* Global variables.  Not to be altered unless you know what you're doing.     *
* User-configurable options are at the end of this document.                  *
******************************************************************************/

var MTMLoaded = false;
var MTMLevel;
var MTMBar = new Array();
var MTMIndices = new Array();
var MTMBrowser = null;
var MTMNN3 = false;
var MTMNN4 = false;
var MTMIE4 = false;
var MTMUseStyle = true;

/*
 * (For a standalone JS shell test, we will simply set these as follows:)
 */
MTMBrowser = true;
MTMNN4 = true;


var MTMClickedItem = false;
var MTMExpansion = false;

var MTMSubNumber = 1;
var MTMTrackedItem = false;
var MTMTrack = false;

var MTMPreHREF = "";
if(MTMIE4 || MTMNN3) {
  MTMPreHREF += ""; // document.location.href.substring(0, document.location.href.lastIndexOf("/") +1);
}

var MTMFirstRun = true;
var MTMCurrentTime = 0; // for checking timeout.
var MTMUpdating = false;
var MTMWinSize, MTMyval;
var MTMOutputString = "";

/******************************************************************************
* Code that picks up frame names of frames in the parent frameset.            *
******************************************************************************/


/******************************************************************************
* Dummy function for sub-menus without URLs                                   *
* Thanks to Michel Plungjan for the advice. :)                                *
******************************************************************************/

function myVoid() { ; }

/******************************************************************************
* Functions to draw the menu.                                                 *
******************************************************************************/

function MTMSubAction(SubItem, ReturnValue) {

  SubItem.expanded = (SubItem.expanded) ? false : true;
  if(SubItem.expanded) {
    MTMExpansion = true;
  }

  MTMClickedItem = SubItem.number;

  if(MTMTrackedItem && MTMTrackedItem != SubItem.number) {
    MTMTrackedItem = false;
  }

  if(!ReturnValue) {
    setTimeout("MTMDisplayMenu()", 10);
  }

  return ReturnValue;
}


function MarkChildren(item, selected)
{
  if (item.submenu != null)
   {
     for (var x = 0; x < item.submenu.items.length; x++)
	 {
	    item.submenu.items[x].selected = selected;
		item.submenu.items[x].childSelected = false;
		MarkChildren(item.submenu.items[x],selected);
	 }
   }
 
}

function isAllChildrenSelected(item)
{
   if (item.submenu != null)
   {
     for (var x = 0; x < item.submenu.items.length; x++)
	 {
	    if (!item.submenu.items[x].selected)
		{
		   return false;
		}
	 }
   }
   return true;
}

function isSomeChildrenSelected(item)
{
   var retValue = false;
   
   if (item.submenu != null)
   {
     for (var x = 0; x < item.submenu.items.length; x++)
	 {
	    if (item.submenu.items[x].selected || item.submenu.items[x].childSelected)
		{
		   retValue = true;
		}
	 } 
   }
   
   return retValue;
}

function ToggleSelected(item, ReturnValue) {

  item.selected = (item.selected) ? false : true;
  item.childSelected = false;

  var currentNode = item;

  while (currentNode.parent)
  {  
       currentNode.parent.selected = isAllChildrenSelected(currentNode.parent);
       currentNode.parent.childSelected = isSomeChildrenSelected(currentNode.parent);
	   currentNode = currentNode.parent;
  }
  
  MarkChildren(item,item.selected);
  
  if(!ReturnValue) {
    setTimeout("MTMDisplayMenu()", 10);
  }

  return ReturnValue;
}


function MTMStartMenu() {
  MTMLoaded = true;
  if(MTMFirstRun) {
    MTMCurrentTime++;
    if(MTMCurrentTime == MTMTimeOut) { // call MTMDisplayMenu
      setTimeout("MTMDisplayMenu()",10);
    } else {
      setTimeout("MTMStartMenu()",100);
    }
  } 
}

function MTMDisplayMenu() {
  if(MTMBrowser && !MTMUpdating) {
    MTMUpdating = true;
    MTMFirstRun = false;

    if(MTMTrack) { MTMTrackedItem = MTMTrackExpand(menu); }

    if(MTMExpansion && MTMSubsAutoClose) { MTMCloseSubs(menu); }

    MTMLevel = 0;
    MTMDoc = parent.frames[MTMenuFrame].document
    MTMDoc.open("text/html", "replace");
    MTMOutputString = '<html><head>';
    if(MTMLinkedSS) {
      MTMOutputString += '<link rel="stylesheet" type="text/css" href="' + MTMPreHREF + MTMSSHREF + '">';
    } else if(MTMUseStyle) {
      MTMOutputString += '<style type="text/css">body {color:' + MTMTextColor + ';background:';
      MTMOutputString += (MTMBackground == "") ? MTMBGColor : MTMakeBackImage(MTMBackground);
      MTMOutputString += ';} #root {color:' + MTMRootColor + ';background:' + ((MTMBackground == "") ? MTMBGColor : 'transparent') + ';font-family:' + MTMRootFont + ';font-size:' + MTMRootCSSize + ';} ';
      MTMOutputString += 'a {font-family:' + MTMenuFont + ';letter-spacing:-0.05em;font-weight:bold;font-size:' + MTMenuCSSize + ';text-decoration:none;color:' + MTMLinkColor + ';background:' + MTMakeBackground() + ';} ';
      MTMOutputString += MTMakeA('pseudo', 'hover', MTMAhoverColor);
      MTMOutputString += MTMakeA('class', 'tracked', MTMTrackColor);
      MTMOutputString += MTMakeA('class', 'subexpanded', MTMSubExpandColor);
      MTMOutputString += MTMakeA('class', 'subclosed', MTMSubClosedColor) + '</style>';
    }

    MTMOutputString += '</head><body ';
    if(MTMBackground != "") {
      MTMOutputString += 'background="' + MTMPreHREF + MTMenuImageDirectory + MTMBackground + '" ';
    }
    MTMOutputString += 'bgcolor="' + MTMBGColor + '" text="' + MTMTextColor + '" link="' + MTMLinkColor + '" vlink="' + MTMLinkColor + '" alink="' + MTMLinkColor + '">';
    
    MTMOutputString += '<table border="0" cellpadding="0" cellspacing="0" width="' + MTMTableWidth + '">';
    MTMOutputString += '<tr valign="top"><td nowrap>'; //REMOVED ROOT ICON <img src="' + MTMPreHREF + MTMenuImageDirectory + MTMRootIcon + '" align="left" border="0" vspace="0" hspace="0">';
    if(MTMUseStyle) {
      MTMOutputString += ''; //REMOVED ROOT CAPTION <span id="root">&nbsp;' + MTMenuText + '</span>';
    } else {
      MTMOutputString += ''; //REMOVED ROOT CAPTION <font size="' + MTMRootFontSize + '" face="' + MTMRootFont + '" color="' + MTMRootColor + '">' + MTMenuText + '</font>';
    }
    MTMDoc.writeln(MTMOutputString + '</td></tr>');

    MTMListItems(menu);
    MTMDoc.writeln('</table>');

	MTMDoc.writeln('</body></html>');
    MTMDoc.close();

    if((MTMClickedItem || MTMTrackedItem) && (MTMNN4 || MTMIE4) && !MTMFirstRun) {
      MTMItemName = "sub" + (MTMClickedItem ? MTMClickedItem : MTMTrackedItem);
      if(document.layers && parent.frames[MTMenuFrame].scrollbars) {    
        MTMyval = parent.frames[MTMenuFrame].document.anchors[MTMItemName].y;
        MTMWinSize = parent.frames[MTMenuFrame].innerHeight;
      } else {
        MTMyval = MTMGetPos(parent.frames[MTMenuFrame].document.all[MTMItemName]);
        MTMWinSize = parent.frames[MTMenuFrame].document.body.offsetHeight;
      }
      if(MTMyval > (MTMWinSize - 60)) {
        parent.frames[MTMenuFrame].scrollBy(0, parseInt(MTMyval - (MTMWinSize * 1/3)));
      }
    }

    MTMClickedItem = false;
    MTMExpansion = false;
    MTMTrack = false;
  }
MTMUpdating = false;
}

function MTMListItems(menu) {
  var i, isLast;
  for (i = 0; i < menu.items.length; i++) {
    MTMIndices[MTMLevel] = i;
    isLast = (i == menu.items.length -1);
    MTMDisplayItem(menu.items[i], isLast);

    if (menu.items[i].submenu && menu.items[i].expanded) {
      MTMBar[MTMLevel] = (isLast) ? false : true;
      MTMLevel++;
      MTMListItems(menu.items[i].submenu);
      MTMLevel--;
    } else {
      MTMBar[MTMLevel] = false;
    } 
  }

}

function MTMDisplayItem(item, last) {
  var i, img, more;

  var MTMfrm = "parent.frames['code']";
  var MTMref = '.menu.items[' + MTMIndices[0] + ']';
  
  if(item.submenu) {
    var MTMouseOverText;

    var MTMClickCmd;
    var MTMDblClickCmd = false;


    if(MTMLevel > 0) {
      for(i = 1; i <= MTMLevel; i++) {
        MTMref += ".submenu.items[" + MTMIndices[i] + "]";
      }
    }

    if(!MTMEmulateWE && !item.expanded && (item.url != "")) {
      MTMClickCmd = "return " + MTMfrm + ".MTMSubAction(" + MTMfrm + MTMref + ",true);";
    } else {
      MTMClickCmd = "return " + MTMfrm + ".MTMSubAction(" + MTMfrm + MTMref + ",false);";
    }

    if(item.url == "") {
      MTMouseOverText = (item.text.indexOf("'") != -1) ? MTMEscapeQuotes(item.text) : item.text;
    } else {
      MTMouseOverText = "Expand/Collapse";
    }
  }

  MTMOutputString = '<tr valign="top"><td nowrap>';
  if(MTMLevel > 0) {
    for (i = 0; i < MTMLevel; i++) {
      MTMOutputString += (MTMBar[i]) ? MTMakeImage("menu_bar.gif") : MTMakeImage("menu_pixel.gif");
	}
  }

  more = false;
  if(item.submenu) {
    if(MTMSubsGetPlus || MTMEmulateWE) {
      more = true;
    } else {
      for (i = 0; i < item.submenu.items.length; i++) {
        if (item.submenu.items[i].submenu) {
          more = true;
        }
      }
    }
  }
  if(!more) {
    img = (last) ? "menu_corner.gif" : "menu_tee.gif";
  } else {
    if(item.expanded) {
      img = (last) ? "menu_corner_minus.gif" : "menu_tee_minus.gif";
    } else {
      img = (last) ? "menu_corner_plus.gif" : "menu_tee_plus.gif";
    }
    if(item.url == "" || item.expanded || MTMEmulateWE) {
      MTMOutputString += MTMakeVoid(item, MTMClickCmd, MTMouseOverText);
    } else {
      MTMOutputString += MTMakeLink(item, true)  + ' onclick="' + MTMClickCmd + '">';
    }
  }
  MTMOutputString += MTMakeImage(img);
/////////////////////////////////////////

var MTMCheckRef = '.menu.items[' + MTMIndices[0] + ']';  
if(MTMLevel > 0) {
  for(i = 1; i <= MTMLevel; i++) {
    MTMCheckRef += ".submenu.items[" + MTMIndices[i] + "]";
   }
  }
 
MTMOutputString += MTMakeVoid(item, "return " + MTMfrm + ".ToggleSelected(" + MTMfrm + MTMCheckRef + ",false);", "Checked Status") ;
var checkedImage = item.selected ? "checked.gif" : "uchecked.gif";
if (!item.selected)
{  
   checkedImage = item.childSelected ? "gchecked.gif" : "uchecked.gif";
}
MTMOutputString += MTMakeImage(checkedImage);
MTMOutputString += '</a>';
/////////////////////////////////////////////////


  if(item.submenu) {
    if(MTMEmulateWE && item.url != "") 
	{
      MTMOutputString += '</a>' + MTMakeLink(item, false) + '>';
	}

    img = (item.expanded) ? "menu_folder_open.gif" : "menu_folder_closed.gif";

    if(!more) {
      if(item.url == "" || item.expanded) {
        MTMOutputString += MTMakeVoid(item, MTMClickCmd, MTMouseOverText);
      } else {
        MTMOutputString += MTMakeLink(item, true) + ' onclick="' + MTMClickCmd + '">';
      }
    }
    MTMOutputString += MTMakeImage(img);

  } else {
    MTMOutputString += MTMakeLink(item, true) + '>';
    img = (item.icon != "") ? item.icon : MTMFetchIcon(item.url);
    MTMOutputString += MTMakeImage(img);
  }

  if(item.submenu && (item.url != "") && (item.expanded && !MTMEmulateWE)) {
    MTMOutputString += '</a>' + MTMakeLink(item, false) + '>';
  }

  if(MTMNN3 && !MTMLinkedSS) {
    var stringColor;
    if(item.submenu && (item.url == "") && (item.number == MTMClickedItem)) {
      stringColor = (item.expanded) ? MTMSubExpandColor : MTMSubClosedColor;
    } else if(MTMTrackedItem && MTMTrackedItem == item.number) {
      stringColor = MTMTrackColor;
    } else {
      stringColor = MTMLinkColor;
    }
    MTMOutputString += '<font color="' + stringColor + '" size="' + MTMenuFontSize + '" face="' + MTMenuFont + '">';
  }
  MTMOutputString += '&nbsp;' + item.text + ((MTMNN3 && !MTMLinkedSS) ? '</font>' : '') + '</a>' ;
  MTMDoc.writeln(MTMOutputString + '</td></tr>');
}

function MTMEscapeQuotes(myString) {
  var newString = "";
  var cur_pos = myString.indexOf("'");
  var prev_pos = 0;
  while (cur_pos != -1) {
    if(cur_pos == 0) {
      newString += "\\";
    } else if(myString.charAt(cur_pos-1) != "\\") {
      newString += myString.substring(prev_pos, cur_pos) + "\\";
    } else if(myString.charAt(cur_pos-1) == "\\") {
      newString += myString.substring(prev_pos, cur_pos);
    }
    prev_pos = cur_pos++;
    cur_pos = myString.indexOf("'", cur_pos);
  }
  return(newString + myString.substring(prev_pos, myString.length));
}

function MTMTrackExpand(thisMenu) {
  var i, targetPath;
  var foundNumber = false;
  for(i = 0; i < thisMenu.items.length; i++) {
    if(thisMenu.items[i].url != "" && MTMTrackTarget(thisMenu.items[i].target)) {
	  targetPath = parent.frames[thisMenu.items[i].target].location.protocol + '//' + parent.frames[thisMenu.items[i].target].location.host + parent.frames[thisMenu.items[i].target].location.pathname;
	
	  if(targetPath.lastIndexOf(thisMenu.items[i].url) != -1 && (targetPath.lastIndexOf(thisMenu.items[i].url) + thisMenu.items[i].url.length) == targetPath.length) {
        return(thisMenu.items[i].number);
      }
    }
    if(thisMenu.items[i].submenu) {
      foundNumber = MTMTrackExpand(thisMenu.items[i].submenu);
      if(foundNumber) {
        if(!thisMenu.items[i].expanded) {
          thisMenu.items[i].expanded = true;
          if(!MTMClickedItem) { MTMClickedItem = thisMenu.items[i].number; }
          MTMExpansion = true;
        }
        return(foundNumber);
      }
    }
  }
return(foundNumber);
}

function MTMCloseSubs(thisMenu) {
  var i, j;
  var foundMatch = false;
  for(i = 0; i < thisMenu.items.length; i++) {
    if(thisMenu.items[i].submenu && thisMenu.items[i].expanded) {
      if(thisMenu.items[i].number == MTMClickedItem) {
        foundMatch = true;
        for(j = 0; j < thisMenu.items[i].submenu.items.length; j++) {
          if(thisMenu.items[i].submenu.items[j].expanded) {
            thisMenu.items[i].submenu.items[j].expanded = false;
          }
        }
      } else {
        if(foundMatch) {
          thisMenu.items[i].expanded = false; 
        } else {
          foundMatch = MTMCloseSubs(thisMenu.items[i].submenu);
          if(!foundMatch) {
            thisMenu.items[i].expanded = false;
          }
        }
      }
    }
  }
return(foundMatch);
}

function MTMFetchIcon(testString) {
  var i;
  for(i = 0; i < MTMIconList.items.length; i++) {
    if((MTMIconList.items[i].type == 'any') && (testString.indexOf(MTMIconList.items[i].match) != -1)) {
      return(MTMIconList.items[i].file);
    } else if((MTMIconList.items[i].type == 'pre') && (testString.indexOf(MTMIconList.items[i].match) == 0)) {
      return(MTMIconList.items[i].file);
    } else if((MTMIconList.items[i].type == 'post') && (testString.indexOf(MTMIconList.items[i].match) != -1)) {
      if((testString.lastIndexOf(MTMIconList.items[i].match) + MTMIconList.items[i].match.length) == testString.length) {
        return(MTMIconList.items[i].file);
      }
    }
  }
return("menu_link_default.gif");
}

function MTMGetPos(myObj) {
  return(myObj.offsetTop + ((myObj.offsetParent) ? MTMGetPos(myObj.offsetParent) : 0));
}

function MTMCheckURL(myURL) {
  var tempString = "";
  if((myURL.indexOf("http://") == 0) || (myURL.indexOf("https://") == 0) || (myURL.indexOf("mailto:") == 0) || (myURL.indexOf("ftp://") == 0) || (myURL.indexOf("telnet:") == 0) || (myURL.indexOf("news:") == 0) || (myURL.indexOf("gopher:") == 0) || (myURL.indexOf("nntp:") == 0) || (myURL.indexOf("javascript:") == 0)) {
    tempString += myURL;
  } else {
    tempString += MTMPreHREF + myURL;
  }
return(tempString);
}

function MTMakeVoid(thisItem, thisCmd, thisText) {
  var tempString = "";
  tempString +=  '<a name="sub' + thisItem.number + '" href="javascript:parent.frames[\'code\'].myVoid();" onclick="' + thisCmd + '" onmouseover="window.status=\'' + thisText + '\';return true;" onmouseout="window.status=\'' + window.defaultStatus.replace(/'/g,"") + '\';return true;"';
  if(thisItem.number == MTMClickedItem) {
    var tempClass;
    tempClass = thisItem.expanded ? "subexpanded" : "subclosed";
    tempString += ' class="' + tempClass + '"';
  }
  return(tempString + '>');
}

function MTMakeLink(thisItem, addName) {
  var tempString = '<a';

  if(MTMTrackedItem && MTMTrackedItem == thisItem.number) {
    tempString += ' class="tracked"'
  }
  if(addName) {
    tempString += ' name="sub' + thisItem.number + '"';
  }
  tempString += ' href="' + MTMCheckURL(thisItem.url) + '"';
  if(thisItem.target != "") {
    tempString += ' target="' + thisItem.target + '"';
  }
return tempString;
}

function MTMakeImage(thisImage) {
  return('<img src="' + MTMPreHREF + MTMenuImageDirectory + thisImage + '" align="left" border="0" vspace="0" hspace="0" width="18" height="18">');
}

function MTMakeBackImage(thisImage) {
  var tempString = 'transparent url("' + ((MTMPreHREF == "") ? "" : MTMPreHREF);
  tempString += MTMenuImageDirectory + thisImage + '")'
  return(tempString);
}

function MTMakeA(thisType, thisText, thisColor) {
  var tempString = "";
  tempString += 'a' + ((thisType == "pseudo") ? ':' : '.');
  return(tempString + thisText + '{color:' + thisColor + ';background:' + MTMakeBackground() + ';}');
}

function MTMakeBackground() {
  return((MTMBackground == "") ? MTMBGColor : 'transparent');
}

function MTMTrackTarget(thisTarget) {
  if(thisTarget.charAt(0) == "_") {
    return false;
  } else {
    for(i = 0; i < MTMFrameNames.length; i++) {
      if(thisTarget == MTMFrameNames[i]) {
        return true;
      }
    }
  }
  return false;
}




/******************************************************************************
* User-configurable options.                                                  *
******************************************************************************/

// Menu table width, either a pixel-value (number) or a percentage value.
var MTMTableWidth = "100%";

// Name of the frame where the menu is to appear.
var MTMenuFrame = "tocmain";

// variable for determining whether a sub-menu always gets a plus-sign
// regardless of whether it holds another sub-menu or not
var MTMSubsGetPlus = true;


// variable that defines whether the menu emulates the behaviour of
// Windows Explorer
var MTMEmulateWE = true;

// Directory of menu images/icons
var MTMenuImageDirectory = "/ndk/doc/docui2k/menu-images/";

// Variables for controlling colors in the menu document.
// Regular BODY atttributes as in HTML documents.
var MTMBGColor = "#cc0000";
var MTMBackground = "";
var MTMTextColor = "white";

// color for all menu items
var MTMLinkColor = "#ffffcc";

// Hover color, when the mouse is over a menu link
var MTMAhoverColor = "#FF9933";

// Foreground color for the tracking & clicked submenu item
var MTMTrackColor ="#FF9933";
var MTMSubExpandColor = "#ffffcc";
var MTMSubClosedColor = "#ffffcc";

// All options regarding the root text and it's icon
var MTMRootIcon = "menu_new_root.gif";
var MTMenuText = "Site contents:";
var MTMRootColor = "white";
var MTMRootFont = "Verdana";
var MTMRootCSSize = "84%";
var MTMRootFontSize = "-1";

// Font for menu items.
var MTMenuFont = "Verdana";
var MTMenuCSSize = "74%";
var MTMenuFontSize = "-1";

// Variables for style sheet usage
// 'true' means use a linked style sheet.
var MTMLinkedSS = false;
var MTMSSHREF = "style/menu.css";

// Whether you want an open sub-menu to close automagically
// when another sub-menu is opened.  'true' means auto-close
var MTMSubsAutoClose = false;

// This variable controls how long it will take for the menu
// to appear if the tracking code in the content frame has
// failed to display the menu. Number if in tenths of a second
// (1/10) so 10 means "wait 1 second".
var MTMTimeOut = 25;

/******************************************************************************
* User-configurable list of icons.                                            *
******************************************************************************/

var MTMIconList = null;
MTMIconList = new IconList();
// examples:
//MTMIconList.addIcon(new MTMIcon("menu_link_external.gif", "http://", "pre"));
//MTMIconList.addIcon(new MTMIcon("menu_link_pdf.gif", ".pdf", "post"));

/******************************************************************************
* User-configurable menu.                                                     *
******************************************************************************/


// navigation link is an object used to store the extracted information from 
// the search request.  The stored information will be used to build the 
// navigation tree.
 function navigationLink(title,URL,level,elementIndex,levelIndex,parentIndex,author)
 {       
 	var returnArray = new Array();
 	returnArray.title = title;
    returnArray.URL = URL;
 	returnArray.level = level;
 	returnArray.hasChild = false;
 	returnArray.elementIndex = elementIndex;
 	returnArray.parentIndex = parentIndex;   
 	returnArray.levelIndex = levelIndex;
	returnArray.author = author;
	 	
 	return returnArray;
 }

// Variables used for tracking state as the search iterates through the list
// of documents returned.
var index = 0;
var currentLevel = 0;
var levelParents = new Array();
var levelIndexes = new Array();
var navigationTree = new Array();
var treeNodes = new Array();
var levelIndex = 0;
top.printList = "";
top.printCount = 0;   

// asign the menu handle to the created tree
var menu = null;


function getNextChecked(item)
{
  // case that root of tree is selected
  if ( item.parent == null && item.selected)
  {
       for (var i = 0 ; i < top.authors.length; i++)
			{
			    var re = /\s$/;
			    
			    if (top.titles[i].replace(re,"") == item.text.replace(re,""))
				{
				   top.printList +=  (top.authors[i].length + 3) + "_" + top.authors[i].replace(/\s/g,"+") + "+en";
				   top.printCount ++;
				}
		    }		
  }
  else if (item.submenu != null)
   {
     for (var x = 0; x < item.submenu.items.length; x++)
	 {  
	    if (item.submenu.items[x].selected)
		{
		    var name = item.submenu.items[x].text;
			for (var i = 0 ; i < top.authors.length; i++)
			{         
			    var re = /\s$/;
			    if (top.titles[i].replace(re,"") == name.replace(re,""))
				{
				   top.printList +=  (top.authors[i].length + 3) + "_" + top.authors[i].replace(/\s/g,"+") + "+en";
				   top.printCount ++;
				}
		    }			  
			
		}
		else
		{
		     getNextChecked(item.submenu.items[x]);
		}
	 }
   }
   
}

// Get a URL to pass checked topics to the Print Servlet



function getPrintUrl(menu)
{
 top.printList = "";
 top.printCount = 0;
  
  getNextChecked(menu.items[0]);
  top.printList = top.printCount + "_" + top.printList;

  return top.printList;
}
	
function setLevels()
{
   
  // Tracking the parent of the next node.
  levelParents[currentLevel + 1] = index;
	
  // levelIndex is the child index under a branch
  if (levelIndexes[currentLevel] == null)
  {
     levelIndexes[currentLevel] = 0;
  }
  else
  {
     levelIndexes[currentLevel] = levelIndexes[currentLevel] + 1;
     levelIndexes[currentLevel + 1] = -1;
  }
}	
	
function buildTree()
{
 
// Determine which nodes have children and assign the correct property
for (var i = 0; i < navigationTree.length-1; i++)
{
  // see if the current node has chilren
  var thisLevel = navigationTree[i]["level"];
  var nextLevel = navigationTree[i+1]["level"];
  
  if (nextLevel > thisLevel)
  {
  	navigationTree[i]["hasChild"] = true;
  }
  else
  {
  	navigationTree[i]["hasChild"] = false;
  }
} 
	                                 

// create tree object nodes.
for( var j = 0; j < navigationTree.length; j++)
{
   treeNodes[j] = null;
   treeNodes[j] = new MTMenu();
}


// add all items to nodes - 
// NOTE, index to add to is the parent index + 1 for node tree offset of root=0
for( var j3 = 0; j3 < navigationTree.length; j3++)
{
   if (navigationTree[j3]["parentIndex"] == null)
   {
     var nsearchID = navigationTree[j3]["author"];
      treeNodes[0].MTMAddItem(new MTMenuItem(navigationTree[j3]["title"], navigationTree[j3]["URL"].replace(/http...developer.novell.com.ndk/gi,"/ndk") , "content_frame", nsearchID)); 
   }
   else
   { 
     var nsearchID = navigationTree[j3]["author"];
     treeNodes[navigationTree[j3]["parentIndex"] + 1 ].MTMAddItem(new MTMenuItem(navigationTree[j3]["title"], navigationTree[j3]["URL"].replace(/http...developer.novell.com.ndk/gi,"/ndk"), "content_frame",nsearchID)); 
   }
}

// create submenu structure
// NOTE: add 1 to parent nodes for root = 0 offset.
for( var j4 = 0; j4 < navigationTree.length; j4++)
{   
   if (navigationTree[j4]["hasChild"])
   {
      var pindex = null;
	  if (navigationTree[j4]["parentIndex"] == null)
	  {
	  
          pindex = 0;
	  }
	  else
	  {
	      pindex = navigationTree[j4]["parentIndex"]+1;
      }
	  
	  var lindex = navigationTree[j4]["levelIndex"];
	//  document.write('treeNodes[' + pindex +'].items['+ lindex +'].MTMakeSubmenu(treeNodes['+(j4+1)+']);<br>');
	  
	  treeNodes[pindex].items[lindex].MTMakeSubmenu(treeNodes[j4+1]);
   }
}

 menu = treeNodes[0];

//expand the second item to display the sub contents on first display
if (menu.items[0] != null )
{
   menu.items[0].expanded = true;     

}
    

         
}

        

currentLevel++;

setLevels();
var navElement = navigationLink("NDS Libraries for C ","http://developer.novell.com/ndk/doc/ndslib/treetitl.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NDS Backup Services ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/hevgtl7k.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Functions ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/h7qwv271.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NDSBackupServerData ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/sdk5.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSFreeNameList ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/sdk12.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSGetReplicaPartitionNames ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/sdk19.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSIsOnlyServerInTree ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/sdk26.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSSYSVolumeRecovery ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/sdk33.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSVerifyServerInfo ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/sdk40.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Structures ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/hqp7vveq.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NAMEID_TYPE ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/sdk48.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Values ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/hmmmal7s.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NDS Reason Flags ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/h3r99io5.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Server Flags ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/hnlckbki.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Revision History ","http://developer.novell.com/ndk/doc/ndslib/dsbk_enu/data/a5i29ah.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Event Services ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hmwiqbwd.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Concepts ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hj3udfo7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NDS Event Introduction ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hmgeu8a1.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Event Functions ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hxwcemsz.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Event Priorities ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hux0tdup.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Event Data Filtering ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/ha7nqbpy.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Event Types ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/h741eryw.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Global Network Monitoring ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/h9alatk4.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Tasks ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/huypg52u.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Monitoring NDS Events ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hhkihe7f.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Registering for NDS Events ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/h0xmzt1h.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Unregistering for NDS Events ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hk3fvwed.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Functions ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/h7qwv271.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NWDSEConvertEntryName ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk28.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSEGetLocalAttrID ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk33.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSEGetLocalAttrName ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk39.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSEGetLocalClassID ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk45.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSEGetLocalClassName ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk51.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSEGetLocalEntryID ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk57.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSEGetLocalEntryName ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk63.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSERegisterForEvent ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk69.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSERegisterForEventWithResult ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk75.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSEUnRegisterForEvent ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk81.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Structures ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hqp7vveq.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("DSEACL ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk88.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEBackLink ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk92.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEBinderyObjectInfo ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk96.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEBitString ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk100.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEChangeConnState ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk104.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSECIList ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk108.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEDebugInfo ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk112.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEEmailAddress ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk116.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEEntryInfo ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk120.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEEntryInfo2 ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk124.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEEventData ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk128.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEFaxNumber ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk132.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEHold ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk135.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEModuleState ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk139.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSENetAddress ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk143.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEOctetList ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk147.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEPath ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk151.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEReplicaPointer ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk155.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSESEVInfo ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk159.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSETimeStamp ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk163.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSETraceInfo ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk167.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSETypedName ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk172.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEVALData ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk176.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSEValueInfo ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/sdk179.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Values ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hmmmal7s.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Event Priorities ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hlerfllh.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Event Types ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/hiz5y84y.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Revision History ","http://developer.novell.com/ndk/doc/ndslib/dsev_enu/data/a6hw6zr.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Technical Overview ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h6tvg4z7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NDS as the Internet Directory ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h273w870.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Requirements for Networks and the Internet ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/a2lh37b.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Compliance to X.500 Standard ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h0jj42d7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Compliance with LDAP v3 ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/a2b6k5w.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Directory Access Protocols ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/a2b6k5x.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Programming Interfaces for NDS ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h2qzzkq8.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Architecture ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h6mny7fl.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Objects ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hp4dslw5.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NDS Names ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h0yh1byj.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Types of Information Stored in NDS ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hci52ynf.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Retrieval of Information from NDS ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hwwz5mda.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Tree Walking ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h2xhaphc.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Object Management ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h3mq2rf0.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Security ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hl8x1zxc.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Authentication ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hp901s8a.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Access Control Lists ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hr8sqtoi.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Inheritance ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hh9881ul.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NetWare File System ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h64btfhk.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Partitions and Replicas ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hmq60r6h.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Partitioning ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hqx5hvrp.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Replication ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hj5l8npv.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Distributed Reference Management ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hzap47de.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Partition Operations ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hgbpk7x9.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Synchronization ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hsiplgn4.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Background Processes ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hz2kcp2e.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Bindery Services ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hwug6ytv.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NDS Bindery Context ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h8dwby8o.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bindery Context Path ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h6y3yva6.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bindery Context Eclipsing ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hwcqk80m.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Bindery Objects ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hq4w9le6.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Return Values ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hbjry4gt.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NDS Return Values from the Operating System ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h5h16q77.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Client Return Values ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/he2lvhfy.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Agent Return Values ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hcvwzt90.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Directory Services Trace Utilities ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hujirj2n.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Using the DSTrace NLM ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hmg1e5gn.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Using Basic SET DSTrace Commands ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hdn0smja.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Starting Background Processes with SET DSTrace ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/h5pjd8fv.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Tuning Background Processes ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hhv9cqpk.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Enabling DSTrace Messages with SET DSTrace ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/hcah5j8v.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Revision History ","http://developer.novell.com/ndk/doc/ndslib/dsov_enu/data/a5i29ah.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Core Services ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h2y7hdit.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Programming Concepts ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h2x9gqr9.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Context Handles ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/huynzi7a.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Buffer Management ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h9xiygoj.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Read Requests for Object Information ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h7d6try4.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Search Requests ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h11es6ae.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Developing in a Loosely Consistent Environment ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hsaqomj7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Add Object Requests ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hqjws9hi.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Security and Applications ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h3xwyggn.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Authentication of Client Applications ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h0m1k6ck.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Multiple Tree Support ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hu5a8flo.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Effective Rights Function ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/he06edkq.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Partition Functions ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/ha7fzu9h.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Replica Functions ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hpmsr4w7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Read Requests for Schema Information ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h0a2o4v9.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Schema Extension Requests ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hrgy5k6e.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Tasks ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/huypg52u.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Context Handle Tasks ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hw34ixeu.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Buffer Tasks ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hb1nkqk4.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Authentication and Connection Tasks ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/huzx6sda.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Object Tasks ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hddp9m9i.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Partition and Replica Tasks ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hpx2o69b.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Schema Tasks ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hp85l75p.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Functions ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h7qwv271.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NWDSAbbreviateName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk135.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAbortPartitionOperation ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk144.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAddFilterToken ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk153.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAddObject ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk162.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAddPartition (obsolete&#45;&#45;&#45;moved from .h file 11/99) ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk171.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAddReplica ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk180.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAddSecurityEquiv ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk189.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAllocBuf ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk198.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAllocFilter ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk207.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAuditGetObjectID ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk216.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAuthenticate ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk225.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAuthenticateConn ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk234.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSAuthenticateConnEx ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/a3fvxoz.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSBackupObject ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk243.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSBeginClassItem ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk252.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSCanDSAuthenticate ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk261.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSCanonicalizeName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk270.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSChangeObjectPassword ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk279.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSChangeReplicaType ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk288.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSCIStringsMatch ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk297.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSCloseIteration ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk305.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSCompare ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk314.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSComputeAttrValSize ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk360.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSCreateContext (obsolete&#45;&#45;&#45;moved from .h file 6/99) ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk369.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSCreateContextHandle ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk371.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSDefineAttr ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk382.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSDefineClass ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk391.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSDelFilterToken ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk402.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSDuplicateContext (obsolete 03/99) ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk412.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSDuplicateContextHandle ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk423.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSExtSyncList ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk434.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSExtSyncRead ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk443.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSExtSyncSearch ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk455.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSFreeBuf ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk465.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSFreeContext ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk474.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSFreeFilter ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk491.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGenerateObjectKeyPair ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk501.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetAttrCount ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk511.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetAttrDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk521.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetAttrName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk530.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetAttrVal ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk540.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetAttrValFlags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk550.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetAttrValModTime ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk558.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetBinderyContext ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk566.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetClassDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk603.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetClassDefCount ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk691.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetClassItem ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk769.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetClassItemCount ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk838.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetContext ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk919.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetCountByClassAndName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk972.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetCurrentUser ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1031.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetDefNameContext ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1041.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetDSIInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1117.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetDSVerInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1209.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetEffectiveRights ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1274.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetMonitoredConnRef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1346.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetNDSInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1425.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetObjectCount ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1528.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetObjectHostServerAddress ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1604.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetObjectName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1640.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetObjectNameAndInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1700.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetPartitionExtInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1781.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetPartitionExtInfoPtr ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1830.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetPartitionInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk1938.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetPartitionRoot ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2001.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetServerAddresses (obsolete 3/98) ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2021.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetServerAddresses2 ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2030.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetServerDN ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2039.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetServerName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2047.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetSyntaxCount ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2056.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetSyntaxDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2065.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSGetSyntaxID ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2074.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSInitBuf ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2082.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSInspectEntry ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2091.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSJoinPartitions ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2099.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSList ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2108.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSListAttrsEffectiveRights ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2117.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSListByClassAndName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2126.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSListContainableClasses ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2135.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSListContainers ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2144.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSListPartitions ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2153.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSListPartitionsExtInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2162.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSLogin ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2171.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSLoginAsServer ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2180.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSLogout ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2187.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSMapIDToName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2196.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSMapNameToID ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2205.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSModifyClassDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2214.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSModifyDN ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2223.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSModifyObject ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2232.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSModifyRDN ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2241.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSMoveObject ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2250.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSMutateObject ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/a37nkf6.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSOpenConnToNDSServer ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2259.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSOpenMonitoredConn ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2268.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSOpenStream ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2277.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPartitionReceiveAllUpdates ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2285.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPartitionSendAllUpdates ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2294.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutAttrName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2303.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutAttrNameAndVal ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2312.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutAttrVal ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2321.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutChange ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2330.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutChangeAndVal ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2339.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutClassItem ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2348.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutClassName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2357.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutFilter ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2364.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSPutSyntaxName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2373.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRead ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2380.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReadAttrDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2389.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReadClassDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2398.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReadNDSInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2407.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReadObjectDSIInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2416.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReadObjectInfo ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2425.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReadReferences ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2434.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReadSyntaxDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2443.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReadSyntaxes ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2451.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReloadDS ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2459.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRemoveAllTypes ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2467.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRemoveAttrDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2475.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRemoveClassDef ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2484.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRemoveObject ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2493.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRemovePartition ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2501.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRemoveReplica ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2510.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRemSecurityEquiv ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2519.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRepairTimeStamps ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2528.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReplaceAttrNameAbbrev ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2536.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSResolveName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2544.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSRestoreObject ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2553.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSReturnBlockOfAvailableTrees ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2562.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSScanConnsForTrees ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2573.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSScanForAvailableTrees ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2582.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSearch ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2591.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSetContext ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2600.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSetCurrentUser ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2609.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSetDefNameContext ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2615.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSetMonitoredConnection ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2624.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSplitPartition ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2633.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSyncPartition ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2642.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSyncReplicaToServer ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2651.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSSyncSchema ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2660.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSUnlockConnection ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2669.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSVerifyObjectPassword ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2678.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSWhoAmI ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2687.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWGetDefaultNameContext ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2695.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWGetFileServerUTCTime ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2704.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWGetNumConnections ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2712.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWGetNWNetVersion ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2720.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWGetPreferredConnName ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2727.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWIsDSAuthenticated ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2736.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWIsDSServer ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2743.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWNetInit ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2750.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWNetTerm ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2759.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWSetDefaultNameContext ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2767.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWSetPreferredDSTree ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2776.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Structures ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hqp7vveq.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Asn1ID_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2785.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Attr_Info_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2790.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Back_Link_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2795.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bit_String_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2800.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Buf_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2805.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("CI_List_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2810.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Class_Info_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2815.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("EMail_Address_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2820.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Fax_Number_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2826.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Filter_Cursor_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2831.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Filter_Node_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2836.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Hold_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2841.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSOSVersion_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2846.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSStatsInfo_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2850.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Net_Address_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2855.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDS_TimeStamp_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2860.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Object_ACL_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2865.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Object_Info_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2870.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Octet_List_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2875.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Octet_String_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2880.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Path_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2885.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Replica_Pointer_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2890.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Syntax_Info_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2895.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("TimeStamp_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2900.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Typed_Name_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2906.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Unknown_Attr_T ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/sdk2911.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Values ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hmmmal7s.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Attribute Constraint Flags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hudjk3k4.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Attribute Value Flags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h6anqw6h.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Buffer Operation Types and Related Functions ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h8bn0lfm.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Class Flags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hpj620k3.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Change Types for Modifying Objects ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hc4p686b.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Context Keys and Flags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h1btx3en.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Default Context Key Values ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hlkcqs3t.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DCK_FLAGS Bit Values ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/he1wcp92.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DCK_NAME_FORM Values ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hmd7uuiw.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DCK_CONFIDENCE Bit Values ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h7hy5yg3.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DCK_DSI_FLAGS Values ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/huh0ri39.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSI_ENTRY_FLAGS Values ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hqwiyl1u.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Filter Tokens ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h487zxy3.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Information Types for Attribute Definitions ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hdqx1cns.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Information Types for Class Definitions ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hcq403ms.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Information Types for Search and Read ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/ha682lf8.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Name Space Types ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hs6qj0yl.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Access Control Rights ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h12s89uj.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDS Ping Flags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hf0fdqhd.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DSP Replica Information Flags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hw42a7qg.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Network Address Types ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hniuyp90.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Scope Flags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h6wfyyfk.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Replica Types ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/he290q86.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Replica States ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/h9br9yt1.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Syntax Matching Flags ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hd8fn0rm.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Syntax IDs ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hn1dsa7y.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Example Code ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/hb05g04v.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Context Handle ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/a2sofgc.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Object and Attribute ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/a2snp6e.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Browsing and Searching ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/a2snu78.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Batch Modification of Objects and Attributes ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/a2snzot.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Schema ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/a2snqyd.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Revision History ","http://developer.novell.com/ndk/doc/ndslib/nds__enu/data/a5i29ah.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Schema Reference ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h4q1mn1i.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Schema Concepts ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h282spjh.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Schema Structure ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hpmkggmh.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Schema Components ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hvt5bdoi.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Object Classes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hbna398k.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Naming Attributes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h9vf1k0r.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Containment Classes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hh1izaro.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Super Classes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hmdjysrx.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Object Class Flags ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h6rvyaky.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Mandatory and Optional Attributes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h2vnta8j.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Default ACL Templates ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hr9sm1l0.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Auxiliary Classes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hlh5m1af.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Attribute Type Definitions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hotadinr.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Attribute Syntax Definitions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h2m59phc.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Schema Extensions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/he5mef3b.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Base Object Class Definitions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hmv2qd15.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("AFP Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk75.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Alias ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk83.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("applicationEntity ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk91.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("applicationProcess ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk99.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:File Object ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk107.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bindery Object ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk115.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bindery Queue ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk123.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("certificationAuthority ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk131.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("CommExec ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk139.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Computer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk147.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Country ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk155.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("cRLDistributionPoint ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk163.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("dcObject ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk171.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Device ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk179.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Directory Map ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk187.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("domain ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk195.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("dSA ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk203.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("External Entity ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk219.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Group ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk227.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Group ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a38rj6z.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk243.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk251.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Locality ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk259.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("MASV:Security Policy ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk267.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Message Routing Group ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk275.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Messaging Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk283.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NCP Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk291.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("ndsLoginProperties ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk347.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Certificate Authority ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk355.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Key Material ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk363.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:SD Key Access Partition ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2okvd6.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:SD Key List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2okvdx.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Trusted Root ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2okvbk.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Trusted Root Object ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2okvcf.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:groupOfCertificates ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk421.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailGroup1 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk445.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailRecipient ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk466.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:NetscapeMailServer5 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk474.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:NetscapeServer5 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk482.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nginfo3 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk510.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nsLicenseUser ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk518.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Organization ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk530.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Organizational Person ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk541.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Organizational Role ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk550.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Organizational Unit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk561.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Partition ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk570.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Person ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk578.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("pkiCA ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk586.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("pkiUser ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk594.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Print Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk602.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk610.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Profile ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk618.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Queue ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk626.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Resource ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk634.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SAS:Security ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk642.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SAS:Service ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk650.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk658.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("strongAuthenticationUser ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk698.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Template ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk706.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Top ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk714.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Tree Root ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk722.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Unknown ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk730.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("User ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk738.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("userSecurityInformation ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk746.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Volume ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk754.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("WANMAN:LAN Area ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk762.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Novell Object Class Extensions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3fh4x1.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Entrust:CRLDistributionPoint ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk211.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("inetOrgPerson ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk235.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Broker ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk299.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Manager ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk307.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Printer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk315.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Catalog ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk323.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Master Catalog ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk331.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Slave Catalog ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk339.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NetSvc ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk379.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:License Certificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk386.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:License Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk394.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Product Container ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk412.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:groupOfUniqueNames5 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk432.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailGroup5 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk454.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:Nginfo ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk491.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:Nginfo2 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk502.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("residentialPerson ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3omhcl.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Scope Unit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk666.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Directory Agent ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk674.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Service ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk682.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SMS SMDR Class ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk690.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Graphical View of Object Class Inheritance ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hzah4ydk.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Alias and Bindery Object Classes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hw8hr9jx.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Tree Root, domain, and Unknown ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hu1mitlx.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Computer, Country, Device, and Printer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hnf7uif9.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("List and Locality ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h48ynbap.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Organizational Role and Partition ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hrfg9w4e.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("ndsLoginProperties, Organization, and Organizational Unit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hzvb48kg.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("ndsLoginProperties, Person, Organizational Person, and User ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hknzjmiv.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Directory Map, Profile, Queues, Resource, and Volume ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h8jovuwl.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Servers (AFP, Messaging, NCP, Print) and CommExec ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/ha47y85g.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("External Entity, Group, and Message Routing Group ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hds3w6ie.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Base Attribute Definitions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/hf9qbbni.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Aliased Object Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk782.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Account Balance ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk788.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("ACL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk794.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Allow Unlimited Credit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk800.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("associatedName ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a7bbra4.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("attributeCertificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk806.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:A Encryption Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk812.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:B Encryption Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk818.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:Contents ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk824.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:Current Encryption Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk830.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:File Link ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk836.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:Link List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk842.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:Path ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk848.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:Policy ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk854.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Audit:Type ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk860.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("authorityRevocationList ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk866.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Authority Revocation ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk872.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("AuxClass Object Class Backup ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk878.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Auxiliary Class Flag ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk884.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Back Link ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk890.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bindery Object Restriction ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk896.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bindery Property ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk902.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bindery Restriction Level ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk908.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Bindery Type ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk914.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("businessCategory ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk920.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk932.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("cACertificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk938.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("CA Private Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk944.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("CA Public Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk950.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Cartridge ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk956.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("certificateRevocationList ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk962.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Certificate Revocation ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk968.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Certificate Validity Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk974.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("crossCertificatePair ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk926.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk986.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Convergence ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk998.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Cross Certificate Pair ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1004.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("dc ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1034.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Default Queue ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1040.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("deltaRevocationList ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1052.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("departmentNumber ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3on5am.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Description ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1058.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("destinationIndicator ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1064.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Detect Intruder ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1070.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Device ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1076.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("dmdName ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1082.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("dn ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1088.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("dnQualifier ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1094.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("DS Revision ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1100.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("EMail Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1106.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("employeeType ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3on9iy.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("enhancedSearchGuide ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1120.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Equivalent To Me ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1138.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("External Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1144.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("External Synchronizer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1150.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Facsimile Telephone Number ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1156.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Full Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1162.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Generational Qualifier ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1168.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("generationQualifier ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1174.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1180.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Given Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1186.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1192.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("GUID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1198.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("High Convergence Sync Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1216.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Higher Privileges ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1222.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Home Directory ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1228.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Home Directory Rights ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1234.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("homePhone ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3onbgn.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("homePostalAddress ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3ondem.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("houseIdentifier ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1258.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Host Device ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1240.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Host Resource Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1246.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Host Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1252.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Inherited ACL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1264.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Initials ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1270.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("internationaliSDNNumber ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1276.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Internet EMail Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1282.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Intruder Attempt Reset Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1288.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Intruder Lockout Reset Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1294.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("knowledgeInformation ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1312.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1318.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Language ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1324.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Last Login Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1330.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Last Referenced Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1336.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP ACL v11 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1342.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Allow Clear Text Password ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1348.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Anonymous Identity ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1354.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Attribute Map v11 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1360.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Backup Log Filename ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1366.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Class Map v11 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1378.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Enable SSL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1384.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Enable TCP ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1390.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Enable UDP ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1396.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Group ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1402.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Host Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1408.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Log Filename ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1414.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Log Level ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1420.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Log Size Limit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1426.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Referral ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1432.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Screen Level ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1438.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Search Size Limit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1444.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Search Time Limit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1450.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1456.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Server Bind Limit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1462.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Server Idle Timeout ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1468.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Server List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1474.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP SSL Port ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1480.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Suffix ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1486.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP TCP Port ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1492.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP UDP Port ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1498.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:bindCatalog ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1516.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:bindCatalogUsage ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1522.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:keyMaterialName ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1546.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:otherReferralUsage ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1552.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:searchCatalog ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1558.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:searchCatalogUsage ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1564.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:searchReferralUsage ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1570.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Locked By Intruder ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1576.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Lockout After Detection ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1582.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Allowed Time Map ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1588.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Disabled ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1594.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Expiration Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1600.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Grace Limit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1606.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Grace Remaining ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1612.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Intruder Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1618.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Intruder Attempts ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1624.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Intruder Limit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1630.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Intruder Reset Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1636.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Maximum Simultaneous ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1642.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Script ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1648.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Login Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1654.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Low Convergence Reset Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1660.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Low Convergence Sync Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1666.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Mailbox ID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1672.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Mailbox Location ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1678.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("manager ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3onljj.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("masvAuthorizedRange ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1684.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("masvDefaultRange ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1690.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("masvDomainPolicy ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1696.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("masvLabel ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1702.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("masvProposedLabel ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1708.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Member ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1726.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Members Of Template ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1732.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Memory ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1738.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Message Routing Group ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1744.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Message Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1750.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Messaging Database Location ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1756.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Messaging Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1762.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1768.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Minimum Account Balance ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1786.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("mobile ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3oojmc.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Certificate Chain ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4104.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Given Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4110.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Key File ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4116.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Key Material DN ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4122.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Keystore ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknqe.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Not After ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknpk.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Not Before ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknpe.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Parent CA ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4128.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Parent CA DN ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4134.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Private Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4140.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Public Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4146.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Public Key Certificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4152.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:SD Key Cert ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknq2.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:SD Key ID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknq8.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:SD Key Server DN ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknpq.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:SD Key Struct ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknpw.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Subject Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4158.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Tree CA DN ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4164.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:Trusted Root Certificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknp8.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSPKI:userCertificateInfo ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2oknp2.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Network Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4170.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Network Address Restriction ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4176.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("New Object's DS Rights ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4182.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("New Object's FS Rights ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4188.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("New Object's Self Rights ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4194.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NNS Domain ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4338.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Notify ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4374.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:administratorContactInfo ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4392.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:adminURL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4398.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailAccessDomain ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4404.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailAlternateAddress ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4410.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailAutoReplyMode ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4416.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailAutoReplyText ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4422.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailDeliveryOption ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4428.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailForwardingAddress ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4434.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailHost ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4440.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailMessageStore ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4446.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailProgramDeliveryInfo ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4452.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AmailQuota ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4458.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AnsLicenseEndTime ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4464.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AnsLicensedFor ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4470.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:AnsLicenseStartTime ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4476.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:employeeNumber ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4482.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:installationTimeStamp ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4488.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailRoutingAddress ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a2ixy4e.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:memberCertificateDesc ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4554.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mgrpRFC822mailmember ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4560.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:ngcomponentCIS ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4572.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nsaclrole ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4578.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nscreator ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4584.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nsflags ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4590.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nsnewsACL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4614.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nsprettyname ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4620.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:serverHostName ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4626.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:serverProductName ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4632.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:serverRoot ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4638.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:serverVersionNumber ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4644.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:subtreeACI ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4650.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4656.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Obituary ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4662.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Obituary Notify ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4668.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Object Class ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4674.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Operator ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4680.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Other GUID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4686.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4692.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Owner ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4698.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Page Description Language ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4704.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("pager ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3oojmj.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Partition Control ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4716.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Partition Creation Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4722.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Partition Status ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4728.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Password Allow Change ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4734.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Password Expiration Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4740.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Password Expiration Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4746.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Password Management ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4752.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Password Minimum Length ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4758.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Password Required ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4764.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Password Unique Required ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4770.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Passwords Used ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4776.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Path ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4782.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Permanent Config Parms ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4788.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Physical Delivery Office Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4794.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Postal Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4800.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Postal Code ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4806.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Postal Office Box ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4812.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Postmaster ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4818.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("preferredDeliveryMethod ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4824.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("presentationAddress ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4830.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Print Job Configuration ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4848.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Print Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4854.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4860.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer Configuration ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4872.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer Control ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4878.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Private Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4914.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Profile ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4920.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Profile Membership ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4926.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("protocolInformation ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4932.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Public Key ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4944.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Purge Vector ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4950.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Queue ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4956.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Queue Directory ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4962.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Received Up To ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4968.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Reference ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4974.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("registeredAddress ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4980.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Replica ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5010.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Replica Up To ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5016.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Resource ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5028.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Revision ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5064.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Role Occupant ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5070.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("roomNumber ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5076.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Run Setup Script ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5082.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5088.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5094.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SAP Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5100.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SAS:Security DN ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5106.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SAS:Service DN ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5112.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("searchGuide ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5118.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("searchSizeLimit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5124.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("searchTimeLimit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5130.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Security Equals ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5136.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Security Flags ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5142.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("See Also ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5148.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Serial Number ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5154.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5160.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Server Holds ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5166.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Set Password After Create ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5172.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Setup Script ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5178.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Status ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5286.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("supportedAlgorithms ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5298.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("supportedApplicationContext ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5304.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Supported Connections ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5310.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Supported Gateway ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5316.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Supported Services ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5322.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Supported Typefaces ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5328.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Surname ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5334.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Synchronization Tolerance ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5358.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Synchronized Up To ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5364.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5370.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Telephone Number ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5376.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("telexNumber ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5382.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("telexTerminalIdentifier ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5388.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Timezone ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5394.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Title ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5400.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Transitive Vector ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5406.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Trustees Of New Object ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5412.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Type Creator Map ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5418.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink(" ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5424.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("uniqueID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5430.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Unknown ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5436.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Unknown Auxiliary Class ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5442.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Unknown Base Class ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5448.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Used By ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5454.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("User ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5460.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("userCertificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5466.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("userPassword ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6m1fnz.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Uses ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5472.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Version ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5478.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Volume ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5484.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Volume Space Restrictions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5490.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("WANMAN:Cost ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5496.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("WANMAN:Default Cost ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5502.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("WANMAN:LAN Area Membership ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5508.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("WANMAN:WAN Policy ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5514.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("x121Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5520.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("x500UniqueIdentifier ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5526.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Novell Attribute Extensions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3fh5xp.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("audio ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3omwno.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("carLicense ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3on4e7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Client Install Candidate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk980.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Color Supported ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk992.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Database Dir Path ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1010.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Database Volume Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1016.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Datapool Location ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1022.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Datapool Locations ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1028.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Delivery Methods Installed ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1046.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("displayName ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3oorbo.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Employee ID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1114.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Entrust:AttributeCertificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1126.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Entrust:User ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1132.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("GW API Gateway Directory Path ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1204.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("GW API Gateway Directory Volume ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1210.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("IPP URI ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1300.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("IPP URI Security Scheme ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1306.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("jpegPhoto ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3onfdu.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("labeledUri ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3onkke.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP Class Map ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1372.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("ldapPhoto ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3op8zp.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAPUserCertificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1504.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:ARL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1510.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:caCertificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1528.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:CRL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1534.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("LDAP:crossCertificatePair ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1540.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("MASV:Authorized Range ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a9j2co5.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("MASV:Default Range ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a9j2cob.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("MASV:Domain Policy ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a9j2coh.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("MASV:Label ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a9j2con.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("MASV:Proposed Label ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a9j2cot.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Maximum Speed ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1714.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Maximum Speed Units ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1720.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("MHS Send Directory Path ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1774.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("MHS Send Directory Volume ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1780.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Accountant Role ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1792.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Control Flags ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1798.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Database Saved Timestamp ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1804.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Database Saved Data Image ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1810.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Database Saved Index Image ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1816.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Default Printer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1822.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Default Public Printer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1828.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Job Configuration ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1834.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Manager Status ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1840.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Operator Role ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1846.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Printer Install List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1852.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Printer Install Timestamp ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1858.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Printer Queue List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1864.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Printer Siblings ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1870.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Public Printer Install List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1876.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS Replace All Client Printers ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1882.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS SMTP Server ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1888.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDPS User Role ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1894.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual All Attributes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1900.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Attribute Count ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1906.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Attributes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1912.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Base Object ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1918.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Catalog Size ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1924.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual End Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1930.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Filter ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1936.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Object Count ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1942.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Return Code ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1948.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Scope ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1954.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Search Aliases ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1960.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Start Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1966.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Actual Value Count ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1972.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:All Attributes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1978.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:AttrDefTbl ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1984.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Attributes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1990.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Auto Dredge ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk1996.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Base Object ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk2002.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:CatalogDB ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk2008.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Catalog List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk2014.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Dredge Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4008.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Filter ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4014.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:IndexDefTbl ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4020.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Indexes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4026.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Label ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4032.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Log ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4038.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Master Catalog ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4044.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Max Log Size ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4050.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Max Retries ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4056.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Max Threads ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4062.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Retry Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4068.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Scope ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4074.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Search Aliases ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4080.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Slave Catalog List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4086.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Start Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4092.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NDSCat:Synch Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4098.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Common Certificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4200.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Current Installed ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4206.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Current Peak Installed ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4212.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Current Peak Used ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4218.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Current Used ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4224.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Hourly Data Size ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4230.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:License Database ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4236.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:License ID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4242.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:License Service Provider ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4248.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:LSP Revision ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4254.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Owner ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4260.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Peak Installed Data ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4266.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Peak Used Data ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4272.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Product ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4278.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Publisher ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4284.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Revision ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4290.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Search Type ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4296.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Summary Update Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4302.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Summary Version ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4308.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Transaction Database ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4314.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Transaction Log Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4320.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Transaction Log Size ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4326.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NLS:Version ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4332.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Notification Consumers ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4344.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Notification Profile ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4350.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Notification Service Enabled ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4356.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Notification Srvc Net Addr ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4362.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Notification Srvc Net Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4368.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NRD:Registry Data ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4380.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NRD:Registry Index ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4386.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailAccessDomain ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4494.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailAlternateAddress ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4500.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailAutoReplyMode ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4506.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailAutoReplyText ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4512.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailDeliveryOption ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4518.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailForwardingAddress ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4524.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailHost ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4530.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailMessageStore ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4536.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailProgramDeliveryInfo ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4542.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:mailQuota ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4548.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:ngComponent ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4566.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nsLicenseEndTime ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4596.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nsLicensedFor ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4602.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NSCP:nsLicenseStartTime ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4608.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Page Description Languages ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4710.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("preferredLanguage ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3oon3t.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Primary Notification Service ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4836.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Primary Resource Service ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4842.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer Agent Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4866.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer Manufacturer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4884.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer Mechanism Types ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4890.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer Model ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4896.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer Status ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4902.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printer to PA ID Mappings ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4908.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("PSM Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4938.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Registry Advertising Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4986.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Registry Service Enabled ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4992.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Registry Srvc Net Addr ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk4998.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Registry Srvc Net Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5004.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Resolution ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5022.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Resource Mgmt Srvc Net Addr ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5034.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Resource Mgmt Srvc Net Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5040.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Resource Mgmt Service Enabled ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5046.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Resource Mgr Database Path ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5052.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Resource Mgr Database Volume ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5058.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("secretary ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3oon40.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Sides Supported ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5184.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Attribute ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5190.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Cache Limit ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5196.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP DA Back Link ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5202.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Directory Agent DN ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5208.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Language ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5214.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Lifetime ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5220.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Scope Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5226.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Scope Unit DN ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5232.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Start Purge Hour ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5238.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Status ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5244.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP SU Back Link ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5250.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP SU Type ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5256.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP Type ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5262.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SLP URL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5268.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SMS Protocol Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5274.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SMS Registered Service ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5280.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SU ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5292.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SvcInfo ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5340.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SvcType ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5346.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("SvcTypeID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5352.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("userSMIMECertificate ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a3oorbh.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("LDAP Operational Attributes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a7lnqjy.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("createTimeStamp ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fur3q.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("creatorsName ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fur3f.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("entryFlags ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fuxcp.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("federationBoundary ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fzxsm.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("localEntryID ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fzcam.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("modifiersName ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fur3j.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("modifyTimeStamp ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fur3x.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("structuralObjectClass ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fuxcb.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("subordinateCount ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fuxci.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("subschemaSubentry ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a6fuxc4.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Attribute Syntax Definitions ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/h55cqjqs.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Back Link ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5533.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Boolean ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5540.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Case Exact String ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5547.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Case Ignore List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5554.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Case Ignore String ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5561.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Class Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5568.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Counter ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5575.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Distinguished Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5582.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("EMail Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5589.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Facsimile Telephone Number ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5596.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Hold ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5603.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Integer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5610.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Interval ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5617.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Net Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5624.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Numeric String ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5631.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Object ACL ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5638.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Octet List ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5645.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Octet String ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5652.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Path ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5659.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Postal Address ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5666.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Printable String ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5673.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Replica Pointer ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5680.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Stream ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5687.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Telephone Number ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5694.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Time ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5701.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Timestamp ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5708.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Typed Name ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5715.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Unknown ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/sdk5722.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Index of Classes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx1.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("A through B ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx2.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("C through D ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx3.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("E through K ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx4.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("L through M ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx5.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("N ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx6.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("O ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("P through R ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx8.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("S ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx9.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("T through Z ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx10.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Index of Attributes ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx11.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("A ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx12.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("B ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx13.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("C ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx14.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("D ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx15.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("E ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx16.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("F through G ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx17.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("H ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx18.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("I through K ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx19.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("L ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx20.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("M ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx21.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("N ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx22.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("O ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx23.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("P ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx24.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Q ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx25.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("R ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx26.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("S ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx27.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("T ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx28.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("U ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx29.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("V through Z ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx30.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Index of ASN.1 IDs ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx31.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("0 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx32.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("1 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx33.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("2 through 2.4 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx34.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("2.5 through 2.9 ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx35.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Index of LDAP Names ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx36.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("A through B ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx37.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("C ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx38.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("D ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx39.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("E through F ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx40.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("G ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx41.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("H ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx42.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("I through K ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx43.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("L ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx44.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("M ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx45.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("N ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx46.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("O ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx47.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("P ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx48.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Q through R ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx49.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("S ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx50.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("T ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx51.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("U through Z ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/schidx52.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Revision History ","http://developer.novell.com/ndk/doc/ndslib/schm_enu/data/a5i29ah.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Iterator Services ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hnv8aaj7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Concepts ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hj3udfo7.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Iterator Objects ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hwiuqovp.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Creation of an Iterator Object ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hrb7xece.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Iterator Indexes ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hqngpqag.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Positions of an Iterator Object ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/h25zhm0d.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Current Position Movement with Retrieval Functions ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hn9jdbnd.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Retrieval of Data ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hy7j1t07.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Tasks ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/huypg52u.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Creating a Search Iterator Object ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hcyx2utx.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Retrieving and Unpacking Object and Attribute Name Data ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/h9evr0ru.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Retrieving and Unpacking Object, Attribute, and Value Data ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/htq89y7t.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Functions ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/h7qwv271.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("NWDSItrAtEOF ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk29.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrAtFirst ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk36.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrClone ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk43.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrCount ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk50.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrCreateList ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk57.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrCreateSearch ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk64.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrDestroy ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk71.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrGetCurrent ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk77.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrGetInfo ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk84.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrGetNext ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk91.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrGetPosition ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk98.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrGetPrev ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk105.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrSetPosition ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk112.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrSetPositionFromIterator ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk119.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrSkip ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk126.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("NWDSItrTypeDown ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/sdk133.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("NDS Iterator Example Code ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hw9m9u6o.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

currentLevel++;

setLevels();
var navElement = navigationLink("Cloning an Iterator Object: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hur66hmi.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Counting with NDS Iterators: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hgllfzfg.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Creating and Using a List Iterator: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hfnbz1tw.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Creating a Search Iterator and Displaying the Results: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hhe6xegc.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Getting Iterator Information: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hfg59w8k.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Getting and Setting the Iterator's Position: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hh03dp06.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Listing in Reverse Order: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hsj5zfs1.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Positioning the Iterator with Typedown: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/hqvieqdk.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

setLevels();
var navElement = navigationLink("Skipping Objects in the List: Example ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/ho81tg5d.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

setLevels();
var navElement = navigationLink("Revision History ","http://developer.novell.com/ndk/doc/ndslib/skds_enu/data/a5i29ah.html",currentLevel,index,levelIndexes[currentLevel],levelParents[currentLevel],"");
navigationTree[index] = navElement;
index++;

if (currentLevel > 1) currentLevel--

if (currentLevel > 1) currentLevel--
	
   
