#include <osdep/port.h>

#include <rtthread.h>

static void *_alloc(size_t n, size_t size, gfp_t flags)
{
    void *p;

    if (flags & __GFP_ZERO)
        p = rt_calloc(n, size);
    else
        p = rt_malloc(n * size);

    return p;
}

void *kvzalloc(size_t size, gfp_t flags)
{
    return _alloc(1, size, flags | __GFP_ZERO);
}

void *devm_kzalloc(struct rt_device *dev, size_t size, gfp_t gfp)
{
    return kvzalloc(size, gfp);
}

void *devm_kmalloc(struct rt_device *dev, size_t size, gfp_t gfp)
{
    return _alloc(1, size, gfp);
}
