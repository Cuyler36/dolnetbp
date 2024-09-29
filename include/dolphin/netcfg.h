#ifndef _DOLPHIN_OS_NETCFG_H_
#define _DOLPHIN_OS_NETCFG_H_

#include <dolphin/types.h>
#include <dolphin/card.h>

#ifdef __cplusplus
extern "C" {
#endif

void NETCFGInit(void);
s32 NETCFGSetConfigData(void* addr);
s32 NETCFGGetConnectionType(u8* type);
s32 NETCFGGetDnsType(u32* type);
s32 NETCFGGetDnsAddress(s32 dnsNo, u32* addr);
s32 NETCFGGetConnectionId(const char** str);
s32 NETCFGGetConnectionPassword(const char** str);
s32 NETCFGGetMtu(u32* mtu);
s32 NETCFGGetISPCode(u8* code);
s32 NETCFGGetVersion(u8* ver);

u32 NETCFGCalcDataSum(u8* data);
u8 NETCFGCalcSwapTableIndex(u8* data);
void NETCFGEncode(u8* data);
void NETCFGDecode(u8* data);
BOOL NETCFGCheckData(void* addr, u32 len, u8* md5Digest);
BOOL NETCFGIsConfigFile(u8 attr, const CARDStat* stat);

s32 NETCFGGetDialType(u8* type);
s32 NETCFGGetDialPrefix(const char** str);
s32 NETCFGGetDialNumber(s32 dialNo, const char** str);
s32 NETCFGGetBlindDialType(u8* type);
s32 NETCFGGetDialRule(u8* type);
s32 NETCFGGetCountryCode(const char** str);
s32 NETCFGGetAreaCode(const char** str);
s32 NETCFGGetCallWait(const char** str);

s32 NETCFGGetIpAddress(u32* addr);
s32 NETCFGGetNetmask(u32* mask);
s32 NETCFGGetDefaultRouter(u32* addr);
s32 NETCFGGetDhcpHostname(const char** str);
s32 NETCFGGetPppoeServiceName(const char** str);


#ifdef __cplusplus
}
#endif

#endif
