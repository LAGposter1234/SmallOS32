#pragma once
#define IDT_ENTRIES 256

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));


struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));


static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;


#define SYS_EXIT 0

void syscall_handle(uint32_t number) {
    switch(number) {
        case SYS_EXIT: __asm__("call shell");
    };
    return;
}

extern void syscallstub(void);
__asm__(
    ".global syscallstub\n"
    "syscallstub:\n"
    "    push %eax\n"
    "    call syscall_handle\n"
    "    iret   \n"
);

static void idt_set_gate(int n,uint32_t handler) {
    idt[n].offset_low =
    handler & 0xFFFF;
    idt[n].selector = 0x08;
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E;
    idt[n].offset_high =
    (handler >> 16) & 0xFFFF;
}

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

void fault_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base = (uint32_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].zero = 0;
        idt[i].type_attr = 0;
        idt[i].offset_high = 0;
    }
    // eventually add support for SmallOS16 style code here
    idt_set_gate(0x80, (uint32_t)syscallstub);

    asm volatile(
        "lidt %0"
        :
        : "m"(idtp)
    );
}
