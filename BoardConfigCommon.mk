# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This file sets variables that control the way modules are built
# thorughout the system. It should not be used to conditionally
# disable makefiles (the proper mechanism to control what gets
# included in a build is to use PRODUCT_PACKAGES in a product
# definition file).

DEVICE_PACKAGE_OVERLAYS += device/htc/qsd8k-common/overlay

TARGET_NO_BOOTLOADER := true

# Arch
TARGET_BOARD_PLATFORM := qsd8k
TARGET_BOARD_PLATFORM_GPU := qcom-adreno200
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_VARIANT := scorpion
TARGET_ARCH_LOWMEM := true
TARGET_USE_QCOM_BIONIC_OPTIMIZATION := true
ARCH_ARM_HAVE_TLS_REGISTER := true

# Audio
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_LEGACY_ALSA_AUDIO := true
USE_LEGACY_AUDIO_POLICY := true
AUDIO_FEATURE_ENABLED_INCALL_MUSIC := false
AUDIO_FEATURE_ENABLED_COMPRESS_VOIP := false
AUDIO_FEATURE_ENABLED_PROXY_DEVICE := false

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true
BOARD_BLUEDROID_VENDOR_CONF := device/htc/qsd8k-common/bluetooth/vnd_qsd8k.txt
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := device/htc/qsd8k-common/bluetooth/include

# Camera
COMMON_GLOBAL_CFLAGS += -DNEEDS_VECTORIMPL_SYMBOLS
BOARD_USE_OLD_AVC_ENCODER := true
BOARD_NO_BFRAMES := true
BOARD_USES_QCOM_LEGACY_CAM_PARAMS := true
TARGET_NEEDS_PRELINK_SUPPORT := true
TARGET_NEEDS_BIONIC_PRELINK_SUPPORT := true
BOARD_USES_PMEM_ADSP := true
TARGET_NEEDS_NON_PIE_SUPPORT := true
TARGET_DISABLE_ARM_PIE := true
TARGET_USES_SUBMIT_ONE_INPUT_BUFFER := true

# Compass/Accelerometer
BOARD_VENDOR_USE_AKMD := akm8973

# Display
BOARD_USES_LEGACY_QCOM_DISPLAY := true
BOARD_EGL_CFG := device/htc/qsd8k-common/egl.cfg
USE_OPENGL_RENDERER := true
BOARD_ADRENO_DECIDE_TEXTURE_TARGET := true
TARGET_DISABLE_TRIPLE_BUFFERING := true
BOARD_NEEDS_MEMORYHEAPPMEM := true
BOARD_EGL_WORKAROUND_BUG_10194508 := true
TARGET_NO_HW_VSYNC := true
TARGET_USES_ION := true
COMMON_GLOBAL_CFLAGS += -DTARGET_8x50
COMMON_GLOBAL_CFLAGS += -DQCOM_NO_SECURE_PLAYBACK
HWUI_COMPILE_FOR_PERF := true

# Kernel
TARGET_KERNEL_SOURCE := kernel/htc/qsd8k
BUILD_KERNEL := true
TARGET_RAMDISK_COMPRESSION := xz --check=crc32 --arm --lzma2=dict=1MiB
#KERNEL_TOOLCHAIN_PREFIX:=$(ANDROID_BUILD_TOP)/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi-

# Fix slow boot
TARGET_RUNNING_WITHOUT_SYNC_FRAMEWORK := true
BOARD_USES_HWCOMPOSER := true

# GPS
TARGET_QCOM_GPS_VARIANT := legacy
BOARD_VENDOR_QCOM_AMSS_VERSION := 3200

# Use custom healthd HAL
BOARD_HAL_STATIC_LIBRARIES := libhealthd.qsd8k

# Skip recovery for now to keep things moving
TARGET_SKIP_RECOVERY_BUILD := true

# Screenshot fix
TARGET_FORCE_SCREENSHOT_CPU_PATH := true

# Wifi
WIFI_BAND                        := 802_11_ABG
WPA_SUPPLICANT_VERSION           := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_bcmdhd
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_bcmdhd
BOARD_WLAN_DEVICE                := bcmdhd
WIFI_DRIVER_FW_PATH_STA          := "/vendor/firmware/fw_bcmdhd.bin"
WIFI_DRIVER_FW_PATH_AP           := "/vendor/firmware/fw_bcmdhd_apsta.bin"
WIFI_DRIVER_FW_PATH_PARAM        := "/sys/module/bcmdhd/parameters/firmware_path"
BOARD_LEGACY_NL80211_STA_EVENTS  := true
