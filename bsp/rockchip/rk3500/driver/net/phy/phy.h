/*
 * Copyright (c) 2013-2022, Shanghai Real-Thread Electronic Technology Co.,Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-6-2     songchao     the first version
 */

#ifndef _PHY_H
#define _PHY_H

#include <mdio.h>
#include <rtthread.h>
#include "../net.h"

#ifdef RT_USING_LWIP
#include <netif/ethernetif.h>
#endif

/* Generic MII registers. */
#define MII_BMCR        0x00    /* Basic mode control register */
#define MII_BMSR        0x01    /* Basic mode status register  */
#define MII_PHYSID1        0x02    /* PHYS ID 1                   */
#define MII_PHYSID2        0x03    /* PHYS ID 2                   */
#define MII_ADVERTISE        0x04    /* Advertisement control reg   */
#define MII_LPA            0x05    /* Link partner ability reg    */
#define MII_EXPANSION        0x06    /* Expansion register          */
#define MII_CTRL1000        0x09    /* 1000BASE-T control          */
#define MII_STAT1000        0x0a    /* 1000BASE-T status           */
#define MII_MMD_CTRL        0x0d    /* MMD Access Control Register */
#define MII_MMD_DATA        0x0e    /* MMD Access Data Register */
#define MII_ESTATUS        0x0f    /* Extended Status             */
#define MII_DCOUNTER        0x12    /* Disconnect counter          */
#define MII_FCSCOUNTER        0x13    /* False carrier counter       */
#define MII_NWAYTEST        0x14    /* N-way auto-neg test reg     */
#define MII_RERRCOUNTER        0x15    /* Receive error counter       */
#define MII_SREVISION        0x16    /* Silicon revision            */
#define MII_RESV1        0x17    /* Reserved...                 */
#define MII_LBRERROR        0x18    /* Lpback, rx, bypass error    */
#define MII_PHYADDR        0x19    /* PHY address                 */
#define MII_RESV2        0x1a    /* Reserved...                 */
#define MII_TPISTATUS        0x1b    /* TPI status for 10mbps       */
#define MII_NCONFIG        0x1c    /* Network interface config    */

/* Basic mode control register. */
#define BMCR_RESV        0x003f    /* Unused...                   */
#define BMCR_SPEED1000        0x0040    /* MSB of Speed (1000)         */
#define BMCR_CTST        0x0080    /* Collision test              */
#define BMCR_FULLDPLX        0x0100    /* Full duplex                 */
#define BMCR_ANRESTART        0x0200    /* Auto negotiation restart    */
#define BMCR_ISOLATE        0x0400    /* Isolate data paths from MII */
#define BMCR_PDOWN        0x0800    /* Enable low power state      */
#define BMCR_ANENABLE        0x1000    /* Enable auto negotiation     */
#define BMCR_SPEED100        0x2000    /* Select 100Mbps              */
#define BMCR_LOOPBACK        0x4000    /* TXD loopback bits           */
#define BMCR_RESET        0x8000    /* Reset to default state      */
#define BMCR_SPEED10        0x0000    /* Select 10Mbps               */

/* Basic mode status register. */
#define BMSR_ERCAP        0x0001    /* Ext-reg capability          */
#define BMSR_JCD        0x0002    /* Jabber detected             */
#define BMSR_LSTATUS        0x0004    /* Link status                 */
#define BMSR_ANEGCAPABLE    0x0008    /* Able to do auto-negotiation */
#define BMSR_RFAULT        0x0010    /* Remote fault detected       */
#define BMSR_ANEGCOMPLETE    0x0020    /* Auto-negotiation complete   */
#define BMSR_RESV        0x00c0    /* Unused...                   */
#define BMSR_ESTATEN        0x0100    /* Extended Status in R15      */
#define BMSR_100HALF2        0x0200    /* Can do 100BASE-T2 HDX       */
#define BMSR_100FULL2        0x0400    /* Can do 100BASE-T2 FDX       */
#define BMSR_10HALF        0x0800    /* Can do 10mbps, half-duplex  */
#define BMSR_10FULL        0x1000    /* Can do 10mbps, full-duplex  */
#define BMSR_100HALF        0x2000    /* Can do 100mbps, half-duplex */
#define BMSR_100FULL        0x4000    /* Can do 100mbps, full-duplex */
#define BMSR_100BASE4        0x8000    /* Can do 100mbps, 4k packets  */

/* Advertisement control register. */
#define ADVERTISE_SLCT        0x001f    /* Selector bits               */
#define ADVERTISE_CSMA        0x0001    /* Only selector supported     */
#define ADVERTISE_10HALF    0x0020    /* Try for 10mbps half-duplex  */
#define ADVERTISE_1000XFULL    0x0020    /* Try for 1000BASE-X full-duplex */
#define ADVERTISE_10FULL    0x0040    /* Try for 10mbps full-duplex  */
#define ADVERTISE_1000XHALF    0x0040    /* Try for 1000BASE-X half-duplex */
#define ADVERTISE_100HALF    0x0080    /* Try for 100mbps half-duplex */
#define ADVERTISE_1000XPAUSE    0x0080    /* Try for 1000BASE-X pause    */
#define ADVERTISE_100FULL    0x0100    /* Try for 100mbps full-duplex */
#define ADVERTISE_1000XPSE_ASYM    0x0100    /* Try for 1000BASE-X asym pause */
#define ADVERTISE_100BASE4    0x0200    /* Try for 100mbps 4k packets  */
#define ADVERTISE_PAUSE_CAP    0x0400    /* Try for pause               */
#define ADVERTISE_PAUSE_ASYM    0x0800    /* Try for asymetric pause     */
#define ADVERTISE_RESV        0x1000    /* Unused...                   */
#define ADVERTISE_RFAULT    0x2000    /* Say we can detect faults    */
#define ADVERTISE_LPACK        0x4000    /* Ack link partners response  */
#define ADVERTISE_NPAGE        0x8000    /* Next page bit               */

#define ADVERTISE_FULL        (ADVERTISE_100FULL | ADVERTISE_10FULL | ADVERTISE_CSMA)
#define ADVERTISE_ALL        (ADVERTISE_10HALF | ADVERTISE_10FULL | ADVERTISE_100HALF | ADVERTISE_100FULL)

/* Link partner ability register. */
#define LPA_SLCT        0x001f    /* Same as advertise selector  */
#define LPA_10HALF        0x0020    /* Can do 10mbps half-duplex   */
#define LPA_1000XFULL        0x0020    /* Can do 1000BASE-X full-duplex */
#define LPA_10FULL        0x0040    /* Can do 10mbps full-duplex   */
#define LPA_1000XHALF        0x0040    /* Can do 1000BASE-X half-duplex */
#define LPA_100HALF        0x0080    /* Can do 100mbps half-duplex  */
#define LPA_1000XPAUSE        0x0080    /* Can do 1000BASE-X pause     */
#define LPA_100FULL        0x0100    /* Can do 100mbps full-duplex  */
#define LPA_1000XPAUSE_ASYM    0x0100    /* Can do 1000BASE-X pause asym*/
#define LPA_100BASE4        0x0200    /* Can do 100mbps 4k packets   */
#define LPA_PAUSE_CAP        0x0400    /* Can pause                   */
#define LPA_PAUSE_ASYM        0x0800    /* Can pause asymetrically     */
#define LPA_RESV        0x1000    /* Unused...                   */
#define LPA_RFAULT        0x2000    /* Link partner faulted        */
#define LPA_LPACK        0x4000    /* Link partner acked us       */
#define LPA_NPAGE        0x8000    /* Next page bit               */

#define LPA_DUPLEX        (LPA_10FULL | LPA_100FULL)
#define LPA_100            (LPA_100FULL | LPA_100HALF | LPA_100BASE4)

#define ESTATUS_1000_XFULL    0x8000    /* Can do 1000BX Full */
#define ESTATUS_1000_XHALF    0x4000    /* Can do 1000BX Half */
#define ESTATUS_1000_TFULL    0x2000    /* Can do 1000BT Full          */
#define ESTATUS_1000_THALF    0x1000    /* Can do 1000BT Half          */

/* N-way test register. */
#define NWAYTEST_RESV1        0x00ff    /* Unused...                   */
#define NWAYTEST_LOOPBACK    0x0100    /* Enable loopback for N-way   */
#define NWAYTEST_RESV2        0xfe00    /* Unused...                   */

/* 1000BASE-T Control register */
#define ADVERTISE_1000FULL    0x0200  /* Advertise 1000BASE-T full duplex */
#define ADVERTISE_1000HALF    0x0100  /* Advertise 1000BASE-T half duplex */
#define CTL1000_AS_MASTER    0x0800
#define CTL1000_ENABLE_MASTER    0x1000

/* Flow control flags */
#define FLOW_CTRL_TX        0x01
#define FLOW_CTRL_RX        0x02

/* MMD Access Control register fields */
#define MII_MMD_CTRL_DEVAD_MASK    0x1f    /* Mask MMD DEVAD*/
#define MII_MMD_CTRL_ADDR    0x0000    /* Address */
#define MII_MMD_CTRL_NOINCR    0x4000    /* no post increment */
#define MII_MMD_CTRL_INCR_RDWT    0x8000    /* post increment on reads & writes */
#define MII_MMD_CTRL_INCR_ON_WT    0xC000    /* post increment on writes only */

/* Indicates what features are supported by the interface. */
#define SUPPORTED_10baseT_Half        (1 << 0)
#define SUPPORTED_10baseT_Full        (1 << 1)
#define SUPPORTED_100baseT_Half        (1 << 2)
#define SUPPORTED_100baseT_Full        (1 << 3)
#define SUPPORTED_1000baseT_Half    (1 << 4)
#define SUPPORTED_1000baseT_Full    (1 << 5)
#define SUPPORTED_Autoneg        (1 << 6)
#define SUPPORTED_TP            (1 << 7)
#define SUPPORTED_AUI            (1 << 8)
#define SUPPORTED_MII            (1 << 9)
#define SUPPORTED_FIBRE            (1 << 10)
#define SUPPORTED_BNC            (1 << 11)
#define SUPPORTED_10000baseT_Full    (1 << 12)
#define SUPPORTED_Pause            (1 << 13)
#define SUPPORTED_Asym_Pause        (1 << 14)
#define SUPPORTED_2500baseX_Full    (1 << 15)
#define SUPPORTED_Backplane        (1 << 16)
#define SUPPORTED_1000baseKX_Full    (1 << 17)
#define SUPPORTED_10000baseKX4_Full    (1 << 18)
#define SUPPORTED_10000baseKR_Full    (1 << 19)
#define SUPPORTED_10000baseR_FEC    (1 << 20)
#define SUPPORTED_1000baseX_Half    (1 << 21)
#define SUPPORTED_1000baseX_Full    (1 << 22)

/* Indicates what features are advertised by the interface. */
#define ADVERTISED_10baseT_Half        (1 << 0)
#define ADVERTISED_10baseT_Full        (1 << 1)
#define ADVERTISED_100baseT_Half    (1 << 2)
#define ADVERTISED_100baseT_Full    (1 << 3)
#define ADVERTISED_1000baseT_Half    (1 << 4)
#define ADVERTISED_1000baseT_Full    (1 << 5)
#define ADVERTISED_Autoneg        (1 << 6)
#define ADVERTISED_TP            (1 << 7)
#define ADVERTISED_AUI            (1 << 8)
#define ADVERTISED_MII            (1 << 9)
#define ADVERTISED_FIBRE        (1 << 10)
#define ADVERTISED_BNC            (1 << 11)
#define ADVERTISED_10000baseT_Full    (1 << 12)
#define ADVERTISED_Pause        (1 << 13)
#define ADVERTISED_Asym_Pause        (1 << 14)
#define ADVERTISED_2500baseX_Full    (1 << 15)
#define ADVERTISED_Backplane        (1 << 16)
#define ADVERTISED_1000baseKX_Full    (1 << 17)
#define ADVERTISED_10000baseKX4_Full    (1 << 18)
#define ADVERTISED_10000baseKR_Full    (1 << 19)
#define ADVERTISED_10000baseR_FEC    (1 << 20)
#define ADVERTISED_1000baseX_Half    (1 << 21)
#define ADVERTISED_1000baseX_Full    (1 << 22)

/* The following are all involved in forcing a particular link
 * mode for the device for setting things.  When getting the
 * devices settings, these indicate the current mode and whether
 * it was foced up into this mode or autonegotiated.
 */

/* The forced speed, 10Mb, 100Mb, gigabit, 2.5Gb, 10GbE. */
#define SPEED_10        10
#define SPEED_100        100
#define SPEED_1000        1000
#define SPEED_2500        2500
#define SPEED_10000        10000

/* Duplex, half or full. */
#define DUPLEX_HALF        0x00
#define DUPLEX_FULL        0x01

/* Enable or disable autonegotiation.  If this is set to enable,
 * the forced link modes above are completely ignored.
 */
#define AUTONEG_DISABLE        0x00
#define AUTONEG_ENABLE        0x01

#define PHY_FIXED_ID        0xa5a55a5a
#define PHY_NCSI_ID            0xbeefcafe
#define PHY_LINK_MASK 0x04
/*
 * There is no actual id for this.
 * This is just a dummy id for gmii2rgmmi converter.
 */
#define PHY_GMII2RGMII_ID    0x5a5a5a5a
#define PHY_MAX_ADDR 32

#define PHY_FLAG_BROKEN_RESET    (1 << 0) /* soft reset not supported */
#define PHY_DEFAULT_FEATURES    (SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII)
#define PHY_10BT_FEATURES    (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full)
#define PHY_100BT_FEATURES    (SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full)
#define PHY_1000BT_FEATURES    (SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full)
#define PHY_BASIC_FEATURES    (PHY_10BT_FEATURES | PHY_100BT_FEATURES | PHY_DEFAULT_FEATURES)
#define PHY_GBIT_FEATURES    (PHY_BASIC_FEATURES | PHY_1000BT_FEATURES)
#define PHY_10G_FEATURES    (PHY_GBIT_FEATURES | SUPPORTED_10000baseT_Full)
#define PHY_ANEG_TIMEOUT    4000

struct phy_device;

#define MDIO_NAME_LEN 32

struct mdio_bus 
{
    struct rt_device parent;
    struct rt_list_node link;
    char name[MDIO_NAME_LEN];
    void *priv;// point to the eth device
    int (*read)(struct mdio_bus *bus, int addr, int dev_addr, int reg);
    int (*write)(struct mdio_bus *bus, int addr, int dev_addr, int reg, uint16_t val);
    int (*reset)(struct mdio_bus *bus);
    struct phy_device *phydev_list[PHY_MAX_ADDR];
    struct rt_list_node phy_driver_list;
    rt_mutex_t mutex;
};

struct phy_driver 
{
    char *name;
    unsigned int uid;
    unsigned int mask;
    unsigned int mmds;
    uint32_t features;
    int (*phy_probe)(struct phy_device *phydev); /*Called during discovery.  Used to setup device-specific structures */
    int (*phy_config)(struct phy_device *phydev); /* Called to configure the PHY, and modify the controller */
    int (*phy_startup)(struct phy_device *phydev); /* Called when starting up the controller */
    int (*phy_shutdown)(struct phy_device *phydev); /* Called when bringing down the controller */
    int (*phy_read_mmd)(struct phy_device *phydev, int dev_addr, int reg); /* Phy specific driver override for reading a MMD register */
    int (*phy_write_mmd)(struct phy_device *phydev, int dev_addr, int reg, uint16_t val); /* Phy specific driver override for writing a MMD register */
    struct rt_list_node list;
    void *data; /* driver private data */
};

struct phy_device 
{
    struct rt_device parent;
    struct mdio_bus *bus;
    struct phy_driver *drv;
    void *priv;
    int speed;
    int duplex;
    uint32_t phy_id;    
    int addr;    
    int link;
    phy_interface_t interface;
    uint32_t advertising;
    uint32_t supported;
    uint32_t mmds;
    int autoneg;
    rt_bool_t is_c45;
    uint32_t flags;
    int pause;
    int asym_pause;
};

struct phy_fixed_link 
{
    uint32_t phy_id;
    int duplex;
    int link_speed;
};

static inline int phy_read(struct phy_device *phydev, int dev_addr, int regnum)
{
    struct mdio_bus *bus = phydev->bus;
    int ret;
    if (!bus || !bus->read) 
    {
        return -1;
    }
    rt_mutex_take(bus->mutex, RT_WAITING_FOREVER);
    ret = bus->read(bus, phydev->addr, dev_addr, regnum);
    rt_mutex_release(bus->mutex);
    return ret;    
}
static inline int phy_write(struct phy_device *phydev, int dev_addr, int regnum, uint16_t val)
{
    struct mdio_bus *bus = phydev->bus;
    int ret;
    if (!bus || !bus->read) 
    {
        return -1;
    }
    rt_mutex_take(bus->mutex, RT_WAITING_FOREVER);
    ret = bus->write(bus, phydev->addr, dev_addr, regnum, val);
    rt_mutex_release(bus->mutex);
    return ret;
}

struct phy_device *phy_find_by_mask(struct mdio_bus *bus, uint addr, int dev_addr, phy_interface_t interface);
struct phy_device *phy_device_get_by_bus(struct mdio_bus *bus, int addr, int dev_addr, phy_interface_t interface);

int phy_reset(struct phy_device *phydev);
int phy_startup(struct phy_device *phydev);
int phy_config(struct phy_device *phydev);
int phy_shutdown(struct phy_device *phydev);
int phy_register(struct phy_driver *drv,struct rt_list_node *list);
int phy_set_supported(struct phy_device *phydev, rt_uint32_t max_speed);

int get_phy_id(struct mdio_bus *bus, int addr, int dev_addr, uint32_t *phy_id);
int phy_get_link_status(struct phy_device *phydev);
struct phy_device *get_phy_device(struct mdio_bus *bus, uint addr);

int phy_drv_register(struct phy_driver *drv,struct rt_list_node *list);


int phy_write_mmd(struct phy_device *phydev, int devad, int regnum, uint16_t val);
int phy_read_mmd(struct phy_device *phydev, int devad, int regnum);


int general_phy_config_aneg(struct phy_device *phydev);
int general_phy_restart_aneg(struct phy_device *phydev);
int general_phy_update_link(struct phy_device *phydev);
int general_phy_parse_link(struct phy_device *phydev);
int general_phy_config(struct phy_device *phydev);
int general_phy_startup(struct phy_device *phydev);
int general_phy_shutdown(struct phy_device *phydev);

#endif
