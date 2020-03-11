CXXFLAGS=-std=c++11
DEBUG_FLAGS=-O0 -g
RELEASE_FLAGS=-O1

release:
	mkdir -p build
	g++ $(CXXFLAGS) $(RELEASE_FLAGS) ThreadPoolTest.cpp -o build/ThreadPoolTest

debug:
	mkdir -p build
	g++ $(CXXFLAGS) $(DEBUG_FLAGS) ThreadPoolTest.cpp -o build/ThreadPoolTest

clean:
	rm -rf build