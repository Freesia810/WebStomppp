#include "WebStompClient.h"
#include "WebStompType.h"
#include <thread>

int main() {
	webstomppp::WebStompClient client;
	client.Connect("ws://****");
	std::thread t(&webstomppp::WebStompClient::Run, &client);
	Sleep(5000);
	client.Disconnect();
	t.join();
	return 0;
}