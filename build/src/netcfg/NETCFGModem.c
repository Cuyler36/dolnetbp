#include "netcfg/__netcfg.h"

s32 NETCFGGetDialType(u8* type) {
    s32 ret = 0;
    u8 tmp;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        tmp = __NETCFGCfg->conf->dialType;
        switch (tmp) {
            case 0:
                *type = tmp;
                break;
            case 1:
                *type = tmp;
                break;
            default:
                *type = 0;
                break;
        }
    }

    return ret;
}

s32 NETCFGGetDialPrefix(const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *str = __NETCFGCfg->conf->dialPrefix;
    }

    return ret;
}

s32 NETCFGGetDialNumber(s32 dialNo, const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        ASSERTLINE(99, (dialNo >= 0 ) && (dialNo < 2));
        switch (dialNo) {
            case 0:
                if (strcmp(__NETCFGCfg->conf->dialNo1, "") == 0 &&
                    strcmp(__NETCFGCfg->conf->dialNo2, "") != 0) {
                    *str = __NETCFGCfg->conf->dialNo2;
                } else {
                    *str = __NETCFGCfg->conf->dialNo1;
                }
                break;
            case 1:
                if (strcmp(__NETCFGCfg->conf->dialNo1, "") == 0 &&
                    strcmp(__NETCFGCfg->conf->dialNo2, "") != 0) {
                    *str = __NETCFGCfg->conf->dialNo1;
                } else {
                    *str = __NETCFGCfg->conf->dialNo2;
                }
                break;
        }
    }
    
    if (strcmp(*str, "") == 0) {
        ret = -5;
    }

    return ret;
}

s32 NETCFGGetBlindDialType(u8* type) {
    s32 ret = 0;
    u8 tmp;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        tmp = __NETCFGCfg->conf->blindDial;
        switch (tmp) {
            case 1:
                *type = tmp;
                break;
            case 0:
                *type = tmp;
                break;
            default:
                *type = 0;
                break;
        }
    }

    return ret;
}

s32 NETCFGGetDialRule(u8* type) {
    s32 ret = 0;
    u8 tmp;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        tmp = __NETCFGCfg->conf->dialRule;
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

s32 NETCFGGetCountryCode(const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *str = __NETCFGCfg->conf->countryCode;
    }

    return ret;
}

s32 NETCFGGetAreaCode(const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *str = __NETCFGCfg->conf->areaCode;
    }

    return ret;
}

s32 NETCFGGetCallWait(const char** str) {
    s32 ret = 0;

    if (__NETCFGCfg->conf == 0) {
        ret = -2;
    } else {
        *str = __NETCFGCfg->conf->callWait;
    }

    return ret;
}
