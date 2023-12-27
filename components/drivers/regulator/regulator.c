/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-09-23     GuEe-GUI     first version
 */

#include <rtthread.h>
#include <rtservice.h>

#define DBG_TAG "rtdm.regulator"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#include <drivers/ofw.h>
#include <drivers/regulator.h>

struct rt_regulator
{
    struct rt_regulator_node *reg_np;
};

static struct rt_spinlock _regulator_lock = { 0 };

static void regulator_release(struct rt_ref *r)
{
    struct rt_regulator_node *reg_np = rt_container_of(r, struct rt_regulator_node, rt_ref);

    LOG_E("%s is release", reg_np->supply_name);

    RT_ASSERT(0);
}

rt_inline struct rt_regulator_node *regulator_get(struct rt_regulator_node *reg_np)
{
    rt_ref_get(&reg_np->rt_ref);

    return reg_np;
}

rt_inline void regulator_put(struct rt_regulator_node *reg_np)
{
    rt_ref_put(&reg_np->rt_ref, &regulator_release);
}

rt_err_t rt_regulator_register(struct rt_regulator_node *reg_np)
{
    const struct rt_regulator_param *param;

    if (!reg_np || !reg_np->dev || !reg_np->param || !reg_np->ops)
    {
        return -RT_EINVAL;
    }

    rt_list_init(&reg_np->list);
    rt_list_init(&reg_np->children_nodes);
    rt_list_init(&reg_np->notifier_nodes);
    rt_ref_init(&reg_np->rt_ref);

    param = reg_np->param;

    reg_np->parent = RT_NULL;
    reg_np->is_enabled = param->boot_on || param->always_on;

#ifdef RT_USING_OFW
    if (reg_np->dev->ofw_node)
    {
        rt_ofw_data(reg_np->dev->ofw_node) = reg_np;
    }
#endif /* RT_USING_OFW */

    return RT_EOK;
}

rt_err_t rt_regulator_unregister(struct rt_regulator_node *reg_np)
{
    rt_err_t err = RT_EOK;

    if (!reg_np)
    {
        return -RT_EINVAL;
    }

    rt_spin_lock(&_regulator_lock);

    if (reg_np->is_enabled &&
        !(reg_np->param->boot_on || reg_np->param->always_on))
    {
        err = -RT_EBUSY;

        LOG_E("%s was enabled by consumer", reg_np->supply_name);

        goto _unlock;
    }

    if (!rt_list_isempty(&reg_np->children_nodes) ||
        rt_ref_read(&reg_np->rt_ref) > 1)
    {
        err = -RT_EBUSY;

        goto _unlock;
    }

    reg_np->parent = RT_NULL;
    rt_list_remove(&reg_np->list);

_unlock:
    rt_spin_unlock(&_regulator_lock);

    return err;
}

rt_err_t rt_regulator_notifier_register(struct rt_regulator *reg,
        struct rt_regulator_notifier *notifier)
{
    struct rt_regulator_node *reg_np;

    if (!reg || !notifier)
    {
        return -RT_EINVAL;
    }

    rt_spin_lock(&_regulator_lock);

    reg_np = reg->reg_np;
    notifier->regulator = reg;

    rt_list_init(&notifier->list);
    rt_list_insert_after(&reg_np->notifier_nodes, &notifier->list);

    rt_spin_unlock(&_regulator_lock);

    return RT_EOK;
}

rt_err_t rt_regulator_notifier_unregister(struct rt_regulator *reg,
        struct rt_regulator_notifier *notifier)
{
    if (!reg || !notifier)
    {
        return -RT_EINVAL;
    }

    rt_spin_lock(&_regulator_lock);

    rt_list_remove(&notifier->list);

    rt_spin_unlock(&_regulator_lock);

    return RT_EOK;
}

static rt_err_t regulator_notifier_call_chain(struct rt_regulator_node *reg_np,
        rt_ubase_t msg, void *data)
{
    rt_err_t err = RT_EOK;
    struct rt_regulator_notifier *notifier;
    rt_list_t *head = &reg_np->notifier_nodes;

    if (rt_list_isempty(head))
    {
        return err;
    }

    rt_list_for_each_entry(notifier, head, list)
    {
        err = notifier->callback(notifier, msg, data);

        if (err == -RT_EIO)
        {
            break;
        }
    }

    return err;
}

static rt_err_t regulator_enable(struct rt_regulator_node *reg_np)
{
    rt_err_t err = RT_EOK;

    if (reg_np->ops->enable)
    {
        err = reg_np->ops->enable(reg_np);

        if (!err)
        {
            reg_np->is_enabled = RT_TRUE;
            err = regulator_notifier_call_chain(reg_np, RT_REGULATOR_MSG_ENABLE, RT_NULL);
        }
    }

    if (!err && reg_np->parent)
    {
        err = regulator_enable(reg_np->parent);
    }

    return err;
}

rt_err_t rt_regulator_enable(struct rt_regulator *reg)
{
    rt_err_t err;

    if (!reg)
    {
        return -RT_EINVAL;
    }

    if (rt_regulator_is_enabled(reg))
    {
        return RT_EOK;
    }

    rt_spin_lock(&_regulator_lock);

    err = regulator_enable(reg->reg_np);

    rt_spin_unlock(&_regulator_lock);

    return err;
}

static rt_err_t regulator_disable(struct rt_regulator_node *reg_np)
{
    rt_err_t err = RT_EOK;

    if (reg_np->ops->disable)
    {
        err = reg_np->ops->disable(reg_np);

        if (!err)
        {
            err = regulator_notifier_call_chain(reg_np, RT_REGULATOR_MSG_DISABLE, RT_NULL);
        }
    }

    if (!err && reg_np->parent)
    {
        err = regulator_disable(reg_np->parent);
    }

    return err;
}

rt_err_t rt_regulator_disable(struct rt_regulator *reg)
{
    rt_err_t err;

    if (!reg)
    {
        return -RT_EINVAL;
    }

    if (!rt_regulator_is_enabled(reg))
    {
        return RT_EOK;
    }

    rt_spin_lock(&_regulator_lock);

    err = regulator_disable(reg->reg_np);

    rt_spin_unlock(&_regulator_lock);

    return err;
}

rt_bool_t rt_regulator_is_enabled(struct rt_regulator *reg)
{
    if (!reg)
    {
        return -RT_EINVAL;
    }

    return reg->reg_np->is_enabled;
}

static rt_err_t regulator_set_voltage(struct rt_regulator_node *reg_np, int min_uvolt, int max_uvolt)
{
    rt_err_t err = RT_EOK;

    if (reg_np->ops->set_voltage)
    {
        union rt_regulator_notifier_args args;

        RT_ASSERT(reg_np->ops->get_voltage != RT_NULL);

        args.old_uvolt = reg_np->ops->get_voltage(reg_np);
        args.min_uvolt = min_uvolt;
        args.max_uvolt = max_uvolt;

        err = regulator_notifier_call_chain(reg_np, RT_REGULATOR_MSG_VOLTAGE_CHANGE, &args);

        if (!err)
        {
            err = reg_np->ops->set_voltage(reg_np, min_uvolt, max_uvolt);
        }

        if (err)
        {
            regulator_notifier_call_chain(reg_np, RT_REGULATOR_MSG_VOLTAGE_CHANGE_ERR,
                    (void *)(rt_base_t)args.old_uvolt);
        }
    }

    if (!err && reg_np->parent)
    {
        err = regulator_set_voltage(reg_np->parent, min_uvolt, max_uvolt);
    }

    return err;
}

rt_bool_t rt_regulator_is_supported_voltage(struct rt_regulator *reg, int min_uvolt, int max_uvolt)
{
    const struct rt_regulator_param *param;

    RT_ASSERT(reg != RT_NULL);

    param = reg->reg_np->param;

    if (!param)
    {
        return RT_FALSE;
    }

    return param->min_uvolt <= min_uvolt && param->max_uvolt >= max_uvolt;
}

rt_err_t rt_regulator_set_voltage(struct rt_regulator *reg, int min_uvolt, int max_uvolt)
{
    rt_err_t err;

    if (!reg)
    {
        return -RT_EINVAL;
    }

    rt_spin_lock(&_regulator_lock);

    err = regulator_set_voltage(reg->reg_np, min_uvolt, max_uvolt);

    rt_spin_unlock(&_regulator_lock);

    return err;
}

int rt_regulator_get_voltage(struct rt_regulator *reg)
{
    int uvolt = RT_REGULATOR_UVOLT_INVALID;
    struct rt_regulator_node *reg_np;

    if (!reg)
    {
        return -RT_EINVAL;
    }

    rt_spin_lock(&_regulator_lock);

    reg_np = reg->reg_np;

    if (reg_np->ops->get_voltage)
    {
        uvolt = reg_np->ops->get_voltage(reg->reg_np);
    }
    else
    {
        uvolt = -RT_ENOSYS;
    }

    rt_spin_unlock(&_regulator_lock);

    return uvolt;
}

static void regulator_check_parent(struct rt_regulator_node *reg_np)
{
    if (reg_np->parent)
    {
        return;
    }
    else
    {
    #ifdef RT_USING_OFW
        rt_phandle parent_phandle = 0;
        struct rt_ofw_node *np = reg_np->dev->ofw_node;

        while (np)
        {
            if (rt_ofw_prop_read_u32(np, "vin-supply", &parent_phandle))
            {
                break;
            }

            if (!(np = rt_ofw_find_node_by_phandle(parent_phandle)))
            {
                break;
            }

            if (!(reg_np->parent = rt_ofw_data(np)))
            {
                LOG_W("%s parent ofw node = %s not init",
                        reg_np->supply_name, rt_ofw_node_full_name(np));

                break;
            }

            rt_list_insert_after(&reg_np->parent->children_nodes, &reg_np->list);
            rt_ofw_node_put(np);
        }
    #endif
    }
}

struct rt_regulator *rt_regulator_get_optional(struct rt_device *dev, const char *id)
{
    struct rt_regulator *reg = RT_NULL;
    struct rt_regulator_node *reg_np = RT_NULL;

    if (!dev || !id)
    {
        goto _end;
    }

#ifdef RT_USING_OFW
    if (dev->ofw_node)
    {
        rt_phandle supply_phandle;
        struct rt_ofw_node *np = dev->ofw_node;
        char supply_name[64];

        rt_snprintf(supply_name, sizeof(supply_name), "%s-supply", id);

        if (rt_ofw_prop_read_u32(np, supply_name, &supply_phandle))
        {
            goto _end;
        }

        if (!(np = rt_ofw_find_node_by_phandle(supply_phandle)))
        {
            goto _end;
        }

        reg_np = rt_ofw_data(np);
    }
#endif

    if (!reg_np)
    {
        goto _end;
    }

    rt_spin_lock(&_regulator_lock);

    regulator_check_parent(reg_np);

    rt_spin_unlock(&_regulator_lock);

    reg = rt_calloc(1, sizeof(*reg));

    if (!reg)
    {
        goto _end;
    }

    reg->reg_np = reg_np;

_end:
    return reg;
}

void rt_regulator_put(struct rt_regulator *reg)
{
    if (reg)
    {
        regulator_put(reg->reg_np);
        rt_free(reg);
    }
}
