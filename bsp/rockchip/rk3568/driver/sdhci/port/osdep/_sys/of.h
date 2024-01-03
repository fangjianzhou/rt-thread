#ifndef _SDHCI_OF_H
#define _SDHCI_OF_H

#include "types.h"

struct device;

struct property
{
	char	*name;
	int	length;
	unsigned long value;
	struct property *next;
};

struct device_node
{
	const char *name;
	struct property *properties;
};

void dn_pp_set_and_add_u32(struct device_node *dn, struct property *p,
                           const char *name, u32 val);

void dn_pp_set_and_add_bool(struct device_node *dn, struct property *p,
                           const char *name);

bool of_property_read_bool(const struct device_node *np,
					 const char *propname);
int of_property_read_u32(const struct device_node *np,
					   const char *propname, u32 *val);
int of_property_read_u64(const struct device_node *np,
					   const char *propname, u64 *val);

bool device_property_read_bool(struct device *dev, const char *propname);
int device_property_read_u32(struct device *dev,
					   const char *propname, u32 *val);
bool device_property_present(struct device *dev, const char *propname);
extern int of_alias_get_id(struct device_node *np, const char *stem);

#endif
