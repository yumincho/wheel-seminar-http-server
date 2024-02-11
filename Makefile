server.out: http.o main.o
	gcc -o server.out http.o main.o

http.o: http.c http.h
	gcc -c http.c

main.o: main.c
	gcc -c main.c

run: server.out
	./server.out

clean:
	rm -f *.o *.out