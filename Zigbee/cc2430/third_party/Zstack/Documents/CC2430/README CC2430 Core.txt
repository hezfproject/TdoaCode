
Texas Instruments, Inc.

Z-Stack CC2430DB/EM Release Notes

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

ZigBee 2006 Release
Version 1.4.3
December 21, 2007


Notices:

 - This release has been ZigBee certified.

 - The library files have been built and tested with EW8051 version 7.30B/W32 
   (7.30.2.3) and will not work with previous versions of the IAR tools.

 - When programming a target for the first time with this release, make sure
   that you check the "Erase Flash" box in the "Debugger->Chipcon->Download"
   tab in the project options. When programming completes, it is recommended
   that the "Erase Flash" box gets un-checked so that NV items are retained
   during later re-programming.

 - Please review the document "Upgrading To Z-Stack v1.4.3 For The CC2430"
   for information about moving existing v1.4.2 applications to v1.4.3.


Changes:

 - Support for CC2430 Rev-B devices has been removed. Attempts to run Z-Stack
   on Rev-C or earlier devices will ASSERT (see macMcuInit() in mac_mcu.c).

 - Upgrading to EW8051 v7.30B results in a significant code size reduction,
   as compared to version 1.4.2 of Z-Stack. Optimization of banked function
   calls produces most of this code reduction - a new function, ?BDISPATCH,
   was added to the chipcon_banked_code_support.s51 file to perform shared,
   indirect banked function calls and BANK_RELAYS (code segment) was added
   to the linker script (f8w2430.xcl).

 - The ZDO layer was modified to include a generic interface that delivers
   ZDO service discovery information to an application without changing code
   in ZDO modules (ZDApp.c, ZDObject.c, and ZDProfile.c). This reduces code
   size of the ZDO module by only including parsing functions when needed. 
   Reference the ZStack Developer's Guide (section 6) and the ZStack API 
   documents.

 - The OSAL tasks table is changed from a linked list that was populated by
   an application initialization module to a constant table. Significant
   code size reduction is achieved by eliminating the linked list handling
   function in OSAL_Tasks.c. See OSAL_SampleApp.c for an example task table.

 - Support for backward compatibility to the ZigBee 2004 protocol has been
   removed. Elimination of this obsolete feature results in some code size
   reduction and reduced software complexity.

 - Support for Coordinator Binding has been removed since it is not part of
   the ZigBee 2006 specification. Elimination of this legacy feature results
   in code size reduction and reduced software complexity.

 - Device startup time after reset has been greatly reduced. Changes were
   introduced with ZStack v1.4.2 to increase reliability of the Non-Volatile
   (NV) memory system, resulting in lengthy start-up times, when the NV_INIT
   and NV_RESTORE features were used. The OSAL_Nv.c and AccessCodeSpace.s51
   files have been modified to support block transfers for NV operations.

 - The use of DMA for non-volatile memory functions (NV) has been eliminated,
   resulting in reduced code size and significantly faster operation. The use
   of DMA for security functions (AES and CCM) is now optional, defaulting to
   “not used” to minimize code size. Enabling DMA for security uses more code
   space to provide slightly higher throughput.

 - Forced all messages going through AF_DataRequest() to DiscoverRoute = TRUE.


Bug Fixes:

 - Fixed problem in the low-level MAC where the FIFOP interrupt glitches when
   two beacons overlap. This problem has only been observed in beacon-enabled
   networks.

 - Fixed ZDO Managment Response memory leaks for ZDO Mgmt Network Discovery
   Response, ZDO Mgmt LQI Response, ZDO Mgmt Routing Response, and ZDO Mgmt
   Bind Response.

 - Fixed the ZDO Active Endpoint Response that used to report NOT_SUPPORTED
   for devices with 4 endpoints (3 or 5 worked).

 - When a Coordinator starts a PAN and detects a duplicate existing PAN ID,
   it now calls ZDApp_CoordStartPANIDConflictCB() [in ZDApp.c] to assign a
   different PAN ID. 

 - Monitor and Test (MT) now supports the AF interface properly. Previous
   releases did not support the AF interface through MT.  

 - Added a rejoin timeout to end devices so they don't get stuck in the 
   rejoin state when it doesn't get a rejoin response. 

 - Fixed a security bug encrypting data blocks sizes multiples of 16.


Known Issues:

 - The ZDO Complex Descriptor is not supported.

 - To disable security, use the "SECURE=0" compile option. Do not attempt to
   disable security by setting the SECURITY_LEVEL to zero.

 - To test power-saving with a Rev-E CC2430EM and SmartRF04 board, R104 should
   be removed from the bottom of the SmartRF04 board. Note that this disables
   the microphone input.

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------

For technical support please contact:

Texas Instruments, Inc.
Low Power Wireless
1455 Frazee Rd, Suite 800
San Diego, CA 92108 USA
+1 (619) 497-3845
lpwsupport@ti.com
