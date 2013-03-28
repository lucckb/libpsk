# Automatic makefile for GNUARM (C/C++)

#Target binary file name
TARGET	   = psk31test

#Optimalization [0,1,2,3,s]
# 0 - none optimalization, s - size optimalization 3 - most optimized
OPT ?= 2


#Common flags 
COMMON_FLAGS = -pipe -Wall -pedantic -Wextra -Wno-vla -I. 
COMMON_FLAGS += -D__STDC_CONSTANT_MACROS -I./libpsk/include

#C compiler options
CFLAGS += $(COMMON_FLAGS)
CFLAGS += -std=gnu99 

#C++ compiler options 
CXXFLAGS += $(COMMON_FLAGS) -std=c++11 -ftemplate-depth=2048

#LDflags libraries etc.
LDFLAGS += -lavformat -lavcodec -lz -lavutil -lswresample

#Per file listing
LISTING = n

#Debug version
DEBUG ?=  y

#Source C++ files
CPPSRC += $(wildcard *.cpp) $(wildcard libpsk/src/*.cpp)

include unix.mk


