PROGRAM := cdp-server
SOURCES := $(wildcard ./*.c)
SOURCES += $(wildcard ./vendor/libyuv/source/*.cc)
OBJS    := $(patsubst %.c,%.o,$(SOURCES))
OBJS    := $(patsubst %.cc,%.o,$(OBJS))
INCLUDE := -I ./vendor/libyuv/include
LIB     := -lx264 -lm -lxcb -lpthread -lxcb-composite

$(PROGRAM): $(OBJS)
	$(CC) -g -o $@ $^ $(LIB) $(INCLUDE)
%.o: %.c
	$(CC) -g -o $@ -c $< $(CFLAGS) $(INCLUDE)
%.o: %.cc
	$(CC) -g -o $@ -c $< $(CFLAGS) $(INCLUDE)
clean:
	rm $(OBJS) $(PROGRAM) -f