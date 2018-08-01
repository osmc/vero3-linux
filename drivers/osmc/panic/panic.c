#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>

/*
 * OSMC diagnostic fault-injection module.
 *
 * Loading it injects one kernel fault so the panic / lockup-detector /
 * reboot paths can be verified on a UART (and so we can confirm whether a
 * given fault produces a captured ramoops + automatic reboot, or a silent
 * hang). Select the fault with the "mode" parameter:
 *
 *   modprobe panic              -> mode 0, plain panic() (back-compat)
 *   modprobe panic mode=1       -> soft lockup (spin, preemption disabled, IRQs on)
 *   modprobe panic mode=2       -> hard lockup (spin, local IRQs disabled)
 *   modprobe panic mode=3       -> hung task   (kthread parked in TASK_UNINTERRUPTIBLE)
 *
 * mode 0  exercises the clean panic -> kernel.panic reboot -> ramoops path.
 * mode 1  needs CONFIG_BOOTPARAM_SOFTLOCKUP_PANIC (or kernel.softlockup_panic=1).
 * mode 2  needs the cross-CPU hard-lockup detector + CONFIG_BOOTPARAM_HARDLOCKUP_PANIC.
 * mode 3  needs CONFIG_DETECT_HUNG_TASK (+ kernel.hung_task_panic=1 to reboot).
 *
 * Modes 1 and 2 never return from init by design (the modprobe thread is the
 * wedged context); mode 3 loads cleanly and leaves an unkillable parked task,
 * so a reboot is required to clear it.
 */

static int mode;
module_param(mode, int, 0644);
MODULE_PARM_DESC(mode, "0=panic 1=softlockup 2=hardlockup 3=hungtask");

static struct task_struct *hung_thread;

static int hung_task_fn(void *data)
{
	/* Park forever in D state so the hung-task detector trips. */
	set_current_state(TASK_UNINTERRUPTIBLE);
	schedule_timeout(MAX_SCHEDULE_TIMEOUT);
	return 0;
}

static int __init panic_osmc_init(void)
{
	switch (mode) {
	case 1:
		pr_emerg("panic_osmc: forcing SOFT lockup (preempt off, spin)\n");
		preempt_disable();
		while (1)
			cpu_relax();
		break;
	case 2:
		pr_emerg("panic_osmc: forcing HARD lockup (local IRQs off, spin)\n");
		local_irq_disable();
		while (1)
			cpu_relax();
		break;
	case 3:
		pr_emerg("panic_osmc: parking a task in D state (hung task)\n");
		hung_thread = kthread_run(hung_task_fn, NULL, "osmc_hung");
		return 0;
	case 0:
	default:
		panic("panic_osmc: creating kernel panic.\n");
	}

	return 0;
}

static void __exit panic_osmc_deinit(void)
{
   return;
}

module_init(panic_osmc_init);
module_exit(panic_osmc_deinit);

MODULE_LICENSE("GPL");
