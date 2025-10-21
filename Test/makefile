.PHONY:all
all:Test EchoServer EchoClient

Test:Test.cpp
	g++ $^ -o $@ -g
EchoServer:EchoServer.cpp
	g++ $^ -o $@ -g
EchoClient:EchoClient.cpp
	g++ $^ -o $@ -g

.PHONY:clean
clean:
	rm -f Test EchoServer EchoClient