# 编译为独立内核模块 (.ko)
obj-m += hello.o
hello-objs := Hello.o

# 本地编译用 (可选)
# KDIR ?= /lib/modules/$(shell uname -r)/build
# 
# all:
# 	make -C $(KDIR) M=$(PWD) modules
# 
# clean:
# 	make -C $(KDIR) M=$(PWD) clean
