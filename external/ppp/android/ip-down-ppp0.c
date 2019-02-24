/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/route.h>

#include <android/log.h>
#include <cutils/properties.h>

int main(int argc, char **argv)
{
/* args is like this:
    argv[1] = ifname;
    argv[2] = devnam;
    argv[3] = strspeed;
    argv[4] = strlocal;
    argv[5] = strremote;
    argv[6] = ipparam;
*/
    if (argc < 6)
	return EINVAL;

    __android_log_print(ANDROID_LOG_INFO, "ip-down-ppp0", "ip-down-ppp0 script is launched with \n");
    __android_log_print(ANDROID_LOG_INFO, "ip-down-ppp0", "\tInterface\t:\t%s", 	argv[1]);
    __android_log_print(ANDROID_LOG_INFO, "ip-down-ppp0", "\tTTY device\t:\t%s",	argv[2]);
    __android_log_print(ANDROID_LOG_INFO, "ip-down-ppp0", "\tTTY speed\t:\t%s",	argv[3]);
    __android_log_print(ANDROID_LOG_INFO, "ip-down-ppp0", "\tLocal IP\t:\t%s",	argv[4]);
    __android_log_print(ANDROID_LOG_INFO, "ip-down-ppp0", "\tRemote IP\t:\t%s",	argv[5]);

    /* We don't need delete "default" entry in route table since it will be deleted 
       automatically by kernel when ppp0 interface is down
    */

    /* Reset all those properties like net.ppp0.xxx which are set by ip-up-ppp0 */
    {
	property_set("net.ppp0.local-ip", "");
	property_set("net.ppp0.remote-ip", "");
	property_set("net.ppp0.dns1", "");
	property_set("net.ppp0.dns2", "");
	property_set("net.ppp0.gw", "");
    }
    return 0;
}
