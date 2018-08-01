#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OSMC");
MODULE_DESCRIPTION("Amlogic WiFi enumeration module");

extern void extern_wifi_set_enable(int);
extern void sdio_notify(int);
extern void sdio_reinit(void);

static int __init wifi_scan_init(void) {
	pr_info("WiFi enumeration module initialised\n");

	extern_wifi_set_enable(1);
	msleep(200);
	sdio_notify(1);
	sdio_reinit();

	return 0;

}

static void __exit wifi_scan_exit(void) {
    pr_info("WiFi enumeration module exited\n");
}

module_init(wifi_scan_init);
module_exit(wifi_scan_exit);
