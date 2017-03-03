testik : main.o player.o videopicture.o packetqueue.o
	g++ -o testik $(CXXFLAGS) main.o player.o videopicture.o packetqueue.o

main.o : main.cpp player.h listener.h
	g++ -c $(CXXFLAGS) main.cpp

player.o : player.cpp videopicture.h packetqueue.h
	g++ -c $(CXXFLAGS) player.cpp

videopicture.o : videopicture.cpp
	g++ -c $(CXXFLAGS) videopicture.cpp

packetqueue.o : packetqueue.cpp
	g++ -c $(CXXFLAGS) packetqueue.cpp

CXXFLAGS = -lavcodec -lavformat -lswscale -lavutil -lSDL

clean:
	rm -f -r *.o *.~ *.core
