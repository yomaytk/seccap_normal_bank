build: server.cpp deal_manager.cpp account.cpp
	g++ -o server server.cpp deal_manager.cpp account.cpp

clean: server
	rm server