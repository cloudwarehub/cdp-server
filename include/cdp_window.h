#ifndef CDP_WINDOW_H
#define CDP_WINDOW_H

#include "cdp_server.h"
#include "cdp_protocol.h"

cdp_window_t *cdp_window_configure(u32 wid, i16 x, i16 y, u16 width, u16 height, u8 override, u32 above);
#endif