// include/tun_manager.h

#ifndef TUN_MANAGER_H
#define TUN_MANAGER_H

#include <stdint.h>
#include <sys/types.h>

// Create TUN Interface
int create_tun_interface(const char *dev_name);

// TUN Interface IP Configuration
// dev: device name
// ip: IP
// netmask: subnet mask beat
// return 0, -1(fail)
int configure_tun_ip(const char *dev, const char *ip, int netmask);

// Turn ON TUN Interface
int bring_tun_up(const char  *dev);

// IP packet stdout
void print_ip_packet(const uint8_t *packet, ssize_t len);

#endif // TUN MANAGER
