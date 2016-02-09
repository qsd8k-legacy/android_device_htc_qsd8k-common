/*
 * Copyright (C) 2012, The CyanogenMod Project
 * Copyright (C) 2012 Raviprasad V Mummidi.
 * Copyright (C) 2011 Code Aurora Forum. All rights reserved.
 * Copyright (C) 2012 The Evervolv Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file QcomCameraWrapper.cpp
*
* This file wraps a vendor camera module.
*
*/

//#define LOG_NDEBUG 0

#define LOG_TAG "QcomCameraWrapper"
#include <cutils/log.h>

#include <utils/threads.h>
#include <utils/String8.h>
#include <hardware/hardware.h>
#include <hardware/camera.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>

#include <binder/IMemory.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <linux/msm_mdp.h>
#include <gralloc_priv.h>
#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>
#include <dlfcn.h>

#include "CameraHardwareInterface.h"
/* include QcomCamera Hardware Interface Header*/
#include "QcomCamera.h"
extern "C" {
#include <sys/time.h>
}

/* Prototypes and extern functions. */
android::sp<android::CameraHardwareInterface> (*LINK_openCameraHardware)(int id);
int (*LINK_getNumberofCameras)(void);
void (*LINK_getCameraInfo)(int cameraId, struct camera_info *info);

/* Global variables. */
camera_notify_callback         origNotify_cb    = NULL;
camera_data_callback           origData_cb      = NULL;
camera_data_timestamp_callback origDataTS_cb    = NULL;
camera_request_memory          origCamReqMemory = NULL;

android::CameraParameters camSettings;
preview_stream_ops_t      *mWindow = NULL;
android::sp<android::CameraHardwareInterface> qCamera;

static android::Mutex gCameraWrapperLock;
static camera_module_t *gVendorModule = 0;

static char **fixed_set_params = NULL;

static int camera_device_open(const hw_module_t* module, const char* id,
                hw_device_t** hw_device);
static int camera_device_close(hw_device_t* device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);
static int camera_send_command(struct camera_device * device, int32_t cmd,
                int32_t arg1, int32_t arg2);

static struct hw_module_methods_t camera_module_methods = {
    .open = android::camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    .common = {
         .tag = HARDWARE_MODULE_TAG,
         .module_api_version = CAMERA_MODULE_API_VERSION_1_0,
         .hal_api_version = HARDWARE_HAL_API_VERSION,
         .id = CAMERA_HARDWARE_MODULE_ID,
         .name = "Qualcomm QSD8K Camera Wrapper",
         .author = "Ported over to Android 5 by spezi77)",
         .methods = &camera_module_methods,
         .dso = NULL, /* remove compilation warnings */
         .reserved = {0}, /* remove compilation warnings */
    },
    .get_number_of_cameras = android::camera_get_number_of_cameras,
    .get_camera_info = android::camera_get_camera_info,
    .set_callbacks = NULL, /* remove compilation warnings */
    .get_vendor_tag_ops = NULL, /* remove compilation warnings */
    .open_legacy = NULL, /* remove compilation warnings */
    .set_torch_mode = NULL, /* remove compilation warnings */
    .init = NULL, /* remove compilation warnings */
    .reserved = {0}, /* remove compilation warnings */
};

camera_device_ops_t camera_ops = {
  set_preview_window:		android::camera_set_preview_window,
  set_callbacks:				android::camera_set_callbacks,
  enable_msg_type:			android::camera_enable_msg_type,
  disable_msg_type:			android::camera_disable_msg_type,
  msg_type_enabled:		android::camera_msg_type_enabled,

  start_preview:			android::camera_start_preview,
  stop_preview:				android::camera_stop_preview,
  preview_enabled:			android::camera_preview_enabled,
  store_meta_data_in_buffers:	android::camera_store_meta_data_in_buffers,

  start_recording:			android::camera_start_recording,
  stop_recording:			android::camera_stop_recording,
  recording_enabled:			android::camera_recording_enabled,
  release_recording_frame:	android::camera_release_recording_frame,

  auto_focus:				android::camera_auto_focus,
  cancel_auto_focus:			android::camera_cancel_auto_focus,

  take_picture:				android::camera_take_picture,
  cancel_picture:			android::camera_cancel_picture,

  set_parameters:			android::camera_set_parameters,
  get_parameters:			android::camera_get_parameters,
  put_parameters:			android::camera_put_parameters,
  send_command:			android::camera_send_command,

  release:					android::camera_release,
  dump:					android::camera_dump,
};

namespace android {

typedef struct wrapper_camera_device {
    camera_device_t base;
    int id;
    camera_device_t *vendor;
} wrapper_camera_device_t;

#define VENDOR_CALL(device, func, ...) ({ \
    wrapper_camera_device_t *__wrapper_dev = (wrapper_camera_device_t*) device; \
    __wrapper_dev->vendor->ops->func(__wrapper_dev->vendor, ##__VA_ARGS__); \
})

#define CAMERA_ID(device) (((wrapper_camera_device_t *)(device))->id)

static int check_vendor_module()
{
    int rv = 0;
    ALOGV("%s", __FUNCTION__);

    if (gVendorModule)
        return 0;

    rv = hw_get_module_by_class("camera", "vendor",
            (const hw_module_t**)&gVendorModule);
    if (rv)
        ALOGE("failed to open vendor camera module");
    return rv;
}

static char *camera_fixup_getparams(int id, const char *settings)
{
    android::CameraParameters params;
    params.unflatten(android::String8(settings));

#if !LOG_NDEBUG
    ALOGV("%s: original parameters:", __FUNCTION__);
    params.dump();
#endif

    // Some QCOM related framework changes expect max-saturation, max-contrast
    // max-sharpness or the Camera app will crash.
    const char* value;
    if((value = params.get("saturation-max"))) {
        params.set("max-saturation", value);
    }
    if((value = params.get("contrast-max"))) {
        params.set("max-contrast", value);
    }
    if((value = params.get("sharpness-max"))) {
        params.set("max-sharpness", value);
    }

#if !LOG_NDEBUG
    ALOGV("%s: fixed parameters:", __FUNCTION__);
    params.dump();
#endif

    android::String8 strParams = params.flatten();
    char *ret = strdup(strParams.string());

    return ret;
}

char * camera_fixup_setparams(struct camera_device * device, const char * settings)
{
    int id = CAMERA_ID(device);
    android::CameraParameters params;
    params.unflatten(android::String8(settings));

#if !LOG_NDEBUG
    ALOGV("%s: original parameters:", __FUNCTION__);
    params.dump();
#endif

    bool isVideo = !strcmp(params.get(android::CameraParameters::KEY_RECORDING_HINT), "true");

    if (isVideo) {
        params.set(android::CameraParameters::KEY_ROTATION, "0");
    }

#if !LOG_NDEBUG
    ALOGV("%s: fixed parameters:", __FUNCTION__);
    params.dump();
#endif

    android::String8 strParams = params.flatten();
    if (fixed_set_params[id])
        free(fixed_set_params[id]);
    fixed_set_params[id] = strdup(strParams.string());
    char *ret = fixed_set_params[id];

    return ret;
}

void internal_decode_sw(unsigned int* rgb, char* yuv420sp, int width, int height)
{
   int frameSize = width * height;

   if (!qCamera->previewEnabled()) return;

   for (int j = 0, yp = 0; j < height; j++) {
      int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
      for (int i = 0; i < width; i++, yp++) {
         int y = (0xff & ((int) yuv420sp[yp])) - 16;
         if (y < 0) y = 0;
         if ((i & 1) == 0) {
            v = (0xff & yuv420sp[uvp++]) - 128;
            u = (0xff & yuv420sp[uvp++]) - 128;
         }

         int y1192 = 1192 * y;
         int r = (y1192 + 1634 * v);
         int g = (y1192 - 833 * v - 400 * u);
         int b = (y1192 + 2066 * u);

         if (r < 0) r = 0; else if (r > 262143) r = 262143;
         if (g < 0) g = 0; else if (g > 262143) g = 262143;
         if (b < 0) b = 0; else if (b > 262143) b = 262143;

         rgb[yp] = 0xff000000 | ((b << 6) & 0xff0000) |
                   ((g >> 2) & 0xff00) | ((r >> 10) & 0xff);
      }
   }
}

void internal_copybuffers_sw(char *dest, char *src, int size)
{
   int       i;
   int       numWords  = size / sizeof(unsigned);
   unsigned *srcWords  = (unsigned *)src;
   unsigned *destWords = (unsigned *)dest;

   for (i = 0; i < numWords; i++) {
      if ((i % 8) == 0 && (i + 8) < numWords) {
         __builtin_prefetch(srcWords  + 8, 0, 0);
         __builtin_prefetch(destWords + 8, 1, 0);
      }
      *destWords++ = *srcWords++;
   }
   if (__builtin_expect((size - (numWords * sizeof(unsigned))) > 0, 0)) {
      int numBytes = size - (numWords * sizeof(unsigned));
      char *destBytes = (char *)destWords;
      char *srcBytes  = (char *)srcWords;
      for (i = 0; i < numBytes; i++) {
         *destBytes++ = *srcBytes++;
      }
   }
}

void internal_handle_preview(const sp<IMemory>& dataPtr,
                            preview_stream_ops_t *mWindow,
                            camera_request_memory getMemory,
                            int32_t previewWidth, int32_t previewHeight)
{
   if (mWindow != NULL && getMemory != NULL) {
      ssize_t  offset;
      size_t   size;
      int32_t  previewFormat = MDP_Y_CBCR_H2V2;
      int32_t  destFormat    = MDP_RGBX_8888;

      status_t retVal;
      sp<IMemoryHeap> mHeap = dataPtr->getMemory(&offset,
                                                                   &size);

      ALOGV("%s: previewWidth:%d previewHeight:%d offset:%#x size:%#x base:%p",
            __FUNCTION__, previewWidth, previewHeight,
           (unsigned)offset, size, mHeap != NULL ? mHeap->base() : 0);

      mWindow->set_usage(mWindow,
                         GRALLOC_USAGE_SW_READ_OFTEN);

      retVal = mWindow->set_buffers_geometry(mWindow,
                                             previewWidth, previewHeight,
                                             HAL_PIXEL_FORMAT_RGBX_8888);
      if (retVal == NO_ERROR) {
         int32_t          stride;
         buffer_handle_t *bufHandle = NULL;

         ALOGV("%s: dequeueing buffer",__FUNCTION__);
         retVal = mWindow->dequeue_buffer(mWindow, &bufHandle, &stride);
         if (retVal == NO_ERROR) {
            retVal = mWindow->lock_buffer(mWindow, bufHandle);
            if (retVal == NO_ERROR) {
               private_handle_t const *privHandle =
                  reinterpret_cast<private_handle_t const *>(*bufHandle);

                  void *bits;
                  Rect bounds;
                  GraphicBufferMapper &mapper = GraphicBufferMapper::get();

                  bounds.left   = 0;
                  bounds.top    = 0;
                  bounds.right  = previewWidth;
                  bounds.bottom = previewHeight;

                  mapper.lock(*bufHandle, GRALLOC_USAGE_SW_READ_OFTEN, bounds,
                              &bits);
                  ALOGV("CameraHAL_HPD: w:%d h:%d bits:%p",
                       previewWidth, previewHeight, bits);
                  internal_decode_sw((unsigned int *)bits, (char *)mHeap->base() + offset,
                                      previewWidth, previewHeight);

                  // unlock buffer before sending to display
                  mapper.unlock(*bufHandle);

               mWindow->enqueue_buffer(mWindow, bufHandle);
               ALOGV("%s: enqueued buffer",__FUNCTION__);
            } else {
               ALOGE("%s: ERROR locking the buffer",__FUNCTION__);
               mWindow->cancel_buffer(mWindow, bufHandle);
            }
         } else {
            ALOGE("%s: ERROR dequeueing the buffer",__FUNCTION__);
         }
      }
   }
}

camera_memory_t * internal_generate_client_data(const sp<IMemory> &dataPtr,
                        camera_request_memory reqClientMemory,
                        void *user)
{
   ssize_t          offset;
   size_t           size;
   camera_memory_t *clientData = NULL;
   sp<IMemoryHeap> mHeap = dataPtr->getMemory(&offset, &size);

   ALOGV("%s: offset:%#x size:%#x base:%p", __FUNCTION__
        ,(unsigned)offset, size, mHeap != NULL ? mHeap->base() : 0);

   clientData = reqClientMemory(-1, size, 1, user);
   if (clientData != NULL) {
      internal_copybuffers_sw((char *)clientData->data,
                               (char *)(mHeap->base()) + offset, size);
   } else {
      ALOGE("%s: ERROR allocating memory from client",__FUNCTION__);
   }
   return clientData;
}

static void cam_data_callback(int32_t msgType,
                              const sp<IMemory>& dataPtr,
                              void* user)
{
   ALOGV("cam_data_callback: msgType:%d user:%p", msgType, user);
   if (msgType == CAMERA_MSG_PREVIEW_FRAME) {
      int32_t previewWidth, previewHeight;
      CameraParameters hwParameters = qCamera->getParameters();
      hwParameters.getPreviewSize(&previewWidth, &previewHeight);
      internal_handle_preview(dataPtr, mWindow, origCamReqMemory,
                                  previewWidth, previewHeight);
   }
   if (origData_cb != NULL && origCamReqMemory != NULL) {
      camera_memory_t *clientData = internal_generate_client_data(dataPtr,
                                       origCamReqMemory, user);
      if (clientData != NULL) {
         ALOGV("cam_data_callback: Posting data to client");
         origData_cb(msgType, clientData, 0, NULL, user);
         clientData->release(clientData);
      }
   }
}

static void cam_data_callback_timestamp(nsecs_t timestamp,
                                        int32_t msgType,
                                        const sp<IMemory>& dataPtr,
                                        void* user)
{
   ALOGV("cam_data_callback_timestamp: timestamp:%lld msgType:%d user:%p",
        timestamp /1000, msgType, user);

   if (origDataTS_cb != NULL && origCamReqMemory != NULL) {
      camera_memory_t *clientData = internal_generate_client_data(dataPtr,
                                       origCamReqMemory, user);
      if (clientData != NULL) {
         ALOGV("cam_data_callback_timestamp: Posting data to client timestamp:%lld",
              systemTime());
         origDataTS_cb(timestamp, msgType, clientData, 0, user);
         qCamera->releaseRecordingFrame(dataPtr);
         clientData->release(clientData);
      } else {
         ALOGE("cam_data_callback_timestamp: ERROR allocating memory from client");
      }
   }
}

void internal_fixup_settings(CameraParameters &settings)
{
   const char *preview_sizes =
      "1280x720,800x480,768x432,720x480,640x480,576x432,480x320,384x288,352x288,320x240,240x160,176x144";
   const char *video_sizes =
      "1280x720,800x480,720x480,640x480,352x288,320x240,176x144";
   const char *preferred_size       = "640x480";
   const char *preview_frame_rates  = "30,27,24,15";
   const char *preferred_frame_rate = "15";
   const char *frame_rate_range     = "(15,30)";

   settings.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,
                CameraParameters::PIXEL_FORMAT_YUV420SP);

   if (!settings.get(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES)) {
      settings.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
                   preview_sizes);
   }

   if (!settings.get(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES)) {
      settings.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
                   video_sizes);
   }

   if (!settings.get(CameraParameters::KEY_VIDEO_SIZE)) {
      settings.set(CameraParameters::KEY_VIDEO_SIZE, preferred_size);
   }

   if (!settings.get(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO)) {
      settings.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,
                   preferred_size);
   }

   if (!settings.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES)) {
      settings.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
                   preview_frame_rates);
   }

   if (!settings.get(CameraParameters::KEY_PREVIEW_FRAME_RATE)) {
      settings.set(CameraParameters::KEY_PREVIEW_FRAME_RATE,
                   preferred_frame_rate);
   }

   if (!settings.get(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE)) {
      settings.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,
                   frame_rate_range);
   }
}

void cam_notify_callback(int32_t msg_type, int32_t ext1,
                   int32_t ext2, void *user)
{
   ALOGV("cam_notify_callback: msg_type:%d ext1:%d ext2:%d user:%p",
        msg_type, ext1, ext2, user);
   if (origNotify_cb != NULL) {
      origNotify_cb(msg_type, ext1, ext2, user);
   }
}


/*******************************************************************
 * implementation of camera_device_ops functions
 *******************************************************************/

static int camera_set_preview_window(struct camera_device *device,
        struct preview_stream_ops *window)
{
  ALOGV("set_preview_window : Window :%p", window);
   if (device == NULL) {
      ALOGE("set_preview_window : Invalid device.");
      return -EINVAL;
   } else {
      ALOGV("set_preview_window : window :%p", window);
      mWindow = window;
      return 0;
   }
}

static void camera_set_callbacks(struct camera_device *device,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
  ALOGV("set_callbacks: notify_cb: %p, data_cb: %p "
        "data_cb_timestamp: %p, get_memory: %p, user :%p",
        notify_cb, data_cb, data_cb_timestamp, get_memory, user);

   origNotify_cb    = notify_cb;
   origData_cb      = data_cb;
   origDataTS_cb    = data_cb_timestamp;
   origCamReqMemory = get_memory;
   qCamera->setCallbacks(cam_notify_callback, cam_data_callback,
                         cam_data_callback_timestamp, user);
}

static void camera_enable_msg_type(struct camera_device *device,
        int32_t msg_type)
{
   ALOGV("enable_msg_type: msg_type:%#x", msg_type);
   if (msg_type == 0xfff) {
      msg_type = 0x1ff;
   } else {
      msg_type &= ~(CAMERA_MSG_PREVIEW_METADATA | CAMERA_MSG_RAW_IMAGE_NOTIFY);
   }
   qCamera->enableMsgType(msg_type);
}

static void camera_disable_msg_type(struct camera_device *device,
        int32_t msg_type)
{
  ALOGV("disable_msg_type: msg_type:%#x", msg_type);
   if (msg_type == 0xfff) {
      msg_type = 0x1ff;
   }
   qCamera->disableMsgType(msg_type);
}

static int camera_msg_type_enabled(struct camera_device *device,
        int32_t msg_type)
{
   ALOGV("msg_type_enabled: msg_type:%d", msg_type);
   return qCamera->msgTypeEnabled(msg_type);
}

static int camera_start_preview(struct camera_device *device)
{
   ALOGV("start_preview: Enabling CAMERA_MSG_PREVIEW_FRAME");

   /* TODO: Remove hack. */
   ALOGV("qcamera_start_preview: Preview enabled:%d msg enabled:%d",
        qCamera->previewEnabled(),
        qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME));
   if (!qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
      qCamera->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);
   }
   return qCamera->startPreview();
}

static void camera_stop_preview(struct camera_device *device)
{
   ALOGV("stop_preview: msgenabled:%d",
         qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME));

   /* TODO: Remove hack. */
   if (qCamera->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
      qCamera->disableMsgType(CAMERA_MSG_PREVIEW_FRAME);
   }
   return qCamera->stopPreview();
}

static int camera_preview_enabled(struct camera_device *device)
{
   ALOGV("preview_enabled:");
   return qCamera->previewEnabled() ? 1 : 0;
}

static int camera_store_meta_data_in_buffers(struct camera_device *device,
        int enable)
{
   ALOGV("store_meta_data_in_buffers:");
   return NO_ERROR;
}

static int camera_start_recording(struct camera_device *device)
{
   ALOGV("start_recording");

   /* TODO: Remove hack. */
   qCamera->enableMsgType(CAMERA_MSG_VIDEO_FRAME);
   qCamera->startRecording();
   return NO_ERROR;
}

static void camera_stop_recording(struct camera_device *device)
{
   ALOGV("stop_recording:");

   /* TODO: Remove hack. */
   qCamera->disableMsgType(CAMERA_MSG_VIDEO_FRAME);
   qCamera->stopRecording();

}

static int camera_recording_enabled(struct camera_device *device)
{
   ALOGV("recording_enabled:");
   return (int)qCamera->recordingEnabled();

}

static void camera_release_recording_frame(struct camera_device *device,
        const void *opaque)
{
   /*
    * We release the frame immediately in cam_data_callback_timestamp after making a
    * copy. So, this is just a NOP.
    */
   ALOGV("release_recording_frame: opaque:%p", opaque);
}

static int camera_auto_focus(struct camera_device *device)
{
   ALOGV("auto_focus:");
   qCamera->autoFocus();
   return NO_ERROR;
}

static int camera_cancel_auto_focus(struct camera_device *device)
{
   ALOGV("cancel_auto_focus:");
   qCamera->cancelAutoFocus();
   return NO_ERROR;
}

static int camera_take_picture(struct camera_device *device)
{
   ALOGV("take_picture:");

   /* TODO: Remove hack. */
   qCamera->enableMsgType(CAMERA_MSG_SHUTTER |
                         CAMERA_MSG_POSTVIEW_FRAME |
                         CAMERA_MSG_RAW_IMAGE |
                         CAMERA_MSG_COMPRESSED_IMAGE);

   qCamera->takePicture();
   return NO_ERROR;
}

static int camera_cancel_picture(struct camera_device *device)
{
   ALOGV("cancel_picture:");
   qCamera->cancelPicture();
   return NO_ERROR;
}

String8 g_str;
static int camera_set_parameters(struct camera_device *device,
        const char *params)
{
   ALOGV("set_parameters: %s", params);
   g_str = String8(params);
   camSettings.unflatten(g_str);
   qCamera->setParameters(camSettings);
   return NO_ERROR;
}

static char *camera_get_parameters(struct camera_device *device)
{
   char *rc = NULL;
   ALOGV("get_parameters");
   camSettings = qCamera->getParameters();
   ALOGV("get_parameters: after calling qCamera->getParameters()");
   internal_fixup_settings(camSettings);
   g_str = camSettings.flatten();
   rc = strdup((char *)g_str.string());
   ALOGV("get_parameters: returning rc:%p :%s",
        rc, (rc != NULL) ? rc : "EMPTY STRING");
   return rc;
}

static void camera_put_parameters(struct camera_device *device, char *params)
{
   ALOGV("put_parameters: params:%p %s", params, params);
   free(params);
}

static int camera_send_command(struct camera_device *device,
        int32_t cmd, int32_t arg1, int32_t arg2)
{
   ALOGV("send_command: cmd:%d arg1:%d arg2:%d",
        cmd, arg1, arg2);
	if (cmd == CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG) {
		// not supported
		return 0;
	}
   return qCamera->sendCommand(cmd, arg1, arg2);
}

static void camera_release(struct camera_device *device)
{
   ALOGV("release:");
   qCamera->release();
}

static int camera_dump(struct camera_device *device, int fd)
{
   ALOGV("dump:");
   Vector<String16> args;
   return qCamera->dump(fd, args);
}

extern "C" void heaptracker_free_leaked_memory(void);

static int camera_device_close(hw_device_t *device)
{
    int ret = 0;
    wrapper_camera_device_t *wrapper_dev = NULL;

    ALOGV("%s", __FUNCTION__);

    android::Mutex::Autolock lock(gCameraWrapperLock);

    if (!device) {
        ret = -EINVAL;
        goto done;
    }

    for (int i = 0; i < camera_get_number_of_cameras(); i++) {
        if (fixed_set_params[i])
            free(fixed_set_params[i]);
    }

    wrapper_dev = (wrapper_camera_device_t*) device;

    wrapper_dev->vendor->common.close((hw_device_t*)wrapper_dev->vendor);
    if (wrapper_dev->base.ops)
        free(wrapper_dev->base.ops);
    free(wrapper_dev);
done:
#ifdef HEAPTRACKER
    heaptracker_free_leaked_memory();
#endif
    return ret;
}

/*******************************************************************
 * implementation of camera_module functions
 *******************************************************************/

/* open device handle to one of the cameras
 *
 * assume camera service will keep singleton of each camera
 * so this function will always only be called once per camera instance
 */

static int camera_device_open(const hw_module_t* module, const char* id,
                   hw_device_t** hw_device)
{
    ALOGD("%s:++",__FUNCTION__);
    int rc = -1;
    camera_device *device = NULL;

    if(module && id && hw_device) {
        int cameraId = atoi(id);
        void * libcameraHandle = ::dlopen("libcamera.so", RTLD_NOW);

        if (libcameraHandle) {
            ALOGD("%s: loaded libcamera at %p", __FUNCTION__, libcameraHandle);

            if (::dlsym(libcameraHandle, "openCameraHardware") != NULL) {
                *(void**)&LINK_openCameraHardware =
                    ::dlsym(libcameraHandle, "openCameraHardware");
            } else if (::dlsym(libcameraHandle, "HAL_openCameraHardware") != NULL) {
                *(void**)&LINK_openCameraHardware =
                    ::dlsym(libcameraHandle, "HAL_openCameraHardware");
            } else {
                ALOGE("FATAL ERROR: Could not find openCameraHardware");
                dlclose(libcameraHandle);
                return rc;
            }

            qCamera = LINK_openCameraHardware(cameraId);

            ::dlclose(libcameraHandle);

            device = (camera_device *)malloc(sizeof (struct camera_device));

            if(device) {
                //memset(device, 0, sizeof(*device));
                device->common.tag              = HARDWARE_DEVICE_TAG;
                device->common.version          = CAMERA_DEVICE_API_VERSION_1_0;
                device->common.module           = (hw_module_t *)(module);
                device->common.close            = android::close_camera_device;
                device->ops                     = &camera_ops;
                rc = 0;
            }
        }
    }
    *hw_device = (hw_device_t*)device;
    ALOGD("%s:--",__FUNCTION__);
    return rc;
}

static int close_camera_device(hw_device_t* hw_dev)
{
    int rc = -1;
    ALOGD("%s:++",__FUNCTION__);
    camera_device_t *device = (camera_device_t *)hw_dev;
    if (device) {
        if (qCamera != NULL)
            qCamera.clear();
        free(device);
        rc = 0;
    }
    ALOGD("%s:--",__FUNCTION__);
    return rc;
}

static int camera_get_number_of_cameras(void)
{
   int numCameras = 1;

   ALOGV("get_number_of_cameras:");
   void *libcameraHandle = ::dlopen("libcamera.so", RTLD_NOW);
   ALOGD("HAL_get_number_of_cameras: loading libcamera at %p", libcameraHandle);
   if (!libcameraHandle) {
       ALOGE("FATAL ERROR: could not dlopen libcamera.so: %s", dlerror());
   } else {
      if (::dlsym(libcameraHandle, "HAL_getNumberOfCameras") != NULL) {
         *(void**)&LINK_getNumberofCameras =
                  ::dlsym(libcameraHandle, "HAL_getNumberOfCameras");
         numCameras = LINK_getNumberofCameras();
         ALOGD("HAL_get_number_of_cameras: numCameras:%d", numCameras);
      }
      dlclose(libcameraHandle);
   }
   return numCameras;
}

static int camera_get_camera_info(int camera_id, struct camera_info *info)
{
   bool dynamic = false;
   ALOGV("get_camera_info:");
   void *libcameraHandle = ::dlopen("libcamera.so", RTLD_NOW);
   ALOGD("HAL_get_camera_info: loading libcamera at %p", libcameraHandle);
   if (!libcameraHandle) {
       ALOGE("FATAL ERROR: could not dlopen libcamera.so: %s", dlerror());
       return EINVAL;
   } else {
      if (::dlsym(libcameraHandle, "HAL_getCameraInfo") != NULL) {
         *(void**)&LINK_getCameraInfo =
                  ::dlsym(libcameraHandle, "HAL_getCameraInfo");
         LINK_getCameraInfo(camera_id, info);
         dynamic = true;
      }
      dlclose(libcameraHandle);
   }
   if (!dynamic) {
      info->facing      = CAMERA_FACING_BACK;
      info->orientation = 90;
   }
   return NO_ERROR;
}

}; // namespace android
