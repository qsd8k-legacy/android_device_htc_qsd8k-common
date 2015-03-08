/*
** Copyright (c) 2011 Code Aurora Forum. All rights reserved.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_HARDWARE_QCOM_CAMERA_H
#define ANDROID_HARDWARE_QCOM_CAMERA_H

#include <hardware/camera.h>

  static hw_device_t * open_camera_device(int cameraId);

namespace android {

  static int camera_get_number_of_cameras();
  
  static int camera_get_camera_info(int camera_id, struct camera_info *info);

  static int camera_device_open(const struct hw_module_t* module, const char* id,
          struct hw_device_t** hw_device);

  static int close_camera_device( hw_device_t *);
	
  static int camera_set_preview_window(struct camera_device *,
          struct preview_stream_ops *window);
  
  static void camera_set_callbacks(struct camera_device *,
          camera_notify_callback notify_cb,
          camera_data_callback data_cb,
          camera_data_timestamp_callback data_cb_timestamp,
          camera_request_memory get_memory,
          void *user);

  static void camera_enable_msg_type(struct camera_device *, int32_t msg_type);

  static void camera_disable_msg_type(struct camera_device *, int32_t msg_type);
  
  static int camera_msg_type_enabled(struct camera_device *, int32_t msg_type);

  static int camera_start_preview(struct camera_device *);

  static void camera_stop_preview(struct camera_device *);

  static int camera_preview_enabled(struct camera_device *);

  static int camera_store_meta_data_in_buffers(struct camera_device *, int enable);

  static int camera_start_recording(struct camera_device *);

  static void camera_stop_recording(struct camera_device *);

  static int camera_recording_enabled(struct camera_device *);

  static void camera_release_recording_frame(struct camera_device *,
                  const void *opaque);

  static int camera_auto_focus(struct camera_device *);

  static int camera_cancel_auto_focus(struct camera_device *);

  static int camera_take_picture(struct camera_device *);

  static int camera_cancel_picture(struct camera_device *);

  static int camera_set_parameters(struct camera_device *, const char *parms);

  static char* camera_get_parameters(struct camera_device *);

  static void camera_put_parameters(struct camera_device *, char *);

  static int camera_send_command(struct camera_device *,
              int32_t cmd, int32_t arg1, int32_t arg2);

  static void camera_release(struct camera_device *);

  static int camera_dump(struct camera_device *, int fd);

}; // namespace android

#endif
