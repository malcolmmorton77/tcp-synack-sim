all: tcpserver tcpclient

tcpserver: tcpserver.o
	gcc -o tcpserver tcpserver.c
tcpclient: tcpclient.o
	gcc -o tcpclient tcpclient.c
tcpclient.o:
	gcc -c tcpclient.c
tcpserver.o:
	gcc -c tcpserver.c

clean:
	rm tcpclient.o tcpserver.o tcpserver tcpclient
update:
	make clean
	make