#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#define DBG_TAG "i2c.rk3x"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#include "../i2c_dm.h"

/************************** I2C Registers *************************************/
#define TCA6424A_INPUT_REG0			0x00		// Input status register
#define TCA6424A_INPUT_REG1			0x01		// Input status register
#define TCA6424A_INPUT_REG2			0x02		// Input status register
#define TCA6424A_OUTPUT_REG0		0x04		// Output register to change state of output BIT set to 1, output set HIGH
#define TCA6424A_OUTPUT_REG1		0x05		// Output register to change state of output BIT set to 1, output set HIGH
#define TCA6424A_OUTPUT_REG2		0x06		// Output register to change state of output BIT set to 1, output set HIGH
#define TCA6424A_POLARITY_REG0 	    0x08		// Polarity inversion register. BIT '1' inverts input polarity of register 0x00
#define TCA6424A_POLARITY_REG1 	    0x09		// Polarity inversion register. BIT '1' inverts input polarity of register 0x00
#define TCA6424A_POLARITY_REG2 	    0x0A		// Polarity inversion register. BIT '1' inverts input polarity of register 0x00
#define TCA6424A_CONFIG_REG0   	    0x0C		// Configuration register. BIT = '1' sets port to input BIT = '0' sets port to output
#define TCA6424A_CONFIG_REG1   	    0x0D		// Configuration register. BIT = '1' sets port to input BIT = '0' sets port to output
#define TCA6424A_CONFIG_REG2   	    0x0E		// Configuration register. BIT = '1' sets port to input BIT = '0' sets port to output


struct tca6424_io
{
    struct rt_device parent;
    struct rt_i2c_client *client;
};


#define raw_to_tca6424(raw) ((struct tca6424_io *)((raw)->user_data))

/* 读传感器寄存器数据 */
static rt_err_t read_regs(struct rt_i2c_client *client, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = client->client_addr;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    /* 调用I2C设备接口传输数据 */
    if (rt_i2c_transfer(client->bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/* 写传感器寄存器 */
static rt_err_t write_reg(struct rt_i2c_client *client, rt_uint8_t reg, rt_uint8_t *data)
{
    rt_uint8_t buf[2];
    struct rt_i2c_msg msgs;
    rt_uint32_t buf_size = 1;

    buf[0] = reg; //cmd
    if (data != RT_NULL)
    {
        buf[1] = *data;
        buf_size = 2;
    }

    msgs.addr = client->client_addr;
    msgs.flags = RT_I2C_WR;
    msgs.buf = buf;
    msgs.len = buf_size;

    /* 调用I2C设备接口传输数据 */
    if (rt_i2c_transfer(client->bus, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

// static void tca6424_pin_mode(rt_device_t dev, rt_base_t pin, rt_uint8_t mode)
// {
//     rt_uint8_t data[1];
//     rt_uint8_t tca6424_bit_pin = pin & 0x07;
//     rt_uint8_t tca6424_pin_group = ((pin & 0x18) >> 3) + TCA6424A_CONFIG_REG0;
//     struct tca6424_io *tca6424 = raw_to_tca6424(dev);

//     write_reg(tca6424->client, tca6424_pin_group, RT_NULL);
//     read_regs(tca6424->client, 1, data);

//     switch (mode)
//     {
//     case PIN_MODE_OUTPUT:
//         data[0] &= ~RT_BIT(tca6424_bit_pin);
//         break;
//     case PIN_MODE_INPUT:
//         data[0] |= RT_BIT(tca6424_bit_pin);
//         break;
//     default:
//         break;
//     }
//     write_reg(tca6424->client, tca6424_pin_group, data);
// }

static rt_ssize_t tca6424_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    rt_uint8_t data;
    rt_uint8_t tca6424_bit_pin = pos & 0x07;
    rt_uint8_t tca6424_pin_group = ((pos & 0x18) >> 3) + TCA6424A_CONFIG_REG0;

    struct tca6424_io *tca6424 = raw_to_tca6424(dev);

    write_reg(tca6424->client, tca6424_pin_group, RT_NULL);
    read_regs(tca6424->client, 1, &data);

    tca6424_pin_group -= (((data >> tca6424_bit_pin) & 0x01) + 2) * 4;

    write_reg(tca6424->client, tca6424_pin_group, RT_NULL);
    read_regs(tca6424->client, 1, &data);

    rt_kprintf("*value : %d\r\n",(data >> tca6424_bit_pin) & 0x01);
    *((rt_uint8_t *)buffer) = (data >> tca6424_bit_pin) & 0x01;
    
    return (rt_size_t)1;
}

static rt_ssize_t tca6424_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    rt_uint8_t data;
    rt_uint8_t *value = (rt_uint8_t *)buffer;
    rt_uint8_t tca6424_bit_pin = pos & 0x07;
    rt_uint8_t tca6424_pin_group = ((pos & 0x18) >> 3) + TCA6424A_CONFIG_REG0;
    struct tca6424_io *tca6424 = raw_to_tca6424(dev);

    write_reg(tca6424->client, tca6424_pin_group, RT_NULL);
    read_regs(tca6424->client, 1, &data);

    rt_kprintf("data : %d\r\n",data);
    if(!((data >> tca6424_bit_pin) & 0x01))
    {
        tca6424_pin_group -= (TCA6424A_CONFIG_REG0 - TCA6424A_OUTPUT_REG0);
        write_reg(tca6424->client, tca6424_pin_group, RT_NULL);
        read_regs(tca6424->client, 1, &data);
        
        if (*value){
            data |= RT_BIT(tca6424_bit_pin);
        }else{
            data &= ~RT_BIT(tca6424_bit_pin);
        }
        write_reg(tca6424->client, tca6424_pin_group, &data);

        return (rt_size_t)1;
    }
    return (rt_size_t)0;
}

static rt_err_t tca6424_control(rt_device_t dev, int cmd, void *args)
{
    struct tca6424_io *tca6424 = raw_to_tca6424(dev);
    if(cmd & 0x10)
    {
        write_reg(tca6424->client, cmd & ~0x10, RT_NULL);
        read_regs(tca6424->client, 1, (rt_uint8_t *)args);
    }
    else
        write_reg(tca6424->client, cmd, (rt_uint8_t *)args);
    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops tca6424_ops =
{
    .read = tca6424_read,
    .write = tca6424_write,
    .control = tca6424_control,
};
#endif

static rt_err_t ti_i2c_probe(struct rt_i2c_client *client)
{
    struct tca6424_io *tca6424 = rt_calloc(1, sizeof(*tca6424));
    tca6424->client = client;

    tca6424->parent.type         = RT_Device_Class_Pin;
    tca6424->parent.user_data = tca6424;
#ifdef RT_USING_DEVICE_OPS
    tca6424->parent.ops         = &tca6424_ops;
#else
    tca6424->parent.init        = RT_NULL;
    tca6424->parent.open        = RT_NULL;
    tca6424->parent.close       = RT_NULL;
    tca6424->parent.read        = tca6424_read;
    tca6424->parent.write       = tca6424_write;
    tca6424->parent.control     = tca6424_control;
#endif

    return rt_device_register(&tca6424->parent, "tca6424", RT_DEVICE_FLAG_RDWR);;
}

static const struct rt_i2c_device_id tca6424_ids[] =
{
    { .name = "tca6424" },
    { /* sentinel */ },
};

static const struct rt_ofw_node_id ti_i2c_ofw_ids[] =
{
    { .compatible = "ti,tca6424"},
    { /* sentinel */ }
};

static struct rt_i2c_driver tca6424_i2c_driver =
{
    .ids = tca6424_ids,
    .ofw_ids = ti_i2c_ofw_ids,
    .probe = ti_i2c_probe,
};

RT_I2C_DRIVER_EXPORT(tca6424_i2c_driver);
