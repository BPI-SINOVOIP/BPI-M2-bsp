//#define SUNXI_MMCDBG

#ifdef SUNXI_MMCDBG
#define MMCINFO(fmt...)	printf("[mmc]: "fmt)//err and info
#define MMCDBG(fmt...)	printf("[mmc]: "fmt)//dbg
#define MMCMSG(fmt...)	printf(fmt)//data or register and so on
#else
#define MMCINFO(fmt...)	printf("[mmc]: "fmt)
#define MMCDBG(fmt...)
#define MMCMSG(fmt...)	
#endif


#define DRIVER_VER  "2014-01-26 13:55"
