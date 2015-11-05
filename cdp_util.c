#include "cdp_util.h"

void padto4(int *num)
{
    if ((*num % 4) == 0) {
        return;
    }
    *num = *num + (4 - (*num % 4));
}