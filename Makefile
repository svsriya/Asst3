all: WTFserver WTF

WTFserver: ./server/WTFserver.c
	gcc -g ./server/WTFserver.c -o WTFserver

WTF: ./client/WTFclient.c
	gcc -g -lssl -lcrypto ./client/WTFclient.c -o WTF

clean:
	rm -rf WTFserver
	rm -rf WTF
