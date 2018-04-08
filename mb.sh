		VERSION=3.14.29
                kernel_path=$(pwd)
                pushd media_build
                make untar
                cp -a "../drivers/amlogic/video_dev" "linux/drivers/media/"
                sed -i 's,common/,,g; s,"trace/,",g' $(find linux/drivers/media/video_dev/ -type f)
                sed -i 's,\$(CONFIG_V4L_AMLOGIC_VIDEO),m,g' "linux/drivers/media/video_dev/Makefile"
                echo "obj-y += video_dev/" >> "linux/drivers/media/Makefile"
                echo "source drivers/media/video_dev/Kconfig " >> "linux/drivers/media/Kconfig"
                cp -a "${kernel_path}/drivers/media/v4l2-core/videobuf-res.c" "linux/drivers/media/v4l2-core/"
                cp -a "${kernel_path}/include/media/videobuf-res.h" "linux/include/media/"
                echo "obj-m += videobuf-res.o" >> "linux/drivers/media/v4l2-core/Makefile"
                make VER=${VERSION} SRCDIR=$(pwd)/../

