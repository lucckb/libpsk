# Automatic makefile for GNUARM (C/C++)


#Definicje programow
CC      = $(CROSS_COMPILE)gcc
CXX		= $(CROSS_COMPILE)c++
AR      = $(CROSS_COMPILE)ar
CP      = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump 
SIZE 	= $(CROSS_COMPILE)size


#Pozostale ustawienia kompilatora
ASFLAGS += 
CFLAGS  += -O$(OPT)
CXXFLAGS += -O$(OPT)
CPFLAGS =  -S
ARFLAGS = rcs

ifeq ($(LISTING),y)
ASLST = -Wa,-adhlns=$(<:%.S=%.lst)
CLST = -Wa,-adhlns=$(<:%.c=%.lst)
CPPLST = -Wa,-adhlns=$(<:%.cpp=%.lst)
LSSTARGET = $(TARGET).lss
endif

ifeq ($(DEBUG),y)
CFLAGS += -g -DPDEBUG
CXXFLAGS += -g -DPDEBUG
LDFLAGS += -g -DPDEBUG
ASFLAGS += -gstabs -DPDEBUG
else
CFLAGS += -fomit-frame-pointer 
CXXFLAGS += -fomit-frame-pointer
LDFLAGS += -fomit-frame-pointer
ASFLAGS += -fomit-frame-pointer
#Remove unused functions 
CFLAGS += -ffunction-sections -fdata-sections
CXXFLAGS += -ffunction-sections -fdata-sections
LDFLAGS+= -Wl,--gc-sections
endif


all:	build

install: build program

clean:
	rm -f $(TARGET)
	rm -f $(TARGET).map
	rm -f $(TARGET).lss
	rm -f lib$(TARGET).a
	rm -f $(OBJ) $(LST) $(DEPFILES) $(LIBS) $(LIBS_OBJS)



ifeq ($(LIBRARY),y)
build:	compile  
else
build:	compile size-calc
endif



ifeq ($(LIBRARY),y)
compile:	lib$(TARGET).a
else
compile: $(TARGET) $(LSSTARGET)
endif

#Calculate size
size-calc: $(TARGET)
	$(SIZE) $<
	

#wszystkie zaleznosci
$(TARGET): $(OBJ) $(LSCRIPT) 
#Tworzenie biblioteki
lib$(TARGET).a: $(OBJ)

#Depend files
DEPFILES += $(SRC:%.c=%.dep) $(CPPSRC:%.cpp=%.dep) $(ASRC:%.S=%.dep)

-include $(DEPFILES)
 
 
#Objects files
OBJ = $(SRC:%.c=%.o) $(CPPSRC:%.cpp=%.o) $(ASRC:%.S=%.o)
# Define all listing files.
LST = $(SRC:%.c=%.lst) $(CPPSRC:%.cpp=%.lst) $(ASRC:%.S=%.lst)
#Objects files
.PRECIOUS : $(OBJ)
ifeq ($(LIBRARY),y)
.SECONDARY: lib$(TARGET).a
else
.SECONDARY: $(TARGET)
endif
.DEFAULT_GOAL := all


%.dep: %.c
	$(CC) -MM -MF $@ -MP -MT $(subst .dep,.o,$@) $(CFLAGS) $< 

%.dep: %.cpp 
	$(CXX) -MM -MF $@ -MP -MT $(subst .dep,.o,$@) $(CXXFLAGS) $< 


%.dep: %.S
	$(CC) -MM -MF $@ -MP -MT $(subst .dep,.o,$@) $(ASFLAGS) $< 

%.lss: $(TARGET)
	@echo "Create extended listing..."
	$(OBJDUMP) -h -S $< > $@

%.hex: %.elf
	@echo "Converting to hex..."
	$(CP) -O ihex $(CPFLAGS) $< $@ 

%.bin: %.elf
	@echo "Converting to bin..."
	$(CP) -O binary $(CPFLAGS) $< $@ 

$(TARGET): $(OBJ) $(CRT0_OBJECT) $(ADDITIONAL_DEPS) $(LIBS)
	@echo "Linking..."
	$(CXX) $(CXXFLAGS) $(OBJ) $(CRT0_OBJECT) $(LIBS) -o $@ $(LDFLAGS)

%.o : %.S
	@echo "Assembling..."
	$(CC) -c $(ASFLAGS) $(ASLST) $< -o $@ 


%.o : %.c	
	@echo "Compiling C..."
	$(CC) -c $(CFLAGS) $(CLST) $< -o $@

%.o : %.cpp
	@echo "Compiling C++..."
	$(CXX) -c $(CXXFLAGS) $(CPPLST) $< -o $@

lib$(TARGET).a : $(OBJ)
	@echo "Creating library ..."
	$(AR) $(ARFLAGS) $@ $(OBJ)

