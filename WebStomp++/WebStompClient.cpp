#include "WebStompClient.h"
#include <sstream>
#include <string>

void webstomppp::WebStompClient::_message_dispatcher_ws(websocketpp::connection_hdl hdl, ws_client::message_ptr msg){
	switch (msg->get_opcode())
	{
	case websocketpp::frame::opcode::TEXT:
		{
			StompFrame stomp_msg(msg->get_payload().c_str());
			switch (stomp_msg.type)
			{
			case StompCommandType::CONNECTED:
				this->OnConnected();
				break;
			case StompCommandType::MESSAGE:
			{
				auto it = _topic_callback_map.find(stomp_msg.header["destination"]);
				if (it != _topic_callback_map.end()) {
					
					(it->second)(StompCallbackMsg(stomp_msg.header, stomp_msg.body.c_str()));

					StompAckFrame frame(stomp_msg.header["message-id"].c_str());
					char* raw_str = nullptr;
					char* buf = nullptr;
					frame.toRawString(raw_str);
					size_t len = 0;
					StompAckFrame::toByteFrame(raw_str, buf, len);
					delete raw_str;
					_con_ws->send(buf, len, websocketpp::frame::opcode::TEXT);
					delete buf;
				}
				else {
					throw StompException(StompExceptionType::SubcribeTopicNotFoundException);
				}
			}
				break;
			case StompCommandType::RECEIPT:
				if (stomp_msg.header["receipt-id"] == std::string(disconnect_receipt_id)) {
					_is_connected.store(false);
					this->OnDisconnected();
				}
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}
}

void webstomppp::WebStompClient::_message_dispatcher_wss(websocketpp::connection_hdl hdl, wss_client::message_ptr msg){
	switch (msg->get_opcode())
	{
	case websocketpp::frame::opcode::TEXT:
		{
			StompFrame stomp_msg(msg->get_payload().c_str());
			switch (stomp_msg.type)
			{
			case StompCommandType::CONNECTED:
				this->OnConnected();
				break;
			case StompCommandType::MESSAGE:
			{
				auto it = _topic_callback_map.find(stomp_msg.header["destination"]);
				if (it != _topic_callback_map.end()) {
					
					(it->second)(StompCallbackMsg(stomp_msg.header, stomp_msg.body.c_str()));

					StompAckFrame frame(stomp_msg.header["message-id"].c_str());
					char* raw_str = nullptr;
					char* buf = nullptr;
					frame.toRawString(raw_str);
					size_t len = 0;
					StompAckFrame::toByteFrame(raw_str, buf, len);
					delete raw_str;
					_con_wss->send(buf, len, websocketpp::frame::opcode::TEXT);
					delete buf;
				}
				else {
					throw StompException(StompExceptionType::SubcribeTopicNotFoundException);
				}
			}
				break;
			case StompCommandType::RECEIPT:
				if (stomp_msg.header["receipt-id"] == std::string(disconnect_receipt_id)) {
					_is_connected.store(false);
					this->OnDisconnected();
				}
				break;
			default:
				break;
			}
		}
		break;
	default:
		break;
	}
}

void webstomppp::WebStompClient::_on_open_ws(ws_client* c, websocketpp::connection_hdl hdl)
{
	auto tmp = _uri.substr(5);
	auto host = tmp.substr(0, tmp.find_first_of('/'));

	StompConnectFrame frame(host.c_str());
	char* raw_str = nullptr;
	frame.toRawString(raw_str);
	char* buf = nullptr;
	size_t len = 0;
	StompConnectFrame::toByteFrame(raw_str, buf, len);
	delete raw_str;
	_con_ws->send(buf, len, websocketpp::frame::opcode::TEXT);
	delete buf;
}

void webstomppp::WebStompClient::_on_open_wss(wss_client* c, websocketpp::connection_hdl hdl)
{
	auto tmp = _uri.substr(6);
	auto host = tmp.substr(0, tmp.find_first_of('/'));

	StompConnectFrame frame(host.c_str());
	char* raw_str = nullptr;
	frame.toRawString(raw_str);
	char* buf = nullptr;
	size_t len = 0;
	StompConnectFrame::toByteFrame(raw_str, buf, len);
	delete raw_str;
	_con_wss->send(buf, len, websocketpp::frame::opcode::TEXT);
	delete buf;
}

static std::shared_ptr<boost::asio::ssl::context> on_tls_init() {
    // establishes a SSL connection
    std::shared_ptr<boost::asio::ssl::context> ctx = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (std::exception &e) {
        std::cout << "Error in context pointer: " << e.what() << std::endl;
    }
    return ctx;
}

webstomppp::WebStompClient::WebStompClient(bool ssl)
{
	enable_ssl = ssl;
	if(enable_ssl){
		// Set logging to be pretty verbose (everything except message payloads)
		_wss_client.clear_access_channels(websocketpp::log::alevel::all);
		_wss_client.clear_error_channels(websocketpp::log::elevel::all);

		// Initialize ASIO
		_wss_client.init_asio();

		_wss_client.set_tls_init_handler(bind(&on_tls_init));
	}
	else{
		// Set logging to be pretty verbose (everything except message payloads)
		_ws_client.clear_access_channels(websocketpp::log::alevel::all);
		_ws_client.clear_error_channels(websocketpp::log::elevel::all);

		// Initialize ASIO
		_ws_client.init_asio();
	}
}

webstomppp::WebStompClient::~WebStompClient() {
	if(enable_ssl){
		if (_is_connected.load()) {
			_wss_client.stop_perpetual();
			_wss_client.close(_con_wss->get_handle(), websocketpp::close::status::going_away, "", _ec);
		}
	}
	else{
		if (_is_connected.load()) {
			_ws_client.stop_perpetual();
			_ws_client.close(_con_ws->get_handle(), websocketpp::close::status::going_away, "", _ec);
		}
	}
}

void webstomppp::WebStompClient::Connect(const char* uri)
{
	_uri = uri;

	if(enable_ssl){
		_con_wss = _wss_client.get_connection(uri, _ec);
		if (_ec) {
			throw StompException(StompExceptionType::ConnectFailedException);
		}

		// Register our message handler
		_con_wss->set_open_handler(websocketpp::lib::bind(&WebStompClient::_on_open_wss, this, &_wss_client, websocketpp::lib::placeholders::_1));
		_con_wss->set_message_handler(websocketpp::lib::bind(&WebStompClient::_message_dispatcher_wss, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));


		// Note that connect here only requests a connection. No network messages are
		// exchanged until the event loop starts running in the next line.
		_wss_client.connect(_con_wss);
	}
	else{
		_con_ws = _ws_client.get_connection(uri, _ec);
		if (_ec) {
			throw StompException(StompExceptionType::ConnectFailedException);
		}

		// Register our message handler
		_con_ws->set_open_handler(websocketpp::lib::bind(&WebStompClient::_on_open_ws, this, &_ws_client, websocketpp::lib::placeholders::_1));
		_con_ws->set_message_handler(websocketpp::lib::bind(&WebStompClient::_message_dispatcher_ws, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));


		// Note that connect here only requests a connection. No network messages are
		// exchanged until the event loop starts running in the next line.
		_ws_client.connect(_con_ws);
	}
}

void webstomppp::WebStompClient::Run()
{
	if(enable_ssl){
		_wss_client.run();
	}
	else{
		_ws_client.run();
	}
}

void webstomppp::WebStompClient::Subscribe(const char* destination, webstomppp::callback_func callback){
	auto it = this->_topic_id_map.find(destination);
	if (it != _topic_id_map.end()) return;

	StompSubscribeFrame frame(destination, subscribe_id_gen);
	char* raw_str = nullptr;
	frame.toRawString(raw_str);
	char* buf = nullptr;
	size_t len = 0;
	StompSubscribeFrame::toByteFrame(raw_str, buf, len);
	delete raw_str;
	if(enable_ssl){
		_con_wss->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	else{
		_con_ws->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	delete buf;

	_topic_id_map.insert(std::make_pair(destination, subscribe_id_gen++));
	_topic_callback_map.insert(std::make_pair(destination, callback));
}
void webstomppp::WebStompClient::Unsubscribe(const char* destination)
{
	auto it = this->_topic_id_map.find(destination);
	if (it != _topic_id_map.end()) return;

	StompUnsubscribeFrame frame(it->second);
	char* raw_str = nullptr;
	frame.toRawString(raw_str);
	char* buf = nullptr;
	size_t len = 0;
	StompUnsubscribeFrame::toByteFrame(raw_str, buf, len);
	delete raw_str;
	if(enable_ssl){
		_con_wss->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	else{
		_con_ws->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	delete buf;

	_topic_id_map.erase(destination);
	_topic_callback_map.erase(destination);
}
void webstomppp::WebStompClient::Disconnect()
{
	StompDisconnectFrame frame;
	char* raw_str = nullptr;
	frame.toRawString(raw_str);
	char* buf = nullptr;
	size_t len = 0;
	StompDisconnectFrame::toByteFrame(raw_str, buf, len);
	delete raw_str;
	if(enable_ssl){
		_con_wss->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	else{
		_con_ws->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	delete buf;
	while (true) {
		if (!_is_connected.load()) {
			if(enable_ssl){
				_con_wss->close(websocketpp::close::status::normal, "close");
			}
			else{
				_con_ws->close(websocketpp::close::status::normal, "close");
			}
			break;
		}
	}
}
void webstomppp::WebStompClient::Send(const char* raw_str) {
	char* buf = nullptr;
	size_t len = 0;
	StompSendFrame::toByteFrame(raw_str, buf, len);
	if(enable_ssl){
		_con_wss->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	else{
		_con_ws->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
}

void webstomppp::WebStompClient::SendJson(const char* des, const char* content) {
	StompJsonSendFrame frame(des, content);
	char* raw_str = nullptr;
	frame.toRawString(raw_str);
	char* buf = nullptr;
	size_t len = 0;
	StompSendFrame::toByteFrame(raw_str, buf, len);
	delete raw_str;
	if(enable_ssl){
		_con_wss->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	else{
		_con_ws->send(buf, len, websocketpp::frame::opcode::TEXT);
	}
	delete buf;
}
