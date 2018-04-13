LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libusb1.0
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../android/libs/$(TARGET_ARCH_ABI)/libusb1.0.so
include $(PREBUILT_SHARED_LIBRARY)



include $(CLEAR_VARS)
#LOCAL_LDLIBS := -llog
LOCAL_MODULE := libcdrom
LOCAL_SRC_FILES := ../libcdrom.c 

LOCAL_C_INCLUDES += $(LOCAL_PATH)../  \
  $(LOCAL_PATH)/../.. \
  $(LOCAL_PATH)/../../libusb \
  $(LOCAL_PATH)/../../libusb/os
  
LOCAL_SHARED_LIBRARIES := libusb1.0 libutils libcutils liblog
#LOCAL_SHARED_LIBRARIES := -L$(LOCAL_PATH)/../../android/libs/$(TARGET_ARCH_ABI)/  -libusb1.0 
LOCAL_CFLAGS := -O2 -D__ANDROID__
#LOCAL_LDFLAGS := -L$(LOCAL_PATH)/../../android/libs/$(TARGET_ARCH_ABI)/  -lusb1.0 -llog
LOCAL_LDLIBS := -llog
#LOCAL_LDFLAGS :=  -llog
#LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog
include $(BUILD_SHARED_LIBRARY)
