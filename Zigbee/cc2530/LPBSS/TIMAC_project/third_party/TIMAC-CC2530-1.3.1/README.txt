Texas Instruments, Inc.

TIMAC-CC2530 Release Notes

---------------------------------------------------------------------------------------
---------------------------------------------------------------------------------------

Version 1.3.1
December 13, 2010


Notices:
 - TIMAC-CC2530 has been certified for 802.15.4 compliance.

 - The library files have been built and tested with EW8051 version 7.60.5
   (7.60.5.40066) and may not work with previous versions of the IAR tools.
   You can obtain the 7.60 installer and patches from the IAR website.

 - TIMAC has been built and tested with IAR's CLIB library, which provides a
   light-weight C library which does not support Embedded C++. Use of DLIB
   is not recommended since TIMAC is not tested with that library.

 - Support for the new CC2533 has been added to this release.


Changes:
 - Modified all IAR library and sample application project files to work with the
   EW8051-7.60 toolchain. Optimization has been set to "high, size" with the
   "Code Motion" option disabled. [3444]

 - Added capability to control the "OSAL task processing loop" from an external
   process. A new function, osal_run_system(), runs one pass of the OSAL task
   processor, being called from the forever loop in osal_start_system(). [3437]

 - Enhanced beacon mode operation to permit end-devices to send data during
   the Outgoing CAP. [3382]

 - The OSAL_Timer API has been extended to add an automatically reloading
   timer. In addition to simplifying the use of periodic timers, this feature also
   can be used to eliminate problems with failing to obtain a timer when heap
   resources are depleted (during heavy traffic). Section 5.3 of the "OSAL API"
   document gives details on use of "osal_start_reload_timer()". [3070]

 - The default IDE setting for "Virtual Registers" has been increased from 8
   to 16. The results in a typical CODE size reduction of 1.5% at the expense
   of 8 bytes of DATA memory. [3068]

 - The OSAL_MSG API has been extended to add capability to search for an
   existing OSAL message in the queue. See section 3.6 of the "OSAL API"
   document for details on use of "osal_msg_find()". [3065]

 - Modified the osal_msg_receive() function to check for additional messages
   to process and set the task's event flag. Under very heavy traffic situations,
   this improves event processing of other tasks. [2947]

 - Added new "MAC_NO_ACK" error return for association response messages.
   This indicates that the MAC transmitted the association response frame but
   did not get an ACK from the child device. [2915]

 - Added return values for MAC_MlmeAssociateRsp() and macSendDataMsg()
   functions. This allows callers to determine whether the function call failed
   due to resource allocation problems. [2914]

 - Added "PREFETCH_ENABLE()" and "PREFETCH_DISABLE()" macros to the
   hal_board_cfg.h file. Cache pre-fetch enabled in HAL_BOARD_INIT(). [2560]

 - Power consumption during CSMA is reduced. The "RX-on" time was reduced
   by one backoff period, except when the requested backoff time is zero. [1601]


Bug Fixes:
 - Modified the default value of MAC_RADIO_RECEIVER_SENSITIVITY_DBM, to
   match the latest CC2530 data sheet (from -91 to -97). This will improve the
   quality of the calculated LQI. [3488]

 - Fixed a problem where sleep time was programmed with an incorrect long
   duration instead of the expected short duration. Now, the device will not
   enter IDLE sleep if the sleep request is less than 4 x 320us. [3478]

 - Fixed a problem in beacon mode where RX might be turned off during the
   outgoing CAP when MAC_RX_ON_WHEN_IDLE was set to FALSE. [3461]

 - Fixed an obscure problem where the MAC could calculate an incorrect time
   if macMcuPrecisionCount() got interrupted in a small time window. [3423]

 - Fixed a race condition between a device entering sleep mode and handling
   an OSAL event posted by an interrupt - the event would not get processed
   until the device woke from sleep mode. [3372]

 - The non-volatile memory driver (OSAL_Nv.c) has been upgraded to close
   vulnerability to corruption of NV memory if a device reset occured during
   compaction to a "clean" NV page. Device reset can occur by cycling power,
   voltage drops below brown-out threshold, or program assert. [3267]

 - Fixed a problem which produced random, incorrect delays in beacon mode
   coordinator when the device is sleeping/waking often. [3258]

 - Fixed a rare problem where the device would not enter sleep mode - raised
   the T2 interrupt priority to match the RF interrupt priority. [3236]

 - The "mac_cfg.c" file was moved out of the MAC library in order to permit
   user configuration of queue size parameters (MAC_CFG_TX_DATA_MAX,
   MAC_CFG_TX_MAX, MAC_CFG_RX_DATA_MAX) when tuning is needed
   for larger networks. Overrides for these parameters were also added to
   the ZStack ZigBee device configuration files (f8wCoord.cfg, f8wRouter.cfg,
   f8wEndev.cfg) to increase MAC frame buffers for larger networks. [3212]

 - The "mac_pib.c" file was moved out of the MAC library in order to permit
    compile-time use of the HAL_PA_LNA option. The MAC library previously
    was built with this option disabled. [3210]

 - Fixed two bounds checks in the non-volatile (NV) memory driver that could
   cause corrupted data at the end of an NV page and possibly fail to perform
   a page compaction when necessary. [3188]

 - Fixed a problem in which the MAC could drop valid beacons during an
   active/passive scan process. This could occasionally result in not using the
   "best" beacon because it never made it to the selection algorithm. [3124]

 - Closed a small "window" after waking up from sleep where the MAC timer
   interrupt could be missed, causing a scan timeout to not be triggered. [3001]

 - Corrected a problem in processing pending messages for polling end-devices
   when the number end-devices with pending messages exceeds 16. [2977]

 - Reworked buffer management for the MAC to prevent a rare "double free"
   scenario that could happen under heavy traffic conditions. This included a
   change to OSAL_Memory.c to enable ASSERTS in osal_mem_free(). [2896]

 - Fixed a compile error in the OSAL_Memory.c file when using the compile
   option "OSALMEM_PROFILER". [2846]

 - Fixed a build error when using the compile options "OSALMEM_PROFILER"
   and "OSAL_TOTAL_MEM" to compute the stack "high-water-mark" . [1679]


Known Issues:
- Device may not track beacon if beacon payload is too long. [1441]

- Battery life extension for beacon-enabled networks is not supported.

- 802.15.4 security is not supported.

- GTS is not supported.

-------------------------------------------------------------------------------
-------------------------------------------------------------------------------
