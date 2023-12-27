/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-3-1       zhouxiaohu   first version
 * 2023-5-18      GuEe-GUI     implemented by rtatomic
 */

#ifndef __UTIL_REF_H__
#define __UTIL_REF_H__

#include <rtatomic.h>

/**
 * struct rt_ref must be embedded in an object.
 * it acts as a rt_reference counter for the object.
 */
struct rt_ref
{
    rt_atomic_t rt_refcount;
};

#define RT_REF_INIT(n)  { .rt_refcount = n, }

rt_inline void rt_ref_init(struct rt_ref *r)
{
    rt_atomic_store(&r->rt_refcount, 1);
}

rt_inline unsigned int rt_ref_read(struct rt_ref *r)
{
    return rt_atomic_load(&r->rt_refcount);
}

/**
 * rt_ref_get
 * increment rt_reference counter for object.
 */
rt_inline void rt_ref_get(struct rt_ref *r)
{
    rt_atomic_add(&r->rt_refcount, 1);
}

/**
 * rt_ref_put
 * decrement rt_reference counter for object.
 * If the rt_reference counter is zero, call release().
 *
 * Return 1 means the object's rt_reference counter is zero and release() is called.
 */
rt_inline int rt_ref_put(struct rt_ref *r, void (*release)(struct rt_ref *r))
{
    if (rt_atomic_dec_and_test(&r->rt_refcount))
    {
        release(r);

        return 1;
    }

    return 0;
}

/**
 * rt_ref_get_unless_zero - Increment rt_refcount for object unless it is zero.
 * Return non-zero if the increment succeeded. Otherwise return 0.
 */
rt_inline int rt_ref_get_unless_zero(struct rt_ref *r)
{
    return (int)rt_atomic_inc_not_zero(&r->rt_refcount);
}

#endif /* __UTIL_REF_H__ */
