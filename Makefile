CXX ?= c++
CXXFLAGS = -std=c++17 -Wall -g -pthread

SRCS = net.cpp ui.cpp
HEADERS = $(wildcard *.h)

thclient: th_client.cpp $(SRCS) $(HEADERS) third_party/json.hpp
	$(CXX) $(CXXFLAGS) -o $@ th_client.cpp $(SRCS)

TEST_FLAGS = $(CXXFLAGS) -I. -fsanitize=address,undefined -fno-omit-frame-pointer

tests/run_tests: $(wildcard tests/*.cpp) $(SRCS) $(HEADERS) third_party/json.hpp
	$(CXX) $(TEST_FLAGS) -o $@ $(wildcard tests/*.cpp) $(SRCS)

test: tests/run_tests
	./tests/run_tests

clean:
	rm -f *.o thclient tests/run_tests

.PHONY: test clean
