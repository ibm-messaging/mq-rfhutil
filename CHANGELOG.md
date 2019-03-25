
* 30 Oct 2000 - Original release of SupportPac IH03


* Build 100
   * Changed about dialog box to use version string resource


* Build 101
   * Added menu items for file reads and writes
   * Corrected Clear Data to clear only user data area
   * Removed Read ASCII button (replaced with menu item)
   * Fixed bug where Browse Next and End Browse buttons not initially disabled


* Build 102
   * Changed qm connection to keep and reuse connections to qm


* Build 103
   * Added support for group and segmentation options
   * Fixed bug in output of report options (incorrect switch statements)


* Build 104
   * Message id and sequence number fields updated to reflect the last message written
   * Added close Q button to allow closure of any open Q and disconnect from current QM
   * Added Save RFH menu option on file reads


* Build 105
   * Fixed bug where inform if retained option was being ignored
   * Added separate check box for isRetainedPub
   * Allow feedback to be changed and used on MQ puts


* Build 107
   * Allowed building of publish/subscribe response folder entries (pscr)


* Build 108
   * Added support for alternate user ids


* Build 109
   * Release build for September 23, 2002 as R3.1


* Build 110 (First build of R3.1.1)
  * Changed pscr tab so that reason and other parts of response are handled correctly.
  * Corrected bug with Unix encoding with data in PC format not handled correctly (looking at the rfh encoding instead of the mqmd encoding when building the rfh.


* Build 111
  * Added code so that recent file list so that works.
  * Allow application identity to be set


* Build 112 (first build of V3.2)
  * Added support for V1 pub/sub
  * Support for both V1 and V2 header in the same message
  * Fixed read file support to set MQMD when RFH header is found in file
  * Fixed miscellaneous tool tips (some were missing or wrong tool tip displayed)
  * Added support for event broker fields
  * Added get by correlation id option
  * Added report option of pass correl id
  * Automatically adjust window size when large fonts are being used


* Build 113 (created 10/13/2002)
  * Fixed bugs in parsing and building jms folder (incl. delivery mode and sequence)
  * Added jms timestamp and user defined fields
  * Fixed bugs in parsing and building rfh1 pubsub folders


* Build 114 (release candidate 1 for V3.2 - created 10/20/2002)


* Build 115 (release build for V3.2 - created 10/23/2002)


* Build 116 (initial build for V3.21 - created 10/25/2002)
  * Changed open options for browse to not include input_shared - to fix problem with setmqaut +browse -get


* Build 117 (initial build for V3.22 - created 10/31/2002)
  * Change main window title to match queue name from last MQ operation
  * Queue name drop down list is populated with queue names for server version


* Build 118 (created 11/2/2002)
  * Added QNames class to gather and maintain queue names for server version
  * Added code to treat blank queue manager name as default queue manager
  * Greatly improved drop down performance with large numbers of queues
  * Load names button now refreshes QNames cache
  * Added memory leak detector to debug version of project


* Build 119 (created 11/14/2002)
  * Added menu option to control whether system queues are shown in dropdown list
  * Shipped as V3.23


* Build 120 (initial build for 3.23 - created 11/20/2002)
  * Save the last 20 queue names used in the registry and reload when starting (client)


* Build 121 (created 11/22/2002)
  * Expand escape sequences in attribute values and data in XML displays.
  * Fixed bug in routine used to find MQSeries directory prefix (used by client QM name lookup for client table).


* Build 122 (created 12/2/2002)
  * Fixed bug in No RFH menu item not working for write file without RFH.
  * Added date compiled to About dialog display.


* Build 123 (created 1/29/2003)
  * Changed cobol copybook processing to drop <= blank at end of cobol lines


* Build 124 (created 2/4/2003)
  * Corrected error in hex display (last charcter being dropped).


* Build 125 (created 3/10/2003)
  * Corrected error handling of include stream name parameter in V1 RFH.


* Build 126 (created 3/17/2003)
  * Added missing code that was ignoring persistent reqistration options.


* Build 128 (created 3/27/2003)
  * Fixed bug where change to MQMD code page or encoding was not releasing the existing RFH header area.  The RFH header area must be invalidated and rebuilt when a message or file is written.
  * Fixed bug where change in the RFH CCSID field did not invalidate the existing RFH header area.
  * Changed the name of the Publish Queue Manager and Publish Queue to Subscription Queue Manager and Subscription Queue, to better describe the function of these fields.


* Build 129 (created 4/23/2003)
  * Added tool tip help for pub/sub other field and load queue names button.


* Build 130 (created 4/29/2003)
  * Added CICS header tab


* Build 131 (created 5/5/2003)
  * Fixed bug where COBOL name is missing and first entry is PIC or  PICTURE


* Build 132 (Created 6/3/2003)
  * Made accounting token field longer to avoid truncation


* Build 133 (Created 7/14/2003)
  * Fixed problems with CICS bridge code, including problem
    with UOW radio buttons being one off.
  * Fixed problem with having program name even if tran selected.
  * Made prog the default.in CIH


* Build 134 (Created 7/26/2003)
  * Changed CIH program name field to be translated based on CIH code page, not MQMD
  * Changed ADS options to be checkboxes, to allow more than one option to be specified
  * Added menu options to save and restore message, correlation and group ids
  * Fixed bug with lower case queue names not being enumerated
  * Fixed bug when using same CICS header in several successive WriteQs - would cause trap


* Build 135 (Created 8/21/2003)
  * Made major changes to handling of client channel handling, including support for direct entry of client channel information in format like the MQSERVER variable.


* Build 136 (Created 9/5/2003)
  * Fixed bug in buildrfh2 where using mqmd charset rather than m_RFH_charset.
  * Added tooltip help for fields on CICS tab
  * Fixed bugs in CICS tab.
  * Moved some fields on the CICS tab to more appropriate locations
  * Added save and restore menu items for CICS facility.
  * Added cut, copy, paste and select all support for CICS fields.
  * Removed TRACE statements and unneeded handlers for onChar, onKeyDown, etc.


* Build 137 (Created 9/12/2003)
  * Added Read and Write menu items to read and write files
  * Fixed bug with first entry in MRU file list not rereading file.
  * Fixed bug in client not remembering full name when using MQSERVER format.


* Build 138 (Created 9/19/2003)
  * Fixed bug in message log display when reading using menu option.
  * Created V3.4
  * Delivered V3.4 as new SupportPac


* Build 139 (Created 10/14/2003)
  * Added wait cursor to purge, etc requests


* Build 140 (Created 11/21/2003)
  * Added save and restore MQMD options
  * Moved feedback field to MQMD page
  * Added priority to MQMD options
  * Added original length field to MQMD options
  * Allow modification of most MQMD fields


* Build 141 (Created 1/8/2003)
  * Fixed bug - missing vertical scroll bars on QM dropdown menu.


* Build 142 (Created 1/8/2003)
  * Delivered to SupportPac Web Site


* Build 143 (Created 2/9/2003)
  * Fixed bug where qname variable drop down was not selecting properly
  * Fixed bug where the maximum message size for channel connections was 4MB.  Now allow 100MB.


* Build 144 (Created 3/18/2003)
  * Fixed problem where recent file list was not working correctly (being corrupted)
  * Fixed problem with multiple 01 levels in same copy book.  Now ignore any beyond first.


* Build 145 (Created 3/29/2003)
  * Fixed problem with getBinaryValue and getVarValue in Copybook.cpp (2-byte host-encoding)
  * Fixed problem with depending on clause with min or max values.
  * Made internal improvements to copybook code, including introduction of the names class for more efficient handling of name storage and overall efficiency.


* Build 146 (Created 4/12/2004)
  * Fixed bug in redefines clauses not working - introduced in previous build.
  * Added file name to "Copybook file read" message


* Build 147 (Created 5/12/2004) V3.5
  * Added support for search, including hex and character find options.
  * Added goto offset option to search menu.
  * Changed to remember current data position when switching tabs
  * Added 2004 to copyright statement
  * Distributed SupportPac Update


* Build 148 (Created 5/22/2004) V3.5.1
  * Fixed bug in search for find string at end of data for XML and Parsed displays.
  * Changed search to highlight first character of actual data in XML and Parsed displays
  * Fixed problem with different display formats remembering searches in other formats.
  * Fixed problem where view was not reseting the parsed display.
  * Added accelerator keys for find and goto dialogs (Ctrl-F and Ctrl-G).
  * Fixed bug where queue names were not being populated in dropdown correctly.
  * Make some internal improvements in the Names object.


* Build 149 (Created 6/7/2004) V3.5.1
  * Delivered as new release of RFHUtil (to correct the queue names bug above).


* Build 150 (Created 9/17/2004) V3.5.2
  * Fixed bug where invalid remote queue name was being remembered and caused 2087 errors.


* Build 151 (Created 10/5/2004) V3.5.3
  * Added code to recognize and handle non-ascii mqmd or big-endian mqmd in file.
  * Added support for MQ Version 6
  * Fixed bug with invalid entry in <mcd> folder - was causing loop
  * Changed q and qm to be pointers rather than objects - to try and allow use of RFHUtil if MQ is not installed


* Build 152 (Created 11/21/2004) V3.5.3
  * Fixed problem with first file in MRU list not being read.  Not perfect if multipe RFHUtil instances running.


* Build 153 (Created 12/08/2004) V3.6
  * Created new release version.  Released in December, 2004.


* Build 154 (Created 1/05/2005) V3.6.1
  * Created new release version, to distinguish new changes from the last release.
  * Fixed problem with finding deleted temporary dynamic queues.


* Build 155 (Created 3/06/2005) V3.6.2
  * Added support for browse previous button.
  * Added accelerator keys for browse next and browse previous.
  * Added dialog to display all messages in queue and allow selection of message to read
  * Added support for dead letter queue headers
  * Rewrote and changed chaining of headers - this needed cleaning up
  * Fixed bug in handling of user defined tag in jms folder of rfh2 header
  * Changed default to save mqmd with data on file writes
  * Added dialog to save multiple messages in queue to a file or files
  * Changed default display for message id, correl id and group id to be hex instead of ascii
  * Moved common subroutines out of dataarea.cpp into comsubs.cpp, mqsubs.cpp and xmlsubs.cpp
  * Sent copy of this level for testing to various people


* Build 156 (Created 3/14/2005) V3.6.2
  * Added load queue dialog
  * Fixed bug where set all was not working (open option was not getting set)
  * Fixed bug where display Q was not showing all messages (was not resetting correlid after get)
  * Sent to one customer for review


* Build 157 (Created 3/15/2005) V3.6.4
  * Testing of save q and load q dialog options


* Build 158 (Created 3/15/2005) V3.7
  * Fixed bug where RFHUtil was crashing after reading past the end of the queue
  * Changed so Load Q and Save Q buttons are properly disabled during browse operations
  * Distributed for testing to small group


* Build 159 (Created 3/16/2005) V3.7.1
  * Added load names support for client version
  * Fixed bug where message priority was not being set when writing a message


* Build 160 (Created 3/24/2005) V3.7.1
  * Implemented starting message number on save messages dialog
  * Fixed bug where start browse was trapping when invalid queue name was specified
  * Fixed bug where queue manager name was not in document variable when show cluster was selected
  * Improved implementation of the combo box for the queue manager name on the main dialog
  * Made same changes to combo box implementations for queue name and pubsub queue manager


* Build 161 (Created 4/02/2005) V3.7.2
  * Changed the dialogs to use fixed font and removed automatic adjustment at startup
  * Changed dialog fonts to use msdialog font
  * Changed default original length field in clearMQMD to undefined (-1) from 0.
  * Changed default sequence number in clearMQMD to 1 from 0.
  * Minor change to layout of the main dialog.


* Build 162 (Created 4/06/2005) V3.7.3
  * Allow specification of queue manager name on command line and avoid an error otherwise, due to the use of CWinApp.
  * Fixed bug in start browse on client version where start browse for > 4M msg caused a trap.  Also added code to use MQCONNX rather than MQCONN.
  * Changed the sizing of the MainFrame window to set an arbitrary initial size in the preCreateWindow routine and then resize the frame window in the OnInitDialog routine in general.ccp.
  * Changed icon to smiling dog.


* Build 163 (Created 4/9/2005) V3.8
  * Sent to various people to assess if new screen sizing is working
  * Changed the implementation of finding queue managers to use dspmq.
  * Fixed bug where resetClientQnames was not reinserting qm name in table


* Build 164 (Created 4/11/2005) V3.9
  * Made changes to algorithms to find queue names to use approved interfaces.
  * Changed the algorithm to find the name of the default queue manager.


* Build 165 (Created 4/12/2005) V3.91
  * First test build for new release.  Distributed to limited set of testers.


* Build 166 (Created 4/13/2005) V3.92
  * Eliminated separate qm and q objects for browses, to fix already connected problem.
  * Update queue depth after loading a queue.
  * Changed displayQ options to buttons rather than radio buttons.
  * Changed file naming algorithm for saveQ and loadQ.
  * Changed find queue managers to not search for word Running (NLS considerations).
  * Changed message data edit box title to reflect the source of the data.
  * Eliminated last in group indicator and added convert option on main panel
  * Added commit logic when putting a message with automatic segmentation
  * Segmentation allowed check box moved to MQMD page
  * Complete message option added to get options on main page
  * added IMS header tab and class
  * Fixed bug with browse next and browse prev accelerator keys causing trap if used before start browse
  * Changed accelerator keys for browse next and browse prev
  * Added accelerator keys for Ids and View menus.
  * Disable browse previous key if logical order is specified. (removed this change)
  * Do not allow certain options to be changed during browse operation.
  * Change implementation of browse previous to fix problems with logical order.
  * Fixed problems with IMS parse of flags and not noticing code page is changed.
  * Added group and segment indicators to queue display.


* Build 167 (Created 4/26/2005) V3.93
  * Added error message if allocation for display Q fails.
  * Fixed problem with chaining of IMS, CICS and DLQ headers clearing format field.  Problem was allowing entry of format field before putting the header in the chain.  The entered format field was overwritten.


* Build 168 (Created 4/27/2005) V3.94
  * Added ability to place LLBB field at front of IMS data
  * Changed EBCDIC/ASCII translation tables for Euro (EBCDIC 9F = ASCII 80)
  * Changed character and both displays to show Euro code point (80)
  * Recognize additional EBCDIC code pages
  * Fixed problem with 2024 message on purge (exceeding max uncommitted count)
  * Fixed bug with overflow in offset of first selected character in display Q.
  * Added wait cursor to browse next and browse prev
  * Increased max display messages to 50000.
  * Changed EBCDIC/ASCII translation tables and character displays.


* Build 169 (Created 4/29/2005) V3.95
  * Fixed bug in copybook processing where a non-copybook file caused an exception.
  * Changed dlq, ims and cics tabs to automatically set focus if no header selected
  * Recognize code pages for far east and set display accordingly.
  * Added limited support for input messages in Unicode UCS2 code pages 1200, etc.
  * Improved handling of national characters in single-byte code pages.
  * Improved recognition of lead bytes for multi-byte code pages.
  * Changed CNO version in QNames.cpp to 2 from 4 for better backwards compatibility (MQ V5.2, etc)
  * Improved EBCDIC/ASCII translation to use Windows translation where possible
  * Added check for queue name before doing loadQ, saveQ, displayQ and purgeQ.


* Build 170 (Created 5/05/2005) V3.96
  * Fixed bugs in character display - neither ASCII nor EBCDIC was working correctly
  * Fixed bug where reading a file with an MQMD was not setting the fileCcsid variable
  * Improved the file search for character data to translate search string to EBCDIC if data is EBCDIC
  * Improved EBCDIC translation for XML and parsed displays (use code pages when known).
  * Added display of error message when Load Names fails.
  * Added save and restore capability for IMS transaction instance ID.
  * Changed display of invalid characters in XML and parsed displays


* Build 171 (Created 5/14/2005) V3.97
  * Enhanced EBCDIC translation of COBOL display to use Windows if possible.
  * Fixed problem with character displays not working with certain code points.
  * Fixed bug where font was not being updated when file was read with data page displayed
  * Fixed bug where data page not refreshed when most recently used file list used to read file.
  * Removed option of handle share none from qnames MQCONNX


* Build 172 (Created 5/17/2005) V3.98
  * Added support for Z/OS load names (needed PCF V3 support)
  * Fixed bug with PCF replies that were in big-Endian format
  * Made syncpointing explicit to support Z/OS with client version
  * Fixed bug with load names hitting syncpoing limit if large number of queues defined
  * Fixed bug where browse next and browse prev buttons on msg data page were not updating the font in the edit box.


* Build 173 (Created 5/17/2005) V4.00
  * Release candidate 1.
  * Fixed bug in handling of units of work with groups
  * Changed queue display to distinguish between last in group and in group.
  * Same change as 3 above for segments.
  * Fixed bug in saving messages from queue without group index (2394 error) - turn off the group options on the read.
  * Fixed bug where save messages with remove from queue option was not working unless startcount was > 1.
  * Added support to use MQMD ccsid and encoding when using Convert option on read Q and browse Q operations.
  * Fixed bug where convert was not working on browse previous.
  * Increased width of IMS transaction instance id so that 32 zeros display without scrolling.
  * Fixed bug where length of MQMD was not being reported on file writes.
  * Released May 21, 2005 as V4.00


* Build 174 (Created 6/7/2005) V4.01
  * Fixed bug where switching to CICS page was acting like selection of Version 2 radio button.
  * Fixed bug where XML escape sequences in lower case were not being recognized in input data for XML displays.
  * Fixed bug in browse previous where some messages had a correlation id.
  * Corrected bug where pub/sub connect to queue manager name was limited to 20 characters.
  * Corrected double-byte character length routine.


* Build 175 (Created 6/23/2005) V4.02
  * Fixed bug where client version was using new message id on load queue for MVS versions before V6.
  * Removed copybook button from main screen and added support for specifying user id and password for connecting to QM.
  * Added alternate user id check box to main screen, to distinguish between set user context and alternate user id options.
  * Fixed bug where JMS user defined field was not being cleared when another message was read.


* Build 176 (Created 7/29/2005) V4.03
  * Fixed bug with processing of channel tables - two errors where using wrong field in MQCD to build entries.
  * Fixed bug in character translation where failed windows translate did not use the default translation.
  * Expanded maximum characters allowed in code page field to 5 from 4.


* Build 177 (Created 9/15/2005) V4.03
  * Fixed bug with rfh1 as entire message thinking the rfh length was invalid (< instread of <=)
  * Fixed bug where accounting token max length was set to 16 rather than 64 (32 bytes displayed in hex).


* Build 178 (Created 10/10/2005) V4.04
  * Released new version 4.04 with bug fixes.


* Build 179 (Created 10/12/2005) V4.05
  * Fixed inconsistency between displayed message sizes between browseNext and browsePrev.
  * Added support to remember the last used queue manager and queue names in the registry and retrieve them the next time RFHUtil is started.
  * Changed the initial processing of local queue managers to check for a status of ended and to not try to retrieve a list of queues.  This speeds up the startup time.
  * Fixed bug where losing changes to RFH2 format field (forgot to set changed flag).
  * Fixed bug where was enabling RFH V2 fields when switch to RFH page even if no RFH V2 was selected
  * Added check for invalid code pages
  * Added trace to file capability


* Build 180 (Created 3/29/2006) V4.06
  * Fixed missing PACKED-DECIMAL usage type in COBOL copy book.
  * Fixed message data dialog so that the scroll keys (like ctrl + home and ctrl + end) work.
  * Added new message report options from MQ V6 (activity, discard, etc)


* Build 181 (Created 4/28/2006) V4.07
  * Added new menu options to allow connection/disconnect to queue manager and opening/closing queues explicitly
  * Corrected display queue display of no messages found when real problem was MQ error (such as not auth)
  * Enhanced trace (trace QNames.ccp)
  * Added transmission queue name to MQ connect message.


* Build 182 (Created 6/1/2006) V4.07
  * Allow paste into usr and other edit boxes.
  * Added indenting to EDI displays for groups and transactions
  * Fixed bug where was not picking up new delimiter for EDI documents when another ISA segment was processed.
  * Issue better error message when error 2003 (commit failed) occurs during purge queue operation.
  * Made WriteFile routine more efficient by not copying the user data to a temporary buffer.


* Build 183 (Created 8/1/2006) V4.08
  * Added support for drag and drop of files
  * Added support for tool tips to find dialogs and connection user id dialog
  * Fixed bug in formating of XML display data (was not removing escape sequences correctly).


* Build 184 (Created 8/10/2006) V4.09
  * New release for PowerPack


* Build 185 (Created 8/14/2006) V4.10
  * Added routine to catch save (Ctrl+S) requests and process as file write.
  * Delivered as new SupportPac V4.0.9 8/31/2006


* Build 186 (Created 9/1/2006) V4.11
  * Added accelerator keys for all functions that are currently buttons only
  * Fixed bug in Close Q when q variable was null.
  * New release of IH03


* Build 187 (Created 9/5/2006) V4.20
  * Added support for tabbing between controls on all pages
  * Fixed tab order in a number of cases.
  * Changed all groups of radio buttons so only the first is a tab stop.
  * Added ability to use Alt+N to move to next tab and Alt+P to move to previous tab


* Build 188 (Created 9/16/2006) V4.21
  * Added check to make sure trace file open was actually successful before using it


* Build 190 (Created 10/11/2006) V5.0
  * Internal restructure to split up DataArea class and move function to the individual display classes
  * Updated application type display to handle new values of 31 through 34
  * Fixed problems with editing of correl and group ids on MQMD page by creating a typeover CEdit class.
  * Changed display of RFH2 usr values to name=value pairs.
  * Added support for multiple other folders.  Each folder is on a separate line.
  * Added special subclass for correlation and group id editing on the MQMD page.  Supports overtype and validates hex characters.
  * Added handlers in IDEdit class for delete and backspace keys.  This preserves the caret location.
  * Added support for UCS-2 data when creating XML or parsed displays.
  * Added check for queue browse operation before enabling file get menu option.
  * Corrected file menu write operation to Save operation and accelerator key to Ctrl+S
  * Capture locale information in the trace file when starting up.
  * Added support for get by group id.
  * Fixed bug when proper code page tables were not installed in Windows and Windows translation failed - no character and both data was generated.
  * Fixed problem where top and bottom edit menu items were not working.
  * Save the remote queue manager name as well as the queue and queue manager names
  * Added 15 second cut-off to display queue.  Display queue will now end after either 50,000 messages or 15 seconds in addition to end of queue.
  * Fixed bug where show cluster queues would not turn off or on because the system queue setting was checked instead of the view cluster setting.
  * Fixed bugs where show cluster queues was not working (needed to have the equivalent of the clusinfo parameter in the request message)
  * Optimized PCF request message so only queue name and type are returned in the reply messages.
  * Fixed bug where the initial queue manager name was not being propagated to the dataarea object until an MQ operation was attempted, which caused a failure if the view cluster queues menu option was selected before any MQ operation was attempted.
  * Added support for SSL client connections.
  * Decrement queue depth counter as progress indicator when purging messages from queue.
  * Save last used connection user id and password.
  * Save last used SSL connection information (client version only).


* Build 191 (Created 4/13/2007) V5.0
  * Release candidate 1 for V5.


* Build 192 (Created 4/17/2007) V5.01
  * Fixed bug when trying to set the message type.  There was an invalid statement in the kill focus routine.
  * Removed invalid check for option and set of m_ps_subpoint from pubsub.cpp.
  * Corrected setting of RFH V1 length field to binary number rather than string value in RFH.cpp.
  * Changed several instances of unsigned short  * to LPWSTR.


* Build 193 (Created 4/30/2007) V5.01
  * Release build for V5.0.1


* Build 194 (Created 5/21/2007) V5.02
  * Fixed problem with backspace not working in IdEdit class.
  * Changed QM and Queue combo boxes on main tab to use special subclass so that backspace works.
  * Changed QM and Queue combo boxes on main tab to use special subclass so that they will autocomplete.
  * Fixed bug in XML display format where data was not XML - was releasing the same storage twice.  The second free failed.
  * Fixed EBCDIC translation routine used by XML processing routines to use MQ tables when Windows translation fails.
  * Fixed bug where certain Publish and Subscribe options were not being recognized in an RFH V1 header
  * Fixed bug where wrong options were being enabled when the focus was switched to the pub/sub page
  * Added check when setting focus to a control when a page received the focus was causing a routine to be invoked even though the user did not select a button.
  * Fixed bug where reason text in RFH V1 was being mistaken for reason since they both start with MQPSReason - reversed the order of the check.
  * Fixed bug when writing message with multiple headers (such as MQDEAD pointing to MQHRF2 resulted in loop in getHdrCcsid routine.
  * Fixed bug in RFH user area parsing and rendering where attributes such as dt were not being recognized.
  * Fixed bug where selection of include RFH V1 or RFH V2 was not enabling the appropriate folder check boxes.
  * Added support to allow setting of message id in MQMD.
  * Updated LoadEditBox() routine in PubSub.cpp to use same code as in General.cpp, avoiding reading registry entries directly.
  * Fixed bug where accounting token was not being set when set user id or set all was specified.
  * Bug where null XML values in RFH2 folders not being properly recognized.


* Build 195 (Created 5/21/2007) V5.0.3
  * Fixed problem with large value in usr section of RFH2 if trace was enabled.
  * Fixed bug where pscr user id and some other fields were not being properly reset when new data was read.
  * Corrected the file source display on the data tab to reflect the length of the data after removal of any headers
  * Fixed bug where currentSelection was not being maintained when tabbing was using the accelerator keys.
  * Added accelerator keys for the display controls on the data tab.
  * Updated the help to display the accelerator keys for the controls on the data tab.
  * Fixed bug where user id field in PSCR was not being included when building an RFH V1.
  * Fixed bug where no blank was inserted before the other field when building an RFH V1.
  * Fixed bug where RFH headers were being created even if they existed due to passing charFormat rather than ccsid to buildRFH routines.
  * Fixed bug where buildCICSheader was not copying header to output area unless it was a prog type request.
  * Fixed bug where getRFH1Format was returning format field from RFH2 header, not RFH1 header
  * Corrected problem where CICS header was always being rebuild.
  * Fixed problem where task end status was not being properly parsed.
  * Fixed bug where CICS facility output was overwriting the next 8 bytes in the CICS header.
  * Fixed problem where backspace was not working in CEdit controls.
  * Fixed bug where accounting id was not being set when set all or set user id was selected.
  * Fixed bug where application name was not being set if application name was 28 characters long (less than rather than less than or equal comparison)
  * Enable setting of original length and sequence number fields when allowed by MQ.
  * Released 6/25/2007


* Build 196 (Created 5/21/2007) V5.0.4
  * Support for UCS-2 in RFH2 header folders.
  * Fixed bug where selection of JMS tab was causing update of RFH2 message domain due to spurious OnJMSText event from setting focus.
  * Fixed bug where IMS header was not being translated to EBCDIC properly resulting in MQRC 2148.
  * Fixed bug where the selections for the RFH2 folders on the RFH tab was not working.  They could not be selected.
  * Fixed bug where ignore RFH headers selection in menu was causing a crash.
  * Removed print menu option since it did not work anyway.
  * Fixed bug where non-XML data was causing a crash when the parsed button was selected on the data tab.
  * Fixed bug where read or write file was causing crash if the find or find hex dialogs had been left active.


* Build 197 (Created 7/17/2007) V5.0.4
  * Corrected rounding character for RFH other folder when encoding is big-endian.  Was coded as little-endian.
  * Fixed crash when reading file data and message exceeded 64 bytes.


* Build 198 (Created 7/24/2007) V5.0.4
  * Fixed bug where adding a CICS header after an RFH2 header was causing a crash.
  * Fixed bug where inserting a CICS header after another header was setting the wrong encoding and ccsid in the previous header.
  * Fixed bug where inserting an IMS header after another header was setting the wrong encoding and ccsid in the previous header.
  * Fixed bug where dynamic insertion of a dead letter header was not working (MQ reason code 2141).
  * Added support for application types 31 through 34 in drop down combo boxes on MQMD and DLQ pages.
  * Improved setting of MQMD ccsid and encoding when reading in file with MQ header but no MQMD in the file.
  * Remember and set the last used file path.
  * Set some basic file patterns in file open dialogs.


* Build 199 (Created 7/31/2007) V5.0.5
  * Released on download web site.


* Build 200 (Created 8/23/2007) V5.0.6
  * Fixed bug where MyComboBox class was jumping to end after an insert at beginning.
  * Added crude hack to main frame class to prevent unhandled exceptions when enter is pressed while drop down menu is displayed.
  * Fixed bug where the user id was being used as the password in QNames when doing an MQCONNX
  * Added code to clear previous MQ headers in parseMsgHeaders routine in DataArea.
  * Added code to process an MQMD extension header if present in an input message.
  * Changed MQMD parsing to recognize a V1 MQMD header and to ignore the new fields introduced in an MQMD V2.
  * Fixed bug where selection of request type on the PubSub tab was not setting the data changed variable.  Switching tabs lost the new queue name.
  * Fixed problem where broker qm on PubSub tab was not being propagated to the Remote QM name on the Main tab.


* Build 201 (Created 9/15/2007) V5.1
  * Removed use of C++ interface.  Now use the MQI throughout RFHUtil.  This was to support backwards compatiblity for the client version.
  * Fixed bug where auto HScroll was not selected on pub sub connection queue manager field.
  * Added audit file capability to log all activity to a file.
  * Corrected bug with ccsid not recognizing code page 37 as EBCDIC.  (changed comparison from 037 to 37 since 037 was interpreted as Octal)
  * fixed bug where building Usr folder causes exception if there was no equal sign found on a line.
  * Added support for specifying a security exit and user data to the connection parameters dialog.
  * Corrected MQMD Put Date/Time formatting problem - changed to use common routine.
  * Corrected setting of MQMD Put Date/Time - as using wrong length in memcpy.
  * Added support for local address in client connections.
  * Added Japanese EBCDIC code pages 290, 930 and 5026 to routine which looks for EBCDIC code pages.
  * Fixed bug where save messages without header was not handling big Endian encoding properly and was getting the wrong structure length.
  * Fixed bug where persistence setting in pub/sub for RFH1 on subscription message did not have the leading delimiter (space) resulting in rc=3082
  * Fixed problem where the Save Headers setting on the Read menu was not working.
  * Allow Sub Name and variable user fields to be used with request publication on pub/sub tab.
  * Fixed several problems with using individual files when saving a queue
  * Fixed problem where selecting a jms message domain did not automatically force the inclusion of an RFH2 header.
  * SaveMsgs will now honor get by correlid, group id or message id.
  * Fixed bug where hex delimiter was being rejected incorrectly in savemsgs dialog.


* Build 202 (Created 3/25/2008) V6.0
  * Ship version of RFHUtil V6.0


* Build 203 (Created 4/30/2008) V6.0.1
  * Fixed problem where remote queue was being counted as cluster queue in queue counts in trace in QNames.cpp.


* Build 204 (Created 6/28/2008) V7.0.0
  * Added support for User Properties, including new tab.
  * Added support for MQ publish and subscribe (V7) including new tab.
  * Added support for message selectors (MQ V7).
  * Changed so that the mqm dll is loaded directly rather than using the MQI so that backward compatibility can be achieved.
  * Fixed bug where IIH LLBB field was being treated as 32-bit rather than 16-bit integer.


* Build 205
  * Moved loading of queue names to DataArea object and removed QNames object from the project


* Build 206
  * Added discQM call to ExitInstance in application object
  * Changed syncpoint option to syncpoint if persistent option on MQGet
  * Added support for multiple XML levels for the usr folder
  * Added RFHUTIL_VERBOSE_TRACE environment variable for more verbose trace output
  * Fixed bug where xml and formatted displays were not working due to getTranslatedData not setting length for codepage 1208.
  * Moved PS tab after MQMD tab.
  * Fixed bug in ExtractProperties where was rejecting lengths > 4 for name or value (resulting in loop)
  * Added support for alter and set identity on V7 publish and subscribe tab.
  * Set change variables in RFH folder classes when backspace is pressed to ensure change is detected.
  * Fixed bug where set all option was not being refreshed when writing a message to a queue.


* Build 207
  * Wrote XML parser class to parse the usr and other folders as well as MQ user properties.
  * Added support for Thai characters
  * Use tab characters in both display to better align non-ASCII displays.
  * Added work arounds for some bugs in MQSUB.
  * Treat any usr data that begins with a bracket as XML and just add <usr> tags.
  * Fixed problem where publish message was not processing message properties
  * Set windows title when loading and saving messages.
  * Get queue and queue manager names when getting subscription names.


* Build 208
  * Released as V7.0.0 on 2/26/2009


* Build 209 (V7.0.1)
  * Changed build options routine in DataArea.cpp to check for z/OS queue manager before setting all segments GMO option
  * Fixed bug where V7 client was connecting to V6 queue manager and was setting properties in handle option.
  * Fixed bug where get by message id option was not being set in the dataarea, leaving the message id field as read only and unable to be set. (4/6/2009)
  * Added JSON parser module
  * Added support for JSON data formatting, including search
  * Added support for FIX data formatting, including search
  * Changed MQMD Format field to use a combo box with standard MQ formats pre-loaded.
  * Changed display of queue manager name when explicitly connecting using MQ menu to use name from MQINQ if the INQ worked.
  * Fixed bug (loop) in getTranslatedData with code page 1208 and invalid characters.  Not handling translate failures properly.
  * Fixed bug where usr area was not being rounded up to multiple of 4 if old style XML format was used.
  * Fixed bug where character displays in EBCDIC were not translated properly.
  * Added code to explicitly recognize a byte order mark for code page 1208 (BOM = 0xefbbbf) for XML displays
  * Added data translation routines for common code pages
  * Open File now associates MQMD ccsid and encoding values with file data if these fields are set first.
  * Fixed bug where user properties was not handled properly when performing a Load Q operation.
  * Fixed bug where MSGDATA was not setting the tab setting properly for both (char/hex) displays
  * Changed default for message properties to AS QUEUE
  * Added support for new no subs option in V7.0.1
  * Fixed bug where usr folder with nothing in it was generating a single equal sign
  * Changed display q to use a list control and added more fields to the display.
  * Released as V7.0.1 on 10/15/2009
  * Fixed bug where was not setting message id and correlation id correctly.
  * Fixed bug where workaround for code page for MQ 7.0 in subscribe and publish failed in MQ 7.0.1 to AIX (affected clients only).
  * Fixed bug where publication level was not being set on publish.


* Build 210 (V7.0.2)
  * Released


* Build 211 (V7.0.3)
  * Set character format automatically based on the file code page on the main tab when reading a file.
  * Fixed title bars in dialogs to include queue name and to appear for loadQ and saveQ dialogs.
  * Fixed bug where was not closing provided queue to MQSUB in the event of an error (such as 2551).  Was causing a handle leak.
  * Fixed problem with line selection being erratic in Display Q dialog.
  * Added environmental override for maximum wait for display Q
  * Added support for use of MQ CSP for longer user ids and passwords.
  * Changed about dialog to include compile time as well as compile date.
  * Fixed memory leak in queue manager NAMESTRUCT objects.  Was not inserting into chain.
  * Optimized performance of Names object (added length and 16 index fields).
  * Fixed bug in allocation of Names object string table.  Was not properly initializing.
  * Added append option to save messages dialog.
  * Added capture option to V7 publish and subscribe.
  * Fixed bug with SYNC clause in COBOL copy book.  Routine to propagate group SYNC clauses was looping.
  * Did more work on SYNC clause processing, including sync clauses within group variables.
  * Fixed bug where display of cluster queue names was causing exception (admin msg area was to small)
  * Added publication capture and replay capability to pub/sub
  * Added delay and batchsize parameters to load Q function.
  * Changed some internal routines to pass structures by reference


* Build 212 (V7.0.4)
  * Created 4/21/2010
  * Enhanced ability to capture published messages into a file, including a new dialog.
  * Enhanced ability to publish captured messages from a file, including a new dialog.
  * Added some individual fields to respond to edit requests (copy, paste, etc)
  * Added some tooltips for fields that were missing the tooltips
  * Fixed bug in loadmsgs where ignore set all was causing 2095 due to setting set all option on MQPUT but not on MQOPEN.


* Build 213 (V7.0.5)
  * Created 6/17/2010
  * Fixed bug where start browse from display messages was not setting previous message id correctly.
  * Fixed memory leak when Enter or Esc was pushed with display Q dialog open.
 8 Fixed bug where savemsgs was always reading messages even if do not remove was selected.
  * Fixed bug where drop down menu for queue names not working in MoveQ dialog.
  * Fixed bug where mqmd time was not being set when set all was specified.
  * Fixed bug in display queue where was using MQMD V1 and group id, etc were coming in as an MQMDE.
  * Fixed bug where last used queue manager name was changing even if no operations were performed.
  * Fixed bug where subscription was overwriting correlation id pointer in parameters structure
  * Fixed bug where CDATA with whitespace afterward was not working in parsed display.
  * Changed UCS character display to ignore Byte Order Marks
  * Fixed bug with detecting invalid characters in XML data stream


* Build 214 (V7.1)
  * Created 8/24/2010 and released as V7.1.


* Build 215 (V7.1.1)
  * Removed use of message handle in MoveQ (MoveMessages)
  * Added error message if MQ or MQ client is not found (e.g. not installed)
  * Fixed problem where loadmsgs was not recognizing if the malloc for the file data area failed
  * Fixed problem with load Q when saved file was greater than 1GB - the file is now read in chuncks of no more than 256 MB
  * Added version of RFHUtil to top of trace file


* Build 216 (V7.1.2)
  * Fixed bug saving file with MQMD - the version will be forced to V2 to match the length of the MQMD.
  * Added more descriptive error messages for file open errors when writing a file.
  * Fixed bug in LOADQ that was introduced in the fix for save file > 1GB.
  * Corrected problem with tab order on V7 pub/sub dialog.


* Build 217 (V7.1.3)
  * Fixed bug in XMLParse processAttrs routine that was starting with last child rather than first child to search for attributes
  * Fixed numerous small problems with XML parser that were primarily affecting the RFH2 other folders.
  * Added trace information when creating lists of queue managers.
  * Changed display of non-ASCII characters in copybook class (call to BuildHex).
  * Changed display of character fields to replace binary zeros with spaces


* Build 218 (V7.1.4)
  * Added support for MQ 7.1 coexistence with MQ 7.0.1
  * Added support for bind on group
  * Added support for no multicast
  * Fixed bug in processing of usr folder input
  * Changed encoding displays so that floating point and binary formats are distinct.
  * Added support for TNS floating point formats.
  * Fixed bug where was including length of program id in CICS header, which resulted in an extra 8 bytes of garbage for TRAN requests.
  * Fixed bug where was clearing first 4 bytes of IMS header rather than the LLBB field.
  * Fixed problem where Ctrl+V was not working in idEdit fields (msg id, correl id, group id).


* Build 218 (V7.1.4)
  * Added suppport for MQ close options
  * Added support for Russian character set


* Build 220 (V7.5)
  * Switched to V8
  * Fixed bug by adding CoInitialize and CoUnitialize() calls in main application thread
  * Delivered July, 2014


* Build 221 (V8.0)
  * Added MQAppliance as platform type


* Build 222 (V8.0 build 222)
  * Added new cipher suites per MQ 8.0 and TLS 1.2


* Build 223 (V8.0 build 223)
  * Changed connect to check for user id, password even if queue manager name provided (to support channel tables)
  * Fixed bug where copy book open was using just file name rather than fully qualified name including path


* Build 224 (V8.0 build 224)  December 10, 2015
  * Fixed problems with JSON parser when invalid numberic values were parsed


* Build 225 (V9.0 build 225) October 18, 2018
  * Updated to VS 2017, including fixing number of errors in sprintf in trace statements


* Build 226 (V9.0 build 226) November 5, 2018
  * Upgraded to VS 2017
  * Fix resolution problem on high res screens - need to scale certain fonts based on dpi of monitor


* Build 227 (V9.1 build 227) December 28 2018
  *  **First release to GitHub**
  * Update license/copyright information.
  * Update CipherSuites to remove SSL and TLS 1.0 ciphers
  * Fixed problem with channel tables not working
  * Changed format of channel table entries to preserve channel name and host name
  * Prevent duplicate QM entries in drop down list

* Build 228(V9.1.1 build 228) Feb 4 2019
  * Remove most of the warnings from VS 2017 code analysis 
  * Fix issue #2 - Passwords not sent when using TLS

* Build 229 (V9.1.2 build 229) Mar 25 2019
  * Fix issue #3 - XML parsing when usr folder attribute includes value with '/'