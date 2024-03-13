ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CC			:=	gcc
LD			:=	$(CC)
SOURCES		:=	$(ROOT_DIR)/source
INCLUDES 	:=	$(ROOT_DIR)/include
TARGET		:=	$(ROOT_DIR)/save
BUILD		:=	$(ROOT_DIR)/build
DEPS		:=	$(wildcard $(ROOT_DIR)/*.d)
VPATH		:=	$(SOURCES)

CFILES		:=	$(foreach sourcedir,$(SOURCES),$(wildcard $(sourcedir)/*.c))
OFILES		:=	$(foreach cfile,$(CFILES),$(BUILD)/$(notdir $(cfile:.c=.o)))
CFLAGS		:=	-std=gnu11 -Werror -Wunused 

ifneq (,$(RELEASE))
	CFLAGS	+=	-O2
else
	CFLAGS	+=	-g
endif

.PHONY: main clean

main: $(BUILD) $(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)

$(TARGET): $(BUILD) $(OFILES)
	@echo [link] $(notdir $@)
	@$(LD) -o $(TARGET) $(OFILES)

$(BUILD):
	@mkdir -p $(BUILD)

$(BUILD)/%.o: %.c
	@echo [c] $(notdir $@)
	@gcc -c $< -o $@ -MMD -MF $(@:.o=.d) $(CFLAGS) $(foreach incl,$(INCLUDES),-I$(incl))

include $(DEPS)
