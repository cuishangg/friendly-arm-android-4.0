#include "config.h"
#include <stdlib.h>
#include <stdio.h>

#define FANETWORK_CONF_FILE "/data/system/fanetwork.conf"
int g_config_isAutoConnect = 0;
int g_config_atCmdDeviceIndex = 0;
int g_config_buildinModem = 0;

int readConfig()
{
    FILE* f = fopen( FANETWORK_CONF_FILE, "r" );
    if (f == NULL) {
        return -1;
    } else {
        fclose(f);
    }

    int value_int;
    int ret;

    ret = getconfigint( "NET3G", "AutoConnect", &value_int, FANETWORK_CONF_FILE );
    if (ret == 0) {
        if (value_int == 0 || value_int == 1) {
            g_config_isAutoConnect = value_int;
        } else {
            return -2;
        }
    } else {
        return -3;
    }

    ret = getconfigint( "NET3G", "ATCmdDeviceIndex", &value_int, FANETWORK_CONF_FILE );
    if (ret == 0) {
        if (value_int >=0) {
            g_config_atCmdDeviceIndex = value_int;
        } else {
            return -4;
        }
    } else {
        return -5;
    }

    ret = getconfigint( "NET3G", "BuildInModem", &value_int, FANETWORK_CONF_FILE );
    if (ret == 0) {
        if (value_int==0 || value_int==1) {
            g_config_buildinModem = value_int;
        } else {
            return -6;
        }
    } else {
        return -7;
    }

    return 0;
}

