PROGRAMS = fbtest_01 fbtest_02 fbtest_03 fbtest_04 fbtest_05 fbtest_06 fbtest_07 fbtest_08 fbfire

all: $(PROGRAMS)

CFLAGS+=-Wall -Wextra -Wmaybe-uninitialized -pedantic-errors -g

fbtest_01: fbtest_01.o
	$(CC) $(CFLAGS) -o fbtest_01 fbtest_01.o

fbtest_02: fbtest_02.o
	$(CC) $(CFLAGS) -o fbtest_02 fbtest_02.o

fbtest_03: fbtest_03.o
	$(CC) $(CFLAGS) -o fbtest_03 fbtest_03.o

fbtest_04: fbtest_04.o
	$(CC) $(CFLAGS) -o fbtest_04 fbtest_04.o

fbtest_05: fbtest_05.o
	$(CC) $(CFLAGS) -o fbtest_05 fbtest_05.o

fbtest_06: fbtest_06.o
	$(CC) $(CFLAGS) -o fbtest_06 fbtest_06.o

fbtest_07: fbtest_07.o
	$(CC) $(CFLAGS) -o fbtest_07 fbtest_07.o -lm

fbtest_08: fbtest_08.o
	$(CC) $(CFLAGS) -o fbtest_08 fbtest_08.o

fbfire: fbfire.o
	$(CC) $(CFLAGS) -o fbfire fbfire.o

clean:
	rm -rf *.o fbtest_01 fbtest_02 fbtest_03 fbtest_04 fbtest_05 fbtest_06 fbtest_08 fbfire

