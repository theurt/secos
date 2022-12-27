/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include <pagemem.h>
#include <info.h>

extern info_t *info;

void show_cr3()
{
   cr3_reg_t cr3 = {.raw = get_cr3()};
   debug("CR3 = 0x%x\n", cr3.raw);
}

/* Activer la pagination */
void enable_paging()
{
   uint32_t cr0 = get_cr0();
   set_cr0(cr0|CR0_PG);
}

/* Une ptb comporte 1024 entrées et chaque entrée couvre 4KB
*Une table de page permet donc de mapper 0x400000 bits
*/
void identity_init()
{
    int i;
    pde32_t *pgd = (pde32_t*)0x600000;
    pte32_t *ptb = (pte32_t*)0x601000;

   for(i=0;i<1024;i++)
      pg_set_entry(&ptb[i], PG_KRN|PG_RW, i);

   memset((void*)pgd, 0, PAGE_SIZE);
   pg_set_entry(&pgd[0], PG_KRN|PG_RW, page_nr(ptb));

   pte32_t *ptb2 = (pte32_t*)0x602000;
   for(i=0;i<1024;i++)
      pg_set_entry(&ptb2[i], PG_KRN|PG_RW, i+1024);

   pg_set_entry(&pgd[1], PG_KRN|PG_RW, page_nr(ptb2));

    set_cr3((uint32_t)pgd);
    enable_paging();

   debug("PTB[1] = 0x%x\n", ptb[1].raw);


   char *v1 = (char*)0x700000;
   char *v2 = (char*)0x7ff000;

   int ptb_idx = pt32_idx(v1);
   pg_set_entry(&ptb2[ptb_idx], PG_KRN|PG_RW, 2);

   ptb_idx = pt32_idx(v2);
   pg_set_entry(&ptb2[ptb_idx], PG_KRN|PG_RW, 2);

   debug("%p = %s | %p = %s\n", v1, v1, v2, v2);

}

void tp()
{
   show_cr3();
   identity_init();
}
