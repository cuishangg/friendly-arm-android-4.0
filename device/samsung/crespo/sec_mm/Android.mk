ifneq ($(filter crespo crespo4g,$(TARGET_DEVICE)),)

WITH_SEC_OMX := true

ifeq ($(WITH_SEC_OMX), true)
  include $(all-subdir-makefiles)
endif

endif
