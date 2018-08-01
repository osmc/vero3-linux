#include <linux/module.h>
#include <linux/kernel.h>

static int __init panic_osmc_init(void)
{
    panic("panic_osmc: creating kernel panic.\n");

    return 0;
}

static void __exit panic_osmc_deinit(void)
{
   return;
}

module_init(panic_osmc_init);
module_exit(panic_osmc_deinit);

MODULE_LICENSE("GPL");
