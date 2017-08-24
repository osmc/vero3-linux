#
#Makefile for the gpio dirver
#
ifneq ($(CONFIG_ARM64),)
obj-$(CONFIG_PINCTRL_AMLOGIC) += pinctrl_amlogic.o sample_code.o \
                                 pinctrl_gxbb.o pinctrl_gxtvbb.o \
                                 pinctrl_gxl.o pinctrl_txl.o     \
                                 pinctrl_txlx.o pinctrl_gxlx.o   \
                                 pinctrl-meson.o pinctrl-txhd.o
else
obj-$(CONFIG_PINCTRL_AMLOGIC) += pinctrl_amlogic.o sample_code.o \
                                 pinctrl_m8.o
endif
