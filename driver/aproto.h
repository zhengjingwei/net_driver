#ifndef __APROTO_H
#define __APROTO_H


#include <net/sock.h>
#include <linux/scatterlist.h>
#include <linux/highmem.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/mii.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/ethtool.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/of.h>
#include <mach/board.h>
#include <mach/slcr.h>
#include <linux/interrupt.h>
#include <linux/clocksource.h>
#include <linux/timecompare.h>
#include <linux/net_tstamp.h>

#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/list.h>
#include "linux/fs.h"   
#include "linux/types.h"  
#include "linux/errno.h"  
#include "linux/uaccess.h"  
#include "linux/kdev_t.h" 

/* Debug code */
#define XEMACPS_DBG 0

#if XEMACPS_DBG
#define XEMACPS_PRINT(pri, pri_str, fmt, arg...) \
        printk(pri KBUILD_MODNAME "[%s] <%s:%d> " fmt, pri_str, __FUNCTION__, __LINE__, ##arg)
// #define xemacps_dbg(fmt, arg...) XEMACPS_PRINT(KERN_INFO, "DEBUG", fmt, ##arg)
#define xemacps_dbg(fmt, arg...) XEMACPS_PRINT(KERN_ERR, "DEBUG", fmt, ##arg)
#else
#define XEMACPS_PRINT(pri, pri_str, fmt, arg...) \
        printk(pri KBUILD_MODNAME "[%s] " fmt, pri_str, ##arg)
#define xemacps_dbg(fmt, arg...)
#endif

#define xemacps_info(fmt, arg...)     XEMACPS_PRINT(KERN_INFO, "INFO", fmt, ##arg)
#define xemacps_warn(fmt, arg...)     XEMACPS_PRINT(KERN_WARNING, "WARN", fmt, ##arg)
#define xemacps_err_sb(sb, s, args...)    xemacps_error_mng(sb, s, ## args)
#define xemacps_err(fmt, arg...)      XEMACPS_PRINT(KERN_ERR, "ERROR", fmt, ##arg)


#define AF_AS   21    //xchange original RDS socket
#define PF_AS   AF_AS

#define u8 unsigned char
#define u32 unsigned int

#define cmd_len 256
#define mac_len 16
#define block_len 4096
#define large_cmd_len 1024

struct buff
{
    u32 addr;
    u32 stat;
    struct list_head list;
};

struct bd2buff
{
    struct buff* buff;
};

struct asend_ops{

    int (*net_buff_send)(u32 paddr, u32 size, struct net_device *ndev);
    int (*xemacps_SetRxBD_onebd)(u32 paddr, u32 numbd, struct net_device *ndev);
    int (*xemacps_ResetRxBD)(struct net_device *ndev);
    int (*xemacps_SetRxBD)(struct net_device *ndev);
    int (*cmd_parse)(u32 paddr, u32 size, u32 bdidx);
    int (*data_recv)(u32 paddr, u32 size, u32 bdidx);
};

struct ring_info {
    struct sk_buff *skb;
    dma_addr_t mapping;
    size_t len;
};

/* DMA buffer descriptor structure. Each BD is two words */
struct xemacps_bd {
    u32 addr;
    u32 ctrl;
};

/* Our private device data. */
struct net_local {
    void __iomem *baseaddr;

    void *s_fs_info;
    void *super_block;
    struct asend_ops asend;
    u32 recv_addr;
    u32 recv_size;
    u32 id;
    u8 *mac;
    u32 mac_mapping;

    struct buff* buff_head;
    struct list_head *pos;

    u32 rx_package;
    wait_queue_head_t rev_irp;

    unsigned int           slcr_div0_1000Mbps;
    unsigned int           slcr_div1_1000Mbps;
    unsigned int           slcr_div0_100Mbps;
    unsigned int           slcr_div1_100Mbps;
    unsigned int           slcr_div0_10Mbps;
    unsigned int           slcr_div1_10Mbps;

    struct device_node *phy_node;
    struct device_node *gmii2rgmii_phy_node;
    struct ring_info *tx_skb;
    struct ring_info *rx_skb;

    struct xemacps_bd *rx_bd;
    struct xemacps_bd *tx_bd;

    dma_addr_t rx_bd_dma; /* physical address */
    dma_addr_t tx_bd_dma; /* physical address */

    u32 tx_bd_ci;
    u32 tx_bd_tail;
    u32 rx_bd_ci;
    u32 rx_bd_tail;

    u32 tx_bd_freecnt;
    u32 rx_bd_freecnt;

    spinlock_t tx_lock;
    spinlock_t rx_lock;
    spinlock_t nwctrlreg_lock;
    spinlock_t mdio_lock;

    struct platform_device *pdev;
    struct net_device *ndev; /* this device */
    struct tasklet_struct tx_bdreclaim_tasklet;
    struct workqueue_struct *txtimeout_handler_wq;
    struct work_struct txtimeout_reinit;

    struct napi_struct napi; /* napi information for device */
    struct net_device_stats stats; /* Statistics for this device */

    struct timer_list gen_purpose_timer; /* Used for stats update */

    struct mii_bus *mii_bus;
    struct phy_device *phy_dev;
    struct phy_device *gmii2rgmii_phy_dev;
    phy_interface_t phy_interface;
    unsigned int link;
    unsigned int speed;
    unsigned int duplex;
    /* RX ip/tcp/udp checksum */
    unsigned ip_summed;
    unsigned int enetnum;
    unsigned int lastrxfrmscntr;
    unsigned int has_mdio;
    bool timerready;
#ifdef CONFIG_XILINX_PS_EMAC_HWTSTAMP
    struct hwtstamp_config hwtstamp_config;
    struct ptp_clock *ptp_clock;
    struct ptp_clock_info ptp_caps;
    spinlock_t tmreg_lock;
    int phc_index;
    unsigned int tmr_add;
    struct cyclecounter cc;
    struct timecounter tc;
    struct timer_list time_keep;
#endif
};

static inline struct net_device* get_eth0(void)
{
    
    struct net_device *ndev;
    char *name = "eth0";
    ndev = __dev_get_by_name(&init_net,name);
    return ndev;
}

static inline int u8_to_u32(u8* caddr, u32* paddr )
{
    *paddr = (u32)caddr[3];
    *paddr = (*paddr) + ((u32)caddr[2] << 8 );
    *paddr = (*paddr) + ((u32)caddr[1] << 16);
    *paddr = (*paddr) + ((u32)caddr[0] << 24);
    return 0;
}

#endif

