# From mm/Makefile

obj-$(CONFIG_FRAME_VECTOR) += frame_vector.o

KDIRA          := /lib/modules/$(KERNELRELEASE)/kernel

mm-install install-mm::
	@dir="mm"; \
	files='frame_vector.ko'; \
	if [ -f $$files ]; then \
	echo -e "\nInstalling $(KDIRA)/$$dir files:"; \
	install -d $(KDIRA)/$$dir; \
	for i in $$files;do if [ -e $$i ]; then echo -n "$$i "; \
	install -m 644 -c $$i $(KDIRA)/$$dir; fi; done; echo; fi
