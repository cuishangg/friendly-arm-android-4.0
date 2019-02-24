/* //device/system/rild/rild.c
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <features.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <string.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

#include <telephony/ril.h>
#define LOG_TAG "RILD"
#include <utils/Log.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include <linux/capability.h>
#include <linux/prctl.h>

#include <private/android_filesystem_config.h>
#include "hardware/qemu_pipe.h"

#define LIB_PATH_PROPERTY   "rild.libpath"
#define LIB_ARGS_PROPERTY   "rild.libargs"
#define MAX_LIB_ARGS        16

static void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s -l <ril impl library> [-- <args for impl library>]\n", argv0);
    exit(-1);
}

extern void RIL_register (const RIL_RadioFunctions *callbacks);

extern void RIL_onRequestComplete(RIL_Token t, RIL_Errno e,
    void *response, size_t responselen);

extern void RIL_onUnsolicitedResponse(int unsolResponse, const void *data,
    size_t datalen);

extern void RIL_requestTimedCallback (RIL_TimedCallback callback,
    void *param, const struct timeval *relativeTime);


static struct RIL_Env s_rilEnv = {
    RIL_onRequestComplete,
    RIL_onUnsolicitedResponse,
    RIL_requestTimedCallback
};

extern void RIL_startEventLoop();

static int make_argv(char * args, char ** argv)
{
    // Note: reserve argv[0]
    int count = 1;
    char * tok;
    char * s = args;

    while ((tok = strtok(s, " \0"))) {
        argv[count] = tok;
        s = NULL;
        count++;
    }
    return count;
}

/*
 * switchUser - Switches UID to radio, preserving CAP_NET_ADMIN capabilities.
 * Our group, cache, was set by init.
 */
void switchUser() {
    prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0);
    setuid(AID_RADIO);

    struct __user_cap_header_struct header;
    struct __user_cap_data_struct cap;
    header.version = _LINUX_CAPABILITY_VERSION;
    header.pid = 0;
    cap.effective = cap.permitted = (1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW);
    cap.inheritable = 0;
    capset(&header, &cap);
}

////////////////////////////////////////////
// by F@R@I@E@N@D@L@Y@A@R@M
#include "config.h"
static int _debug = 0;
#define LOG_FILE_NAME "/var/run/rild.log"
static void _log2file(const char* fmt, va_list vl)
{
    static int firstTime = 1;
    FILE* file_out;
    file_out = fopen(LOG_FILE_NAME,"a+");
    if (file_out == NULL) {
        return;
    }
    vfprintf(file_out, fmt, vl);
    fclose(file_out);

    if (firstTime == 1) {
        firstTime = 0;
        chown(LOG_FILE_NAME, 10000, 10000 );
        chmod(LOG_FILE_NAME, 0666);
    }
}
static void log2file(const char *fmt, ...)
{
    if (_debug) {
        va_list vl;
        va_start(vl, fmt);
        _log2file(fmt, vl);
        va_end(vl);
    }
}


#define USB3GMODEM_LIB_PATH "/system/lib/libusb3gmodem-ril.so"
#include "modemconfig.h"

static int isModemModeFinish()
{
    if (g_config_buildinModem == 1) {
        return 1;
    }

    char buff[255];
    memset(buff,0,255);
    int ret = property_get("status.usb3g.modemmode.status", buff, "pending");
    if (ret > 1) {
        if (strncmp(buff,"ok",2) == 0) {
            return 1;
        }
    }

    return 0;
}

static void setAdvancedOptions(int fd, speed_t baud) {
    struct termios options;
    struct termios options_cpy;

    fcntl(fd, F_SETFL, 0);

    // get the parameters
    tcgetattr(fd, &options);

    // Do like minicom: set 0 in speed options
    cfsetispeed(&options, 0);
    cfsetospeed(&options, 0);

    options.c_iflag = IGNBRK;

    // Enable the receiver and set local mode and 8N1
    options.c_cflag = (CLOCAL | CREAD | CS8 | HUPCL);
    // enable hardware flow control (CNEW_RTCCTS)
    // options.c_cflag |= CRTSCTS;
    // Set speed
    options.c_cflag |= baud;

    /*
       options.c_cflag &= ~PARENB;
       options.c_cflag &= ~CSTOPB;
       options.c_cflag &= ~CSIZE; // Could this be wrong!?!?!?
       */

    // set raw input
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(INLCR | ICRNL | IGNCR);

    // set raw output
    options.c_oflag &= ~OPOST;
    options.c_oflag &= ~OLCUC;
    options.c_oflag &= ~ONLRET;
    options.c_oflag &= ~ONOCR;
    options.c_oflag &= ~OCRNL;

    // Set the new options for the port...
    options_cpy = options;
    tcsetattr(fd, TCSANOW, &options);
    options = options_cpy;

    // Do like minicom: set speed to 0 and back
    options.c_cflag &= ~baud;
    tcsetattr(fd, TCSANOW, &options);
    options = options_cpy;

    sleep(1);

    options.c_cflag |= baud;
    tcsetattr(fd, TCSANOW, &options);
}

static int open_serialport(char *dev)
{
    int fd;

    fd = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd >= 0) {
        struct termios options;
        // The old way. Let's not change baud settings
        fcntl(fd, F_SETFL, 0);

        // get the parameters
        tcgetattr(fd, &options);

        // Set the baud rates to 57600...
        // cfsetispeed(&options, B57600);
        // cfsetospeed(&options, B57600);

        // Enable the receiver and set local mode...
        options.c_cflag |= (CLOCAL | CREAD);

        // No parity (8N1):
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;

        // enable hardware flow control (CNEW_RTCCTS)
        // options.c_cflag |= CRTSCTS;

        // set raw input
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_iflag &= ~(INLCR | ICRNL | IGNCR);

        // set raw output
        options.c_oflag &= ~OPOST;
        options.c_oflag &= ~OLCUC;
        options.c_oflag &= ~ONLRET;
        options.c_oflag &= ~ONOCR;
        options.c_oflag &= ~OCRNL;

        // Set the new options for the port...
        tcsetattr(fd, TCSANOW, &options);
    } else {
        if(_debug)
            log2file("#### open serialport(%s) ERROR. errno = %d\n", dev, errno );
    }
    return fd;
}



///////////////////////////////

int isLibPathConfigureSeted()
{
    char buff[255];
    memset(buff,0,255);
    int ret = property_get("rild.libpath", buff, "");
    if (ret > 1) {
        if (strlen(buff) > 0) {
            return 1;
        }
    }

    return 0;
}

int isLibArgsConfigureSeted()
{
    char buff[255];
    memset(buff,0,255);
    int ret = property_get("rild.libargs", buff, "");
    if (ret > 1) {
        if (strlen(buff) > 0) {
            return 1;
        }
    }

    return 0;
}

static int isAndroidRilSetOnKernelCommandLine()
{
#define  KERNEL_OPTION  "android.ril="
    char          buffer[1024], *p, *q;
    int           len;
    int           fd = open("/proc/cmdline",O_RDONLY);

    if (fd < 0) {
        log2file("could not open /proc/cmdline:%s", strerror(errno));
        return 0;
    }

    do {
        len = read(fd,buffer,sizeof(buffer)); }
    while (len == -1 && errno == EINTR);

    if (len < 0) {
        log2file("could not read /proc/cmdline:%s", strerror(errno));
        close(fd);
        return 0;
    }
    close(fd);


    p = strstr( buffer, KERNEL_OPTION );
    if (p == NULL) {
        return 0;
    }
    return 1;
}


int main(int argc, char **argv)
{
    const char * rilLibPath = NULL;
    char **rilArgv;
    void *dlHandle;
    const RIL_RadioFunctions *(*rilInit)(const struct RIL_Env *, int, char **);
    const RIL_RadioFunctions *funcs;
    char libPath[PROPERTY_VALUE_MAX];
    unsigned char hasLibArgs = 0;
    int i;

    char faLibArgs[PROPERTY_VALUE_MAX];
    memset(faLibArgs, 0, PROPERTY_VALUE_MAX); //FriendlyARM

    log2file("start rild\n");

    umask(S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
    for (i = 1; i < argc ;) {
        if (0 == strcmp(argv[i], "-l") && (argc - i > 1)) {
            rilLibPath = argv[i + 1];
            i += 2;
        } else if (0 == strcmp(argv[i], "--")) {
            i++;
            hasLibArgs = 1;
            break;
        } else {
            usage(argv[0]);
        }
    }

    log2file("isAndroidRilSetOnKernelCommandLine = %d\n", isAndroidRilSetOnKernelCommandLine());

    //when cmdline set android.ril=, fa-network-service will disable 3g autoconnect
    if (isAndroidRilSetOnKernelCommandLine() != 1) {
        // by F@R@I@E@N@D@L@Y@A@R@M
        int readConfigRet = readConfig();
        if (readConfigRet != 0) {
            log2file("#### readConfig failed, result=%d\n", readConfigRet);
        }

        if (g_config_isAutoConnect == 1 && g_config_atCmdDeviceIndex > 0) {
            for (i=0; i<30; i++) {
                if (isModemModeFinish()==1) {
                    break;
                }
                sleep(1);
            }

            if (isLibPathConfigureSeted()==1 && isLibArgsConfigureSeted()==1) {
                //nothing to do
            } else {
                if (rilLibPath == NULL && hasLibArgs == 0) {

                    log2file("#### using usb 3g modem\n");

                    property_set("rild.libpath",
                        USB3GMODEM_LIB_PATH);
                    rilLibPath = libPath;
                    strncpy( rilLibPath, USB3GMODEM_LIB_PATH, strlen(USB3GMODEM_LIB_PATH) );

                    if (isModemModeFinish()==1) {
                        char libargs[255];
                        char fileName[255];
                        int serial_fd;
                        int i;
                        int tryTimes;
                        sprintf(fileName, "/dev/ttyUSB%i", g_config_atCmdDeviceIndex);
                        sprintf(libargs, "-d %s", fileName);
                        property_set("rild.libargs", libargs);
                        strcpy( faLibArgs,  libargs );

                        for (tryTimes = 0; tryTimes < 3; tryTimes++) {
                            serial_fd = open_serialport(fileName);
                            if (serial_fd >= 0) {
                                log2file("#### open %s OK\n", fileName);
                                fd_set rfds;
                                struct timeval timeout;
                                unsigned char buf[1024];
                                int sel;
                                size_t len;

                                for (i = 0; i < 100; i++) {
                                    FD_ZERO(&rfds);
                                    FD_SET(serial_fd, &rfds);

                                    timeout.tv_sec = 0;
                                    timeout.tv_usec = 10000;

                                    if ((sel = select(serial_fd + 1, &rfds, NULL,
                                                NULL, &timeout)) > 0) {
                                        if (FD_ISSET(serial_fd, &rfds)) {
                                            memset(buf, 0, sizeof(buf));
                                            len = read(serial_fd, buf, sizeof(buf));
                                        }

                                        log2file("#### datafrom %s: %s\n", fileName, buf);
                                    } else if (sel < 0) {
                                        break;
                                    }
                                }

                                close(serial_fd);

                                break;
                            } else {
                                log2file("#### try to open %s failed! [%d]\n", fileName, tryTimes);
                                sleep(1);
                            }
                        }

                    } else { 
                        property_set("rild.libargs", "-d /dev/pts/0");
                        strcpy( faLibArgs,  "-d /dev/pts/0" );
                    }
                }
            }
        }
    }



    /* special override when in the emulator */
    {
        static char*  arg_overrides[5];
        static int    arg_overrides_count = 3;
        static char   arg_device[32];
        static char   arg_device2[32];
        int           done = 0;

#define  REFERENCE_RIL_PATH  "/system/lib/libreference-ril.so"

        /* first, read /proc/cmdline into memory */
        char          buffer[1024], *p, *q;
        int           len;
        int           fd = open("/proc/cmdline",O_RDONLY);

        if (fd < 0) {
            LOGD("could not open /proc/cmdline:%s", strerror(errno));
            goto OpenLib;
        }

        do {
            len = read(fd,buffer,sizeof(buffer)); }
        while (len == -1 && errno == EINTR);

        log2file("cmdline buffer = %s\n", buffer);

        if (len < 0) {
            LOGD("could not read /proc/cmdline:%s", strerror(errno));
            close(fd);
            goto OpenLib;
        }
        close(fd);

#if 0
        if (strstr(buffer, "android.qemud=") != NULL)
        {
            /* the qemud daemon is launched after rild, so
             * give it some time to create its GSM socket
             */
            int  tries = 5;
#define  QEMUD_SOCKET_NAME    "qemud"

            while (1) {
                int  fd;

                sleep(1);

                fd = qemu_pipe_open("qemud:gsm");
                if (fd < 0) {
                    fd = socket_local_client(
                        QEMUD_SOCKET_NAME,
                        ANDROID_SOCKET_NAMESPACE_RESERVED,
                        SOCK_STREAM );
                }
                if (fd >= 0) {
                    close(fd);
                    snprintf( arg_device, sizeof(arg_device), "%s/%s",
                        ANDROID_SOCKET_DIR, QEMUD_SOCKET_NAME );

                    arg_overrides[1] = "-s";
                    arg_overrides[2] = arg_device;
                    done = 1;
                    break;
                }
                LOGD("could not connect to %s socket: %s",
                    QEMUD_SOCKET_NAME, strerror(errno));
                if (--tries == 0)
                    break;
            }
            if (!done) {
                LOGE("could not connect to %s socket (giving up): %s",
                    QEMUD_SOCKET_NAME, strerror(errno));
                while(1)
                    sleep(0x00ffffff);
            }
        }
#endif

        /* otherwise, try to see if we passed a device name from the kernel */
        if (!done) do {
#define  KERNEL_OPTION  "android.ril="
#define  DEV_PREFIX     "/dev/"

            p = strstr( buffer, KERNEL_OPTION );
            if (p == NULL)
                break;

            p += sizeof(KERNEL_OPTION)-1;
            q  = strpbrk( p, " \t\n\r" );
            if (q != NULL)
                *q = 0;

            log2file("p = %s\n", p);

            if (strstr(p, ":") == NULL) {
                snprintf( arg_device, sizeof(arg_device), DEV_PREFIX "%s", p );
                arg_device[sizeof(arg_device)-1] = 0;
                arg_overrides[1] = "-d";
                arg_overrides[2] = arg_device;
                arg_overrides_count = 3;
                hasLibArgs = 1;
                rilLibPath = REFERENCE_RIL_PATH;
                done = 1;

                log2file("arg_device = %s\n", arg_device);
            } else {
                char *token;
                int index = 0;
                int ac = 1;

                index = 0;
                char p2[1024];
                strcpy(p2, p);
                token = strtok( p2, ":" );
                while( token != NULL ) {
                    index ++;
                    token = strtok( NULL, ":" );
                }
                ac = index;
                log2file("ac = %d\n", ac);

                index = 0;
                token = strtok( p, ":" );
                while( token != NULL ) {
                    if (index == 0) {
                        snprintf( arg_device, sizeof(arg_device), DEV_PREFIX "%s", token );
                        arg_device[sizeof(arg_device)-1] = 0;
                        arg_overrides[1] = "-d";
                        arg_overrides[2] = arg_device;
                        arg_overrides_count = 3;
                        hasLibArgs = 1;
                        rilLibPath = REFERENCE_RIL_PATH;

                        log2file("arg_device = %s\n", arg_device);
                    } else if (index == 1) {
                        if (ac > 2) {
                            snprintf( arg_device2, sizeof(arg_device2), DEV_PREFIX "%s", token );
                            arg_device2[sizeof(arg_device2)-1] = 0;
                            arg_overrides[3] = "-u";
                            arg_overrides[4] = arg_device2;
                            arg_overrides_count = 5;
                        } else {
                            snprintf( faLibArgs, sizeof(faLibArgs), "/system/lib/%s", token );
                            hasLibArgs = 1;
                            rilLibPath = faLibArgs;
                        }
                    } else if (index == 2) {
                        snprintf( faLibArgs, sizeof(faLibArgs), "/system/lib/%s", token );
                        hasLibArgs = 1;
                        rilLibPath = faLibArgs;
                    }
                    index ++;
                    token = strtok( NULL, ":" );
                }
                if (index > 0) {
                    done = 1;
                }

                log2file("rilLibPath = %s\n", rilLibPath);
            }
        } while (0);

        log2file("done = %d\n", done);

        if (done) {
            argv = arg_overrides;
            argc = arg_overrides_count;
            i    = 1;
            if (arg_overrides_count <= 3) {
                log2file("overriding with %s %s", arg_overrides[1], arg_overrides[2]);
            } else if (arg_overrides_count <= 5) { 
                log2file("overriding with %s %s %s %s", arg_overrides[1], arg_overrides[2], arg_overrides[3], arg_overrides[4]);
            }
        }
    }

    if (rilLibPath == NULL) {
        if ( 0 == property_get(LIB_PATH_PROPERTY, libPath, NULL)) {
            // No lib sepcified on the command line, and nothing set in props.
            // Assume "no-ril" case.
            goto done;
        } else {
            rilLibPath = libPath;
        }
    }
OpenLib:
    switchUser();
    dlHandle = dlopen(rilLibPath, RTLD_NOW);

    if (dlHandle == NULL) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(-1);
    }

    RIL_startEventLoop();
    rilInit = (const RIL_RadioFunctions *(*)(const struct RIL_Env *, int, char **))dlsym(dlHandle, "RIL_Init");
    if (rilInit == NULL) {
        fprintf(stderr, "RIL_Init not defined or exported in %s\n", rilLibPath);
        exit(-1);
    }

    if (hasLibArgs) {
        rilArgv = argv + i - 1;
        argc = argc -i + 1;
    } else {
        static char * newArgv[MAX_LIB_ARGS];
        static char args[PROPERTY_VALUE_MAX];
        rilArgv = newArgv;

        if (strlen(faLibArgs) == 0) {
            property_get(LIB_ARGS_PROPERTY, args, "");
        } else {
            strcpy( args, faLibArgs );
        }

        argc = make_argv(args, rilArgv);
    }

    // Make sure there's a reasonable argv[0]
    rilArgv[0] = argv[0];
    funcs = rilInit(&s_rilEnv, argc, rilArgv);
    RIL_register(funcs);

done:

    while(1) {
        // sleep(UINT32_MAX) seems to return immediately on bionic
        sleep(0x00ffffff);
    }
}

