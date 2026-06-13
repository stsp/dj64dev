#define UBYTE uint8_t
#define UDWORD uint32_t
#define DWORD int32_t
#define UQWORD uint64_t
#define QWORD int64_t
#define UWORD uint16_t
#define WORD int16_t
#define VOID void

#define __ARG(t) t
#define __ARG_PTR(t) t *
#define __ARG_ARR(t) t
#define __ARG_PTR_FAR(t)
#define __ARG_A(t) t
#define __ARG_PTR_A(t) UDWORD
#define __ARG_ARR_A(t) UDWORD
#define __ARG_PTR_FAR_A(t)
#define __RET(t, v) v
#define __RET_PTR(t, v) djaddr2ptr(v)
#define __NORET NORETURN
#define PACKED __attribute__((packed))
