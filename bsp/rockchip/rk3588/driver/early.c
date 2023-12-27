#include <rthw.h>

void *_s = 0xfeb50000;
void earlycon_str(const char *str)
{
    while (*str)
    {
        while ((HWREG32(_s + (0x1f << 2)) & 0x2) == 0)
        {
        }

        HWREG32(_s) = *str++;
    }
}

void earlycon_hex(rt_ubase_t val)
{
    char str[sizeof("0123456789abcdef")];

    str[16] = 0;

    for (int i = 15; i >= 0; --i)
    {
        str[i] = "0123456789abcdef"[(val & 0xf)];

        val >>= 4;
    }

    earlycon_str(str);
    earlycon_str("\n\r");
}
