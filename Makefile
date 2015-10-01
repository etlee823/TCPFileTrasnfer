myapp:
	gcc client.c -o client
	gcc server.c -o server
c:
	rm client.o server.o client server
d:
	gcc client.c -o client -DDEBUG
	gcc server.c -o server
