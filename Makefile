#Makefile for sphinx_http_api
CC  := gcc
XX := g++

#C_FLAGS += -Wall -O2
C_FLAGS += -g -O0 -Wall
CC_FLAGS += -g -O0 -Wall -Wno-write-strings -Wextra -Winline -Wunused
CC_FLAGS += -Wfloat-equal -Wshadow -Werror -Wmissing-include-dirs -Wformat=2
LIB_FLAGS := -lpthread
#LIB = -L""
LIB :=
#INC = -I""
INC :=

SUBDIR := liblog libsphinxclient libthreadpool libjson src
SUBDIR2:= liblog libthreadpool src

SRC    := $(shell find $(SUBDIR) -regextype posix-egrep -regex '.*\.(c|cpp|cc)')
SRC2   := $(shell find $(SUBDIR2) -regextype posix-egrep -regex '.*\.(h|c|cpp|cc)')
SRC2   := $(shell echo $(SRC2) | sed "s/[^ ]*queue\.h//")
OBJ    := $(SRC)
OBJ    := $(OBJ:%.c=%.o)
OBJ    := $(OBJ:%.cpp=%.o)
OBJ    := $(OBJ:%.cc=%.o)

TARGET := sphinx_http_api
INSTALLDIRNAME := sphinx-http-api

all: INFO $(TARGET)

INFO: 
# @echo "SRC"
# @echo $(SRC)
# @echo "OBJ"
# @echo $(OBJ)

$(TARGET):$(OBJ)
	ctags -R
	$(CXX) --coverage -o $@ $^ $(C_FLAGS) $(LIB_FLAGS) $(LIB) --static

.c.o:
	@echo $(@D)/$(<F) " -> " $(@D)/$(@F)
	$(CC) -c $(@D)/$(<F) -o $(@D)/$(@F) $(C_FLAGS) $(LIB_FLAGS) $(INC)

.cpp.o:
	@echo $(@D)/$(<F) " -> " $(@D)/$(@F)
	$(CXX) -c $(@D)/$(<F) -o $(@D)/$(@F) $(C_FLAGS) $(LIB_FLAGS) $(INC)

.cc.o:
	@echo $(@D)/$(<F) " -> " $(@D)/$(@F)
	$(CXX) -c --coverage $(@D)/$(<F) -o $(@D)/$(@F) $(CC_FLAGS) $(LIB_FLAGS) $(INC)

format:
	@echo "format source code style"
	sed "s/\s\+$$//" -i $(SRC2)

check:
	@echo "check c++ code style"
	@echo "check $(SRC2)"
	./cpplint/cpplint.py --filter=-runtime/references,-readability/streams,-build/namespaces $(SRC2)

clean:
	rm -f $(TARGET) $(OBJ) $(OBJ)
	@for d in $(SUBDIR2); do \
		rm -f ./$$d/*.gcno; \
		rm -f ./$$d/*.gcda; \
		rm -f ./$$d/*.o; \
	done

PREFIX ?= $(HOME)/local/sphinx-http-api

install:$(TARGET)
	if ( test -z $(PREFIX) ); then return; fi
	if ( test ! -d $(PREFIX) ); then mkdir -p $(PREFIX); fi
	cp -f ./$(TARGET) $(PREFIX)/
	cp -rf ./web $(PREFIX)/
	cp -rf ./etc $(PREFIX)/
	mkdir -p $(PREFIX)/logs
	echo $(PREFIX) >> install.list.txt
 
uninstall:
	if ( test -d $(PREFIX) ); then rm -rf $(PREFIX)/web $(PREFIX)/etc $(PREFIX)/logs; fi
