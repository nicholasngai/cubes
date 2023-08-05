TARGET = cubes
OBJS = \
	cubes.o \
	hash.o
SRCS = $(OBJS:.o=.c)
DEPS = $(OBJS:.o=.d)

CPPFLAGS = -I. -MMD -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c17 -pedantic -O3 -Wall -Wextra -Werror -fopenmp
LDFLAGS = -fopenmp
LDLIBS = -lpthread

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -c -o $@

clean: FORCE
	rm -rf $(TARGET) $(OBJS) $(DEPS)

FORCE:

-include $(DEPS)
