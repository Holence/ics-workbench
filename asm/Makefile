NAME := $(shell basename $(PWD))
all: $(NAME)-64
export MODULE := Lab2
include ../Makefile

# gcc thinks simple_recursion is infinite-recursion...
CFLAGS += -Wno-infinite-recursion
