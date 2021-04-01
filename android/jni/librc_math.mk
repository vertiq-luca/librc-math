##############################################################################
#  Copyright 2021 ModalAI Inc.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#
#  3. Neither the name of the copyright holder nor the names of its contributors
#     may be used to endorse or promote products derived from this software
#     without specific prior written permission.
#
#  4. The Software is used solely in conjunction with devices provided by
#     ModalAI Inc.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
##############################################################################

LOCAL_PATH:= $(call my-dir)
LIBRC_MATH_ROOT_REL:= ../..
LIBRC_MATH_ROOT_ABS:= $(LOCAL_PATH)/../..

# librc_math

include $(CLEAR_VARS)

LIBRC_MATH_ROOT_REL:= ../..
LIBRC_MATH_ROOT_ABS:= $(LOCAL_PATH)/../..

LOCAL_SRC_FILES := \
  $(LIBRC_MATH_ROOT_ABS)/library/src/algebra_common.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/algebra.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/filter.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/kalman.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/matrix.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/other.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/polynomial.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/quaternion.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/ring_buffer.c \
  $(LIBRC_MATH_ROOT_ABS)/library/src/vector.c

LOCAL_C_INCLUDES += \
  $(LIBRC_MATH_ROOT_ABS)/library/include

LOCAL_EXPORT_C_INCLUDES := \
  $(LIBRC_MATH_ROOT_ABS)/include


# LOCAL_CFLAGS := -I{LIBRC_MATH_ROOT_REL}../full_android_build/include/
LOCAL_LDLIBS := -llog
# LOCAL_LDFLAGS := -L${LIBRC_MATH_ROOT_ABS}/../full_android_build/lib/

LOCAL_MODULE := librc_math

include $(BUILD_SHARED_LIBRARY)



