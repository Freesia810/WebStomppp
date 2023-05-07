#include "WebStompClient.h"
#include "WebStompServer.h"
#include "WebStompType.h"
#include <thread>

int main() {
	//webstomppp::WebStompClient client;
	//client.Connect("ws://127.0.0.1:7285");
	//std::thread t(&webstomppp::WebStompClient::Run, &client);
	//Sleep(10000);
	//client.Disconnect();
	//t.join();

	webstomppp::WebStompServer server;
	server.Init();
	server.Run(7285);
	return 0;
}