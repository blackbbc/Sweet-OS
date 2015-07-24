#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"


PRIVATE void cleanup(struct proc * proc);


PUBLIC int mm_fork()
{
    struct proc* p = proc_table;
    int i;
    for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++)
        if (p->p_flags == FREE_SLOT)
            break;

    int child_pid = i;
    assert(p == &proc_table[child_pid]);
    assert(child_pid >= NR_TASKS + NR_NATIVE_PROCS);
    if (i == NR_TASKS + NR_PROCS)
        return -1;
    assert(i < NR_TASKS + NR_PROCS);

    int pid = mm_msg.source;
    printl("{MM} pid: %d\n", pid);
    u16 child_ldt_sel = p->ldt_sel;
    *p = proc_table[pid];
    p->ldt_sel = child_ldt_sel;
    p->p_parent = pid;
    p->pid = child_pid;
    sprintf(p->name, "%s_%d", proc_table[pid].name, child_pid);

    struct descriptor * ppd;

    ppd = &proc_table[pid].ldts[INDEX_LDT_C];
    printl("{MM} name: %s\n", proc_table[pid].name);
    int caller_T_base  = reassembly(ppd->base_high, 24,
                    ppd->base_mid,  16,
                    ppd->base_low);
    int caller_T_limit = reassembly(0, 0,
                    (ppd->limit_high_attr2 & 0xF), 16,
                    ppd->limit_low);
    int caller_T_size  = ((caller_T_limit + 1) *
                  ((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
                   4096 : 1));
    printl("{MM} %x,%x,%x,%x,%x,%x)\n", ppd->limit_low, ppd->base_low,  ppd->base_high, ppd->base_mid, ppd->attr1, ppd->limit_high_attr2);
    printl("{MM} %x\n", caller_T_limit+1);
    printl("{MM} %x\n", (ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)));
    printl("{MM} %x\n", ((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8))? 4096:1));
    printl("{MM} %x\n", (caller_T_limit+1)*((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8))? 4096:1));
    printl("{MM} %x\n", ((caller_T_limit+1)>>10)*((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8))? 4096:1));

    ppd = &proc_table[pid].ldts[INDEX_LDT_RW];
    printl("{MM} %x,%x,%x,%x,%x,%x)\n", ppd->limit_low, ppd->base_low,  ppd->base_high, ppd->base_mid, ppd->attr1, ppd->limit_high_attr2);
    int caller_D_S_base  = reassembly(ppd->base_high, 24,
                      ppd->base_mid,  16,
                      ppd->base_low);
    int caller_D_S_limit = reassembly((ppd->limit_high_attr2 & 0xF), 16,
                      0, 0,
                      ppd->limit_low);
    int caller_D_S_size  = ((caller_T_limit + 1) *
                ((ppd->limit_high_attr2 & (DA_LIMIT_4K >> 8)) ?
                 4096 : 1));

    printl("{MM} base: %d, limit: %d, size: %d)\n", caller_T_base, caller_T_limit, caller_T_size);
    assert((caller_T_base  == caller_D_S_base ) &&
           (caller_T_limit == caller_D_S_limit) &&
           (caller_T_size  == caller_D_S_size ));

    caller_T_size = caller_D_S_size = 0x100000;
    int child_base = alloc_mem(child_pid, caller_T_size);
    printl("{MM} childpid:%x  0x%x <- 0x%x (0x%x bytes)\n",
            child_pid, child_base, caller_T_base, caller_T_size);
    phys_copy((void*)child_base, (void*)caller_T_base, caller_T_size);

    init_descriptor(&p->ldts[INDEX_LDT_C],
          child_base,
          (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
          DA_LIMIT_4K | DA_32 | DA_C | PRIVILEGE_USER << 5);
    init_descriptor(&p->ldts[INDEX_LDT_RW],
          child_base,
          (PROC_IMAGE_SIZE_DEFAULT - 1) >> LIMIT_4K_SHIFT,
          DA_LIMIT_4K | DA_32 | DA_DRW | PRIVILEGE_USER << 5);

    // MESSAGE msg2fs;
    // msg2fs.type = FORK;
    // msg2fs.PID = child_pid;
    // send_recv(BOTH, TASK_FS, &msg2fs);

    mm_msg.PID = child_pid;

    MESSAGE m;
    m.type = SYSCALL_RET;
    m.RETVAL = 0;
    m.PID = 0;
    send_recv(SEND, child_pid, &m);

    return 0;
}


PUBLIC void mm_exit(int status)
{
}


PRIVATE void cleanup(struct proc * proc)
{
}


PUBLIC void mm_wait()
{
}
