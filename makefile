target ?= debug
ifneq ("$(wildcard config.mk)","")
	include config.mk
endif

debug_cflags := -g -O0 -Wall -Wextra
release_cflags := -O3 -flto
release_ldflags := -flto

sources := $(wildcard */*.c)
objects := $(patsubst %.c,out/${target}/%.o,$(sources))
binary  := 0wm 0wm-as

cflags  = ${${target}_cflags} ${CFLAGS}
ldflags = ${${target}_ldflags} ${LDFLAGS}
strip  ?= ${${target}_strip}

.PHONY: all $(sources)
all: info $(foreach bin,$(binary),out/${target}/$(bin).out)
info:
	@echo "Building under target ${target}"
	@echo "CFLAGS  = ${cflags}"
	@echo "LDFLAGS = ${ldflags}"
	@echo "CC = ${CC}"
	@echo "--------------------------------"
clean:
	rm -fr out/
out/${target}/%.o: %.c
	mkdir -p ${@D}
	${CC} ${cflags} -c -o $@ $<
out/${target}/%.out: ${objects}
	${CC} -o $@ $(filter $(subst .out,,$@)/%.o,$^) ${ldflags}
	test "${strip}" = "" || strip ${strip} $@
