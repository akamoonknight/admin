admin_client: src/client/admin_client.cpp include/client/admin_client.hpp include/message_base.hpp include/message_base-inl.hpp
	g++ -std=c++11 -I./include -I/usr/local/include -L/usr/local/lib src/client/admin_client.cpp -o bin/admin_client -lboost_system -lfolly -lglog -pthread
	
simple_server:
	g++ -std=c++11 -I./include -I/usr/local/include -L/usr/local/lib src/server/simple_server.cpp -o bin/simple_server -lboost_system -lfolly -lglog -pthread
