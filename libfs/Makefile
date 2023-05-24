# Target library
lib := libfs.a
objs := disk.o fs.o

CC := gcc
CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter

all: $(lib)

$(lib): $(objs)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(lib) $(objs)