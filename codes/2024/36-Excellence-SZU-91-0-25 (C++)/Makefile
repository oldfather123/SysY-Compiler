all: bin/bbwf

# Create necessary directories
$(shell mkdir -p build bin)

# Include paths
# INCLUDE_PATHS := $(shell find src -type d)
# INCLUDE_FLAGS := $(addprefix -I, $(INCLUDE_PATHS))
COMPILER := clang++
INCLUDE_FLAGS :=  -Isrc #-g -fsanitize=address

# BabyWolf
sources_bbwf := $(shell find src -name "*.cpp")
objects_bbwf := $(patsubst src/%.cpp, build/%.o, $(sources_bbwf))

# Include dependency files
-include $(objects_bbwf:.o=.d)

bin/bbwf: $(objects_bbwf)
	$(COMPILER) $(INCLUDE_FLAGS) -o bin/bbwf $(objects_bbwf)

build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(COMPILER) $(INCLUDE_FLAGS) -c $< -o $@

build/%.d: src/%.cpp
	@mkdir -p $(dir $@)
	$(COMPILER) $(INCLUDE_FLAGS) -M $< -MT "$(patsubst src/%.cpp, build/%.o, $<)" -MF $@

# Others
clean:
	rm -rf bin/ build/

install: bin/bbwf
	sudo cp bin/bbwf /usr/bin/

uninstall:
	sudo rm /usr/bin/bbwf
