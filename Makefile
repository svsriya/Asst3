all: WTF WTFserver

WTF: WTFclient.c
	gcc -O -g -o WTF WTFclient.c

WTFserver: WTFserver.c
	gcc -O -g -o WTFserver WTFserver.c

clean:
	rm -rf WTF
	rm -rf WTFserver
