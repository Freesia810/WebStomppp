#include "WebStompClient.h"
#include "WebStompType.h"
#include <thread>

int main() {
	webstomppp::WebStompClient client;
	client.Connect("ws://127.0.0.1:7285/meta-trade/stomp");
	std::thread t(&webstomppp::WebStompClient::Run, &client);
	Sleep(5000);
	client.Disconnect();
	t.join();
	return 0;
}