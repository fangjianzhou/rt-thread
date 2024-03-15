#include <osdep/mmc/mmc.h>
#include <osdep/port.h>
#include <drivers/core/dm.h>

static const rt_uint8_t tuning_blk_pattern_4bit[] = {
    0xff, 0x0f, 0xff, 0x00, 0xff, 0xcc, 0xc3, 0xcc,
    0xc3, 0x3c, 0xcc, 0xff, 0xfe, 0xff, 0xfe, 0xef,
    0xff, 0xdf, 0xff, 0xdd, 0xff, 0xfb, 0xff, 0xfb,
    0xbf, 0xff, 0x7f, 0xff, 0x77, 0xf7, 0xbd, 0xef,
    0xff, 0xf0, 0xff, 0xf0, 0x0f, 0xfc, 0xcc, 0x3c,
    0xcc, 0x33, 0xcc, 0xcf, 0xff, 0xef, 0xff, 0xee,
    0xff, 0xfd, 0xff, 0xfd, 0xdf, 0xff, 0xbf, 0xff,
    0xbb, 0xff, 0xf7, 0xff, 0xf7, 0x7f, 0x7b, 0xde,
};

static const rt_uint8_t tuning_blk_pattern_8bit[] = {
    0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00,
    0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc, 0xcc,
    0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff, 0xff,
    0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee, 0xff,
    0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd, 0xdd,
    0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff, 0xbb,
    0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff, 0xff,
    0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee, 0xff,
    0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
    0x00, 0xff, 0xff, 0xcc, 0xcc, 0xcc, 0x33, 0xcc,
    0xcc, 0xcc, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xff,
    0xff, 0xff, 0xee, 0xff, 0xff, 0xff, 0xee, 0xee,
    0xff, 0xff, 0xff, 0xdd, 0xff, 0xff, 0xff, 0xdd,
    0xdd, 0xff, 0xff, 0xff, 0xbb, 0xff, 0xff, 0xff,
    0xbb, 0xbb, 0xff, 0xff, 0xff, 0x77, 0xff, 0xff,
    0xff, 0x77, 0x77, 0xff, 0x77, 0xbb, 0xdd, 0xee,
};

static void _sg_init(struct mmc_host *mmc, struct rt_mmcsd_data  *data)
{
    struct scatterlist *sg = mmc->_sg;
    unsigned char *buf = (unsigned char*)data->buf;
    unsigned int len = data->blks * data->blksize;

    data->sg_len = (len + (64*1024 - 1)) / (64*1024);
    data->sg = sg;

    BUG_ON(data->sg_len > 8);

    sg_init_table(sg, data->sg_len);
    for (rt_uint16_t i = 0; i < data->sg_len; i ++)
    {
        unsigned int l = (len > (64*1024))? 64*1024 : len;

        sg_set_buf(sg, buf, l);
        len -= l;
        buf += l;
        sg ++;
    }
}

static void _mmc_request(struct rt_mmcsd_host *host, struct rt_mmcsd_req *req)
{
    struct mmc_host *mmc = (struct mmc_host *)host;
    rt_uint32_t flags = req->cmd->flags;

    switch (flags & RESP_MASK)
    {
    case RESP_NONE:
        flags |= MMC_RSP_NONE;
        break;
    case RESP_R1:
        flags |= MMC_RSP_R1;
        break;
    case RESP_R1B:
        flags |= MMC_RSP_R1B;
        break;
    case RESP_R2:
        flags |= MMC_RSP_R2;
        break;
    case RESP_R3:
        flags |= MMC_RSP_R3;
        break;
    case RESP_R4:
        flags |= MMC_RSP_R4;
        break;
    case RESP_R5:
        flags |= MMC_RSP_R5;
        break;
    case RESP_R6:
        flags |= MMC_RSP_R6;
        break;
    case RESP_R7:
        flags |= MMC_RSP_R7;
        break;
    }

    if (req->data)
    {
        _sg_init(mmc, req->data);
        pr_debug("cmd(%d)\n", req->cmd->cmd_code);
    }

    req->cmd->flags |= flags;

    mmc->ops->request(mmc, req);
}

static void _set_iocfg(struct rt_mmcsd_host *host, struct rt_mmcsd_io_cfg *iocfg)
{
    struct mmc_host *mmc = (struct mmc_host *)host;

    pr_debug("clock:%d,width:%d,power:%d,vdd:%d,timing:%d\n",
             iocfg->clock, iocfg->bus_width,
             iocfg->power_mode, iocfg->vdd, iocfg->timing);

    mmc->ops->set_ios(mmc, iocfg);
}

static int _execute_tuning(struct rt_mmcsd_host *host, rt_int32_t opcode)
{
    struct mmc_host *mmc = (struct mmc_host *)host;

    return mmc->ops->execute_tuning(mmc, opcode);
}

static const struct rt_mmcsd_host_ops _rmhops = {
    .request = _mmc_request,
    .set_iocfg = _set_iocfg,
    .execute_tuning = _execute_tuning,
};

int mmc_add_host(struct mmc_host *mmc)
{
    mmc->_rmh.ops = &_rmhops;
    mmc->_rmh.flags = mmc->caps;
    mmc->_rmh.valid_ocr = VDD_165_195;
    mmc->_rmh.freq_min = 400000;
    rt_strncpy(mmc->_rmh.name, "emmc", sizeof(mmc->_rmh.name) - 1);

    mmc->max_seg_size = 512*1024;
    mmc->_rmh.max_dma_segs = 1;

    mmcsd_change(&mmc->_rmh);

    return 0;
}

struct mmc_host *mmc_alloc_host(int extra, struct rt_device *dev)
{
    struct mmc_host *mmc;

    mmc = rt_malloc(sizeof(*mmc) + extra);
    if (mmc)
    {
        rt_memset(mmc, 0, sizeof(*mmc) + extra);
        mmc->parent = dev;
        mmcsd_host_init(&mmc->_rmh);
    }

    return mmc;
}

void mmc_retune_needed(struct mmc_host *host)
{
    pr_todo();
}

int mmc_gpio_get_cd(struct mmc_host *host)
{
    return -ENOSYS;
}

int mmc_abort_tuning(struct mmc_host *host, rt_uint32_t opcode)
{
    pr_todo();
    return 0;
}

int mmc_of_parse(struct mmc_host *host)
{
    struct rt_device *dev = host->parent;
    rt_uint32_t bus_width;

    if (!dev || !dev->ofw_node)
        return 0;

    /* "bus-width" is translated to MMC_CAP_*_BIT_DATA flags */
    if (rt_dm_dev_prop_read_u32(dev, "bus-width", &bus_width) < 0)
    {
        dev_dbg(host->parent,
                "\"bus-width\" property is missing, assuming 1 bit.\n");
        bus_width = 1;
    }

    switch (bus_width)
    {
    case 8:
        host->caps |= MMC_CAP_8_BIT_DATA;
        fallthrough; /* Hosts capable of 8-bit can also do 4 bits */
    case 4:
        host->caps |= MMC_CAP_4_BIT_DATA;
        break;
    case 1:
        break;
    default:
        dev_err(host->parent,
                "Invalid \"bus-width\" value %u!\n", bus_width);
        return -EINVAL;
    }

    /* f_max is obtained from the optional "max-frequency" property */
    rt_dm_dev_prop_read_u32(dev, "max-frequency", &host->f_max);

    if (rt_dm_dev_prop_read_bool(dev, "cap-mmc-highspeed"))
    {
        host->caps |= MMC_CAP_MMC_HIGHSPEED;
    }

    if (rt_dm_dev_prop_read_bool(dev, "mmc-hs200-1_8v"))
    {
        host->caps |= MMC_CAP2_HS200_1_8V_SDR;
    }

    if (rt_dm_dev_prop_read_bool(dev, "non-removable"))
    {
        host->caps |= MMC_CAP_NONREMOVABLE;
    }

    if (rt_dm_dev_prop_read_bool(dev, "no-sdio"))
    {
        host->caps2 |= MMC_CAP2_NO_SDIO;
    }

    if (rt_dm_dev_prop_read_bool(dev, "no-sd"))
    {
        host->caps2 |= MMC_CAP2_NO_SD;
    }

    if (rt_dm_dev_prop_read_bool(dev, "mmc-ddr-3_3v"))
    {
        host->caps |= MMC_CAP_3_3V_DDR;
    }

    if (rt_dm_dev_prop_read_bool(dev, "mmc-ddr-1_8v"))
    {
        host->caps |= MMC_CAP_1_8V_DDR;
    }

    if (rt_dm_dev_prop_read_bool(dev, "mmc-ddr-1_2v"))
    {
        host->caps |= MMC_CAP_1_2V_DDR;
    }

    return 0;
}

void mmc_free_host(struct mmc_host *host)
{
    pr_todo();
}

int mmc_regulator_set_ocr(struct mmc_host *mmc,
                          struct regulator *supply,
                          unsigned short vdd_bit)
{
    pr_todo();
    return 0;
}

void mmc_request_done(struct mmc_host *host, struct mmc_request *mrq)
{
    mmcsd_req_complete(&host->_rmh);
}

void mmc_detect_change(struct mmc_host *host, unsigned long delay)
{
}

int mmc_regulator_get_supply(struct mmc_host *mmc)
{
    mmc->supply.vmmc = ERR_PTR(-ENODEV);
    mmc->supply.vqmmc = ERR_PTR(-ENODEV);

    return 0;
}

int mmc_gpio_get_ro(struct mmc_host *host)
{
    return 0;
}

bool mmc_can_gpio_ro(struct mmc_host *host)
{
    return false;
}

void mmc_command_done(struct mmc_host *host, struct mmc_request *mrq)
{
    pr_todo();
}

bool mmc_can_gpio_cd(struct mmc_host *host)
{
    return false;
}

int mmc_send_tuning(struct mmc_host *host, rt_uint32_t opcode, int *cmd_error)
{
    struct mmc_request mrq = {};
    struct mmc_command cmd = {};
    struct mmc_data data = {};
    struct mmc_ios *ios = &host->_rmh.io_cfg;
    const rt_uint8_t *tuning_block_pattern;
    int size, err = 0;
    rt_uint8_t *data_buf;

    if (ios->bus_width == MMC_BUS_WIDTH_8)
    {
        tuning_block_pattern = tuning_blk_pattern_8bit;
        size = sizeof(tuning_blk_pattern_8bit);
    }
    else if (ios->bus_width == MMC_BUS_WIDTH_4)
    {
        tuning_block_pattern = tuning_blk_pattern_4bit;
        size = sizeof(tuning_blk_pattern_4bit);
    }
    else
        return -EINVAL;

    data_buf = (rt_uint8_t*)rt_calloc(1, size);
    if (!data_buf)
        return -ENOMEM;

    mrq.cmd = &cmd;
    mrq.data = &data;

    cmd.opcode = opcode;
    cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

    data.blksz = size;
    data.blocks = 1;
    data.flags = MMC_DATA_READ;
    data.buf = (rt_uint32_t*)data_buf;

    /*
     * According to the tuning specs, Tuning process
     * is normally shorter 40 executions of CMD19,
     * and timeout value should be shorter than 150 ms
     */
    data.timeout_ns = 150 * NSEC_PER_MSEC;

    mmcsd_send_request(&host->_rmh, &mrq);

    if (cmd_error)
        *cmd_error = cmd.error;

    if (cmd.error)
    {
        err = cmd.error;
        goto out;
    }

    if (data.error)
    {
        err = data.error;
        goto out;
    }

    if (rt_memcmp(data_buf, tuning_block_pattern, size))
        err = -EIO;

out:
    rt_free(data_buf);
    return err;
}

int mmc_regulator_set_vqmmc(struct mmc_host *mmc, struct mmc_ios *ios)
{
    pr_todo();
    return 0;
}

void sdio_signal_irq(struct mmc_host *host)
{
}

void mmc_remove_host(struct mmc_host *host)
{
}
