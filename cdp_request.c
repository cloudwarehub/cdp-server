#include "cdp_protocol.h"

void cdp_request_window_resize(cdp_request_window_resize_t *req)
{
    cdp_window_resize(req->wid, req->width, req->height);
    cdp_stream_resize(req->wid, req->width, req->height);
}