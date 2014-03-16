#PSK lib isix header
LIBPSK_CPPSRC += $(wildcard $(LIBPSK_DIR)/libpsk/src/codec/*.cpp)
LIBPSK_CPPSRC += $(wildcard $(LIBPSK_DIR)/libpsk/src/ham/*.cpp)
LIBPSK_CPPSRC += $(wildcard $(LIBPSK_DIR)/libpsk/src/psk/*.cpp)
LIBPSK_INC += -I$(LIBPSK_DIR)/libpsk/include


LIBPSK_LIB += $(LIBPSK_DIR)/libpsk.a
LIBPSK_OBJS += $(LIBPSK_CPPSRC:%.cpp=%.o)
DEPFILES += $(LIBPSK_CPPSRC:%.cpp=%.dep)

.ONESHELL:
$(LIBPSK_LIB): $(LIBPSK_OBJS)
	    $(AR) $(ARFLAGS) $@ $^

LIBS += $(LIBPSK_LIB)
LIBS_OBJS += $(LIBPSK_OBJS)
COMMON_FLAGS += $(LIBPSK_INC) -D_GLIBCXX_USE_C99_MATH_TR1
