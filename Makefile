CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -march=native -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude -MMD -MP
# Static runtime: avoids picking up a mismatched libstdc++-6.dll from PATH (e.g. Git's).
LDFLAGS  ?= -static

# `make debug` builds into its own folder so -O0 and -O3 objects never mix.
ifneq ($(filter debug,$(MAKECMDGOALS)),)
  BUILD    := build/debug
  CXXFLAGS := -std=c++17 -O0 -g -Wall -Wextra -Wpedantic
else
  BUILD    := build
endif
EXE  := $(if $(filter Windows_NT,$(OS)),.exe)
# mingw32-make falls back to cmd.exe when no sh.exe is on PATH (e.g. from PowerShell).
ifeq ($(SHELL),sh.exe)
  MKDIR = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
  RMDIR = if exist "$(1)" rmdir /s /q "$(1)"
  RUN   = $(subst /,\,$(1))
else
  MKDIR = mkdir -p $(1)
  RMDIR = rm -rf $(1)
  RUN   = ./$(1)
endif

LIB_SRCS := $(wildcard src/*.cpp)
APP_SRCS := $(wildcard apps/*.cpp)
LIB_OBJS := $(LIB_SRCS:%.cpp=$(BUILD)/%.o)
APPS     := $(APP_SRCS:apps/%.cpp=$(BUILD)/%$(EXE))

.PHONY: all clean run debug
.SECONDARY:

all: $(APPS)

$(BUILD)/%$(EXE): $(BUILD)/apps/%.o $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

$(BUILD)/%.o: %.cpp
	@$(call MKDIR,$(patsubst %/,%,$(dir $@)))
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: all
	$(call RUN,$(BUILD)/main$(EXE))

debug: all

clean:
	$(call RMDIR,$(BUILD))

-include $(LIB_OBJS:.o=.d) $(APP_SRCS:%.cpp=$(BUILD)/%.d)
