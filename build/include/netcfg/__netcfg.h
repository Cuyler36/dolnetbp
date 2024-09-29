#ifndef _DOLPHIN_OS_NETCFG_PRIVATE_H_
#define _DOLPHIN_OS_NETCFG_PRIVATE_H_

#include <dolphin/netcfg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NETCFGHeader {
    char title[32];
    char description[32];
    u8 md5Digest[16];
    s8 saveCount;
    u8 version;
    u8 reserve[430];
} NETCFGHeader;

typedef struct NETCFGConfig {
    u32 dns1;
    u32 dns2;
    u32 addr;
    u32 netmask;
    u32 router;
    u16 mtu;
    u16 status;
    u8 connectType;
    u8 dialType;
    u8 blindDial;
    u8 dialRule;
    char peerid[256];
    char passwd[256];
    char serviceName[128];
    char dhcpHostname[256];
    char dialPrefix[64];
    char dialNo1[64];
    char dialNo2[64];
    char countryCode[4];
    char areaCode[4];
    char callWait[64];
    u8 reserve[6492];
} NETCFGConfig;

typedef struct CoreBlock {
    NETCFGHeader* header;
    NETCFGConfig* conf;
    char passwd[256];
} CoreBlock;

extern CoreBlock* __NETCFGCfg;

#ifdef __cplusplus
}
#endif

#endif
