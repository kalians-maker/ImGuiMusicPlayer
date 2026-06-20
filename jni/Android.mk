LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE := imGuiTest.sh

LOCAL_CFLAGS := -std=c17
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_CPPFLAGS := -std=c++17
LOCAL_CPPFLAGS += -fvisibility=hidden

LOCAL_CFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_CFLAGS += -DIMGUI_IMPL_VULKAN_NO_PROTOTYPES
LOCAL_CPPFLAGS += -DVK_USE_PLATFORM_ANDROID_KHR
LOCAL_CPPFLAGS += -DIMGUI_IMPL_VULKAN_NO_PROTOTYPES
LOCAL_CPPFLAGS += -DIMGUI_DISABLE_DEBUG_TOOLS #禁用调试工具


#引入头文件到全局#
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/Android_draw
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/Android_Graphics
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/Android_touch
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ImGui
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/ImGui/backends
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/tools
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/stb_image
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/MusicTools
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/Pictures


LOCAL_SRC_FILES := src/main.cpp
LOCAL_SRC_FILES += src/Android_draw/draw_Gui.cpp
LOCAL_SRC_FILES += src/Android_draw/TouchHelperA.cpp
LOCAL_SRC_FILES += src/Android_Graphics/GraphicsManager.cpp
LOCAL_SRC_FILES += src/Android_Graphics/OpenGLGraphics.cpp
LOCAL_SRC_FILES += src/Android_Graphics/VulkanGraphics.cpp 
LOCAL_SRC_FILES += src/Android_Graphics/vulkan_wrapper.cpp
LOCAL_SRC_FILES += src/ImGui/AndroidImgui.cpp
LOCAL_SRC_FILES += src/ImGui/my_imgui.cpp
LOCAL_SRC_FILES += src/ImGui/my_imgui_impl_android.cpp
LOCAL_SRC_FILES += src/ImGui/imgui.cpp
LOCAL_SRC_FILES += src/ImGui/imgui_demo.cpp
LOCAL_SRC_FILES += src/ImGui/imgui_draw.cpp
LOCAL_SRC_FILES += src/ImGui/imgui_tables.cpp
LOCAL_SRC_FILES += src/ImGui/imgui_widgets.cpp
LOCAL_SRC_FILES += src/ImGui/imgui_impl_android.cpp
LOCAL_SRC_FILES += src/ImGui/imgui_impl_opengl3.cpp
LOCAL_SRC_FILES += src/ImGui/imgui_impl_vulkan.cpp
LOCAL_SRC_FILES += src/ImGui/stb_image.cpp
LOCAL_SRC_FILES += src/MusicTools/MusicManager.cpp    


LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3

LOCAL_LDLIBS += -lOpenSLES -llog -landroid # 链接 Android 底层音频库、日志库和原生系统库
include $(BUILD_EXECUTABLE) #可执行文件
