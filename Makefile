# MAKEFILE FOR MACOS AND LINUX #

CXX = g++

# Name of the executable
EXEC = audioAppGuiTest

# Directories
SRC_DIR = ./src
PA_DIR = ./libs/portaudio
IMGUI_DIR = ./libs/imgui
IMGUI_KNOBS_DIR = ./libs/imgui-knobs
DR_DIR = ./libs/dr_libs

# Source files
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/audio.cpp $(SRC_DIR)/gui.cpp 
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp \
	$(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp \
	$(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp \
	$(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
SOURCES += $(IMGUI_KNOBS_DIR)/imgui-knobs.cpp

# Objects, compiles .o files first
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))

UNAME_S := $(shell uname -s)

# Build flags, includes, links
CXXFLAGS = -std=c++17 -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -I$(IMGUI_KNOBS_DIR) -I$(PA_DIR)/include
CXXFLAGS += -I$(DR_DIR) -I./include
CXXFLAGS += -g -Wall -Wformat -pthread

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

LIBS = $(PA_DIR)/lib/.libs/libportaudio.a

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL -ldl `sdl2-config --libs` -lrt -lasound -ljack -lpulse

	CXXFLAGS += `sdl2-config --cflags`
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit \
	-framework CoreVideo -framework CoreAudio -framework AudioToolbox \
	-framework AudioUnit -framework CoreServices \
	$(shell sdl2-config --libs)

	CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_KNOBS_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(EXEC): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

install-portaudio:
	cd $(PA_DIR) && ./configure && $(MAKE) -j
.PHONY: install-portaudio

uninstall-portaudio:
	cd $(PA_DIR) && $(MAKE) uninstall
.PHONY: uninstall-portaudio

clean:
	rm -f $(OBJS) imgui.ini
.PHONY: clean
