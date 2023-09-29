include_lib := -I/mnt/EACAB651CAB619B1/c++/_lib
library_lib := -L/mnt/EACAB651CAB619B1/c++/_lib
INCLUDES := $(include_lib)/gl/linux/include $(include_lib)/../nbt
LIBRARY_PATHS := $(library_lib)/gl/linux
LIBRARIES := -lglfw3 -lfreetype -lm -lz -lX11 -lopenal -logg -lvorbis -lvorbisfile -lvorbisenc -lpng -ljpeg -lbrotlidec

# Heavily referencing https://makefiletutorial.com
target_exec := source

build_dir := ./build
src := src

# Grab all C and C++ files to compile, or enter them manually
srcs := $(shell find $(src) -name '*.cpp' -or -name '*.c' -or -name '*.s')

# Grab all .o files
objs := $(srcs:%=$(build_dir)/%.o)

inc_dirs := $(shell find $(src) -type d)
# grab as include paths
inc_flags := $(addprefix -I,$(inc_dirs))

# autogen makefiles
cppflags := $(inc_flags) -MMD -MP

all: $(target_exec)

release: CFLAGS += -O3
release: $(target_exec)

debug: CFLAGS += -DDEBUG -g -Wall
debug: $(target_exec)

# prep
$(target_exec): $(objs)
	$(CXX) -std=c++17 $(objs) $(LIBRARY_PATHS) $(LIBRARIES) -o $@ $(LDFLAGS)

# c build
$(build_dir)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CXX) -std=c++17 -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING $(cppflags) $(INCLUDES) $(CFLAGS) $(LIBRARY_PATHS) $(LIBRARIES) -c $< -o $@

# c++ build
$(build_dir)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) -std=c++17 -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING $(cppflags) $(INCLUDES) $(CXXFLAGS) $(LIBRARY_PATHS) $(LIBRARIES) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(build_dir)

-include $(DEPS)
