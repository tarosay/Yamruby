LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := vm_module
LOCAL_CFLAGS	:= -Wall
LOCAL_C_INCLUDES	:= jni/mruby/include jni/mruby/src

LOCAL_SRC_FILES := $(shell cd jni; echo *.c)
LOCAL_SRC_FILES += mruby/build/host/mrblib/mrblib.c
LOCAL_SRC_FILES += $(sort $(shell cd jni; echo mruby/src/*.c) mruby/build/host/src/y.tab.c)

LOCAL_SRC_FILES += mruby/build/host/mrbgems/gem_init.c

LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-array-ext/src/*.c)
#LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-enum-ext/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-eval/src/*.c)
#LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-hash-ext/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-math/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-numeric-ext/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-print/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-random/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-sprintf/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-string-ext/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-struct/src/*.c)
LOCAL_SRC_FILES += $(shell cd jni; echo mruby/mrbgems/mruby-time/src/*.c)

LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-array-ext/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-enum-ext/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-hash-ext/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-math/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-numeric-ext/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-print/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-sprintf/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-string-ext/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-struct/gem_init.c
LOCAL_SRC_FILES += mruby/build/host/mrbgems/mruby-time/gem_init.c

LOCAL_LDLIBS    := -llog

MY_SRC_FILES := $(addprefix jni/, $(LOCAL_SRC_FILES))
$(MY_SRC_FILES):
		cd jni/mruby;make

clobber: clean
		cd jni/mruby;make clean

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_EXECUTABLE)
