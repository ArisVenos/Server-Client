pollswayer: pollswayer.c
	gcc -o pollswayer pollswayer.c -pthread

poller: poller.o parties.o
	gcc -o poller poller.o parties.o -pthread

parties.o: parties.c poller.h	
	gcc -c parties.c

poller.o: poller.c poller.h 
	gcc -c poller.c

clean:
	rm -f pollswayer poller *.o *.txt