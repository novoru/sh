CFLAGS=-Wall -g -O0 -lreadline

sh: sh.c
	cc ./sh.c -o $@ $(CFLAGS)

clean:
	rm -rf ./*.o ./#*#* sh ./*~
