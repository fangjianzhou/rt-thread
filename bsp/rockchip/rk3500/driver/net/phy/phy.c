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

int phy_get_link_status(struct phy_device *phydev)
{
    unsigned int mii_reg;
    mii_reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
    if ((mii_reg & PHY_LINK_MASK) == (PHY_LINK_MASK))
    {
        return 1;    
    }
    return 0;
}

int phy_drv_register(struct phy_driver *drv,struct rt_list_node *list)
{
    rt_list_insert_after(list,&drv->list);
    return 0;
}

static struct phy_driver *phy_device_match_driver(struct mdio_bus *bus, struct phy_device *phydev, phy_interface_t interface)
{
    struct rt_list_node *entry;
    int phy_id = phydev->phy_id;
    struct phy_driver *drv = RT_NULL;

    rt_list_for_each(entry, &bus->phy_driver_list) 
    {
        drv = rt_list_entry(entry, struct phy_driver, list);

        LOG_D("drv->uid %x phy_id %x\n",drv->uid,phy_id);
        if ((drv->uid & drv->mask) == (phy_id & drv->mask))
        {
            LOG_I("match success");
            phydev->drv = drv;
            break;
        }
    }
  
    if(!drv)
    {
        return drv;
    }
    if(phydev->drv)
    {
        phydev->advertising = phydev->drv->features;
        phydev->supported = phydev->drv->features;
        phydev->mmds = phydev->drv->mmds;
        if (phydev->drv->phy_probe)
        {
            phydev->drv->phy_probe(phydev);
        }        
    }
    return drv;

}

static struct phy_device *phy_device_create(int addr, uint32_t phy_id, rt_bool_t is_c45, phy_interface_t interface)
{
    struct phy_device *phy_dev;
    phy_dev  = rt_malloc(sizeof(*phy_dev));
    if (!phy_dev ) 
    {
        return RT_NULL;
    }
    rt_memset(phy_dev , 0, sizeof(*phy_dev));
    phy_dev->duplex = -1;
    phy_dev->link = 0;
    phy_dev->interface = interface;
    phy_dev->autoneg = AUTONEG_ENABLE;
    phy_dev->addr = addr;
    phy_dev->phy_id = phy_id;
    phy_dev->is_c45 = is_c45;

    return phy_dev ;
}

static int phy_device_register(struct mdio_bus *bus,struct phy_device *phy_dev)
{
    char phy_name[RT_NAME_MAX];

    if (phy_dev->addr >= 0 && phy_dev->addr < PHY_MAX_ADDR)
    {
        bus->phydev_list[phy_dev->addr] = phy_dev ;
        phy_dev->bus = bus;
        rt_sprintf(phy_name,"phy%x",(phy_dev->phy_id&(0xffff)));
        rt_device_register((rt_device_t)phy_dev, phy_name, RT_DEVICE_FLAG_RDWR);        
    }
    else
    {
        LOG_E("phy device addr %d is invaliled\n",phy_dev->addr);
        return -1;
    }

    return 0;
}

int get_phy_id(struct mdio_bus *bus, int addr, int dev_addr, uint32_t *phy_id)
{
    int phy_reg;

    phy_reg = bus->read(bus, addr, dev_addr, MII_PHYSID1);
    if (phy_reg < 0)
    {
        return -EIO;
    }
    *phy_id = (phy_reg & 0xffff) << 16;
    phy_reg = bus->read(bus, addr, dev_addr, MII_PHYSID2);

    if (phy_reg < 0)
    {
        return -EIO;
    }

    *phy_id |= (phy_reg & 0xffff);

    return 0;
}

struct phy_device *phy_device_get_by_addr(struct mdio_bus *bus, uint addr)
{
    if(!bus)
    {
        return RT_NULL;
    }
    if(addr >= PHY_MAX_ADDR)
    {
        LOG_E("phy addr %d is invalided\n",addr);
        return RT_NULL;
    }
    if (bus->phydev_list[addr]) 
    {
        return bus->phydev_list[addr];
    }
    return RT_NULL;
}

int phy_reset(struct phy_device *phydev)
{
    int reg;
    int timeout = 1000;
    int dev_addr = MDIO_DEVAD_NONE;

    if (phydev->flags & PHY_FLAG_BROKEN_RESET)
    {
        return 0;
    }

    if (phy_write(phydev, dev_addr, MII_BMCR, BMCR_RESET) < 0) 
    {
        LOG_E("PHY reset failed\n");
        return -1;
    }
    reg = phy_read(phydev, dev_addr, MII_BMCR);
    while ((reg & BMCR_RESET) && timeout--) 
    {
        reg = phy_read(phydev, dev_addr, MII_BMCR);
        if (reg < 0) 
        {
            LOG_E("PHY status read failed\n");
            return -1;
        }
        rt_thread_delay(100);
    }

    if (reg & BMCR_RESET) 
    {
        LOG_E("PHY reset timed out\n");
        return -1;
    }

    rt_kprintf("phy reset success\n\n");
    return 0;
}

struct phy_device *phy_device_create_by_addr(struct mdio_bus *bus, uint addr, int dev_addr, phy_interface_t interface)
{
    uint32_t phy_id = 0xffffffff;
    rt_bool_t is_c45;
    struct phy_device *phy_dev = RT_NULL;
    int r = get_phy_id(bus, addr, dev_addr, &phy_id);
    if (r == 0 && phy_id == 0)
    {
        return RT_NULL;
    }

    if (r == 0 && ((phy_id & 0x1fffffff) != 0x1fffffff)) 
    {
        is_c45 = (dev_addr == MDIO_DEVAD_NONE) ? RT_FALSE : RT_TRUE;
        phy_dev = phy_device_create(addr, phy_id, is_c45, interface);
        if(!phy_dev)
        {
            LOG_E("phy_device create failed\n");
            return RT_NULL;
        }

        phy_dev->drv = phy_device_match_driver(bus, phy_dev, interface);
        if(!phy_dev->drv)
        {
            LOG_E("phy device match driver failed\n");
            return RT_NULL;
        }
        phy_device_register(bus,phy_dev);

    }

    return phy_dev;
}

struct phy_device *phy_device_get_by_bus(struct mdio_bus *bus, int addr, int dev_addr, phy_interface_t interface)
{
    struct phy_device *phydev = RT_NULL;
    /* Reset the bus */
    if (bus->reset) 
    {
        bus->reset(bus);
        /* Wait 15ms to make sure the PHY has come out of hard reset */
        rt_thread_delay(15);
    }

    phydev = phy_device_get_by_addr(bus, addr);
    if (phydev)
    {
        return phydev;
    }
    else
    {
        phydev = phy_device_create_by_addr(bus, addr, dev_addr, interface);
    }

    return phydev;
}

int phy_startup(struct phy_device *phydev)
{
    if (phydev)
    {
        if (phydev->drv->phy_startup)
        {
            return phydev->drv->phy_startup(phydev);
        }    
    }
    LOG_E("phy startup function is NULL");
    return -1;    
}

int phy_config(struct phy_device *phydev)
{
    if(phydev)
    {
        if (phydev->drv->phy_config)
        {
            return phydev->drv->phy_config(phydev);
        }
    }
    LOG_E("phy config function is NULL");
    return -1;
}

int phy_shutdown(struct phy_device *phydev)
{
    if(phydev)
    {
        if (phydev->drv->phy_shutdown)
        {
            return phydev->drv->phy_shutdown(phydev);
        }
    }    
    LOG_E("phy shutdown function is NULL");
    return -1;
}

int phy_set_supported(struct phy_device *phydev, rt_uint32_t max_speed)
{
	/* The default values for phydev->supported are provided by the PHY
	 * driver "features" member, we want to reset to sane defaults first
	 * before supporting higher speeds.
	 */
	phydev->supported &= PHY_DEFAULT_FEATURES;

	switch (max_speed) {
	default:
		return -ENOTSUP;
	case SPEED_1000:
		phydev->supported |= PHY_1000BT_FEATURES;
		/* fall through */
	case SPEED_100:
		phydev->supported |= PHY_100BT_FEATURES;
		/* fall through */
	case SPEED_10:
		phydev->supported |= PHY_10BT_FEATURES;
	}

	return 0;
}






/**
 * phy_mmd_start_indirect - Convenience function for writing MMD registers
 * @phydev: the phy_device struct
 * @devad: The MMD to read from
 * @regnum: register number to write
 * @return: None
 */
void phy_mmd_start_indirect(struct phy_device *phydev, int devad, int regnum)
{
	/* Write the desired MMD Devad */
	phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_CTRL, devad);

	/* Write the desired MMD register address */
	phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_DATA, regnum);

	/* Select the Function : DATA with no post increment */
	phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_CTRL,
		  (devad | MII_MMD_CTRL_NOINCR));
}

/**
 * phy_read_mmd - Convenience function for reading a register
 * from an MMD on a given PHY.
 * @phydev: The phy_device struct
 * @devad: The MMD to read from
 * @regnum: The register on the MMD to read
 * @return: Value for success or negative errno for failure
 */
int phy_read_mmd(struct phy_device *phydev, int devad, int regnum)
{
	struct phy_driver *drv = phydev->drv;

	if (regnum > (uint16_t)~0 || devad > 32)
		return -EINVAL;

	/* driver-specific access */
	if (drv->phy_read_mmd)
		return drv->phy_read_mmd(phydev, devad, regnum);

	/* direct C45 / C22 access */
	if ((drv->features & PHY_10G_FEATURES) == PHY_10G_FEATURES ||
	    devad == MDIO_DEVAD_NONE || !devad)
		return phy_read(phydev, devad, regnum);

	/* indirect C22 access */
	phy_mmd_start_indirect(phydev, devad, regnum);

	/* Read the content of the MMD's selected register */
	return phy_read(phydev, MDIO_DEVAD_NONE, MII_MMD_DATA);
}

/**
 * phy_write_mmd - Convenience function for writing a register
 * on an MMD on a given PHY.
 * @phydev: The phy_device struct
 * @devad: The MMD to read from
 * @regnum: The register on the MMD to read
 * @val: value to write to @regnum
 * @return: 0 for success or negative errno for failure
 */
int phy_write_mmd(struct phy_device *phydev, int devad, int regnum, uint16_t val)
{
	struct phy_driver *drv = phydev->drv;

	if (regnum > (uint16_t)~0 || devad > 32)
		return -EINVAL;

	/* driver-specific access */
	if (drv->phy_write_mmd)
		return drv->phy_write_mmd(phydev, devad, regnum, val);

	/* direct C45 / C22 access */
	if ((drv->features & PHY_10G_FEATURES) == PHY_10G_FEATURES ||
	    devad == MDIO_DEVAD_NONE || !devad)
		return phy_write(phydev, devad, regnum, val);

	/* indirect C22 access */
	phy_mmd_start_indirect(phydev, devad, regnum);

	/* Write the data into MMD's selected register */
	return phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_DATA, val);
}