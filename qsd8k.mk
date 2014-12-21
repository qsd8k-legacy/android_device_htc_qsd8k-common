#
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

# English locale
PRODUCT_LOCALES := en

# High Density art
PRODUCT_AAPT_CONFIG := normal hdpi
PRODUCT_AAPT_PREF_CONFIG := hdpi

# Configs
PRODUCT_COPY_FILES += \
    device/htc/qsd8k-common/media_codecs.xml:system/etc/media_codecs.xml \
    device/htc/qsd8k-common/audio_policy.conf:system/etc/audio_policy.conf

# Misc
PRODUCT_COPY_FILES += \
    device/htc/qsd8k-common/init.power.rc:root/init.power.rc

#
# Required Packages
#

# Audio
PRODUCT_PACKAGES += \
    audio.usb.default \
    audio.a2dp.default \
    audio.primary.qsd8k \
    audio_policy.qsd8k

# Camera
PRODUCT_PACKAGES += \
    camera.qsd8k

# Display
PRODUCT_PACKAGES += \
    copybit.qsd8k \
    gralloc.qsd8k \
    hwcomposer.qsd8k \
    memtrack.qsd8k

# Omx
PRODUCT_PACKAGES += \
    libOmxCore \
    libstagefrighthw

# Filesystem management tools
PRODUCT_PACKAGES += \
    make_ext4fs \
    setup_fs

# Misc
PRODUCT_PACKAGES += \
    power.qsd8k \
    com.android.future.usb.accessory \
    libnetcmdiface

#
# Hardware Rendering Properties
# 
# debug.sf.hw = 1 (Render UI with GPU)
#

PRODUCT_PROPERTY_OVERRIDES += \
    debug.sf.hw=1 \
    debug.sf.no_hw_vsync=1 \
    debug.composition.type=mdp \
    debug.egl.profiler=1 \
    debug.egl.hw=1 \
    debug.composition.type=gpu \
    debug.gr.numframebuffers=2 \
    debug.performance.tuning=1 \
    debug.hwui.render_dirty_regions=false \
    persist.sys.ui.hw=1 \
    persist.webview.provider=classic \
    video.accelerate.hw=1 \
    view.scroll_friction=0 \
    ro.zygote.disable_gl_preload=true

#
# Low Mem tweaks
#
PRODUCT_PROPERTY_OVERRIDES += ro.config.low_ram=true

# Disable jit
PRODUCT_PROPERTY_OVERRIDES += dalvik.vm.jit.codecachesize=0

# Allows purging of assets to free up RAM
PRODUCT_PROPERTY_OVERRIDES += persist.sys.purgeable_assets=1

#
# Battery tweaks
#
# Caution: ro.mot.eri.losalert.delay=1000 (might break tethering)
# pm.sleep_mode=0 := MSM_PM_SLEEP_MODE_POWER_COLLAPSE_SUSPEND
# pm.sleep_mode=1 := MSM_PM_SLEEP_MODE_POWER_COLLAPSE
#
PRODUCT_PROPERTY_OVERRIDES += \
    ro.mot.eri.losalert.delay=1000 \
    pm.sleep_mode=1 \
    ro.ril.power_collapse=1
    
#
# Telephony/ring Tweaks
#
PRODUCT_PROPERTY_OVERRIDES += \
    ro.telephony.call_ring.delay=0 \
    ring.delay=0

#
# Proximity (Disable blackscreen issue after call)
#
PRODUCT_PROPERTY_OVERRIDES += \
    mot.proximity.delay=0 \
    ro.lge.proximity.delay=25

#
# Scrolling tweaks
#
# windowsmgr.max_events_per_sec := 60..300
# (any value much higher than 90 is unlikely to have any noticeable impact)
#
PRODUCT_PROPERTY_OVERRIDES += \
    ro.min_pointer_dur=8 \
    ro.max.fling_velocity=12000 \
    ro.min.fling_velocity=8000 \
    windowsmgr.max_events_per_sec=90

#
# Dalvik Properties
#
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.checkjni=false \
    dalvik.vm.dexopt-data-only=1 \
    dalvik.vm.execution-mode=int:jit \
    dalvik.vm.verify-bytecode=false \
    dalvik.vm.lockprof.threshold=500 \
    dalvik.vm.debug.alloc=0 \
    ro.sys.fw.bg_apps_limit=12 \
    sys.mem.max_hidden_apps=7 \
    ro.config.max_starting_bg=7

# Default heap settings for 512mb device
include frameworks/native/build/phone-hdpi-512-dalvik-heap.mk

# We have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

#
# Camera (video recording)
#

# Properties
PRODUCT_PROPERTY_OVERRIDES += \
    debug.camcorder.disablemeta=1 \
    rw.media.record.hasb=0

#
# Wifi
#

# Firmware
$(call inherit-product-if-exists, hardware/broadcom/wlan/bcmdhd/firmware/bcm4329/device-bcm.mk)

# Properties
#
# wifi.supplicant_scan_interval (was before 180; ppl complaint about issues with enabling Wifi. The new value should be fair enough to save battery)
PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0 \
    wifi.supplicant_scan_interval=120

#
# Qcom
#

# Init post-boot script
PRODUCT_COPY_FILES += \
    device/htc/qsd8k-common/init.qcom.post_boot.sh:system/etc/init.qcom.post_boot.sh

#
# Permissions
#

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.distict.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml

# Proprietary blobs
$(call inherit-product-if-exists, vendor/htc/qsd8k-common/qsd8k-vendor.mk)
