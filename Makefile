all: WTFserver WTF

WTFserver: ./server/WTFserver.c
	gcc -O -g -lz ./server/WTFserver.c -o WTFserver

WTF: ./client/WTFclient.c
	gcc -O -g -lz -lssl -lcrypto ./client/WTFclient.c -o WTF

clean:
	rm -rf WTFserver
	rm -rf WTF
