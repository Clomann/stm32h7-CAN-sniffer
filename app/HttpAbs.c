#include "HttpAbs.h"

#include "lwip.h"
#include "lwip/init.h"
#include "lwiperf.h"
#include "tcp_echoserver.h"
#include "ethernetif.h"
#include "app_ethernet.h"

#include "httpd.h"
#include "http_cgi_ssi.h"

static struct netif * gnetif;

void http_init(void)
{
    /* Initialize the LwIP stack */
    lwip_init();

    /* init code for LWIP */
    gnetif = MX_LWIP_Init();

    /* TCP echo server Init */
    tcp_echoserver_init();

    http_server_init();
}

void http_poll(void)
{
    static struct netif * netif;

    netif = gnetif;

    /* Read a received packet from the Ethernet buffers and send it
        to the lwIP for handling */
    ethernetif_input(netif);

    /* Handle timeouts */
    sys_check_timeouts();

#if LWIP_NETIF_LINK_CALLBACK
    Ethernet_Link_Periodic_Handle(netif);
#endif

#if LWIP_DHCP
    DHCP_Periodic_Handle(netif);
#endif

    app_mdns_poll(netif);
}
