EXENAME := air_puck
EXESRCS := $(wildcard *.c)


#below is the original rules.mk
CROSS_COMPILE:=/home/mango/embedded/arm-compiler/bin/aarch64-none-linux-gnu-

CC:=$(CROSS_COMPILE)gcc

CFLAGS:=-Wall -O2
LDFLAGS:=-Wall

INCLUDE := -I../common/external/include
LIB := -L../common/external/lib -ljpeg -lfreetype -lpng -lasound -lz -lc -lm

EXESRCS := ../common/graphic.c ../common/image.c ../common/task.c $(EXESRCS)

EXEOBJS := $(patsubst %.c, %.o, $(EXESRCS))

$(EXENAME): $(EXEOBJS)
	$(CC) $(LDFLAGS) -o $(EXENAME) $(EXEOBJS) $(LIB)
	mv $(EXENAME) ../out/

clean:
	rm -f $(EXENAME) $(EXEOBJS)

# 覆盖编译规则，添加所有本地头文件
%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<


