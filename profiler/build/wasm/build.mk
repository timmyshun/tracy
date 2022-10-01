CC=emcc
CXX=em++

CFLAGS += -sUSE_FREETYPE=1 -pthread
CXXFLAGS := $(CFLAGS) -std=c++17
DEFINES += -DIMGUI_ENABLE_FREETYPE -DIMGUI_IMPL_OPENGL_ES2
INCLUDES := -I../../../imgui -I$(HOME)/.emscripten_cache/sysroot/include/capstone
LIBS += -lpthread -ldl $(HOME)/.emscripten_cache/sysroot/lib/libcapstone.a -sUSE_GLFW=3 -sTOTAL_MEMORY=512mb -sWASM_BIGINT=1 -sPTHREAD_POOL_SIZE=4 --preload-file embed.tracy

PROJECT := Tracy
IMAGE := $(PROJECT)-$(BUILD).html
NO_TBB := 1

FILTER := ../../../nfd/nfd_win.cpp
include ../../../common/src-from-vcxproj.mk

CXXFLAGS += -DTRACY_NO_FILESELECTOR

include ../../../common/unix.mk