/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t   *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

void tp()
{
   debug("kernel mem [0x%p - 0x%p]\n", &__kernel_start__, &__kernel_end__);
   debug("MBI flags 0x%x\n", info->mbi->flags);

   //https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
   mbi_t *mbi;
   multiboot_memory_map_t *start;
   multiboot_memory_map_t *end;

   mbi   = info->mbi;
   start = (multiboot_memory_map_t*) mbi->mmap_addr;
   end   = (multiboot_memory_map_t*)(mbi->mmap_addr + mbi->mmap_length);

   while(start < end) {
      debug("mmap 0x%llx - 0x%llx (%d)\n",start->addr, (start->addr+start->len), start->type);
      start++;
   }
}
