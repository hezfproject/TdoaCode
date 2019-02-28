#ifndef __RTTHREAD_CFG_H__
#define __RTTHREAD_CFG_H__

#define PROTECT_CODE

#define __STM32__
// <RDTConfigurator URL="http://www.rt-thread.com/eclipse">

// <integer name="RT_NAME_MAX" description="Maximal size of kernel object name length" default="6" />
#define RT_NAME_MAX	8
// <integer name="RT_ALIGN_SIZE" description="Alignment size for CPU architecture data access" default="4" />
#define RT_ALIGN_SIZE	4
// <integer name="RT_THREAD_PRIORITY_MAX" description="Maximal level of thread priority" default="2">
// <item description="8">8</item>
// <item description="32">32</item>
// <item description="256">256</item>
// </integer>
#define RT_THREAD_PRIORITY_MAX	32
// <integer name="RT_TICK_PER_SECOND" description="OS tick per second" default="100" />
#define RT_TICK_PER_SECOND	1000
// <integer name="IDLE_THREAD_STACK_SIZE" description="The stack size of idle thread" default="512" />
#define IDLE_THREAD_STACK_SIZE	512
// <bool name="RT_USING_MODULE" description="Using Application Module" default="true" />
#define xRT_USING_MODULE
// <section name="RT_DEBUG" description="Kernel Debug Configuration" default="true" >
#define RT_DEBUG
// <bool name="RT_THREAD_DEBUG" description="Thread debug enable" default="false" />
// #define RT_THREAD_DEBUG
// <bool name="RT_USING_OVERFLOW_CHECK" description="Thread stack over flow detect" default="true" />
#define RT_USING_OVERFLOW_CHECK
// </section>

// <bool name="RT_USING_HOOK" description="Using hook functions" default="true" />
#define RT_USING_HOOK
// <section name="IPC" description="Inter-Thread communication" default="always" >
// <bool name="RT_USING_SEMAPHORE" description="Using semaphore in the system" default="true" />
#define RT_USING_SEMAPHORE
// <bool name="RT_USING_MUTEX" description="Using mutex in the system" default="true" />
#define RT_USING_MUTEX
// <bool name="RT_USING_EVENT" description="Using event group in the system" default="true" />
#define RT_USING_EVENT
// <bool name="RT_USING_MAILBOX" description="Using mailbox in the system" default="true" />
#define RT_USING_MAILBOX
// <bool name="RT_USING_MESSAGEQUEUE" description="Using message queue in the system" default="true" />
#define RT_USING_MESSAGEQUEUE
// </section>

// <section name="MM" description="Memory Management" default="always" >
// <bool name="RT_USING_MEMPOOL" description="Using Memory Pool Management in the system" default="true" />
#define RT_USING_MEMPOOL
// <bool name="RT_USING_HEAP" description="Using Dynamic Heap Management in the system" default="true" />
#define RT_USING_HEAP
// <bool name="RT_USING_SMALL_MEM" description="Optimizing for small memory" default="false" />
#define RT_USING_SMALL_MEM
// <bool name="RT_USING_SLAB" description="Using SLAB memory management for large memory" default="false" />
// #define RT_USING_SLAB
// </section>

// <section name="RT_USING_DEVICE" description="Using Device Driver Framework" default="true" >
#define RT_USING_DEVICE
// <bool name=RT_USING_DEVICE_IPC description="Using IPC in Device Driver Framework" default="true" />
#define RT_USING_DEVICE_IPC
// <bool name="RT_USING_SERIAL" description="Using Serial Device Driver Framework" default="true" />
#define RT_USING_SERIAL
// <bool name="RT_USING_SPI" description="Using SPI Device Driver Framework" default="true" />
#define RT_USING_SPI
// <integer name="RT_UART_BUFFER_SIZE" description="The buffer size for UART reception" default="64" />
#define RT_UART_BUFFER_SIZE	1024
// </section>

// <section name="RT_USING_USB_HOST" description="USB Host stack" default="true" >
#define xRT_USING_USB_HOST
// <bool name="RT_USB_CLASS_MASS_STORAGE" description="Using USB mass storage class" default="true" />
//#define RT_USB_CLASS_MASS_STORAGE
// <bool name="RT_USB_CLASS_ADK" description="Using ADK class" default="true" />
#define xRT_USB_CLASS_ADK
// <string name="UDISK_MOUNTPOINT" description="udisk mount point" default="/" />
//#define UDISK_MOUNTPOINT	"/sd"
// </section>

// <section name="RT_USING_CONSOLE" description="Using console" default="true" >
#define RT_USING_CONSOLE
// <integer name="RT_CONSOLEBUF_SIZE" description="The buffer size for console output" default="128" />
#define RT_CONSOLEBUF_SIZE	128
// <string name="RT_CONSOLE_DEVICE_NAME" description="console device name" default="uart3" />
#define RT_CONSOLE_DEVICE_NAME	"uart3"
// </section>

// <section name="RT_USING_COMPONENTS_INIT" description="Using components init" default="true" >
#define RT_USING_COMPONENTS_INIT
// </section>

// <section name="RT_USING_FINSH" description="Using finsh as shell, which is a C-Express shell" default="true" >
#define xRT_USING_FINSH
// <bool name="FINSH_USING_SYMTAB" description="Using symbol table in finsh shell" default="true" />
#define xFINSH_USING_SYMTAB
// <bool name="FINSH_USING_DESCRIPTION" description="Keeping description in symbol table" default="true" />
#define xFINSH_USING_DESCRIPTION
// <integer name="FINSH_THREAD_STACK_SIZE" description="The stack size for finsh thread" default="4096" />
#define FINSH_THREAD_STACK_SIZE	4096
// </section>

// <section name="LIBC" description="C Runtime library setting" default="always" >
// <bool name="RT_USING_NEWLIB" description="Using newlib library, only available under GNU GCC" default="true" />
// #define RT_USING_NEWLIB
// <bool name="RT_USING_PTHREADS" description="Using POSIX threads library" default="true" />
#define xRT_USING_PTHREADS
// </section>

#define RT_USING_RTC

// <section name="RT_USING_DFS" description="Device file system" default="true" >
#define RT_USING_DFS
// <bool name="DFS_USING_WORKDIR" description="Using working directory" default="true" />
#define DFS_USING_WORKDIR
#define xDFS_DEBUG

// <integer name="DFS_FILESYSTEMS_MAX" description="The maximal number of mounted file system" default="4" />
#define DFS_FILESYSTEMS_MAX	2
// <integer name="DFS_FD_MAX" description="The maximal number of opened files" default="4" />
#define DFS_FD_MAX	4
// <bool name="RT_USING_DFS_ELMFAT" description="Using ELM FatFs" default="true" />
#define RT_USING_DFS_ELMFAT
/* Reentrancy (thread safe) of the FatFs module.  */
#define RT_DFS_ELM_REENTRANT
// <integer name="RT_DFS_ELM_USE_LFN" description="Support long file name" default="0">
// <item description="LFN1">1</item>
// <item description="LFN1">2</item>
// </integer>
//#define RT_DFS_ELM_USE_LFN	1
// <integer name="RT_DFS_ELM_MAX_LFN" description="Maximal size of file name length" default="256" />
#define RT_DFS_ELM_MAX_LFN	256
// <integer name="RT_DFS_ELM_MAX_SECTOR_SIZE" description="Maximal size of sector" default="512" />
#define RT_DFS_ELM_MAX_SECTOR_SIZE  4096
// <bool name="RT_USING_DFS_YAFFS2" description="Using YAFFS2" default="false" />
// #define RT_USING_DFS_YAFFS2
// <bool name="RT_USING_DFS_UFFS" description="Using UFFS" default="false" />
// #define RT_USING_DFS_UFFS
// <bool name="RT_USING_DFS_DEVFS" description="Using devfs for device objects" default="true" />
// #define RT_USING_DFS_DEVFS
// <bool name="RT_USING_DFS_ROMFS" description="Using ROMFS" default="false" />
//#define RT_USING_DFS_ROMFS
// </section>

/* SECTION: lwip, a lighwight TCP/IP protocol stack */
#define RT_USING_LWIP
/* LwIP uses RT-Thread Memory Management */
#define RT_LWIP_USING_RT_MEM
/* Enable ICMP protocol*/
#define RT_LWIP_ICMP
/* Enable UDP protocol*/
#define RT_LWIP_UDP
/* Enable TCP protocol*/
#define RT_LWIP_TCP
/* Enable DNS */
#define RT_LWIP_DNS
/* Enable DHCP */
#define RT_LWIP_DHCP

/* the number of simulatenously active TCP connections*/
#define RT_LWIP_TCP_PCB_NUM	5

/* ip address of target*/
#define RT_LWIP_IPADDR0	192
#define RT_LWIP_IPADDR1	168
#define RT_LWIP_IPADDR2	8
#define RT_LWIP_IPADDR3	30

/* gateway address of target*/
#define RT_LWIP_GWADDR0	192
#define RT_LWIP_GWADDR1	168
#define RT_LWIP_GWADDR2	8
#define RT_LWIP_GWADDR3	1

/* mask address of target*/
#define RT_LWIP_MSKADDR0	255
#define RT_LWIP_MSKADDR1	255
#define RT_LWIP_MSKADDR2	255
#define RT_LWIP_MSKADDR3	0

/* tcp thread options */
#define RT_LWIP_TCPTHREAD_PRIORITY		12
#define RT_LWIP_TCPTHREAD_MBOX_SIZE		4
#define RT_LWIP_TCPTHREAD_STACKSIZE		1024

/* ethernet if thread options */
#define RT_LWIP_ETHTHREAD_PRIORITY		15
#define RT_LWIP_ETHTHREAD_MBOX_SIZE		4
#define RT_LWIP_ETHTHREAD_STACKSIZE		512

/* TCP sender buffer space */
#define RT_LWIP_TCP_SND_BUF	8192
/* TCP receive window. */
#define RT_LWIP_TCP_WND		8192

#define CHECKSUM_CHECK_TCP              0
#define CHECKSUM_CHECK_IP               0
#define CHECKSUM_CHECK_UDP              0

#define CHECKSUM_GEN_TCP                0
#define CHECKSUM_GEN_IP                 0
#define CHECKSUM_GEN_UDP                0

// </RDTConfigurator>

// #define SHOW_RT_VERSION
#define ENABLE_IWDG
#define CONFIG_IWDG_HW
#define ENABLE_HW_WATCHDOG

/* SUPPORT_BIT_ERROR_RATE_TEST为误码率测试宏开关，仅用在安标版本 */
//#define SUPPORT_BIT_ERROR_RATE_TEST

#endif
