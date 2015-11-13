PROGRAM := cdp-server
SOURCES := $(wildcard ./*.c)
SOURCES += $(wildcard ./vendor/libyuv/source/*.cc)
SOURCES += $(wildcard ./x11/*.c)
OBJS    := $(patsubst %.c,%.o,$(SOURCES))
OBJS    := $(patsubst %.cc,%.o,$(OBJS))
INCLUDE := -I ./vendor/libyuv/include -I ./include
LIB     := -lx264 -lm -lxcb -lxcb-xtest -lxcb-composite -lxcb-keysyms -lpthread

$(PROGRAM): $(OBJS)
	$(CC) -g -o $@ $^ $(LIB) $(INCLUDE)
	rm $(OBJS) -f
	mv $(PROGRAM) ./dest
%.o: %.c
	$(CC) -g -o $@ -c $<  $(CFLAGS) $(INCLUDE)
%.o: %.cc
	$(CC) -g -o $@ -c $< $(CFLAGS) $(INCLUDE)
clean:
	rm $(OBJS) $(PROGRAM) -f
	rm ./dest/*
purge:
	rm $(OBJS) -f