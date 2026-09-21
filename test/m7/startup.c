/* Minimal freestanding runtime for QEMU's mps2-an500 (Cortex-M7).
 * No newlib startup: vector table, .data/.bss setup, FPU enable, a bump _sbrk,
 * ARM semihosting for output, and a 64 bit time base built on SysTick. */
#include <stdint.h>
#include <stddef.h>

/* the build drives this file through g++, so force C linkage */
#ifdef __cplusplus
extern "C" {
#endif

extern unsigned __data_load, __data_start, __data_end, __bss_start, __bss_end;
extern unsigned __heap_start, __heap_end, __stack_top;
extern int main(void);

void Reset_Handler(void);
static void Default_Handler(void) { for (;;) {} }

__attribute__((section(".vectors"), used))
void *const __vectors[] = {
    (void *)&__stack_top, (void *)Reset_Handler,
    (void *)Default_Handler, (void *)Default_Handler, (void *)Default_Handler,
    (void *)Default_Handler, (void *)Default_Handler, (void *)Default_Handler,
};

/* ---- ARM semihosting ---------------------------------------------------- */
static inline int semihost(int op, void *arg)
{
    register int r0 __asm__("r0") = op;
    register void *r1 __asm__("r1") = arg;
    __asm__ volatile("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
}
void sh_write0(const char *s) { semihost(0x04, (void *)s); }
void sh_exit(int code)
{
    uint32_t a[2] = {0x20026, (uint32_t)code}; /* ADP_Stopped_ApplicationExit */
    semihost(0x20, a);
    for (;;) {}
}

/* ---- SysTick based 64 bit tick counter ---------------------------------- */
#define SYST_CSR (*(volatile uint32_t *)0xE000E010)
#define SYST_RVR (*(volatile uint32_t *)0xE000E014)
#define SYST_CVR (*(volatile uint32_t *)0xE000E018)

static uint64_t st_high;
static uint32_t st_last;

void systick_init(void)
{
    SYST_RVR = 0x00FFFFFF;
    SYST_CVR = 0;
    SYST_CSR = 5; /* enable, processor clock, no interrupt */
    st_high = 0;
    st_last = 0x00FFFFFF;
}

/* Must be called more often than once per 2^24 ticks or wraps are lost. */
uint64_t systick_now(void)
{
    uint32_t v = SYST_CVR; /* counts down */
    if (v > st_last)
        st_high += 0x01000000u;
    st_last = v;
    return st_high + (0x00FFFFFFu - v);
}

/* ---- heap --------------------------------------------------------------- */
static char *brk;
void *_sbrk(ptrdiff_t n)
{
    if (!brk) brk = (char *)&__heap_start;
    if (brk + n > (char *)&__heap_end) return (void *)-1;
    char *p = brk;
    brk += (n + 7) & ~(ptrdiff_t)7;
    return p;
}

void Reset_Handler(void)
{
    *(volatile uint32_t *)0xE000ED88 |= (0xFu << 20); /* CPACR: CP10/CP11 full access */
    __asm__ volatile("dsb; isb");

    unsigned *s = &__data_load, *d = &__data_start;
    while (d < &__data_end) *d++ = *s++;
    for (d = &__bss_start; d < &__bss_end;) *d++ = 0;

    /* run static constructors; doing it here avoids __libc_init_array, which
       would drag in _init/_fini from the crti/crtn files we skip */
    extern void (*__init_array_start[])(void), (*__init_array_end[])(void);
    for (void (**f)(void) = __init_array_start; f < __init_array_end; ++f) (*f)();

    main();
    sh_exit(0);
}

#ifdef __cplusplus
}
#endif
