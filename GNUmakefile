SHELL := bash

.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables

ifeq ($(origin .RECIPEPREFIX), undefined)
  $(error This make file does not support .RECIPEPREFIX. Please use GNU Make 4.0 or later.)
endif
.RECIPEPREFIX = >

CFLAGS = -Isrc -Iinclude -std=c99

all: liveprompt
.PHONY: all

clean:
> rm *.o
> rm liveprompt
.PHONY: clean

liveprompt: src/liveprompt.c
> pcc $(CFLAGS) -o liveprompt src/liveprompt.c

