# Copyright (C) 2010 The Android Open Source Project
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

LOCAL_PATH := vendor/friendly-arm/mini210

# FriendlyARM Support
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/proprietary/linuxrc:root/linuxrc \
	$(LOCAL_PATH)/proprietary/tsd:root/sbin/tsd \
	$(LOCAL_PATH)/proprietary/fa_codec_ctrl:system/vendor/bin/fa_codec_ctrl \
	$(LOCAL_PATH)/proprietary/lights.mini210.so:system/lib/hw/lights.mini210.so \
	$(LOCAL_PATH)/proprietary/sensors.mini210.so:system/lib/hw/sensors.mini210.so \
	$(LOCAL_PATH)/proprietary/camera.cmos.so:system/lib/hw/camera.cmos.so \
	$(LOCAL_PATH)/proprietary/gps.mini210.so:system/lib/hw/gps.mini210.so


