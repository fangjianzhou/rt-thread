#include <osdep/port.h>

int regulator_get_current_limit(struct regulator *regulator)
{
	return 0;
}

int regulator_is_supported_voltage(struct regulator *regulator,
				   int min_uV, int max_uV)
{
	return 0;
}
