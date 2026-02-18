# AVP Protocol Module Makefile Fragment
#
# Include this in the main app Makefile:
#   include ../avp/avp.mk
#
# SPDX-License-Identifier: Apache-2.0

AVP_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

AVP_SRC := \
	$(AVP_DIR)avp.c \
	$(AVP_DIR)avp_cmd.c

AVP_INC := \
	-I$(AVP_DIR)

# Add to main build
SRC += $(AVP_SRC)
CFLAGS += $(AVP_INC)
