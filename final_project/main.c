#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>  
#include <linux/mm.h>
#include <linux/pid.h>
#include <linux/sched/task.h>

#define MODULE_NAME "main"

// pid: pid of current running program
static int int_pid;
module_param(int_pid, int, 0644);

int vma_page_ctr = 0;
static pte_t *find_pte(struct mm_struct *mm, unsigned long addr);
void print_intv(unsigned long start, unsigned long end);


// filter vma and print
void mtest_list_vma(struct task_struct *ts)
{
    struct mm_struct *mm = ts->mm;
    struct vm_area_struct *vma;
    unsigned long intv_start = 0, intv_end = 0;
    unsigned long i;
    bool flag = false;
    pte_t *pte;

    // traverse vmas
    for (vma = mm->mmap; vma; vma = vma->vm_next)
    {
        // traverse pages
        for (i = vma->start; i < vma->end; ++i)
        {
            pte = find_pte(mm, i);
            if (pte == NULL)
            {
                // now we've detected an interval
                if (flag)
                {
                    flag = false;
                    print_intv(intv_start, intv_end);
                }
            }
            // we've found a page!
            else
            {
                // print info
                printk(KERN_INFO "Heat info of address 0x%lx is %d", i, pte_young(*pte));
                printk(KERN_INFO "\n");
                pte_mkold(*pte);

                vma_page_ctr++;
                // if we meet the end
                if (i == (vma->end - 1))
                {
                    if (flag)
                        print_intv(intv_start, vma->end - 1);
                    else
                        print_intv(vma->end - 1, vma->end - 1);
                }
                // else, general situation
                else
                    if (flag)
                        intv_end = i;
                    else
                    {
                        flag = true;
                        intv_start = i;
                    }                
            }           
        }
    }
}

// print function
void print_intv(unsigned long start, unsigned long end)
{
    printk(KERN_INFO "VMA 0x%lx-0x%lx", start, end);
    printk(KERN_INFO "\n");
}

void analyse_heat(struct task_struct *ts)
{    
    mtest_list_vma(ts);
    printk(KERN_INFO "The total number of pages of our program is %d", );
    printk(KERN_INFO "Filtered VMAs contain %d pages", vma_page_ctr);
    printk(KERN_INFO "Memory contains %lu pages", ts->mm->total_vm);
}



static pte_t *find_pte(struct mm_struct *mm, unsigned long addr)  
{
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    spinlock_t *ptl;

    pgd = pgd_offset(mm, addr);
    if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd)))
    {
        printk(KERN_INFO "pgd not present");
        printk(KERN_INFO "Translation not found");
        return NULL;
    }

    p4d = p4d_offset(pgd, addr);
    if (p4d_none(*p4d) || unlikely(p4d_bad(*p4d)))
    {
        printk(KERN_INFO "p4d not present");
        printk(KERN_INFO "Translation not found");
        return NULL;
    }

    pud = pud_offset(p4d, addr);
    if (pud_none(*pud) || unlikely(pud_bad(*pud)))
    {
        printk(KERN_INFO "pud not present");
        printk(KERN_INFO "Translation not found");
        return NULL;
    }

    pmd = pmd_offset(pud, addr);
    if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
    {
        printk("pmd not present");
        printk("Translation not found");
        return NULL;
    }  


    pte = pte_offset_map_lock(mm, pmd, addr, &ptl);  
    if (!pte)
    {
        printk(KERN_INFO "pte not present");
        printk(KERN_INFO "Translation not found");
        return NULL;
    }

    if (!pte_present(*pte))
    {
        pte_unmap_unlock(pte, ptl);
        return NULL;
    }

    return pte;
}

static int __init main_init(void) 
{
    pid_t nr = int_pid;
    enum pid_type type = PIDTYPE_PID;
    
    // this function defined in kernel/pid.c
    struct pid *heat_pid = find_get_pid(nr);
    struct task_struct *heat_ts = pid_task(heat_pid, type);

    printk(KERN_INFO "module %s created!\n", MODULE_NAME);

    analyse_heat(heat_ts);

    return 0;
}

static void __exit main_exit(void) 
{
    printk(KERN_INFO "module %s removed!\n", MODULE_NAME);
}

module_init(main_init); 
module_exit(main_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module main");
MODULE_AUTHOR("Kenneth Yan");