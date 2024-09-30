#include "netcfg/__netcfg.h"

int NETCFGCheckData(void* addr, u32 len, u8* md5Digest);
void NETCFGDecode(u8* data);

#ifdef DEBUG
const char* __NETCFGVersion = "<< Dolphin SDK - NETCFG\tdebug build: Mar  9 2004 12:31:53 (0x2301) >>";
#else
const char* __NETCFGVersion = "<< Dolphin SDK - NETCFG\trelease build: Mar  9 2004 12:58:21 (0x2301) >>";
#endif

CoreBlock* __NETCFGCfg;

void NETCFGInit(void) {
    static BOOL initialized = FALSE;
    static CoreBlock __core;

    if (!initialized) {
        memset(&__core, 0, sizeof(__core));
        __NETCFGCfg = &__core;
        initialized = TRUE;
    }
    OSRegisterVersion(__NETCFGVersion);
}

s32 NETCFGSetConfigData(void* addr) {
    int i; 
    int useIndex; 
    int ret[2];
    u8 buff[16448];
    u32 offset; 
    u32 diff; 
    NETCFGHeader* header[2];
    NETCFGConfig* data[2];

    if (__NETCFGCfg == NULL) {
        return -2;
    }

    memcpy(&buff, addr, sizeof(NETCFGHeader) * 16);

    offset = sizeof(NETCFGHeader) * 16;
    
    header[0] = (NETCFGHeader*)((char*)addr + offset);
    offset += sizeof(NETCFGHeader);
    
    data[0] = (NETCFGConfig*)((char*)addr + offset);
    offset += sizeof(NETCFGConfig);

    header[1] = (NETCFGHeader*)((char*)addr + offset);
    offset += sizeof(NETCFGHeader);

    data[1] = (NETCFGConfig*)((char*)addr + offset);

    for (i = 0; i < 2; i++) {
        diff = sizeof(NETCFGHeader) * 16;
        
        memcpy((buff + diff), data[i], sizeof(NETCFGConfig));
        diff += sizeof(NETCFGConfig);
        
        memcpy((buff + diff), header[0]->title, sizeof(header[0]->title));
        diff += sizeof(header[0]->title);
        
        memcpy((buff + diff), header[0]->description, sizeof(header[0]->description));
        diff += sizeof(header[0]->description);

        ret[i] = NETCFGCheckData(buff, diff, header[i]->md5Digest);
    }

    if (ret[0] == 0 && ret[1] == 0) {
        return -1;
    }

    if (ret[0] == 0) {
        useIndex = 1;
    } else if (ret[1] == 0) {
        useIndex = 0;
    } else if (header[0]->saveCount - header[1]->saveCount > 0) {
        useIndex = 0;
    } else {
        useIndex = 1;
    }

    __NETCFGCfg->header = header[useIndex];
    __NETCFGCfg->conf = data[useIndex];

    if (__NETCFGCfg->conf->status & 2) {
        memcpy(&__NETCFGCfg->passwd, &__NETCFGCfg->conf->passwd, sizeof(__NETCFGCfg->conf->passwd));
        NETCFGDecode((u8*)__NETCFGCfg->passwd);
    }

    if (__NETCFGCfg->header->version < 1) {
        return -6;
    }
    return 0;
}

s32 NETCFGGetConnectionType(u8* type) {
    s32 ret = 0;
    u8 tmp;

    if (__NETCFGCfg->conf == NULL) {
        ret = -2;
    } else {
        tmp = __NETCFGCfg->conf->connectType;
        switch (tmp) {
            case 0:
                *type = tmp;
                break;
            case 1:
                *type = tmp;
                break;
            case 2:
                *type = tmp;
                break;
            case 3:
                *type = tmp;
                break;
            default:
                *type = 0;
                break;
        }
    }
    
    return ret;
}

s32 NETCFGGetDnsType(u32* type) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == NULL) {
        ret = -2;
    } else {
        *type = __NETCFGCfg->conf->status & 1;
    }

    return ret;
}

s32 NETCFGGetDnsAddress(s32 dnsNo, u32* addr) {
    s32 ret = 0;
    u32 type;

    if (__NETCFGCfg->conf == NULL) {
        ret = -2;
    } else {
        NETCFGGetDnsType(&type);

        if (type == 1) {
            switch (dnsNo) {
                case 0:
                    *addr = __NETCFGCfg->conf->dns1 != 0 ? __NETCFGCfg->conf->dns1 : __NETCFGCfg->conf->dns2;
                    break;
                case 1:
                    *addr = __NETCFGCfg->conf->dns1 != 0 ? __NETCFGCfg->conf->dns2 : 0;
                    break;
            }
        } else {
            *addr = 0;
        }
    }

    if (*addr == 0) {
        ret = -4;
    }

    return ret;
}

s32 NETCFGGetConnectionId(const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == NULL) {
        ret = -2;
    } else {
        *str = __NETCFGCfg->conf->peerid;
    }

    return ret;
}

s32 NETCFGGetConnectionPassword(const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == NULL) {
        ret = -2;
    } else {
        if (!(__NETCFGCfg->conf->status & 0x2)) {
            ret = -3;
        }
        *str = __NETCFGCfg->passwd;
    }

    return ret;
}

s32 NETCFGGetMtu(u32* mtu) {
    s32 ret = 0;
    u32 tmp;

    if (__NETCFGCfg->conf == NULL) {
        ret = -2;
    } else {
        tmp = __NETCFGCfg->conf->mtu;
        switch (tmp) {
            case 576:
                *mtu = tmp;
                break;
            case 1500:
                *mtu = tmp;
                break;
            case 1454:
                *mtu = 1400;
                break;
            default:
                *mtu = 1400;
                break;
        }
    }

    return ret;
}

s32 NETCFGGetISPCode(u8* code) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == NULL) {
        ret = -2;
    } else if (__NETCFGCfg->conf->status & 0x80) {
        *code = 1;
    } else {
        *code = 0;
    }

    return ret;
}

s32 NETCFGGetVersion(u8* ver) {
    s32 ret = 0;

    if (__NETCFGCfg->header == NULL) {
        ret = -2;
    } else {
        *ver = __NETCFGCfg->header->version;
    }

    return ret;
}

