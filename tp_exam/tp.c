/* GPLv2 (c) Airbus */
#include <debug.h>
#include <cr.h>
#include <segmem.h>
#include <pagemem.h>
#include <info.h>
#include <intr.h>

extern info_t *info;

/* Adresses du kernel*/
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

/* Adresses de la mémoire partagée entre les taches*/
extern uint32_t __shared_task_mem_begin__;
extern uint32_t __shared_task_mem_end__; 

/* Adresses de la mémoire réservée à la tâche 1*/
extern uint32_t __user1_begin__;
extern uint32_t __user1_end__; 

/* Adresses de la mémoire réservée à la tâche 2 */
extern uint32_t __user2_begin__;
extern uint32_t __user2_end__; 

/*----------------GDT ----------------------*/
#define c0_idx  1
#define d0_idx  2
#define c3_idx  3
#define d3_idx  4
#define ts_idx  5

#define c0_sel  gdt_krn_seg_sel(c0_idx)
#define d0_sel  gdt_krn_seg_sel(d0_idx)
#define c3_sel  gdt_usr_seg_sel(c3_idx)
#define d3_sel  gdt_usr_seg_sel(d3_idx)
#define ts_sel  gdt_krn_seg_sel(ts_idx)

__attribute__((aligned(8))) seg_desc_t GDT[6];
__attribute__((aligned(8))) tss_t      TSS;

#define gdt_flat_dsc(_dSc_,_pVl_,_tYp_)                                 \
   ({                                                                   \
      (_dSc_)->raw     = 0;                                             \
      (_dSc_)->limit_1 = 0xffff;                                        \
      (_dSc_)->limit_2 = 0xf;                                           \
      (_dSc_)->type    = _tYp_;                                         \
      (_dSc_)->dpl     = _pVl_;                                         \
      (_dSc_)->d       = 1;                                             \
      (_dSc_)->g       = 1;                                             \
      (_dSc_)->s       = 1;                                             \
      (_dSc_)->p       = 1;                                             \
   })

#define tss_dsc(_dSc_,_tSs_)                                            \
   ({                                                                   \
      raw32_t addr    = {.raw = _tSs_};                                 \
      (_dSc_)->raw    = sizeof(tss_t);                                  \
      (_dSc_)->base_1 = addr.wlow;                                      \
      (_dSc_)->base_2 = addr._whigh.blow;                               \
      (_dSc_)->base_3 = addr._whigh.bhigh;                              \
      (_dSc_)->type   = SEG_DESC_SYS_TSS_AVL_32;                        \
      (_dSc_)->p      = 1;                                              \
   })

#define c0_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_CODE_XR)
#define d0_dsc(_d) gdt_flat_dsc(_d,0,SEG_DESC_DATA_RW)
#define c3_dsc(_d) gdt_flat_dsc(_d,3,SEG_DESC_CODE_XR)
#define d3_dsc(_d) gdt_flat_dsc(_d,3,SEG_DESC_DATA_RW)  


/* Tâche 1 */
__attribute__((section(".user1")))void user1(){
    while(1);
}

/* Tâche 2*/
__attribute__((section(".user2")))void user2(){
    while(1);
}

/* Configuration de la GDT et 
*  mise en place des sélecteurs de code
*/
void setup_segmentation(){

    gdt_reg_t gdtr;

    //Entrée 0 de la GDT = NULL
    GDT[0].raw = 0ULL;

    //Placement des 4 différentes descripteurs
    c0_dsc( &GDT[c0_idx] );
    d0_dsc( &GDT[d0_idx] );
    c3_dsc( &GDT[c3_idx] );
    d3_dsc( &GDT[d3_idx] );
    tss_dsc(&GDT[ts_idx], (offset_t)&TSS);

    gdtr.desc  = GDT;
    gdtr.limit = sizeof(GDT) - 1;
    set_gdtr(gdtr);

    set_ds(d0_sel);
    set_es(d0_sel);
    set_fs(d0_sel);
    set_gs(d0_sel);
    set_cs(c0_sel);
    set_ss(d0_sel);
}

/*Pour une adresse de PGD donnée et de PTB données, 
* mise en place d'un identity mapping (cf TP4)
* pour le noyau
* param pgd : adresse de la PGD
* param ptb : adresse de la première PTB
* param flags  : flags à mettre sur les pages
*/
void identity_paging_krn(pde32_t *pgd, pte32_t* ptb,unsigned int flags){

    //Flush les octets de la pgd
    memset((void*)pgd, 0, PAGE_SIZE);

    /* Mapper les adresses 0x000000 à 0x400000 dans la PTB*/
    unsigned int i;
    for(i=0;i<1024;i++)
        pg_set_entry(&ptb[i], flags, i);


    /* Créer une entrée dans la PGD pour cette PTB*/
    pg_set_entry(&pgd[0], flags, page_nr(ptb));

    /* Mapper les adresses 0x400000 à 0x800000 dans une nouvelle PTB*/
    pte32_t *ptb2 = ptb + 0x1000;
    for(i=0;i<1024;i++)
      pg_set_entry(&ptb2[i], PG_KRN|PG_RW, i+1024);
      
    /* Créer une entrée dans la PGD pour cette deuxième PTB*/
    pg_set_entry(&pgd[1], PG_KRN|PG_RW, page_nr(ptb2));
}

/*Pour une adresse de PGD donnée et de PTB données, 
* mise en place d'un identity mapping (cf TP4)
* pour le noyau
* param pgd : adresse de la PGD
* param ptb : adresse de la première PTB
* param flags  : flags à mettre sur les pages
* param offset : offset de l'userspace (0x400000 ou 0x500000 dans notre cas)
*/
void identity_paging_user(pde32_t *pgd, pte32_t* ptb,unsigned int flags,unsigned int offset){

    //Flush les octets de la pgd
    memset((void*)pgd, 0, PAGE_SIZE);

    unsigned int i;

    /* Mapping des adresses allant de offset+ 0x000000 à offset+0x100000
    car on a définit dans linder.lds des userspaces qui font 0x100000 octets */
    for(i=0;i<256;i++) {
        pg_set_entry(&ptb[i], flags, i+offset);
    }    

    /* Les pages suivantes sont marquées absentes */
    for(i=256;i<1024;i++) {
        pg_set_entry(&ptb[i], flags, i+offset);
        ptb[i].p = 0x0;

    }    

    pg_set_entry(&pgd[0], flags, page_nr(ptb));
}


// Fonction de test de changement de tache 
void userland()
{
    debug("TEST\n");
    while(1);
}

// Idem
void switch_user(uint32_t pgd, uint32_t ustack){

    set_ds(d3_sel);
    set_es(d3_sel);
    set_fs(d3_sel);
    set_gs(d3_sel);
    set_tr(ts_sel);
    
    TSS.s0.ss = d0_sel;
    TSS.s0.esp = get_ebp();
    
    debug("ustack %x\n",ustack);
    debug("pgd %x\n",pgd);

    //set_cr3(pgd);
      asm volatile (
      "push %0 \n" // ss
      "push %1 \n" // esp
      "pushf   \n" // eflags
      "push %2 \n" // cs
      "push %3 \n" // eip
      "iret"
      ::
       "i"(d3_sel),
       "m"(ustack),
       "i"(c3_sel),
       "r"(&userland)
      );
}

void tp()
{
    /*Etape 0 : Diviser la mémoire physique en différentes sections, cf linker.lds*/
    debug("\n-------PHYSICAL MEM-------\n");
    debug("kernel mem [%p - %p]\n", &__kernel_start__, &__kernel_end__);
    debug("user1 mem [%p - %p]\n", &__user1_begin__, &__user1_end__);
    debug("user2 mem [%p - %p]\n", &__user2_begin__, &__user2_end__);
    debug("shared mem [%p -%p]\n", &__shared_task_mem_begin__,&__shared_task_mem_end__);

    /* Etape 1 : Configurer la GDT */
    debug("\nSTEP 1: Configuring GDT\n");
    debug("GDT[0] = NULL\n");
    debug("GDT[1] = CODE_RING_0\n" );
    debug("GDT[2] = DATA_RING_0\n" );
    debug("GDT[3] = CODE_RING_3\n" );
    debug("GDT[4] = DATA_RING_3\n" );
    debug("GDT[5] = TSS\n" );
    setup_segmentation();

    /*Etape 2 : Configurer la pagination du noyau => mapping de 0x000000 à 0x800000 */
    debug("\n-------VIRTUAL MEM-------\n");
    pde32_t *pgd_kernel = (pde32_t*)0x200000;
    pte32_t *ptb_kernel = (pte32_t*)0x201000;

    debug("\nSTEP 2: Pagging of kernelspace\n");
    debug("Identity mapping kernelspace to physical adresses 0x000000-0x800000...\n");
    identity_paging_krn(pgd_kernel,ptb_kernel,PG_KRN|PG_RW);
    debug("PGD of kernel is at %p\n",pgd_kernel);
    debug("First PTB of kernel is at %p and map 0x000000-0x400000\n",ptb_kernel);
    debug("Second PTB of kernel is at %p and map 0x400000-0x800000\n\n",ptb_kernel+0x1000);
    
    /* Etape 3 : Activer la pagination */
    debug("\nSTEP 3 : Enabling paging\n");
    set_cr3((uint32_t)pgd_kernel);
    uint32_t cr0 = get_cr0();
    set_cr0(cr0|CR0_PG);


    /* Etape 4 : configurer la pagination dans la section user1 */
    pde32_t *pgd_user1 = (pde32_t*)0x410000;
    pte32_t *ptb_user1 = (pte32_t*)0x411000;

    debug("\nSTEP 4 : Pagging of userspace1\n");
    debug("Identity mapping userspace1 to physical adresses 0x400000-0x500000...\n");
    identity_paging_user(pgd_user1,ptb_user1,PG_USR|PG_RW,1024);
    debug("PGD of user1 is at %p\n",pgd_user1);
    debug("First PTB of user1 is at %p and map 0x400000-0x500000\n\n",ptb_user1);

    // for(int i = 0;i < 1024;i++){
    //     debug("PTB[%d] = 0x%x000\n",i, ptb_user1[i].addr);
    // }

    /* Etape 5 : configurer la pagination de la section user2 */
    pde32_t *pgd_user2 = (pde32_t*)0x510000;
    pte32_t *ptb_user2 = (pte32_t*)0x511000;

    debug("\nSTEP 5: Pagging of userspace2\n");
    debug("Identity mapping userspace2 to physical adresses 0x500000-0x600000...\n");
    identity_paging_user(pgd_user2,ptb_user2,PG_USR|PG_RW,1280);
    debug("PGD of user1 is at %p\n",pgd_user2);
    debug("First PTB of user1 is at %p and map 0x500000-0x600000\n\n",ptb_user2);

    // for(int i = 0;i < 1024;i++){
    //     debug("PTB[%d] = 0x%x000\n",i, ptb_user2[i].addr);
    // }

    /* Etape 6 : Mapper la page partagée entre les deux tâches */
    //mapper pour user1 (on mappe 0x800000-0x801000 à 0x800000-0x801000)

    debug("\nSTEP 6 : Pagging of shared mem\n");
    debug("Identity mapping userspace1 0x800000- 0x801000 to shared memory 0x800000- 0x801000...\n");
    pte32_t *ptb_user1_2 = ptb_user1 + 0x2000;
    pg_set_entry(&pgd_user1[2], PG_USR | PG_RW, page_nr(ptb_user1_2));
    pg_set_entry(&ptb_user1_2[0], PG_USR | PG_RW, page_nr(0x800000));

    //mapper pour user2 (on mappe 0x801000-0x802000 à 0x800000-0x801000)
    debug("Identity mapping userspace1 0x801000- 0x802000 to shared memory 0x800000- 0x801000...\n");
    pte32_t *ptb_user2_2 = ptb_user2 + 0x2000;
    pg_set_entry(&pgd_user2[2], PG_USR | PG_RW, page_nr(ptb_user2_2));
    pg_set_entry(&ptb_user2_2[1], PG_USR | PG_RW, page_nr(0x800000));

    /*Etape 7 : Définir les piles users de chaque tâche allant des @ virtuelles (BASE_USER + 0x0fffff) à (BASE_USER + 0x0fefff) */
    debug("\nSTEP 7 : Defining stacks for tasks\n");
    uint32_t *user_stack_user1_begin = (uint32_t *) (0x400000 + 0x0fffff);
    uint32_t *user_stack_user1_end = (uint32_t *) (0x400000 + 0x0fefff);
    debug("User stack for user1 will be %p-%p\n",user_stack_user1_begin,user_stack_user1_end);

    uint32_t *user_stack_user2_begin = (uint32_t *) (0x500000 + 0x0fffff);
    uint32_t *user_stack_user2_end = (uint32_t *) (0x500000 + 0x0fefff);
    debug("User stack for user2 will be %p-%p\n",user_stack_user2_begin,user_stack_user2_end);

    
    // /*Etape 8 : Définir les piles users de chaque tâche allant des @ virtuelles (BASE_USER + 0x0feffe ) à (BASE_USER + 0x0fdffe) */
    uint32_t *kernel_stack_user1_begin = (uint32_t *) (0x400000 + 0x0feffe);
    uint32_t *kernel_stack_user1_end = (uint32_t *) (0x400000 + 0x0fdffe);
    debug("Kernel stack for user1 will be %p-%p\n",kernel_stack_user1_begin,kernel_stack_user1_end);

    uint32_t *kernel_stack_user2_begin = (uint32_t *) (0x500000 + 0x0feffe);
    uint32_t *kernel_stack_user2_end = (uint32_t *) (0x500000 + 0x0fdffe);
    debug("Kernel stack for user2 will be %p-%p\n",kernel_stack_user2_begin,kernel_stack_user2_end);

    /* TESTS de la pagination */
    //switch_user((uint32_t)pgd_user1,(uint32_t) user_stack_user1_begin);
}
