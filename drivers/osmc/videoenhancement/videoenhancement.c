// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018-2019 Linaro Ltd.
 * Copyright (C) 2021 OSMC
 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/tee_drv.h>
#include <linux/tee.h>
#include <linux/uuid.h>

#define DRIVER_NAME "osmc_videoenhancement"

#define TEEC_SUCCESS	0

#define TA_VIDEOENHANCEMENT_CMD_TONEMAP	5

#define CONTRASTS_SIZE (2 * sizeof(u32))
#define CUSTOM_MAP_SIZE (33 * sizeof(u32))
#define LUT_SHM_SIZE (CONTRASTS_SIZE + CUSTOM_MAP_SIZE)

#ifndef UUID_INIT
	/* Hack to support older kernels */
	#define UUID_INIT(a, b, c, d0, d1, d2, d3, d4, d5, d6, d7)					\
	((uuid_t)																	\
	{ { ((a) >> 24) & 0xff, ((a) >> 16) & 0xff, ((a) >> 8) & 0xff, (a) & 0xff,	\
	   ((b) >> 8) & 0xff, (b) & 0xff,											\
	   ((c) >> 8) & 0xff, (c) & 0xff,											\
	   (d0), (d1), (d2), (d3), (d4), (d5), (d6), (d7) } })
#endif

static struct tee_context *ctx;
static u32 ta_videoenhancement_session_id;
static struct tee_client_device *optee_device;

static struct tee_shm *lut_shm;

int vpp_pq_lut_curve_calc(int target_lumin, int master_lumin, int contrasts[], int custom_map[])
{
	struct tee_ioctl_invoke_arg args = {0};
	struct tee_param param[4] = {0};
	u32 *mem = tee_shm_get_va(lut_shm, 0);
	u32 ret = 0;

	/* invoke TA_VIDEOENHANCEMENT_CMD_TONEMAP function */
	args.func = TA_VIDEOENHANCEMENT_CMD_TONEMAP;
	args.session = ta_videoenhancement_session_id;
	args.num_params = 4;

	param[0].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = target_lumin;

	param[1].attr = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[1].u.value.a = master_lumin;

	param[2].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[2].u.memref.shm = lut_shm;
	param[2].u.memref.shm_offs = 0;
	param[2].u.memref.size = CONTRASTS_SIZE;
	memcpy(mem, contrasts, CONTRASTS_SIZE);

	param[3].attr = TEE_IOCTL_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
	param[3].u.memref.shm = lut_shm;
	param[3].u.memref.shm_offs = CONTRASTS_SIZE;
	param[3].u.memref.size = CUSTOM_MAP_SIZE;

	ret = tee_client_invoke_func(ctx, &args, param);
	if (ret < 0 || args.ret != TEEC_SUCCESS) {
		pr_err("TA_VIDEOENHANCEMENT_CMD_TONEMAP invoke function err: %x\n", args.ret);
		return 0;
	}

	memcpy(contrasts, mem, CONTRASTS_SIZE);
	memcpy(custom_map, mem+2, CUSTOM_MAP_SIZE);

	return 1;
}
EXPORT_SYMBOL(vpp_pq_lut_curve_calc);

static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data)
{
	if (ver->impl_id == TEE_IMPL_ID_OPTEE) {
		return 1;
	} else {
		return 0;
	}
}

static int optee_videoenhancement_probe(struct device *dev)
{
	struct tee_client_device *venh_device = to_tee_client_device(dev);
	int ret = 0, err = -ENODEV;
	struct tee_ioctl_open_session_arg sess_arg = {0};

	/* Open context with TEE driver */
	ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
	if (IS_ERR(ctx)) {
		return -EPROBE_DEFER;
	}

	/* Open session with OSMC videoenhancement TA */
	memcpy(sess_arg.uuid, venh_device->id.uuid.b, TEE_IOCTL_UUID_LEN);
	sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
	sess_arg.num_params = 0;

	ret = tee_client_open_session(ctx, &sess_arg, NULL);
	if (ret < 0 || sess_arg.ret != 0) {
		if (sess_arg.ret != 0xffff000e || sess_arg.ret_origin != 3) {
			/* if it's not a TA communication error, then print the error we've got */
			pr_warn("tee_client_open_session failed, error: 0x%x, origin: 0x%x\n", sess_arg.ret, sess_arg.ret_origin);
		}
		err = -EPROBE_DEFER;
		goto out_ctx;
	}

	ta_videoenhancement_session_id = sess_arg.session;

	/* create TEE shared memory for the lut function */

	lut_shm = tee_shm_alloc(ctx, LUT_SHM_SIZE, TEE_SHM_MAPPED | TEE_SHM_DMA_BUF);
	if (IS_ERR(lut_shm)) {
		pr_err("tee_shm_alloc failed\n");
		err = PTR_ERR(lut_shm);
		goto out_ctx;
	}

	pr_info("osmc videoenhancement: session context configured successfully\n");

	return 0;

out_ctx:
	tee_client_close_context(ctx);

	return err;
}

static int optee_videoenhancement_remove(struct device *dev)
{
	tee_client_close_session(ctx, ta_videoenhancement_session_id);
	tee_client_close_context(ctx);

	return 0;
}

const struct tee_client_device_id optee_videoenhancement_id_table[] = {
	{ UUID_INIT(0x23d6d081, 0x0138, 0x46a4,
			0xbb, 0xa5, 0x21, 0xb1, 0x89, 0xd1, 0xa9, 0x7e) },
	{}
};

static struct tee_client_driver optee_videoenhancement_driver = {
	.id_table	= optee_videoenhancement_id_table,
	.driver		= {
		.name		= DRIVER_NAME,
		.bus		= &tee_bus_type,
		.probe		= optee_videoenhancement_probe,
		.remove		= optee_videoenhancement_remove,
	},
};

static inline void uuid_copy(uuid_t *dst, const uuid_t *src)
{
	memcpy(dst, src, sizeof(uuid_t));
}

static int __init mod_init(void)
{
	int rc;

	rc = driver_register(&optee_videoenhancement_driver.driver);
	if (rc) {
		pr_warn("osmc videoenhancement driver registration failed, err: %d\n", rc);
		return rc;
	}

	optee_device = kzalloc(sizeof(*optee_device), GFP_KERNEL);
	if (!optee_device) {
		return -ENOMEM;
	}

	optee_device->dev.bus = &tee_bus_type;
	dev_set_name(&optee_device->dev, DRIVER_NAME);
	uuid_copy(&optee_device->id.uuid, &optee_videoenhancement_driver.id_table->uuid);

	rc = device_register(&optee_device->dev);
	if (rc) {
		pr_warn("unable to register osmc videoenhancement device, err: %d\n", rc);
		kfree(optee_device);
	}

	return rc;
}

static void __exit mod_exit(void)
{
	if (optee_device) {
		device_unregister(&optee_device->dev);
		kfree(optee_device);
		optee_device = NULL;
	}

	driver_unregister(&optee_videoenhancement_driver.driver);
}

module_init(mod_init);
module_exit(mod_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("OSMC Video Enhancement TA driver");
