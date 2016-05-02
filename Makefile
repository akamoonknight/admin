admin_client: admin_client.cpp admin_client.hpp message_base.hpp message_base-inl.hpp
	g++ -std=c++11 -I. -I/usr/local/include -L/usr/local/include admin_client.cpp -o admin_client -lboost_system -lfolly -lglog -pthread
	
simple_server:
	g++ -std=c++11 -I. -I/usr/local/include -L/usr/local/include simple_server.cpp -o simple_server -lboost_system -lfolly -lglog -pthread
