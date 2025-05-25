# Build command line VoltageCatcher
#
# Use this command to place "vc" into /usr/local/bin 
# $ make clean && make && make install 
#
USELIB     := USE_WIRINGPI_LIB
DIR_src    := src
DIR_tools  := src/tools/util
DIR_include:= src/tools/include
DIR_OBJ    := obj
DIR_BIN    := bin
OBJ_C      := $(wildcard ${DIR_src}/*.c*)
OBJ_O      := $(patsubst %.c,${DIR_BIN}/%.o,$(notdir ${OBJ_C}))
OBJ_2      := $(addprefix $(DIR_OBJ),$(OBJ_O))

CC       := g++
CFLAGS   := -c -O2 -D $(USELIB) -std=gnu++17 
DFLAGS   := -c -O2 -D $(USELIB)
LDFLAGS  := -pthread -lm -llog4pi -lwiringPi -lwiringPiDev -lwiringPiLCD101rpi -lwiringPiPCA9635rpi -lwiringPiMCP23x17rpi -lwiringPiADS1115rpi -lwiringPiPca9685 -luuid
SOURCES  := 

TOOLS    := $(wildcard ${DIR_tools}/*.c*)
TOOLDIR  := src/tools/util

TDIR     := $(addprefix $(TOOLDIR)/,$(TOOLS))
TOBJ     := $(TOOLS:%.cpp=obj/%.o)

SRC      := $(foreach sdir,$(SRCDIR),$(wildcard $(sdir)/*.cpp))

OBJECTS  := $(SOURCES:%.cpp=obj/%.o)
HEADERS  := $(SOURCES:%.cpp=%.h)

EXECUTABLE=bin/streetcar

INCLUDES := src/tools/include

all:   debug $(TOOLS)  $(SOURCES) $(EXECUTABLE)

debug:
	@echo ":::::::: all ::::::::::"
	@echo SOURCES=$(SOURCES)
	@echo TOOLS=$(TOOLS)
	@echo -      
	@echo OBJECTS=$(OBJECTS)
	@echo TOBJ=$(TOBJ)
	@echo -      
	@echo TOOLDIR=$(TOOLDIR)
	@echo TDIR=$(TDIR)
	@echo HEADERS=$(HEADERS)
	@echo SRC=$(SRC)
	@echo OBJ=$(OBJ)
	@echo OBJ_C=$(OBJ_C)
	@echo OBJ_O=$(OBJ_O)
	@echo OBJ_2=$(OBJ_2)
	@echo ":::::::: all ::::::::::"

tasks: all

$(EXECUTABLE): $(OBJECTS) $(TOBJ) 	
	@echo linking...
	@mkdir -p bin
	$(CC) -I $(INCLUDES) $(OBJ_C) -o $@ $(OBJECTS) $(TOBJ) $(LDFLAGS)

obj/$(TOOLDIR)/%.o: 
	@echo compiling tool $(TOOLDIR)/$(*F).cpp
	@echo "$< -> $@"
	@mkdir -p obj/src/tools/util
	$(CC) $(CFLAGS) $(TOOLDIR)/$(*F).cpp -o $@  

obj/%.o: %.h %.cpp
	@echo compiling module $(*D)/$(*F).cpp
	@echo "$< -> $@"
	@mkdir -p obj
	$(CC) $(CFLAGS) $(*D)/$(*F).cpp -o $@ -I $(DIR_Config)  -I $(DIR_LCD)  -I $(DIR_GUI)   -I $(DIR_Fonts)


${DIR_BIN}/%.o : $(DIR_GUI)/%.c
	gcc $(DFLAGS) -c  $< -o $(DIR_OBJ)/$@ $(LIB) -I $(DIR_Config)  -I $(DIR_LCD)  -I $(DIR_Fonts)

${DIR_BIN}/%.o : $(DIR_LCD)/%.c
	gcc $(DFLAGS) -c  $< -o $(DIR_OBJ)/$@ $(LIB) -I $(DIR_Config)

${DIR_BIN}/%.o : $(DIR_Fonts)/%.c
	gcc $(DFLAGS) -c  $< -o $(DIR_OBJ)/$@ $(LIB) -I $(DIR_Config)

${DIR_BIN}/%.o : $(DIR_Config)/%.c
	gcc $(DFLAGS) -c  $< -o $(DIR_OBJ)/$@ $(LIB)




install: all
	@echo Installing to /usr/local/bin

clean:
	@rm -rf obj/ bin/ $(EXECUTABLE)

