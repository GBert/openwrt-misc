all: main

main: main.c
	sdcc --use-non-free -mpic14 -p12f1501 main.c

clean:
	rm *.asm *.cod *.hex *.lst *.o
