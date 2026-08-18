CXX=g++
CXXFLAGS=-std=c++11 -O2 -Wall -Wextra -pedantic

all: serverM serverA serverB serverC client monitor

serverM: serverM.cpp serverM.h common.h serverA.h serverB.h serverC.h client.h monitor.h
	$(CXX) $(CXXFLAGS) -o $@ serverM.cpp

serverA: serverA.cpp serverA.h common.h
	$(CXX) $(CXXFLAGS) -o $@ serverA.cpp

serverB: serverB.cpp serverB.h common.h
	$(CXX) $(CXXFLAGS) -o $@ serverB.cpp

serverC: serverC.cpp serverC.h common.h
	$(CXX) $(CXXFLAGS) -o $@ serverC.cpp

client: client.cpp client.h common.h
	$(CXX) $(CXXFLAGS) -o $@ client.cpp

monitor: monitor.cpp monitor.h common.h
	$(CXX) $(CXXFLAGS) -o $@ monitor.cpp

clean:
	rm -f serverM serverA serverB serverC client monitor
