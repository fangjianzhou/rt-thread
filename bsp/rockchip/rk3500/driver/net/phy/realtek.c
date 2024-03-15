#include "phy.h"
#define DBG_TAG "drv.phy"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* RTL8211F PHY Status Register */
#define MIIM_RTL8211F_PHY_STATUS       0x1a
#define MIIM_RTL8211F_AUTONEG_ENABLE   0x1000
#define MIIM_RTL8211F_PHYSTAT_SPEED    0x0030
#define MIIM_RTL8211F_PHYSTAT_GBIT     0x0020
#define MIIM_RTL8211F_PHYSTAT_100      0x0010
#define MIIM_RTL8211F_PHYSTAT_DUPLEX   0x0008
#define MIIM_RTL8211F_PHYSTAT_SPDDONE  0x0800
#define MIIM_RTL8211F_PHYSTAT_LINK     0x0004

#define MIIM_RTL8211F_PAGE_SELECT      0x1f
#define MIIM_RTL8211F_TX_DELAY		0x100
#define MIIM_RTL8211F_RX_DELAY		0x8
#define MIIM_RTL8211F_LCR		0x10

static int rtl8211f_config(struct phy_device *phydev)
{
	uint16_t reg;
#if 0
	if (phydev->flags & PHY_RTL8211F_FORCE_EEE_RXC_ON) {
		unsigned int reg;

		reg = phy_read_mmd(phydev, MDIO_MMD_PCS, MDIO_CTRL1);
		reg &= ~MDIO_PCS_CTRL1_CLKSTOP_EN;
		phy_write_mmd(phydev, MDIO_MMD_PCS, MDIO_CTRL1, reg);
	}
#endif
	phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);

	phy_write(phydev, MDIO_DEVAD_NONE,
		  MIIM_RTL8211F_PAGE_SELECT, 0xd08);
	reg = phy_read(phydev, MDIO_DEVAD_NONE, 0x11);

	/* enable TX-delay for rgmii-id and rgmii-txid, otherwise disable it */
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)
		reg |= MIIM_RTL8211F_TX_DELAY;
	else
		reg &= ~MIIM_RTL8211F_TX_DELAY;

	phy_write(phydev, MDIO_DEVAD_NONE, 0x11, reg);

	/* enable RX-delay for rgmii-id and rgmii-rxid, otherwise disable it */
	reg = phy_read(phydev, MDIO_DEVAD_NONE, 0x15);
	if (phydev->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID)
		reg |= MIIM_RTL8211F_RX_DELAY;
	else
		reg &= ~MIIM_RTL8211F_RX_DELAY;
	phy_write(phydev, MDIO_DEVAD_NONE, 0x15, reg);

	/* restore to default page 0 */
	phy_write(phydev, MDIO_DEVAD_NONE,
		  MIIM_RTL8211F_PAGE_SELECT, 0x0);

	/* Set green LED for Link, yellow LED for Active */
	phy_write(phydev, MDIO_DEVAD_NONE,
		  MIIM_RTL8211F_PAGE_SELECT, 0xd04);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x10, 0x617f);
	phy_write(phydev, MDIO_DEVAD_NONE,
		  MIIM_RTL8211F_PAGE_SELECT, 0x0);

	general_phy_config_aneg(phydev);

	return 0;
}




static int rtl8211f_parse_status(struct phy_device *phydev)
{
	unsigned int speed;
	unsigned int mii_reg;
	int i = 0;

	phy_write(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PAGE_SELECT, 0xa43);
	mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MIIM_RTL8211F_PHY_STATUS);

	phydev->link = 1;
	while (!(mii_reg & MIIM_RTL8211F_PHYSTAT_LINK)) {
		if (i > 10000) {
			rt_kprintf(" TIMEOUT !\n");
			phydev->link = 0;
			break;
		}

		if ((i++ % 1000) == 0)
			rt_kprintf(".");
		rt_hw_us_delay(1000);
		mii_reg = phy_read(phydev, MDIO_DEVAD_NONE,
				   MIIM_RTL8211F_PHY_STATUS);
	}

	if (mii_reg & MIIM_RTL8211F_PHYSTAT_DUPLEX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	speed = (mii_reg & MIIM_RTL8211F_PHYSTAT_SPEED);

	switch (speed) {
	case MIIM_RTL8211F_PHYSTAT_GBIT:
		phydev->speed = SPEED_1000;
		break;
	case MIIM_RTL8211F_PHYSTAT_100:
		phydev->speed = SPEED_100;
		break;
	default:
		phydev->speed = SPEED_10;
	}

	return 0;
}
static int rtl8211f_startup(struct phy_device *phydev)
{
	int ret;

	/* Read the Status (2x to make sure link is right) */
	ret = general_phy_update_link(phydev);
	if (ret)
		return ret;
	/* Read the Status (2x to make sure link is right) */

	return rtl8211f_parse_status(phydev);
}

struct phy_driver rtl8211f = 
{
    .uid         = 0x1cc916,
    .mask        = 0xffffffff,
    .name        = "RTL8211F Gigabit Ethernet",
    .features    = PHY_GBIT_FEATURES,
    .phy_config      = rtl8211f_config,
    .phy_startup     = rtl8211f_startup,
    .phy_shutdown    = general_phy_shutdown,
};
