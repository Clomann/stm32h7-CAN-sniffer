#pragma once

#include "mdns.h"
#include "igmp.h"

void app_mdns_init(struct netif *netif);

void app_mdns_poll(struct netif *netif);
