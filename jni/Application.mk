
# Uncomment this if you're using STL in your project
# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
# APP_STL := stlport_static
APP_STL := gnustl_static
APP_ABI := armeabi-v7a x86 armeabi
APP_CPPFLAGS += -std=c++11 -Wno-literal-suffix -Wno-unused-function
APP_PLATFORM := android-15
