/* symbols just to satisfy the linker*/
#define TEXT_STUB(name) void name(void) {}
#define DATA_STUB(name) int name;

TEXT_STUB(_start)

TEXT_STUB(__ArenaHi)
TEXT_STUB(__ArenaLo)
TEXT_STUB(__cvt_fp2unsigned)
TEXT_STUB(__div2i)
TEXT_STUB(__init_registers)
TEXT_STUB(__mod2i)
TEXT_STUB(__shl2i)
TEXT_STUB(__shr2i)
TEXT_STUB(__shr2u)
TEXT_STUB(_ctors)
TEXT_STUB(_dtors)
TEXT_STUB(_bss_init_info)
TEXT_STUB(_restfpr_18)
TEXT_STUB(_restfpr_19)
TEXT_STUB(_restfpr_20)
TEXT_STUB(_restfpr_22)
TEXT_STUB(_restfpr_23)
TEXT_STUB(_restfpr_24)
TEXT_STUB(_restfpr_25)
TEXT_STUB(_restfpr_26)
TEXT_STUB(_restfpr_27)
TEXT_STUB(_restfpr_28)
TEXT_STUB(_restfpr_29)
TEXT_STUB(_restfpr_30)
TEXT_STUB(_rom_copy_info)
TEXT_STUB(_savefpr_18)
TEXT_STUB(_savefpr_19)
TEXT_STUB(_savefpr_20)
TEXT_STUB(_savefpr_22)
TEXT_STUB(_savefpr_23)
TEXT_STUB(_savefpr_24)
TEXT_STUB(_savefpr_25)
TEXT_STUB(_savefpr_26)
TEXT_STUB(_savefpr_27)
TEXT_STUB(_savefpr_28)
TEXT_STUB(_savefpr_29)
TEXT_STUB(_savefpr_30)
TEXT_STUB(_stack_addr)
TEXT_STUB(_stack_end)
TEXT_STUB(DSPAddTask)
TEXT_STUB(DSPAssertTask)
TEXT_STUB(DSPCheckInit)
TEXT_STUB(DSPCheckMailToDSP)
TEXT_STUB(DSPGetDMAStatus)
TEXT_STUB(DSPHalt)
TEXT_STUB(DSPInit)
TEXT_STUB(DSPReset)
TEXT_STUB(DSPSendMailToDSP)
TEXT_STUB(EnableMetroTRKInterrupts)
TEXT_STUB(InitMetroTRK)
TEXT_STUB(cosf)
TEXT_STUB(main)
TEXT_STUB(memcmp)
TEXT_STUB(memcpy)
TEXT_STUB(memmove)
TEXT_STUB(memset)
TEXT_STUB(powf)
TEXT_STUB(rand)
TEXT_STUB(sinf)
TEXT_STUB(sprintf)
TEXT_STUB(srand)
TEXT_STUB(strchr)
TEXT_STUB(strcmp)
TEXT_STUB(strcpy)
TEXT_STUB(strlen)
TEXT_STUB(strncpy)
TEXT_STUB(tanf)
TEXT_STUB(tolower)
TEXT_STUB(vprintf)
TEXT_STUB(vsprintf)
