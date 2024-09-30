#include "netcfg/__netcfg.h"

s32 NETCFGGetIpAddress(u32* addr) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *addr = __NETCFGCfg->conf->connectType == 2 ? __NETCFGCfg->conf->addr : 0;
    }

    return ret;
}

s32 NETCFGGetNetmask(u32* mask) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *mask = __NETCFGCfg->conf->connectType == 2 ? __NETCFGCfg->conf->netmask : 0;
    }

    return ret;
}

s32 NETCFGGetDefaultRouter(u32* addr) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *addr = __NETCFGCfg->conf->connectType == 2 ? __NETCFGCfg->conf->router : 0;
    }

    return ret;
}

s32 NETCFGGetDhcpHostname(const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *str = __NETCFGCfg->conf->dhcpHostname;
    }

    return ret;
}

s32 NETCFGGetPppoeServiceName(const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *str = __NETCFGCfg->conf->serviceName;
    }

    return ret;
}
