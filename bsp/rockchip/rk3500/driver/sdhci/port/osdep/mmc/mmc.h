#ifndef _SDHCI_MMC_H
#define _SDHCI_MMC_H

#include <drivers/mmcsd_cmd.h>
#include <drivers/mmcsd_core.h>
#include <drivers/mmcsd_host.h>

#include <stdbool.h>
#include "../_sys/sg.h"
#include "../_sys/dma.h"

struct mmc_host;
struct device;
struct regulator;

#define mmc_request rt_mmcsd_req
#define mmc_command rt_mmcsd_cmd
#define mmc_data rt_mmcsd_data
#define mmc_ios rt_mmcsd_io_cfg
#define opcode cmd_code
#define error err
#define blksz blksize
#define blocks blks

struct mmc_supply
{
    void *vmmc;     /* Card power supply */
    void *vqmmc;    /* Optional Vccq supply */
};

struct mmc_cqe_ops
{

};

struct mmc_host_ops
{
    /*
     * It is optional for the host to implement pre_req and post_req in
     * order to support double buffering of requests (prepare one
     * request while another request is active).
     * pre_req() must always be followed by a post_req().
     * To undo a call made to pre_req(), call post_req() with
     * a nonzero err condition.
     */
    void    (*post_req)(struct mmc_host *host, struct mmc_request *req,
                int err);
    void    (*pre_req)(struct mmc_host *host, struct mmc_request *req);
    void    (*request)(struct mmc_host *host, struct mmc_request *req);

    /*
     * Avoid calling the next three functions too often or in a "fast
     * path", since underlaying controller might implement them in an
     * expensive and/or slow way. Also note that these functions might
     * sleep, so don't call them in the atomic contexts!
     */

    /*
     * Notes to the set_ios callback:
     * ios->clock might be 0. For some controllers, setting 0Hz
     * as any other frequency works. However, some controllers
     * explicitly need to disable the clock. Otherwise e.g. voltage
     * switching might fail because the SDCLK is not really quiet.
     */
    void    (*set_ios)(struct mmc_host *host, struct mmc_ios *ios);

    /*
     * Return values for the get_ro callback should be:
     *   0 for a read/write card
     *   1 for a read-only card
     *   -ENOSYS when not supported (equal to NULL callback)
     *   or a negative errno value when something bad happened
     */
    int (*get_ro)(struct mmc_host *host);

    /*
     * Return values for the get_cd callback should be:
     *   0 for a absent card
     *   1 for a present card
     *   -ENOSYS when not supported (equal to NULL callback)
     *   or a negative errno value when something bad happened
     */
    int (*get_cd)(struct mmc_host *host);

    void    (*enable_sdio_irq)(struct mmc_host *host, int enable);
    /* Mandatory callback when using MMC_CAP2_SDIO_IRQ_NOTHREAD. */
    void    (*ack_sdio_irq)(struct mmc_host *host);

    int (*start_signal_voltage_switch)(struct mmc_host *host, struct mmc_ios *ios);

    /* Check if the card is pulling dat[0:3] low */
    int (*card_busy)(struct mmc_host *host);

    /* The tuning command opcode value is different for SD and eMMC cards */
    int (*execute_tuning)(struct mmc_host *host, unsigned opcode);

    /* Prepare HS400 target operating frequency depending host driver */
    int (*prepare_hs400_tuning)(struct mmc_host *host, struct mmc_ios *ios);

    /* Prepare switch to DDR during the HS400 init sequence */
    int (*hs400_prepare_ddr)(struct mmc_host *host);

    /* Prepare for switching from HS400 to HS200 */
    void    (*hs400_downgrade)(struct mmc_host *host);

    /* Complete selection of HS400 */
    void    (*hs400_complete)(struct mmc_host *host);

    /* Prepare enhanced strobe depending host driver */
    void    (*hs400_enhanced_strobe)(struct mmc_host *host,
                     struct mmc_ios *ios);

    /* Reset the eMMC card via RST_n */
    void    (*hw_reset)(struct mmc_host *host);
    void    (*card_event)(struct mmc_host *host);
};

/* struct */
struct mmc_host
{
    struct rt_mmcsd_host _rmh;
    struct rt_device        *parent;

    struct mmc_cqe_ops *cqe_ops;
    const struct mmc_host_ops *ops;
    rt_uint32_t max_req_size;

    rt_uint32_t max_current_330;
    rt_uint32_t max_current_300;
    rt_uint32_t max_current_180;
    rt_uint32_t actual_clock;

    rt_uint32_t caps;
    rt_uint32_t caps2;
    rt_uint32_t pm_caps;
    rt_uint32_t ocr_avail;
    rt_uint32_t ocr_avail_mmc;
    rt_uint32_t ocr_avail_sd;
    rt_uint32_t ocr_avail_sdio;
    struct mmc_supply   supply;
    unsigned int        max_busy_timeout;
    unsigned int        retune_period;
    unsigned dma_mask;

 #define f_max _rmh.freq_max
 #define f_min _rmh.freq_min
 #define max_segs _rmh.max_dma_segs
 #define max_seg_size _rmh.max_seg_size
 #define max_blk_size _rmh.max_blk_size
 #define max_blk_count _rmh.max_blk_count
    struct mmc_ios ios;

    struct scatterlist _sg[8];

    unsigned long       private[];
};

/* cmd */
#define MMC_SEND_TUNING_BLOCK_HS200 SEND_TUNING_BLOCK_HS200
#define MMC_SEND_TUNING_BLOCK SEND_TUNING_BLOCK
#define MMC_STOP_TRANSMISSION STOP_TRANSMISSION
#define MMC_BUS_TEST_R           14   /* adtc                    R1  */
#define MMC_WRITE_MULTIPLE_BLOCK WRITE_MULTIPLE_BLOCK
#define MMC_READ_MULTIPLE_BLOCK READ_MULTIPLE_BLOCK

//
#define MMC_DATA_READ DATA_DIR_READ
#define MMC_DATA_WRITE DATA_DIR_WRITE

//
#define MMC_RSP_PRESENT (1 << 16)
#define MMC_RSP_136 (1 << 17)       /* 136 bit response */
#define MMC_RSP_CRC (1 << 18)       /* expect valid crc */
#define MMC_RSP_BUSY    (1 << 19)       /* card may send busy */
#define MMC_RSP_OPCODE  (1 << 20)       /* response contains opcode */

/*
 * These are the native response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_NONE    (0)
#define MMC_RSP_R1  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2  (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3  (MMC_RSP_PRESENT)
#define MMC_RSP_R4  (MMC_RSP_PRESENT)
#define MMC_RSP_R5  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R6  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R7  (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)

#define MMC_CMD_ADTC CMD_ADTC


/* caps */
#define MMC_CAP2_HS200_1_8V_SDR MMCSD_SUP_HS200_1V8
#define MMC_CAP_4_BIT_DATA MMCSD_BUSWIDTH_4
#define MMC_CAP_8_BIT_DATA MMCSD_BUSWIDTH_8
#define MMC_CAP2_HS200 MMCSD_SUP_HS200
#define MMC_CAP_MMC_HIGHSPEED MMCSD_SUP_HIGHSPEED
#define MMC_CAP_SD_HIGHSPEED MMCSD_SUP_HIGHSPEED
#define MMC_CAP_1_8V_DDR MMCSD_SUP_DDR_1V8
#define MMC_CAP_3_3V_DDR MMCSD_SUP_DDR_3V3
#define MMC_CAP_1_2V_DDR MMCSD_SUP_DDR_1V2
#define MMC_CAP_NONREMOVABLE MMCSD_SUP_NONREMOVABLE

#define MMC_CAP2_NO_SDIO    (1 << 19)
#define MMC_CAP2_NO_SD      (1 << 21)
#define MMC_CAP2_NO_MMC     (1 << 22)
#define MMC_CAP2_CQE        (1 << 23)

#define MMC_VDD_165_195 VDD_165_195
#define MMC_VDD_20_21 VDD_20_21
#define MMC_VDD_29_30 VDD_29_30
#define MMC_VDD_30_31 VDD_30_31
#define MMC_VDD_32_33 VDD_32_33
#define MMC_VDD_33_34 VDD_33_34

//{TODO
#define MMC_CAP_UHS_DDR50 0
#define MMC_CAP2_HS400 0
#define MMC_CAP_UHS_SDR50 0
#define MMC_CAP_UHS_SDR25 0
#define MMC_CAP_UHS_SDR12 0
#define MMC_CAP_UHS_SDR104 0
#define MMC_CAP_UHS 0
#define MMC_CAP2_HSX00_1_8V 0
#define MMC_CAP2_HS400_ES 0
#define MMC_CAP_NEEDS_POLL 0
#define MMC_CAP2_HSX00_1_2V 0
#define MMC_CAP2_HS400_1_8V 0
#define MMC_CAP_DRIVER_TYPE_D 0
#define MMC_CAP_DRIVER_TYPE_C 0
#define MMC_SET_DRIVER_TYPE_B 0
#define MMC_CAP_DRIVER_TYPE_A 0
#define MMC_CAP2_SDIO_IRQ_NOTHREAD 0
#define MMC_CAP_CMD23 0
#define MMC_CAP_SDIO_IRQ 0
//}

/* timing */
#define MMC_TIMING_UHS_DDR50 MMCSD_TIMING_UHS_DDR50
#define MMC_TIMING_UHS_SDR50 MMCSD_TIMING_UHS_SDR50
#define MMC_TIMING_MMC_HS200 MMCSD_TIMING_MMC_HS200
#define MMC_TIMING_MMC_HS400 MMCSD_TIMING_MMC_HS400
#define MMC_TIMING_UHS_SDR104 MMCSD_TIMING_UHS_SDR104
#define MMC_TIMING_UHS_SDR25 MMCSD_TIMING_UHS_SDR25
#define MMC_TIMING_MMC_DDR52 MMCSD_TIMING_MMC_DDR52
#define MMC_TIMING_UHS_SDR12 MMCSD_TIMING_UHS_SDR12
#define MMC_TIMING_SD_HS MMCSD_TIMING_SD_HS
#define  MMC_TIMING_MMC_HS  MMCSD_TIMING_MMC_HS

/* width */
#define MMC_BUS_WIDTH_8 MMCSD_BUS_WIDTH_8
#define MMC_BUS_WIDTH_4 MMCSD_BUS_WIDTH_4
#define MMC_BUS_WIDTH_1 MMCSD_BUS_WIDTH_1

/* v */
#define MMC_SIGNAL_VOLTAGE_330  0
#define MMC_SIGNAL_VOLTAGE_180  1
#define MMC_SIGNAL_VOLTAGE_120  2

/* power */
#define MMC_POWER_OFF       MMCSD_POWER_OFF
#define MMC_POWER_UP        MMCSD_POWER_UP
#define MMC_POWER_ON        MMCSD_POWER_ON
#define MMC_POWER_UNDEFINED 3

/* drive */
#define MMC_SET_DRIVER_TYPE_B   0
#define MMC_SET_DRIVER_TYPE_A   1
#define MMC_SET_DRIVER_TYPE_C   2
#define MMC_SET_DRIVER_TYPE_D   3

#define mmc_dev(x) ((x)->parent)
#define mmc_hostname(x) dev_name(mmc_dev(x))

static inline void *mmc_priv(struct mmc_host *host)
{
    return (void *)host->private;
}

static inline int mmc_op_multi(unsigned opcode)
{
    return opcode == MMC_WRITE_MULTIPLE_BLOCK ||
           opcode == MMC_READ_MULTIPLE_BLOCK;
}

static inline int mmc_card_is_removable(struct mmc_host *host)
{
    return !(host->caps & MMC_CAP_NONREMOVABLE);
}

static inline enum dma_data_direction mmc_get_dma_dir(struct mmc_data *data)
{
    return data->flags & MMC_DATA_WRITE ? DMA_TO_DEVICE : DMA_FROM_DEVICE;
}

int mmc_of_parse(struct mmc_host *host);
int mmc_add_host(struct mmc_host *host);
struct mmc_host *mmc_alloc_host(int extra, struct rt_device *dev);
void mmc_request_done(struct mmc_host *host, struct mmc_request *mrq);
void mmc_free_host(struct mmc_host *host);
int mmc_regulator_get_supply(struct mmc_host *mmc);
void mmc_retune_needed(struct mmc_host *host);
int mmc_regulator_set_vqmmc(struct mmc_host *mmc, struct mmc_ios *ios);
int mmc_gpio_get_ro(struct mmc_host *host);
int mmc_gpio_get_cd(struct mmc_host *host);
bool mmc_can_gpio_ro(struct mmc_host *host);
void mmc_command_done(struct mmc_host *host, struct mmc_request *mrq);
int mmc_abort_tuning(struct mmc_host *host, unsigned int opcode);
int mmc_regulator_set_ocr(struct mmc_host *mmc,
            struct regulator *supply,
            unsigned short vdd_bit);
void mmc_remove_host(struct mmc_host *);
bool mmc_can_gpio_cd(struct mmc_host *host);
void mmc_detect_change(struct mmc_host *, unsigned long delay);
int mmc_send_tuning(struct mmc_host *host, rt_uint32_t opcode, int *cmd_error);
void sdio_signal_irq(struct mmc_host *host);

#endif
