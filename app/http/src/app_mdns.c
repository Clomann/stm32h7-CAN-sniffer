#include "app_mdns.h"
#include "lwip/timeouts.h"
#include "lwip/sys.h"
#if NO_SYS==0
#include "lwip/tcpip.h"
#endif
#include <stdint.h>

static volatile int mdns_kick_timer_called = 0;
static volatile int mdns_init_called = 0;
static volatile int app_mdns_poll_called = 0;

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
    volatile int dbg_here;
    (void) dbg_here;
}

static void mdns_kick_timer(void *arg) {
    struct netif *netif = (struct netif *)arg;
    
    (void)mdns_kick_timer_called;
    mdns_kick_timer_called++;

    if (netif_is_up(netif) && !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
        /* ensure responder is alive and re-probe/announce */
        /* re-join the IPv4 mDNS multicast group in case IGMP membership was lost */
        ip_addr_t v4group;
        IP_ADDR4(&v4group, 224, 0, 0, 251);
        (void)igmp_joingroup_netif(netif, ip_2_ip4(&v4group));

        mdns_resp_restart(netif);
    }
    sys_timeout(30000, mdns_kick_timer, arg); // 30s; adjust if needed
}

static void mdns_restart_cb(void *arg) {
    struct netif *n = (struct netif *)arg;
    if (netif_is_up(n) && !ip4_addr_isany_val(*netif_ip4_addr(n))) {
        ip_addr_t v4group;
        IP_ADDR4(&v4group, 224, 0, 0, 251);
        (void)igmp_joingroup_netif(n, ip_2_ip4(&v4group));
        mdns_resp_restart(n);
    }
}
#endif

void app_mdns_init(struct netif *netif)
{
#if LWIP_MDNS_RESPONDER
    // Toggle LED or set a global flag you can check in debugger
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

        mdns_kick_timer(netif); /* arm watchdog */
    }
    // Check these error codes in debugger
    volatile err_t debug_err1 = err1;
    volatile s8_t debug_err2  = err2;
    (void)debug_err1;
    (void)debug_err2;
#endif
}

/**
 * @brief Periodic mDNS maintenance hook called from the main loop.
 *
 * Currently used as a watchdog to rejoin/restart mDNS if answers stop (e.g.,
 * IGMP membership dropped or responder state went stale). Future periodic
 * maintenance can be added here.
 *
 * ATTENTION: This restart loop is a workaround. The proper fix for spontaneous
 * loss of answers is to trace why mdns_recv stops seeing queries (IGMP loss,
 * responder state machine, or timer issues) and address that root cause.
 *
 * In NO_SYS builds this runs in the single-threaded LwIP context; with RTOS it
 * schedules into the TCP/IP thread via tcpip_callback_with_block.
 */
void app_mdns_poll(struct netif *netif)
{
    /* Workaround: periodic restart to recover lost IGMP/mDNS state.
     * Proper fix: find why mdns_recv stops seeing queries (e.g. IGMP loss/responder state).
     * Fallback watchdog in case the sys_timeout() based kick is not running */
    static u32_t last_restart_ms = 0;
    
    (void)app_mdns_poll_called;
    
    u32_t now = sys_now();
    if ((now - last_restart_ms) > 30000U) {
        app_mdns_poll_called++;
        last_restart_ms = now;

#if NO_SYS==0
        tcpip_callback_with_block(mdns_restart_cb, netif, 0);
#else
        /* In NO_SYS builds we run in the single-threaded LwIP context already */
        mdns_restart_cb(netif);
#endif
    }
}
