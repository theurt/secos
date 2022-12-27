/* GPLv2 (c) Airbus */
OUTPUT_FORMAT("elf32-i386","elf32-i386","elf32-i386");
OUTPUT_ARCH("i386")

ENTRY(entry)

PHDRS
{
   phboot  PT_LOAD FLAGS (7);
   phstack PT_LOAD FLAGS (6);
   phsetup PT_LOAD FLAGS (7);
}

SECTIONS
{
   /* On descend le kernel à l'adresse la plus basse non réservée : 0x100000 */
   . = 0x100000;
   .mbh      : { KEEP(*(.mbh)) . = ALIGN(4);     } : phboot
   .stack    : { KEEP(*(.stack))                 } : phstack

   __kernel_start__ = .; /*0x102010*/

   .idt_jmp  : { KEEP(*(.idt_jmp))               } : phsetup
   .text     : { *(.text)                        } : phsetup
   .rodata   : { *(.rodata)                      } : phsetup
   .data     : { *(.data)                        } : phsetup
   .bss      : { *(.bss COMMON)                  } : phsetup
   /DISCARD/ : { *(.note* .indent .comment)      } : phsetup

   . = 0x400000;
   __kernel_end__ = .; 

   /* Zone mémoire tâche 1 */
   __user1_begin__ = .;
   .user1 : { KEEP(*(.user1)) } : phsetup
   . = ALIGN(1048576);
   __user1_end__ = .;

   /* Zone mémoire tâche 2 */
   . = 0x500000;
   __user2_begin__ = .;
   .user2 : { KEEP(*(.user2)) } : phsetup
   . = ALIGN(1048576);
   __user2_end__ = .;

   /* Page pour la mémoire partagée entre les deux tâches */
   . = 0x800000;
   __shared_task_mem_begin__ = .;
   . = 0x801000;
   __shared_task_mem_end__ = .;

}
