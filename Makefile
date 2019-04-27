all: WTF WTFserver

WTF: ./client/WTFclient.c
	gcc -O -lssl -lcrypto -g -o WTF ./client/WTFclient.c

WTFserver: ./server/WTFserver.c
	gcc -O -g -o WTFserver ./server/WTFserver.c

clean:
	rm -rf WTF
	rm -rf WTFserver
