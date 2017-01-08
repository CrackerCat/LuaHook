LOCAL_PATH := $(call my-dir)

###################################<inejctor>###################################
include $(CLEAR_VARS)

LOCAL_MODULE	:= injector
LOCAL_ARM_MODE	:= arm
LOCAL_CPP_EXTENSION	:= .cpp
LOCAL_CFLAGS	+= -fvisibility=hidden -pie -fPIE
LOCAL_LDLIBS	+= -L$(SYSROOT)/usr/lib -llog
LOCAL_LDFLAGS 	+= -pie -fPIE
LOCAL_SRC_FILES	:= \
				injector\android-injector.cpp \
				injector\injector.cpp

include $(BUILD_EXECUTABLE)

###################################<liblua>###################################
include $(CLEAR_VARS)

LOCAL_MODULE		:= lua
LOCAL_SRC_FILES		:= ./3rd/lua-5.3.3/obj/local/armeabi-v7a/liblua.a
LOCAL_EXPORT_C_INCLUDES	:= $(LOCAL_PATH)/3rd/lua-5.3.3/jni

include $(PREBUILT_STATIC_LIBRARY)

###################################<luahook>###################################
include $(CLEAR_VARS)

LOCAL_MODULE			:= luahook
LOCAL_ARM_MODE			:= arm
LOCAL_CPP_EXTENSION		:= .cpp
LOCAL_LDLIBS			+= -L$(SYSROOT)/usr/lib -llog 
LOCAL_STATIC_LIBRARIES	+= lua
LOCAL_C_INCLUDES		:= $(LOCAL_PATH)/include/ $(LOCAL_PATH)/3rd/lua-5.3.3/jni
LOCAL_SRC_FILES			:= \
						core\armhook.cpp \
						core\common-help.cpp \
						core\proto.cpp \
						core\ue.cpp \
						core\md5.cpp \
						core\lua-loader.cpp \
						core\special-hook.cpp

include $(BUILD_SHARED_LIBRARY)

############# install ##############
include $(CLEAR_VARS)

temp_path	:= /data/local/tmp

all:
	adb push $(NDK_APP_DST_DIR)/injector $(temp_path)
	adb push $(NDK_APP_DST_DIR)/libluahook.so $(temp_path)
	adb shell "su -c 'chmod 777 $(temp_path)/*'"