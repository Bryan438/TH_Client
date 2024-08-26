CC = g++
CFLAGS = -Wall -g

thclient: th_client.o controller.o card.o player.o cli_transport.o message_content.o observer.o input_thread.o
	$(CC) $(CFLAGS) -o thclient th_client.o controller.o card.o player.o cli_transport.o message_content.o observer.o input_thread.o

th_client.o: th_client.cpp controller.h 
	$(CC) $(CFLAGS) -c th_client.cpp 

controller.o: card.h controller.h controller.cpp player.h cli_transport.h message_content.h observer.h input_thread.h
	$(CC) $(CFLAGS) -c controller.cpp 

card.o: card.h card.cpp
	$(CC) $(CFLAGS) -c card.cpp


player.o: player.h player.cpp
	$(CC) $(CFLAGS) -c player.cpp

cli_transport.o: cli_transport.h cli_transport.cpp observer.h
	$(CC) $(CFLAGS) -c cli_transport.cpp

message_content.o: message_content.h message_content.cpp
	$(CC) $(CFLAGS) -c message_content.cpp

observer.o: observer.h observer.cpp
	$(CC) $(CFLAGS) -c observer.cpp

input_thread.o: input_thread.h input_thread.cpp observer.h
	$(CC) $(CFLAGS) -c input_thread.cpp

clean:
	rm -f *.o thclient

