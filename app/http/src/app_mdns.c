#include "app_mdns.h"

#if LWIP_MDNS_RESPONDER
static void srv_txt(struct mdns_service *service, void *txt_userdata)
{
    err_t res;
    LWIP_UNUSED_ARG(txt_userdata);
    res = mdns_resp_add_service_txtitem(service, "path=/", 6);
    // LWIP_ERROR("mdns add service txt failed\n", (res == ERR_OK), return);
}
#endif

#if LWIP_MDNS_RESPONDER
static void app_mdns_report(struct netif *netif, u8_t result)
{
    // some diag output function
}
#endif

void app_mdns_init(struct netif *netif)
{
#if LWIP_MDNS_RESPONDER
    // Toggle LED or set a global flag you can check in debugger
    static volatile int mdns_init_called = 0;
    (void)mdns_init_called;
    mdns_init_called = 1; // Check this in debugger

    mdns_resp_register_name_result_cb(app_mdns_report);

    (void) mdns_resp_init();
    err_t err1;
    s8_t err2;

    if (1)
    {
        err1 = mdns_resp_add_netif(netif, "can-sniffer", 255);
        err2 = mdns_resp_add_service(
            netif,
            "can-sniffer",
            "_http",
            DNSSD_PROTO_TCP,
            80,
            255,
            srv_txt,
            NULL
        );
    }
    // Check these error codes in debugger
    volatile err_t debug_err1 = err1;
    volatile s8_t debug_err2  = err2;
    (void)debug_err1;
    (void)debug_err2;
#endif
}
