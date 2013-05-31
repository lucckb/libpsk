# Automatic makefile for GNUARM (C/C++)

#Target binary file name
TARGET	   = hamlib

#Optimalization [0,1,2,3,s]
# 0 - none optimalization, s - size optimalization 3 - most optimized
OPT ?= 2


#Common flags 
COMMON_FLAGS = -pipe -Wall -pedantic -Wextra -Wno-vla -I. -I../libpsk/include

#COMMON_FLAGS += -D__STDC_CONSTANT_MACROS 
#Code profiler
#COMMON_FLAGS += -pg


#C compiler options
CFLAGS += $(COMMON_FLAGS)
CFLAGS += -std=gnu99 

#C++ compiler options 
CXXFLAGS += $(COMMON_FLAGS) -std=c++11 -ftemplate-depth=2048

#LDflags libraries etc.
LDFLAGS += -lavformat -lavcodec -lz -lavutil -lswresample -lpulse-simple -lpulse


#Per file listing
LISTING = n

#Debug version
DEBUG ?=  n

#LIBRARY
LIBRARY = y	

#Source C++ files
CPPSRC += $(wildcard ../libpsk/src/psk/*.cpp) $(wildcard ../libpsk/src/codec/*.cpp)
CPPSRC += $(wildcard ../libpsk/src/port/pulse/*.cpp) $(wildcard ../libpsk/src/ham/*.cpp)
include ../unix.mk


