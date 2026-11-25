#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>

bool wifi_is_connected(void);
bool wifi_connect(void);
void wifi_disconnect(void);

#endif /* WIFI_MANAGER_H */
