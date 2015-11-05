#include "cdp_util.h"

void padto4(int *num)
{
    int n = *num;
    *num = n + (4 - (n % 4));
}