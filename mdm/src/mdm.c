#include "mdm/include/mdm.h"

#include <dolphin/os_internal.h>

#ifdef DEBUG
const char* __MDMVersion = "<< Dolphin SDK - MDM\tdebug build: Mar  9 2004 12:31:21 (0x2301) >>";
#else
const char* __MDMVersion = "<< Dolphin SDK - MDM\trelease build: Mar  9 2004 12:57:53 (0x2301) >>";
#endif

static MDM_CONNECTSTAT cs;

typedef struct ModemRegister {
    u16 datarxth;
    u16 datatxth;
    u8 imr;
    u8 fwt;
} ModemRegister;

static ModemRegister mr;

static struct {
    void (*conncallback)(s32);
    BOOL conncallbackflag;
    s32 compara;
    s32 atrxlen;
    s32 atrxpos;
    u8 atrxbuf[128];
} ms;

static s32 mdmstatus;
static BOOL intflag;
static BOOL atAndPFlag;

static SENDSTATUS ss;

static RECVSTATUS rs;

static BOOL OnReset(BOOL final);
static OSResetFunctionInfo ResetFunctionInfo = {
    &OnReset,
    127,
    NULL,
    NULL,
};

static OSThreadQueue threadQ;

static void recvsub1(s32 len);
static void recvsub2(s32 chan, OSContext* context);
static void sendsub1(s32 chan, OSContext* context);
static void sendsub2(void);
static void sendsub3(s32 chan, OSContext* context);
static void exiinthandler(s32 chan, OSContext* context);
static void checkatrxbuf(void);
static void atcommand(char* atcmd);

static void unlockcallback(s32 chan, OSContext* context) {
    OSWakeupThread(&threadQ);
}

static void waitexilock(void) {
    BOOL enabled = OSDisableInterrupts();

    while (TRUE) {
        if (EXILock(0, 2, &unlockcallback) == TRUE) {
            OSRestoreInterrupts(enabled);
            return;
        }
        
        OSSleepThread(&threadQ);
        OSRestoreInterrupts(enabled);
    }
}

static u32 readCID(void) {
    u32 cid;
    u16 mdmcmd;
    BOOL ret;

    ret = EXISelect(0, 2, 0);
    ASSERTLINE(250, ret == TRUE);
    mdmcmd = 0;
    EXIImm(0, &mdmcmd, sizeof(mdmcmd), EXI_WRITE, NULL);
    EXISync(0);
    EXIImm(0, &cid, sizeof(cid), EXI_READ, NULL);
    EXISync(0);
    EXIDeselect(0);
    return cid;
}

static void resetmodem(void) {
    u16 mdmcmd;
    BOOL ret;

    mdmcmd = 0x8000;
    ret = EXISelect(0, 2, 4);
    ASSERTLINE(268, ret == TRUE);
    EXIImm(0, &mdmcmd, sizeof(mdmcmd), EXI_WRITE, NULL);
    EXISync(0);
    EXIDeselect(0);
}

static void writecmd(u16 cmd, u8 wreg) {
    BOOL ret;
    u16 mdmcmd;

    mdmcmd = 0x4000 | cmd;
    ret = EXISelect(0, 2, 4);
    ASSERTLINE(287, ret == TRUE);
    EXIImm(0, &mdmcmd, sizeof(mdmcmd), EXI_WRITE, NULL);
    EXISync(0);
    EXIImm(0, &wreg, sizeof(wreg), EXI_WRITE, NULL);
    EXISync(0);
    EXIDeselect(0);
}

static u8 readcmd(u16 cmd) {
    BOOL ret;
    u16 mdmcmd;
    u8 ru8;

    mdmcmd = cmd;
    ret = EXISelect(0, 2, 4);
    ASSERTLINE(304, ret == TRUE);
    EXIImm(0, &mdmcmd, sizeof(mdmcmd), EXI_WRITE, NULL);
    EXISync(0);
    EXIImm(0, &ru8, sizeof(ru8), EXI_READ, NULL);
    EXISync(0);
    EXIDeselect(0);
    return ru8;
}

static void recvsub1(s32 len) {
    BOOL ret;
    u8* tmp1;
    u8* tmp2;
    u32 mdmcmd2;
    s32 recvpos;

    rs.len = len;
    rs.buf1 = rs.buf2 = rs.buf3 = NULL;
    rs.len1 = rs.len2 = rs.len3 = 0;
    tmp1 = (u8*)OSRoundUp32B(rs.buf);
    tmp2 = (u8*)OSRoundDown32B(&rs.buf[rs.len]);

    if ((u32)tmp1 >= (u32)tmp2) {
        rs.buf1 = rs.buf;
        rs.len1 = rs.len;
    } else {
        if ((u32)rs.buf != (u32)tmp1) {
            rs.buf1 = rs.buf;
            rs.len1 = (s32)tmp1 - (s32)rs.buf;
        }

        rs.buf2 = tmp1;
        rs.len2 = (s32)tmp2 - (s32)tmp1;
        
        if ((u32)tmp2 < (u32)&rs.buf[rs.len]) {
            rs.buf3 = tmp2;
            rs.len3 = (s32)&rs.buf[rs.len] - (s32)tmp2;
        }
    }

    ASSERTLINE(361, rs.len == rs.len1 + rs.len2 + rs.len3);

    mdmcmd2 = 0x28000000 | ((rs.len << 8) & 0x00FFFF00);
    ASSERTLINE(364, rs.buf != NULL);
    
    ret = EXISelect(0, 2, 4);
    ASSERTLINE(367, ret == TRUE);

    EXIImm(0, &mdmcmd2, sizeof(mdmcmd2), EXI_WRITE, NULL);
    EXISync(0);

    for (recvpos = 0; rs.len1 > 0; recvpos += sizeof(u32), rs.len1 -= sizeof(u32)) {
        len = min(rs.len1, 4);
        EXIImm(0, &rs.buf1[recvpos], len, EXI_READ, NULL);
        EXISync(0);
    }

    if (rs.len2 > 0) {
        EXIDma(0, rs.buf2, rs.len2, EXI_READ, &recvsub2);
    } else {
        recvsub2(0, NULL);
    }
}

static void recvsub2(s32 chan, OSContext* context) {
    s32 len;
    s32 recvpos;
    void (*callback)(s32);

    for (recvpos = 0; rs.len3 > 0; recvpos += sizeof(u32), rs.len3 -= sizeof(u32)) {
        len = min(rs.len3, sizeof(u32));
        EXIImm(0, &rs.buf3[recvpos], len, EXI_READ, NULL);
        EXISync(0);
    }

    if (rs.len2 > 0) {
        DCInvalidateRange(rs.buf2, rs.len2);
        rs.len2 = 0;
    }

    rs.gotlen += rs.len;
    rs.buf += rs.len;
    ASSERTLINE(422, rs.maxlen >= rs.gotlen);

    EXIDeselect(0);
    writecmd(0x0100, mr.imr);
    EXIUnlock(0);

    if (rs.maxlen - rs.gotlen < mr.datarxth || rs.flashflag == TRUE) {
        rs.flashflag = FALSE;
        rs.recvbusy = FALSE;

        if (rs.cb != NULL) {
            callback = rs.cb;
            rs.cb = NULL;
            intflag = TRUE;
            (*callback)(rs.gotlen);
            intflag = FALSE;
        }

        rs.gotlen = 0;
    }
}

static void exiinthandler(s32 chan, OSContext* context) {
    BOOL ret;
    u32 mdmcmd2;
    s32 len;
    u8 isr;
    s32 tmplen;

    ret = EXILock(0, 2, &exiinthandler);
    if (ret != FALSE) {
        isr = readcmd(0x0200);
        isr &= mr.imr;

        if (isr == 0) {
            EXIUnlock(0);
            return;
        }

        writecmd(0x0100, 0);
        
        switch (__cntlzw(isr)) {
            case 0x1B: {
                ASSERTLINE(487, ss.sendbusy);
                sendsub2();
                break;
            }
            case 0x1A: {
                if (rs.mode == 0) {
                    rs.syncrecvflag = TRUE;
                    mr.imr &= ~0x20;
                    writecmd(0x0100, mr.imr);
                    EXIUnlock(0);
                } else if (rs.recvbusy) {
                    recvsub1(mr.datarxth);
                } else {
                    mr.imr &= ~0x20;
                    writecmd(0x0100, mr.imr);
                    EXIUnlock(0);
                }
                break;
            }
            case 0x19: {
                rs.datarxfc = (s32)readcmd(0x0B00) << 8;
                rs.datarxfc += (s32)readcmd(0x0C00);

                if (rs.datarxfc == 0 || rs.recvbusy == FALSE) {
                    writecmd(0x0100, mr.imr);
                    EXIUnlock(0);
                } else {
                    ASSERTLINE(524, rs.maxlen > rs.gotlen);

                    tmplen = rs.maxlen - rs.gotlen;
                    if (tmplen < rs.datarxfc) {
                        rs.datarxfc = tmplen;
                    }

                    rs.flashflag = TRUE;
                    recvsub1(rs.datarxfc);
                }
                break;
            }
            case 0x1E: {
                ms.atrxlen = readcmd(0x0500);
                mdmcmd2 = 0x23000000 | ((ms.atrxlen << 8) & 0x0000FF00);

                ret = EXISelect(0, 2, 4);
                ASSERTLINE(539, ret == TRUE);
                
                EXIImm(0, &mdmcmd2, sizeof(mdmcmd2), EXI_WRITE, NULL);
                EXISync(0);

                while (ms.atrxlen > 0) {
                    len = min(ms.atrxlen, sizeof(u32));
                    EXIImm(0, &ms.atrxbuf[ms.atrxpos], len, EXI_READ, NULL);
                    EXISync(0);
                    
                    ms.atrxlen -= len;
                    ms.atrxpos += len;
                    ASSERTLINE(550, ms.atrxpos <= sizeof(ms.atrxbuf));
                }

                EXIDeselect(0);
                checkatrxbuf();
                writecmd(0x0100, mr.imr);
                EXIUnlock(0);

                if (ms.conncallbackflag == TRUE && ms.conncallback != NULL) {
                    ms.conncallbackflag = FALSE;
                    intflag = TRUE;
                    (*ms.conncallback)(ms.compara);
                    intflag = FALSE;
                }
                break;
            }
        }
    }
}

static void checkatrxbuf(void) {
    u8* buf;
    u8* prevbuf;
    u8* endbuf;
    s32 len;

    prevbuf = ms.atrxbuf;
    endbuf = &ms.atrxbuf[ms.atrxpos];
    
    ASSERTLINE(577, ms.atrxpos < sizeof(ms.atrxbuf));

    while (*prevbuf == '\r' || *prevbuf == '\n') {
        prevbuf++;
    }

    for (buf = ms.atrxbuf; (u32)buf < (u32)endbuf; buf++) {
        if (*buf == '\r' && (u32)prevbuf < (u32)buf) {
            len = (s32)buf - (s32)prevbuf;

            if (len == 2 && memcmp(prevbuf, "OK", 2) == 0) {
                // OK
            } else if (len > 7 && memcmp(prevbuf, "CONNECT", 7) == 0) {
                mdmstatus = 3;
                if (ms.conncallback != NULL) {
                    ms.conncallbackflag = TRUE;
                    ms.compara = 3;
                }

                prevbuf += 8;
                cs.dtespeed = atoi(prevbuf);
            } else if (len >= 9 && memcmp(prevbuf, "+MCR: ", 6) == 0) {
                prevbuf += 6;
                ASSERTLINE(609, len <= 10);
                memcpy(cs.modulation, prevbuf, len - 6);
                cs.modulation[len - 6] = 0;
            } else if (len >= 8 && memcmp(prevbuf, "+DR: ", 5) == 0) {
                prevbuf += 5;
                ASSERTLINE(616, len <= 9);
                memcpy(cs.compress, prevbuf, len - 5);
                cs.compress[len - 5] = 0;
            } else if (len >= 8 && memcmp(prevbuf, "+ER: ", 5) == 0) {
                prevbuf += 5;
                ASSERTLINE(623, len <= 9);
                memcpy(cs.protocol, prevbuf, len - 5);
                cs.protocol[len - 5] = 0;
            } else if (len > 5 && memcmp(prevbuf, "+MRR: ", 5) == 0) {
                prevbuf += 5;
                sscanf((const char*)prevbuf, "%d,%d", &cs.txspeed, &cs.rxspeed);
            } else if (len == 4 && memcmp(prevbuf, "ATE0", 4) == 0) {
                // ATE0
            } else if (len == 5 && memcmp(prevbuf, "ERROR", 5) == 0) {
                // ERROR
            } else if (len == 10 && memcmp(prevbuf, "NO CARRIER", 10) == 0) {
                mdmstatus = 1;
                if (ms.conncallback != NULL) {
                    ms.conncallbackflag = TRUE;
                    ms.compara = 1;
                }
            } else if (len == 11 && memcmp(prevbuf, "NO DIALTONE", 11) == 0) {
                mdmstatus = 1;
                if (ms.conncallback != NULL) {
                    ms.conncallbackflag = TRUE;
                    ms.compara = 0;
                }
            } else if (len == 4 && memcmp(prevbuf, "BUSY", 4) == 0) {
                mdmstatus = 1;
                if (ms.conncallback != NULL) {
                    ms.conncallbackflag = TRUE;
                    ms.compara = 2;
                }
            } else if (len == 4 && memcmp(prevbuf, "RING", 4) == 0) {
                if (ms.conncallback != NULL) {
                    ms.conncallbackflag = TRUE;
                    ms.compara = 4;
                }
            } else if (len == 9 && memcmp(prevbuf, "NO ANSWER", 9) == 0) {
                mdmstatus = 1;
                if (ms.conncallback != NULL) {
                    ms.conncallbackflag = TRUE;
                    ms.compara = 5;
                }
            } else if (len >= 7 && memcmp(prevbuf, "DELAYED", 7) == 0) {
                mdmstatus = 1;
                if (ms.conncallback != NULL) {
                    ms.conncallbackflag = TRUE;
                    ms.compara = 6;
                }
            } else if (len == 11 && memcmp(prevbuf, "BLACKLISTED", 11) == 0) {
                mdmstatus = 1;
                if (ms.conncallback != NULL) {
                    ms.conncallbackflag = TRUE;
                    ms.compara = 7;
                }
            }

            prevbuf = buf;
            while (*prevbuf == '\r' || *prevbuf == '\n') {
                prevbuf++;
            }
            
            if ((u32)prevbuf >= (u32)endbuf) {
                prevbuf = NULL;
            }
        }
    }

    if (prevbuf != NULL && (u32)endbuf > (u32)prevbuf) {
        if ((u32)prevbuf != (u32)ms.atrxbuf) {
            ms.atrxpos = (s32)endbuf - (s32)prevbuf;
            memmove(ms.atrxbuf, prevbuf, ms.atrxpos);
        }
    } else {
        ms.atrxpos = 0;
    }
}

static u8 get1AtReply(void) {
    u32 mdmcmd2;
    u8 rdreg;

    mdmcmd2 = 0x23000000 | 0x0100;
    EXISelect(0, 2, 4);
    EXIImm(0, &mdmcmd2, sizeof(mdmcmd2), EXI_WRITE, NULL);
    EXISync(0);
    EXIImm(0, &rdreg, sizeof(rdreg), EXI_READ, NULL);
    EXISync(0);
    EXIDeselect(0);
    return rdreg;
}

static s32 WaitResult(void) {
    u8 atfc;
    s32 i;
    u8 ch;
    BOOL oflag = FALSE;
    s32 count = 0;
    char* errstr = "ERROR";

    while (TRUE) {
        while (TRUE) {
            atfc = readcmd(0x0500);

            if (atfc != 0) {
                break;
            }

            OSYieldThread();
        }

        for (i = 0; i < atfc; i++) {
            ch = get1AtReply();
            ms.atrxbuf[ms.atrxpos++] = ch;

            if (ch == errstr[count]) {
                count++;
                if (count == 5) {
                    return -6;
                }
            } else {
                count = 0;
            }

            if (ch == 'O') {
                oflag = TRUE;
            } else {
                if (oflag == TRUE && ch == 'K') {
                    return MDM_OK;
                }

                oflag = FALSE;
            }
        }
    }
}

long MDMInit(char* countrycode) {
    static BOOL firsttime = FALSE;
    static BOOL regflag = FALSE;

    u32 cid;
    char buf[128];

    if (strcmp(countrycode, "00") == 0 || strcmp(countrycode, "B5") == 0) {
        atAndPFlag = TRUE;
    }

    if (regflag == FALSE) {
        OSRegisterVersion(__MDMVersion);
        OSRegisterResetFunction(&ResetFunctionInfo);
        regflag = TRUE;
    }

    waitexilock();
    mdmstatus = 0;
    mr.datarxth = 480;
    mr.datatxth = 33;
    mr.imr = 0x40 | 0x20 | 0x02;
    mr.fwt = 5;
    memset(&cs, 0, sizeof(cs));
    memset(&rs, 0, sizeof(rs));
    memset(&ss, 0, sizeof(ss));

    ASSERTLINE(858, countrycode);

    OSInitThreadQueue(&threadQ);
    intflag = FALSE;
    ms.conncallbackflag = rs.flashflag = FALSE;
    ms.atrxpos = 0;
    mdmstatus = 1;
    rs.recvbusy = FALSE;
    rs.syncrecvflag = FALSE;
    ss.sendbusy = FALSE;
    rs.maxlen = 0;
    rs.mode = 1;
    
    cid = readCID();
    if (cid != 0x02020000) {
        EXIUnlock(0);
        return -1;
    }
    
    if (firsttime) {
        resetmodem();
        firsttime = FALSE;
    }

    writecmd(0x1300, mr.fwt);
    writecmd(0x0600, 0x32);
    writecmd(0x0700, 0x40);
    writecmd(0x1000, upper(mr.datarxth));
    writecmd(0x1100, lower(mr.datarxth));
    writecmd(0x0E00, upper(mr.datatxth));
    writecmd(0x0F00, lower(mr.datatxth));
    
    /* Country of installation */
    strcpy(buf, "AT+GCI=");
    strcat(buf, countrycode);
    strcat(buf, "\r");
    atcommand(buf);
    WaitResult();

    /* Soft reset */
    atcommand("ATZ\r");
    WaitResult();

    /* Disable command echo */
    atcommand("ATE0\r");
    WaitResult();

    /* ATW1 -> show extended result codes, \V0 -> display current config & profile */
    atcommand("ATW1\\V0\r");
    WaitResult();

    /* Extended response codes, bitmapped, 0x2C, enable carrier [0x04], enable protocol [0x08], enable compression [0x20] */
    /* 1 << 0 [0x01] = connect code */
    /* 1 << 1 [0x02] = append /ARQ */
    /* 1 << 2 [0x04] = enable carrier */
    /* 1 << 3 [0x08] = enable protocol */
    /* 1 << 4 [0x10] = ??? */
    /* 1 << 5 [0x20] = enable compression */
    atcommand("ATS95=44\r");
    WaitResult();

    writecmd(0x0100, mr.imr);
    EXISetExiCallback(2, &exiinthandler);
    EXIUnlock(0);
    return 1;
}

static char* prohibited_commands[22] = {
    "Z",
    "+V",
    "+G",
    "&F",
    "&Y",
    "&W",
    "&Z",
    "E",
    "Q",
    "V",
    "W",
    "L",
    "M",
    "&V",
    "\\V",
    "*B",
    "*D",
    "+MS?",
    "#UD",
    "S24",
    "S95",
    "!",
};

s32 MDMATCommand(char* atcmd) {
    s32 atresult;
    s32 i;
    u32 length;

    length = strlen(atcmd);
    if (length <= 1 || atcmd[length - 1] != '\r') {
        return -8;
    }

    for (i = 0; i < length; i++) {
        atcmd[i] = mytoupper(atcmd[i]);
    }

    /* at commands must start with AT */
    if (memcmp(atcmd, "AT", 2) != 0) {
        return -8;
    }

    for (i = 0; i < ARRAY_COUNT(prohibited_commands); i++) {
        if (memcmp(&atcmd[2], prohibited_commands[i], strlen(prohibited_commands[i])) == 0) {
            return -8;
        }
    }

    waitexilock();
    atcommand(atcmd);
    atresult = WaitResult();
    EXIUnlock(0);

    return atresult;
}

static void MDMAtCommandNoWait(char* atcmd) {
    waitexilock();
    atcommand(atcmd);
    EXIUnlock(0);
}

s32 MDMAnswer(void (*cb)(s32)) {
    s32 ret;

    memset(&cs, 0, sizeof(cs));
    
    if (mdmstatus == 3) {
        return -3;
    }

    ms.conncallback = cb;
    if (cb == NULL) {
        ret = MDMATCommand("ATS0=0\r");
    } else {
        ret = MDMATCommand("ATS0=1\r");
    }

    ASSERTLINE(1008, ret == MDM_OK);
    return MDM_OK;
}

void MDMHangUp(void) {
    memset(&cs, 0, sizeof(cs));
    mdmstatus = 1;
    MDMAtCommandNoWait("ATH0\r");
}

static void atcommand(char* atcmd) {
    int ret;
    u32 mdmcmd;
    s32 len;
    s32 sendlen;

    len = strlen(atcmd);
    while (readcmd(0x0400) > 0x40 - len) { }

    mdmcmd = 0x23000000 | ((len & 0xFFFF) << 8) | 0x40000000;
    
    ret = EXISelect(0, 2, 4);
    ASSERTLINE(1037, ret == TRUE);
    
    EXIImm(0, &mdmcmd, sizeof(mdmcmd), EXI_WRITE, NULL);
    EXISync(0);

    for (len; len > 0; len -= sizeof(u32), atcmd += sizeof(u32)) {
        sendlen = min(sizeof(u32), len);
        EXIImm(0, atcmd, sendlen, EXI_WRITE, NULL);
        EXISync(0);
    }

    EXIDeselect(0);
}

s32 MDMRecv(u8* buf, s32 maxlen, void (*cb)(s32)) {
    ASSERTLINE(1055, rs.recvbusy == FALSE);

    if (rs.recvbusy) {
        return -5;
    }

    if (rs.mode == 0 || (mr.imr & 0x20) == 0) {
        rs.mode = 1;
        waitexilock();
        writecmd(0x1300, mr.fwt);
        mr.imr |= 0x20;
        writecmd(0x0100, mr.imr);
        writecmd(0x1000, upper(mr.datarxth));
        writecmd(0x1100, lower(mr.datarxth));
        EXIUnlock(0);
    }

    rs.recvbusy = TRUE;
    rs.buf = buf;
    rs.maxlen = maxlen;
    rs.cb = cb;
    rs.gotlen = 0;

    return MDM_OK;
}

long MDMRecvSync(u8* buf /* r30 */) {
    s32 len; // r31
    s32 recvlen; // r22
    u32 mdmcmd; // r1+0xC
    BOOL ret; // r19
    u8* tmp1; // r26
    u8* tmp2; // r25
    u8* buf1; // r23
    u8* buf2; // r21
    u8* buf3; // r20
    s32 len1; // r29
    s32 len2; // r28
    s32 len3; // r24

    if (rs.recvbusy != FALSE) {
        return -5;
    }

    if (rs.mode == 0) {
        if (rs.syncrecvflag == FALSE) {
            return 0;
        }

        waitexilock();
    } else {
        rs.mode = 0;
        rs.syncrecvflag = FALSE;
        waitexilock();
        mr.imr |= 0x20;

        writecmd(0x0100, mr.imr);
        writecmd(0x1300, 0);
        writecmd(0x1000, 0);
        writecmd(0x1100, 1);
    }

    rs.syncrecvflag = FALSE;
    len = (readcmd(0x0B00) << 8);
    len += readcmd(0x0C00);
    if (len == 0) {
        EXIUnlock(0);
        return 0;
    }

    mdmcmd = 0x28000000 | ((len & 0xFFFF) << 8);
    ret = EXISelect(0, 2, 4);
    ASSERTLINE(1135, ret == TRUE);

    EXIImm(0, &mdmcmd, sizeof(mdmcmd), EXI_WRITE, NULL);
    EXISync(0);


    buf1 = buf2 = buf3 = NULL;
    len1 = len2 = len3 = 0;

    tmp1 = (u8*)OSRoundUp32B(buf);
    tmp2 = (u8*)OSRoundDown32B(buf + len);

    if ((u32)tmp1 >= (u32)tmp2) {
        buf1 = buf;
        len1 = len;
    } else {
        if ((u32)buf != (u32)tmp1) {
            buf1 = buf;
            len1 = (s32)tmp1 - (s32)buf;
        }

        buf2 = tmp1;
        len2 = (s32)tmp2 - (s32)tmp1;

        if ((u32)tmp2 < (u32)&buf[len]) {
            buf3 = tmp2;
            len3 = (s32)&buf[len] - (s32)tmp2;
        }
    }

    ASSERTLINE(1174, len == len1 + len2 + len3);
    
    for (; len1 > 0; len1 -= sizeof(u32), buf1 += sizeof(u32)) {
        recvlen = min(sizeof(u32), len1);
        EXIImm(0, buf1, recvlen, EXI_READ, NULL);
        EXISync(0);
    }

    if (len2 > 0) {
        EXIDma(0, buf2, len2, EXI_READ, NULL);
        EXISync(0);
    }

    for (; len3 > 0; len3 -= sizeof(u32), buf3 += sizeof(u32)) {
        recvlen = min(sizeof(u32), len3);
        EXIImm(0, buf3, recvlen, EXI_READ, NULL);
        EXISync(0);
    }

    if (len2 > 0) {
        DCInvalidateRange(buf2, len2);
    }

    EXIDeselect(0);
    mr.imr |= 0x20;
    writecmd(0x0100, mr.imr);
    EXIUnlock(0);
    return len;
}

long MDSend(u8* buf, s32 len, void (*cb)(s32)) {
    if (ss.sendbusy) {
        return -4;
    }

    if (mdmstatus != 3) {
        return -7;
    }

    ss.sendbusy = TRUE;
    ss.sendbuf = buf;
    ss.sendlen = ss.sendlen_copy = len;
    ss.sendcallback = cb;
    sendsub1(0, NULL);
    return MDM_OK;
}

static void sendsub1(s32 chan, OSContext* context) {
    BOOL ret;

    ret = EXILock(0, 2, &sendsub1);
    if (ret != FALSE) {
        sendsub2();
    }
}

static void sendsub2(void) {
    BOOL ret;
    u32 mdmcmd;
    u8* tmp1;
    u8* tmp2;
    u8* buf;
    s32 len;
    s32 cansend;

    buf = ss.sendbuf;
    len = ss.sendlen;
    cansend = 0x201 - mr.datatxth;

    if (len >= cansend) {
        len = cansend;
    }

    ss.sendlen -= len;
    ss.sendbuf1 = ss.sendbuf2 = ss.sendbuf3 = NULL;
    ss.sendlen1 = ss.sendlen2 = ss.sendlen3 = 0;

    tmp1 = (u8*)OSRoundUp32B(buf);
    tmp2 = (u8*)OSRoundDown32B(buf + len);

    if ((u32)tmp1 >= (u32)tmp2) {
        ss.sendbuf1 = buf;
        ss.sendlen1 = len;
    } else {
        if (buf != tmp1) {
            ss.sendbuf1 = buf;
            ss.sendlen1 = (s32)tmp1 - (s32)buf;
        }

        ss.sendbuf2 = tmp1;
        ss.sendlen2 = (s32)tmp2 - (s32)tmp1;
        DCStoreRange(ss.sendbuf2, ss.sendlen2);
        if ((u32)tmp2 < (u32)&buf[len]) {
            ss.sendbuf3 = tmp2;
            ss.sendlen3 = (s32)&buf[len] - (s32)tmp2;
        }
    }

    ASSERTLINE(1311, ss.sendlen1 + ss.sendlen2 + ss.sendlen3 == len);
    
    ret = EXISelect(0, 2, 4);
    ASSERTLINE(1315, ret == TRUE);

    mdmcmd = 0x28000000 | ((len & 0xFFFF) << 8) | 0x40000000;
    EXIImm(0, &mdmcmd, sizeof(mdmcmd), EXI_WRITE, sendsub3);
}

static void sendsub3(s32 chan, OSContext* context) {
    s32 len;
    s32 sendpos;
    void (*callback)(s32);

    for (sendpos = 0; ss.sendlen1 > 0; sendpos += sizeof(u32), ss.sendlen1 -= sizeof(u32)) {
        len = min(ss.sendlen1, sizeof(u32));
        EXIImm(0, &ss.sendbuf1[sendpos], len, EXI_WRITE, NULL);
        EXISync(0);
    }

    if (ss.sendlen2 > 0) {
        EXIDma(0, ss.sendbuf2, ss.sendlen2, EXI_WRITE, sendsub3);
        ss.sendlen2 = 0;
    } else {
        for (sendpos = 0; ss.sendlen3 > 0; sendpos += sizeof(u32), ss.sendlen3 -= sizeof(u32)) {
            len = min(ss.sendlen3, sizeof(u32));
            EXIImm(0, &ss.sendbuf3[sendpos], len, EXI_WRITE, NULL);
            EXISync(0);
        }

        EXIDeselect(0);
        if (ss.sendlen > 0) {
            ASSERTLINE(1368, ss.sendlen < ss.sendlen_copy);

            ss.sendbuf += 0x201 - mr.datatxth;
            mr.imr |= 0x10;
        } else {
            ss.sendbusy = FALSE;
            ss.sendbuf = NULL;
            mr.imr &= ~0x10;
        }

        writecmd(0x0100, mr.imr);
        EXIUnlock(0);

        if (ss.sendlen == 0 && ss.sendcallback != NULL) {
            callback = ss.sendcallback;
            ss.sendcallback = NULL;
            intflag = TRUE;
            (*callback)(ss.sendlen_copy);
            intflag = FALSE;
        }
    }
}

BOOL MDMSendBusy(void) {
    u16 datatxfc;
    
    if (ss.sendbusy == TRUE) {
        return TRUE;
    }

    if (intflag == FALSE) {
        waitexilock();
    }

    datatxfc = readcmd(0x0900) << 8;
    datatxfc += readcmd(0x0A00);

    if (intflag == FALSE) {
        EXIUnlock(0);
    }

    if (datatxfc < mr.datatxth) {
        return FALSE;
    } else {
        return TRUE;
    }
}

s32 MDMDial(char* dialstr, s32 dialmode, void (*cb)(s32)) {
    char tmp[64];
    s32 ret;

    memset(&cs, 0, sizeof(cs));
    if (dialmode <= 0 || dialmode > 3) {
        return -2;
    }

    if (mdmstatus == 3) {
        return -3;
    }

    ms.conncallback = cb;

    switch (dialmode) {
        case 1:
            strcpy(tmp, "ATDT");
            break;
        case 2:
            if (atAndPFlag != FALSE) {
                ret = MDMATCommand("AT&P0\r");
                ASSERTLINE(1444, ret == MDM_OK);
                strcpy(tmp, "ATDP");
            }
            break;
        case 3:
            if (atAndPFlag != FALSE) {
                ret = MDMATCommand("AT&P2\r");
                ASSERTLINE(1452, ret == MDM_OK);
            }
            strcpy(tmp, "ATDP");
            break;
    }

    strcat(tmp, dialstr);
    strcat(tmp, "\r");
    mdmstatus = 2;
    MDMAtCommandNoWait(tmp);
    return MDM_OK;
}

s32 MDMChangeThreshold(s32 txth, s32 rxth) {
    #ifdef DEBUG
    if (ss.sendbusy == FALSE) {
        (void)ss.sendbusy;
    }
    #endif
    ASSERTLINE(1473, rxth >= 1 && rxth <= CO_DATAFIFOSIZE);
    ASSERTLINE(1474, txth >= 1 && txth < CO_DATAFIFOSIZE);

    waitexilock();
    mr.datarxth = rxth;
    mr.datatxth = txth;
    
    writecmd(0x1000, upper(mr.datarxth));
    writecmd(0x1100, lower(mr.datarxth));
    writecmd(0x0E00, upper(mr.datatxth));
    writecmd(0x0F00, lower(mr.datatxth));
    
    EXIUnlock(0);
    return MDM_OK;
}

void MDMSetFWT(s32 fwt) {
    mr.fwt = fwt;
    waitexilock();
    writecmd(0x1300, mr.fwt);
    EXIUnlock(0);
}

u8 MDMGetESR(void) {
    u8 esr;

    waitexilock();
    esr = readcmd(0x0D00);
    EXIUnlock(0);
    return esr;
}

u8 MDMGetRawStatus(void) {
    u8 msr;

    waitexilock();
    msr = readcmd(0x1200);
    EXIUnlock(0);
    return msr;
}

static void stopmodem(void) {
    EXISetExiCallback(2, NULL);
    mdmstatus = 0;
}

s32 MDMConnectMode(s32 mode) {
    char buf[128];
    s32 ret;
    char* modestr[6] = { "", "V90", "V34", "V32B", "V22B", "" };

    if (mode <= 0 || mode >= 5) {
        return -2;
    }

    strcpy(buf, "AT+MS=");
    strcpy(buf, modestr[mode]);
    strcpy(buf, "\r");
    ret = MDMATCommand(buf);
    ASSERTLINE(1550, ret == MDM_OK);
    return MDM_OK;
}

s32 MDMErrorCorrectMode(s32 mode) {
    char buf[128];
    s32 ret;
    char* modestr[4] = { "", "3", "4", "0" };

    if (mode <= 0 || mode > 3) {
        return -2;
    }

    strcpy(buf, "AT+ES=");
    strcpy(buf, modestr[mode]);
    strcpy(buf, "\r");
    ret = MDMATCommand(buf);
    ASSERTLINE(1570, ret == MDM_OK);
    return MDM_OK;
}

s32 MDMCompressMode(s32 mode) {
    char buf[128];
    s32 ret;
    char* modestr[5] = { "", "2", "1", "0", "3" };

    if (mode <= 0 || mode > 4) {
        return -2;
    }

    strcpy(buf, "AT%C");
    strcpy(buf, modestr[mode]);
    strcpy(buf, "\r");
    ret = MDMATCommand(buf);
    ASSERTLINE(1589, ret == MDM_OK);
    return MDM_OK;
}

s32 MDMGetStatus(MDM_CONNECTSTAT* stat) {
    if (stat) {
        *stat = cs;
    }

    return mdmstatus;
}

s32 MDMWaitToneMode(s32 mode) {
    char buf[128];
    char* modestr[3] = { "", "3", "4" };
    s32 ret;

    if (mode <= 0 || mode > 2) {
        return -2;
    }

    strcpy(buf, "ATX");
    strcpy(buf, modestr[mode]);
    strcpy(buf, "\r");
    ret = MDMATCommand(buf);
    ASSERTLINE(1619, ret == MDM_OK);
    return MDM_OK;
}

static BOOL OnReset(BOOL final) {
    if (final == FALSE) {
        if (mdmstatus == 3) {
            mdmstatus = 1;
            
            if (EXILock(0, 2, NULL) == FALSE) {
                return TRUE;
            }

            atcommand("ATH0\r");
            EXIUnlock(0);
        }

        stopmodem();
    }

    return TRUE;
}

s32 MDMGetLibraryVersion(void) {
    return 0x103;
}
