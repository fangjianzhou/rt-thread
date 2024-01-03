#include <osdep/port.h>

static struct property *__find_property(const struct device_node *np,
                                  const char *name,
                                  int *lenp)
{
    struct property *pp;

	if (!np)
		return NULL;

	for (pp = np->properties; pp; pp = pp->next) {
		if (strcmp(pp->name, name) == 0) {
			if (lenp)
				*lenp = pp->length;
			break;
		}
	}

    return pp;
}

bool of_property_read_bool(const struct device_node *np,
					 const char *propname)
{
	struct property *prop = __find_property(np, propname, NULL);

	return prop ? true : false;
}

bool device_property_present(struct device *dev, const char *propname)
{
    return of_property_read_bool(dev->of_node, propname);
}

bool device_property_read_bool(struct device *dev, const char *propname)
{
    return of_property_read_bool(dev->of_node, propname);
}

int of_property_read_u32(const struct device_node *np,
					   const char *propname, u32 *val)
{
    struct property *prop = __find_property(np, propname, NULL);

    if (!prop)
        return -EINVAL;
    
    *val = (u32)prop->value;

    return 0;
}

int device_property_read_u32(struct device *dev,
					   const char *propname, u32 *val)
{
    return of_property_read_u32(dev->of_node, propname, val);
}

int of_property_read_u64(const struct device_node *np,
					   const char *propname, u64 *val)
{
    struct property *prop = __find_property(np, propname, NULL);

    if (!prop)
        return -EINVAL;

    *val = (u64)prop->value;

    return 0;
}

int of_alias_get_id(struct device_node *np, const char *stem)
{
	pr_todo();
	return 0;
}

void dn_pp_set_and_add_u32(struct device_node *dn, struct property *p,
                           const char *name, u32 val)
{
    struct property *pre = dn->properties;

    dn->properties = p;
    p->next = pre;
    p->name = (char*)name;
    p->value = val;
    p->length = 4;
}

void dn_pp_set_and_add_bool(struct device_node *dn, struct property *p,
                           const char *name)
{
    struct property *pre = dn->properties;

    dn->properties = p;
    p->next = pre;
    p->name = (char*)name;
    p->value = 0;
    p->length = rt_strlen(name);
}
