#include <app_version.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wallckr, LOG_LEVEL_INF);

int main()
{
    LOG_INF("wallckr-z2 %s", APP_VERSION_EXTENDED_STRING);
    return 0;
}
