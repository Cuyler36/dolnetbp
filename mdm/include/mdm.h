#ifndef _DOLPHIN_OS_MDM_H_
#define _DOLPHIN_OS_MDM_H_

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_OK 1

#define CO_DATAFIFOSIZE 0x200

typedef struct MDM_CONNECTSTAT {
    s32 dtespeed;
    s32 txspeed;
    s32 rxspeed;
    u8 modulation[5];
    u8 protocol[5];
    u8 compress[5];
} MDM_CONNECTSTAT;

typedef struct SENDSTATUS {
    BOOL sendbusy;
    u8* sendbuf;
    s32 sendlen;
    s32 sendlen_copy;
    u8* sendbuf1;
    s32 sendlen1;
    u8* sendbuf2;
    s32 sendlen2;
    u8* sendbuf3;
    s32 sendlen3;
    void (*sendcallback)(s32);
} SENDSTATUS;

typedef struct RECVSTATUS {
    u8* buf;
    u8* buf1;
    u8* buf2;
    u8* buf3;
    BOOL flashflag;
    BOOL recvbusy;
    s32 gotlen;
    s32 len;
    s32 len1;
    s32 len2;
    s32 len3;
    s32 datarxfc;
    s32 maxlen;
    s32 mode;
    BOOL syncrecvflag;
    void (*cb)(s32);
} RECVSTATUS;

inline s32 min(s32 a, s32 b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

static inline u8 lower(u16 x) {
    return x;
}

static inline u8 upper(u16 x) {
    return (x >> 8);
}

inline char mytoupper(char ch) {
    if (ch >= 'a' && ch <= 'z') {
        return ch - 0x20;
    }

    return ch;
}

#ifdef __cplusplus
}
#endif

#endif
