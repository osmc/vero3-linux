#
# Makefile for VDIN.
#
ifeq ($(TARGET_BUILD_VARIANT),userdebug)
ccflags-y := -D DEBUG_SUPPORT
else
ccflags-y := -DDEBUG
endif
obj-$(CONFIG_TVIN_VDIN) = tvin_vdin.o
tvin_vdin-objs := vdin_v4l2.o vdin_vf.o vdin_debug.o vdin_drv.o vdin_ctl.o vdin_sm.o vdin_canvas.o

