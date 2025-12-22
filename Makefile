EXENAME := air_puck
EXESRCS := $(wildcard *.c)

# 覆盖编译规则，添加所有本地头文件
%.o: %.c $(wildcard *.h)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

include ../common/rules.mk