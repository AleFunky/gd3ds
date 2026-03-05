.SUFFIXES:

#---------------------------------------------------------------------------------
# Environment Setup
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPRO")
endif

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/3ds_rules

# ip address of 3ds for hblauncher/fbi target.
IP3DS := 192.168.1.141

#---------------------------------------------------------------------------------
# Directory Setup
#---------------------------------------------------------------------------------
BUILD := build
OUTPUT := output
SOURCES := source libraries
DATA := data
INCLUDES := $(SOURCES) include
ROMFS := romfs
GRAPHICS :=	gfx
RESOURCES := resources
GFXBUILD :=	$(ROMFS)/gfx
BUILDTOOLS := $(TOPDIR)/buildtools
TOOLCHAIN := arm-none-eabi

#---------------------------------------------------------------------------------
# Resource Setup
#---------------------------------------------------------------------------------
APP_INFO := $(RESOURCES)/AppInfo
BANNER_AUDIO := $(RESOURCES)/audio
BANNER_IMAGE := $(RESOURCES)/banner
ICON := $(RESOURCES)/icon.png
RSF := $(TOPDIR)/$(RESOURCES)/template.rsf

#---------------------------------------------------------------------------------
# Build Setup (code generation)
#---------------------------------------------------------------------------------
ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard

COMMON_FLAGS := -g -Wall -Wno-strict-aliasing -O2 -mword-relocations -fomit-frame-pointer \
	-ffunction-sections -fdata-sections -ffast-math \
	$(ARCH) $(INCLUDE) -D__3DS__ $(BUILD_FLAGS) 
CFLAGS := $(COMMON_FLAGS) -std=gnu11
CXXFLAGS := $(COMMON_FLAGS) -std=gnu++11
ifeq ($(ENABLE_EXCEPTIONS),)
	CXXFLAGS += -fno-rtti -fno-exceptions
endif

ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) 

LIBS := -lcitro2d -lcitro3d -lctru -lm -lz -lmpg123 -lm
LIBDIRS := $(PORTLIBS) $(CTRULIB) ./lib

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# Build Variable Setup
#---------------------------------------------------------------------------------
recurse = $(shell find $2 -type $1 -name '$3' 2> /dev/null)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(call recurse,f,$(dir),*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(call recurse,f,$(dir),*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(call recurse,f,$(dir),*.s)))
PICAFILES := $(foreach dir,$(SOURCES),$(notdir $(call recurse,f,$(dir),*.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
GFXFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(call recurse,f,$(dir),*.t3s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export T3XFILES		:=	$(addprefix $(GFXBUILD)/,$(GFXFILES:.t3s=.t3x))

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
	$(PICAFILES:.v.pica=.shbin.o) \
	$(SHLISTFILES:.shlist=.shbin.o) \
	$(CPPFILES:.cpp=.o) \
	$(CFILES:.c=.o) \
	$(SFILES:.s=.o) \

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(CPPFILES)),)
	export LD := $(CC)
else
	export LD := $(CXX)
endif

export DEPSDIR := $(CURDIR)/$(BUILD)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir) $(call recurse,d,$(CURDIR)/$(dir),*)) \
				$(foreach dir,$(DATA),$(CURDIR)/$(dir) $(call recurse,d,$(CURDIR)/$(dir),*)) \
				$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) $(call recurse,d,$(CURDIR)/$(dir),*))

export TOPDIR := $(CURDIR)
OUTPUT_DIR := $(TOPDIR)/$(OUTPUT)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
# Initial Targets
#---------------------------------------------------------------------------------
all: $(BUILD) $(OUTPUT_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

3dsx: $(BUILD) $(OUTPUT_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile $@

cia: $(BUILD) $(OUTPUT_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile $@

3ds: $(BUILD) $(OUTPUT_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile $@

elf: $(BUILD) $(OUTPUT_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile $@

citra: $(BUILD) $(OUTPUT_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile $@

hblauncher: $(BUILD) $(OUTPUT_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile $@

fbi: $(BUILD) $(OUTPUT_DIR)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile $@

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

$(OUTPUT_DIR):
	@[ -d $@ ] || mkdir -p $@

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT)

clean_gfx:
	@echo clean gfx ...
	@rm -fr $(TOPDIR)/$(GFXBUILD)

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# Build Information Setup
#---------------------------------------------------------------------------------
DEPENDS := $(OFILES:.o=.d)

include $(TOPDIR)/$(APP_INFO)
APP_TITLE := $(shell echo "$(APP_TITLE)" | cut -c1-128)
APP_DESCRIPTION := $(shell echo "$(APP_DESCRIPTION)" | cut -c1-256)
APP_AUTHOR := $(shell echo "$(APP_AUTHOR)" | cut -c1-128)
APP_PRODUCT_CODE := $(shell echo $(APP_PRODUCT_CODE) | cut -c1-16)
APP_UNIQUE_ID := $(shell echo $(APP_UNIQUE_ID) | cut -c1-7)
APP_VER_MAJOR := $(shell echo $(APP_VER_MAJOR) | cut -c1-3)
APP_VER_MINOR := $(shell echo $(APP_VER_MINOR) | cut -c1-3)
APP_VER_MICRO := $(shell echo $(APP_VER_MICRO) | cut -c1-3)
ifneq ("$(wildcard $(TOPDIR)/$(BANNER_IMAGE).cgfx)","")
	BANNER_IMAGE_FILE := $(TOPDIR)/$(BANNER_IMAGE).cgfx
	BANNER_IMAGE_ARG := -ci $(BANNER_IMAGE_FILE)
else
	BANNER_IMAGE_FILE := $(TOPDIR)/$(BANNER_IMAGE).png
	BANNER_IMAGE_ARG := -i $(BANNER_IMAGE_FILE)
endif

ifneq ("$(wildcard $(TOPDIR)/$(BANNER_AUDIO).cwav)","")
	BANNER_AUDIO_FILE := $(TOPDIR)/$(BANNER_AUDIO).cwav
	BANNER_AUDIO_ARG := -ca $(BANNER_AUDIO_FILE)
else
	BANNER_AUDIO_FILE := $(TOPDIR)/$(BANNER_AUDIO).wav
	BANNER_AUDIO_ARG := -a $(BANNER_AUDIO_FILE)
endif

EMPTY :=
SPACE := $(EMPTY) $(EMPTY)
OUTPUT_NAME := $(subst $(SPACE),,$(APP_TITLE))
OUTPUT_DIR := $(TOPDIR)/$(OUTPUT)
OUTPUT_FILE := $(OUTPUT_DIR)/$(OUTPUT_NAME)

APP_ICON := $(TOPDIR)/$(ICON)
APP_ROMFS := $(TOPDIR)/$(ROMFS)

COMMON_MAKEROM_PARAMS := -rsf $(RSF) -target t -exefslogo -elf $(OUTPUT_FILE).elf -icon icon.icn -banner banner.bnr \
	-DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="$(APP_PRODUCT_CODE)" -DAPP_UNIQUE_ID="$(APP_UNIQUE_ID)" \
	-DAPP_ROMFS="$(APP_ROMFS)" -DAPP_SYSTEM_MODE="64MB" -DAPP_SYSTEM_MODE_EXT="Legacy" -major "$(APP_VER_MAJOR)" \
	-minor "$(APP_VER_MINOR)" -micro "$(APP_VER_MICRO)"

ifeq ($(OS),Windows_NT)
	MAKEROM = $(BUILDTOOLS)/makerom.exe
	BANNERTOOL = $(BUILDTOOLS)/bannertool.exe
else
	MAKEROM = $(BUILDTOOLS)/makerom
	BANNERTOOL = $(BUILDTOOLS)/bannertool
endif

SILENT	:= $(if $(VERBOSE)$(V),,@)

OBJDUMP	:= $(TOOLCHAIN)-objdump
DUMPFILE := $(OUTPUT_DIR)/$(OUTPUT_NAME).dump

_3DSXFLAGS += --smdh=$(OUTPUT_FILE).smdh
ifneq ("$(wildcard $(TOPDIR)/$(ROMFS))","")
	_3DSXFLAGS += --romfs=$(TOPDIR)/$(ROMFS)
endif

#---------------------------------------------------------------------------------
# Main Targets
#---------------------------------------------------------------------------------
.PHONY: all 3dsx cia elf 3ds citra fbi hblauncher
all: $(OUTPUT_FILE).zip $(OUTPUT_FILE).3ds $(OUTPUT_FILE).cia

banner.bnr: $(BANNER_IMAGE_FILE) $(BANNER_AUDIO_FILE)
	@$(BANNERTOOL) makebanner $(BANNER_IMAGE_ARG) $(BANNER_AUDIO_ARG) -o banner.bnr > /dev/null

icon.icn: $(TOPDIR)/$(ICON)
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_TITLE)" -p "$(APP_AUTHOR)" -i $(TOPDIR)/$(ICON) -o icon.icn > /dev/null

$(OUTPUT_FILE).elf: $(OFILES) $(T3XFILES)

$(OUTPUT_FILE).3dsx: $(OUTPUT_FILE).elf $(OUTPUT_FILE).smdh
	@echo "dump  ... $(notdir $<)"
	$(SILENT)$(OBJDUMP) -h -C -S $< > $(DUMPFILE)
	@echo "built ... $(notdir $@)"
	$(SILENT)3dsxtool $< $@ $(_3DSXFLAGS)


$(OUTPUT_FILE).3ds: $(OUTPUT_FILE).elf banner.bnr icon.icn
	@$(MAKEROM) -f cci -o $(OUTPUT_FILE).3ds -DAPP_ENCRYPTED=true $(COMMON_MAKEROM_PARAMS)
	@echo "built ... $(notdir $@)"

$(OUTPUT_FILE).cia: $(OUTPUT_FILE).elf banner.bnr icon.icn
	@$(MAKEROM) -f cia -o $(OUTPUT_FILE).cia -DAPP_ENCRYPTED=false $(COMMON_MAKEROM_PARAMS)
	@echo "built ... $(notdir $@)"

$(OUTPUT_FILE).zip: $(OUTPUT_FILE).smdh $(OUTPUT_FILE).3dsx
	@cd $(OUTPUT_DIR); \
	mkdir -p 3ds/$(OUTPUT_NAME); \
	cp $(OUTPUT_FILE).3dsx 3ds/$(OUTPUT_NAME); \
	cp $(OUTPUT_FILE).smdh 3ds/$(OUTPUT_NAME); \
	zip -r $(OUTPUT_FILE).zip 3ds > /dev/null; \
	rm -r 3ds
	@echo "built ... $(notdir $@)"

3dsx : $(OUTPUT_FILE).3dsx

cia : $(OUTPUT_FILE).cia

3ds : $(OUTPUT_FILE).3ds

elf : $(OUTPUT_FILE).elf

citra : $(OUTPUT_FILE).3dsx
	citra $(OUTPUT_FILE).3dsx

fbi : $(OUTPUT_FILE).cia
	python ../buildtools/servefiles.py $(IP3DS) $(OUTPUT_FILE).cia

hblauncher : $(OUTPUT_FILE).3dsx
	3dslink -a $(IP3DS) $(OUTPUT_FILE).3dsx


#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)
#---------------------------------------------------------------------------------
# rules for assembling GPU shaders
#---------------------------------------------------------------------------------
define shader-as
	$(eval CURBIN := $(patsubst %.shbin.o,%.shbin,$(notdir $@)))
	picasso -o $(CURBIN) $1
	bin2s $(CURBIN) | $(AS) -o $@
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u32" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(CURBIN) | tr . _)`.h
endef

%.shbin.o : %.v.pica %.g.pica
	@echo $(notdir $^)
	@$(call shader-as,$^)

%.shbin.o : %.v.pica
	@echo $(notdir $<)
	@$(call shader-as,$<)

%.shbin.o : %.shlist
	@echo $(notdir $<)
	@$(call shader-as,$(foreach file,$(shell cat $<),$(dir $<)/$(file)))

$(GFXBUILD)/%.t3x %.h : %.t3s
	@echo $(notdir $<)
	@mkdir -p $(TOPDIR)/$(GFXBUILD)
	@tex3ds -i $< -H $*.h -d $*.d -o $(TOPDIR)/$@

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------