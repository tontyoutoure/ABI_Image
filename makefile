CC=gcc
CINCLUDE=-I/home/likun/.usr/include/ 
CFLAG=-Wall -O3 $(CINCLUDE)
CLIB=-L/home/likun/.usr/lib -lm -lpthread -lgsl -lgslcblas -lmatio
OUTPUT=image_dei_pl

#all:|$(OUTPUT)
#	gcc image_dei_argp.c function.c -o image_dei -lm -lpthread -lpng -ltiff -lgsl -lgslcblas 2>error.txt
image_dei_pl:image_dei_pl.o function.o numcheck.o
	$(CC) $(CFLAG) image_dei_pl.o function.o numcheck.o -o $(OUTPUT) $(CLIB)
image_dei_pl.o:image_dei_pl.c image_dei_pl.h prototype.h
	$(CC) $(CFLAG) -c image_dei_pl.c 
function.o:function.c image_dei_pl.h prototype.h
	$(CC) $(CFLAG) -c function.c 
prototype.h:function.c numcheck.c
	cproto $(CINCLUDE) function.c numcheck.c >prototype.h
clean:
	rm *.o
	rm error*.txt
test:test.c
	$(CC) $(CFLAG) test.c -o test $(CLIB)
