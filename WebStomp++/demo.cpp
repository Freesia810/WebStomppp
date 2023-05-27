#include "WebStompClient.h"
#include "WebStompType.h"
#include <thread>

int main() {
	webstomppp::WebStompClient client(true);
	client.Connect("wss://47.102.200.110/meta-trade/stomp");
	std::thread t(&webstomppp::WebStompClient::Run, &client);
	Sleep(10000);
	client.Disconnect();
	t.join();

	return 0;
}