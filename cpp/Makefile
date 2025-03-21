all: basetest

basetest: main.cpp Makefile
	g++ -std=c++20 -Wall -O3 -march=native -flto -ffast-math -fno-rtti \
    	-o main main.cpp \
    	Requester/CoinbaseRequester.cpp \
    	Requester/BaseRequester.cpp \
    	Coinbase.cpp \
    	-lcurl


clean:
	rm -f main
