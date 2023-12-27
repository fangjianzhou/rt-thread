/*
 * Copyright (c) 2013-2022, Shanghai Real-Thread Electronic Technology Co.,Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-6-2     songchao     the first version
 */

#include "phy.h"
#define DBG_TAG "drv.phy"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>


int general_phy_driver_register(struct rt_list_node *list);
struct phy_driver *generic_for_interface(phy_interface_t interface);

/* Generic PHY support and helper functions */
int general_phy_config(struct phy_device *phydev)
{

    int val;
    uint32_t features;

    features = (SUPPORTED_TP | SUPPORTED_MII | SUPPORTED_AUI | SUPPORTED_FIBRE | SUPPORTED_BNC);

    /* Do we support autonegotiation? */
    val = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

    if (val < 0)
        return val;

    if (val & BMSR_ANEGCAPABLE)
        features |= SUPPORTED_Autoneg;
    if (val & BMSR_100FULL)
        features |= SUPPORTED_100baseT_Full;
    if (val & BMSR_100HALF)
        features |= SUPPORTED_100baseT_Half;
    if (val & BMSR_10FULL)
        features |= SUPPORTED_10baseT_Full;
    if (val & BMSR_10HALF)
        features |= SUPPORTED_10baseT_Half;

    if (val & BMSR_ESTATEN) 
    {
        val = phy_read(phydev, MDIO_DEVAD_NONE, MII_ESTATUS);

        if (val < 0)
            return val;

        if (val & ESTATUS_1000_TFULL)
            features |= SUPPORTED_1000baseT_Full;
        if (val & ESTATUS_1000_THALF)
            features |= SUPPORTED_1000baseT_Half;
        if (val & ESTATUS_1000_XFULL)
            features |= SUPPORTED_1000baseX_Full;
        if (val & ESTATUS_1000_XHALF)
            features |= SUPPORTED_1000baseX_Half;
    }

    phydev->supported &= features;
    phydev->advertising /*&*/= features;
    general_phy_config_aneg(phydev);

    return 0;
}

int general_phy_startup(struct phy_device *phydev)
{
    int ret;
    general_phy_restart_aneg(phydev);
    ret = general_phy_update_link(phydev);
    if (ret)
    {
        return ret;
    }

    return general_phy_parse_link(phydev);
}

int general_phy_shutdown(struct phy_device *phydev)
{
    return 0;
}

struct phy_driver general_phy_driver = 
{
    .uid         = 0xffffffff,
    .mask        = 0xffffffff,
    .name        = "Generic PHY driver",
    .features    = PHY_GBIT_FEATURES | SUPPORTED_MII |
              SUPPORTED_AUI | SUPPORTED_FIBRE |
              SUPPORTED_BNC,
    .phy_config      = general_phy_config,
    .phy_startup     = general_phy_startup,
    .phy_shutdown    = general_phy_shutdown,
};

int general_phy_driver_register(struct rt_list_node *list)
{
    extern struct phy_driver rtl8211f;
    return phy_drv_register(&rtl8211f,list);
}
/**
 * general_phy_config_advert - sanitize and advertise auto-negotiation parameters
 * @phydev: target phy_device struct
 *
 * Description: Writes MII_ADVERTISE with the appropriate values,
 *   after sanitizing the values to make sure we only advertise
 *   what is supported.  Returns < 0 on error, 0 if the PHY's advertisement
 *   hasn't changed, and > 0 if it has changed.
 */
static int general_phy_config_advert(struct phy_device *phydev)
{
    uint32_t advertise;
    int oldadv, adv, bmsr;
    int err = 0, changed = 0;

    /* Only allow advertising what this PHY supports */
    phydev->advertising &= phydev->supported;
    advertise = phydev->advertising;

    /* Setup standard advertisement */
    adv = phy_read(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE);
    oldadv = adv;

    if (adv < 0)
        return adv;

    adv &= ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);
    if (advertise & ADVERTISED_10baseT_Half)
        adv |= ADVERTISE_10HALF;
    if (advertise & ADVERTISED_10baseT_Full)
        adv |= ADVERTISE_10FULL;
    if (advertise & ADVERTISED_100baseT_Half)
        adv |= ADVERTISE_100HALF;
    if (advertise & ADVERTISED_100baseT_Full)
        adv |= ADVERTISE_100FULL;
    if (advertise & ADVERTISED_Pause)
        adv |= ADVERTISE_PAUSE_CAP;
    if (advertise & ADVERTISED_Asym_Pause)
        adv |= ADVERTISE_PAUSE_ASYM;
    if (advertise & ADVERTISED_1000baseX_Half)
        adv |= ADVERTISE_1000XHALF;
    if (advertise & ADVERTISED_1000baseX_Full)
        adv |= ADVERTISE_1000XFULL;

    if (adv != oldadv) 
    {
        err = phy_write(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE, 0xffff);
        if (err < 0)
            return err;
        changed = 1;
    }

    bmsr = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
    if (bmsr < 0)
        return bmsr;

    /* Per 802.3-2008, Section 22.2.4.2.16 Extended status all
     * 1000Mbits/sec capable PHYs shall have the BMSR_ESTATEN bit set to a
     * logical 1.
     */
    if (!(bmsr & BMSR_ESTATEN))
        return changed;

    /* Configure gigabit if it's supported */
    adv = phy_read(phydev, MDIO_DEVAD_NONE, MII_CTRL1000);
    oldadv = adv;

    if (adv < 0)
        return adv;

    adv &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);

    if (phydev->supported & (SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full)) 
    {
        if (advertise & SUPPORTED_1000baseT_Half)
            adv |= ADVERTISE_1000HALF;
        if (advertise & SUPPORTED_1000baseT_Full)
            adv |= ADVERTISE_1000FULL;
    }

    if (adv != oldadv)
        changed = 1;

    err = phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, adv);
    if (err < 0)
        return err;

    return changed;
}

/**
 * general_phy_setup_forced - configures/forces speed/duplex from @phydev
 * @phydev: target phy_device struct
 *
 * Description: Configures MII_BMCR to force speed/duplex
 *   to the values in phydev. Assumes that the values are valid.
 */
static int general_phy_setup_forced(struct phy_device *phydev)
{
    int err;
    int ctl = BMCR_ANRESTART;

    phydev->pause = 0;
    phydev->asym_pause = 0;

    if (phydev->speed == SPEED_1000)
        ctl |= BMCR_SPEED1000;
    else if (phydev->speed == SPEED_100)
        ctl |= BMCR_SPEED100;

    if (phydev->duplex == DUPLEX_FULL)
        ctl |= BMCR_FULLDPLX;

    err = phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, ctl);

    return err;
}

/**
 * general_phy_restart_aneg - Enable and Restart Autonegotiation
 * @phydev: target phy_device struct
 */
int general_phy_restart_aneg(struct phy_device *phydev)
{
    int ctl;

    ctl = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

    if (ctl < 0)
        return ctl;

    ctl |= (BMCR_ANENABLE | BMCR_ANRESTART);

    /* Don't isolate the PHY if we're negotiating */
    ctl &= ~(BMCR_ISOLATE);

    ctl = phy_write(phydev, MDIO_DEVAD_NONE, MII_BMCR, ctl);

    return ctl;
}

/**
 * general_phy_config_aneg - restart auto-negotiation or write BMCR
 * @phydev: target phy_device struct
 *
 * Description: If auto-negotiation is enabled, we configure the
 *   advertising, and then restart auto-negotiation.  If it is not
 *   enabled, then we write the BMCR.
 */
int general_phy_config_aneg(struct phy_device *phydev)
{
    int result;

    if (phydev->autoneg != AUTONEG_ENABLE)
    {
        return general_phy_setup_forced(phydev);
    }

    result = general_phy_config_advert(phydev);
    if (result < 0) /* error */
    {
        return result;
    }
    if (result == 0) 
    {
        /*
         * Advertisment hasn't changed, but maybe aneg was never on to
         * begin with?  Or maybe phy was isolated?
         */
        int ctl = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

        if (ctl < 0)
            return ctl;

        if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE))
            result = 1; /* do restart aneg */
    }

    /*
     * Only restart aneg if we are advertising something different
     * than we were before.
     */
    if (result > 0)
    {
        result = general_phy_restart_aneg(phydev);
    }
    return result;
}

/**
 * general_phy_update_link - update link status in @phydev
 * @phydev: target phy_device struct
 *
 * Description: Update the value in phydev->link to rt_reflect the
 *   current link value.  In order to do this, we need to read
 *   the status register twice, keeping the second value.
 */
int general_phy_update_link(struct phy_device *phydev)
{
    unsigned int mii_reg;

    /*
     * Wait if the link is up, and autonegotiation is in progress
     * (ie - we're capable and it's not done)
     */
    mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

    /*
     * If we already saw the link up, and it hasn't gone down, then
     * we don't need to wait for autoneg again
     */
    if (phydev->link && mii_reg & BMSR_LSTATUS)
    {
        return 0;
    }

    if ((phydev->autoneg == AUTONEG_ENABLE) && (!(mii_reg & BMSR_ANEGCOMPLETE))) 
    {
        int i = 0;

        LOG_D("%s Waiting for PHY auto negotiation to complete",phydev->parent.parent);
        while (!(mii_reg & BMSR_ANEGCOMPLETE)) 
        {
            /*
             * Timeout reached ?
             */
            if (i > (PHY_ANEG_TIMEOUT)) 
            {
                LOG_E(" TIMEOUT !\n");
                phydev->link = 0;
                return -ETIMEDOUT;
            }

            mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
            
            rt_thread_delay(100);    /* 100 ms */
        }
        rt_kprintf(" done\n");
        phydev->link = 1;
    } 
    else 

    {
        /* Read the link a second time to clear the latched state */
        mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

        if (mii_reg & BMSR_LSTATUS)
            phydev->link = 1;
        else
            phydev->link = 0;
    }

    return 0;
}

/*
 * Generic function which updates the speed and duplex.  If
 * autonegotiation is enabled, it uses the AND of the link
 * partner's advertised capabilities and our advertised
 * capabilities.  If autonegotiation is disabled, we use the
 * appropriate bits in the control register.
 */
int general_phy_parse_link(struct phy_device *phydev)
{
    int mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);

    /* We're using autonegotiation */
    if (phydev->autoneg == AUTONEG_ENABLE) 
    {
        uint32_t lpa = 0;
        int gblpa = 0;
        uint32_t estatus = 0;

        /* Check for gigabit capability */
        if (phydev->supported & (SUPPORTED_1000baseT_Full | SUPPORTED_1000baseT_Half)) 
        {
            /* We want a list of states supported by
             * both PHYs in the link
             */
            gblpa = phy_read(phydev, MDIO_DEVAD_NONE, MII_STAT1000);
            if (gblpa < 0) 
            {
                LOG_E("Could not read MII_STAT1000. ");
                LOG_E("Ignoring gigabit capability\n");
                gblpa = 0;
            }
            gblpa &= phy_read(phydev,MDIO_DEVAD_NONE, MII_CTRL1000) << 2;
        }

        /* Set the baseline so we only have to set them
         * if they're different
         */
        phydev->speed = SPEED_10;
        phydev->duplex = DUPLEX_HALF;

        /* Check the gigabit fields */
        if (gblpa & (PHY_1000BTSR_1000FD | PHY_1000BTSR_1000HD)) 
        {
            phydev->speed = SPEED_1000;

            if (gblpa & PHY_1000BTSR_1000FD)
                phydev->duplex = DUPLEX_FULL;

            /* We're done! */
            return 0;
        }

        lpa = phy_read(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE);
        lpa &= phy_read(phydev, MDIO_DEVAD_NONE, MII_LPA);

        if (lpa & (LPA_100FULL | LPA_100HALF)) 
        {
            phydev->speed = SPEED_100;

            if (lpa & LPA_100FULL)
                phydev->duplex = DUPLEX_FULL;

        } 
        else if (lpa & LPA_10FULL) 
        {
            phydev->duplex = DUPLEX_FULL;
        }

        /*
         * Extended status may indicate that the PHY supports
         * 1000BASE-T/X even though the 1000BASE-T registers
         * are missing. In this case we can't tell whether the
         * peer also supports it, so we only check extended
         * status if the 1000BASE-T registers are actually
         * missing.
         */
        if ((mii_reg & BMSR_ESTATEN) && !(mii_reg & BMSR_ERCAP))
            estatus = phy_read(phydev, MDIO_DEVAD_NONE,
                       MII_ESTATUS);

        if (estatus & (ESTATUS_1000_XFULL | ESTATUS_1000_XHALF |
                ESTATUS_1000_TFULL | ESTATUS_1000_THALF)) 
        {
            phydev->speed = SPEED_1000;
            if (estatus & (ESTATUS_1000_XFULL | ESTATUS_1000_TFULL))
                phydev->duplex = DUPLEX_FULL;
        }

    } 
    else 
    {
        uint32_t bmcr = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

        phydev->speed = SPEED_10;
        phydev->duplex = DUPLEX_HALF;

        if (bmcr & BMCR_FULLDPLX)
            phydev->duplex = DUPLEX_FULL;

        if (bmcr & BMCR_SPEED1000)
            phydev->speed = SPEED_1000;
        else if (bmcr & BMCR_SPEED100)
            phydev->speed = SPEED_100;
    }

    return 0;
}

struct phy_driver *generic_for_interface(phy_interface_t interface)
{
    return &general_phy_driver;
}