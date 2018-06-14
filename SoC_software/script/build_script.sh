#!/bin/sh -x


arm-linux-gnueabihf-gcc \
-g \
-O0 \
-Wall \
-Werror \
-I${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include \
-I${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av \
-D soc_cv_av \
../src/main.c \
-o ../tcp_test
