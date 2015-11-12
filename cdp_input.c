#include "cdp_protocol.h"

void cdp_input_mousemove(cdp_request_mousemove_t *input)
{
    cdp_x11_input_mousemove(input->wid, input->x, input->y);
}