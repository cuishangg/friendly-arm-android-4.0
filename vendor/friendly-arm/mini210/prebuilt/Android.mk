#
# Copyright (C) 2008 The Android Open Source Project
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

LOCAL_PATH := $(call my-dir)

define add-friendlyarm-file
    $(eval $(include-friendlyarm-file))
endef

define include-friendlyarm-file
	include $$(CLEAR_VARS)
	LOCAL_SRC_FILES := $(1)
	LOCAL_BUILT_MODULE_STEM := $(1)
	LOCAL_MODULE_SUFFIX := $$(suffix $(1))
	LOCAL_MODULE := $$(basename $(1))
	LOCAL_MODULE_CLASS := $(2)
	LOCAL_MODULE_TAGS := eng
	include $$(BUILD_PREBUILT)
endef

ifeq ($(WPA_SUPPLICANT_VERSION),VER_0_8_X)
$(call add-friendlyarm-file,lib_driver_cmd_fawext.a,STATIC_LIBRARIES)
endif

$(call add-friendlyarm-file,libhdmiclient.so,SHARED_LIBRARIES)
$(call add-friendlyarm-file,libcec.so,SHARED_LIBRARIES)
$(call add-friendlyarm-file,libddc.so,SHARED_LIBRARIES)
$(call add-friendlyarm-file,libedid.so,SHARED_LIBRARIES)
$(call add-friendlyarm-file,libfriendlyarm-hardware.so,SHARED_LIBRARIES)
$(call add-friendlyarm-file,libhdmi.so,SHARED_LIBRARIES)
$(call add-friendlyarm-file,libs3cjpeg-full.so,SHARED_LIBRARIES)
$(call add-friendlyarm-file,libTVOut.so,SHARED_LIBRARIES)
