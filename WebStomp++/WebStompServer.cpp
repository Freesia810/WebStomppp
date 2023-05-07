#include "WebStompServer.h"

void webstomppp::WebStompServer::Init()
{
    _ws_server.init_asio();

    _ws_server.set_open_handler(websocketpp::lib::bind(&WebStompServer::onSocketOpen, this, websocketpp::lib::placeholders::_1));
    _ws_server.set_close_handler(websocketpp::lib::bind(&WebStompServer::onSocketClose, this, websocketpp::lib::placeholders::_1));
    _ws_server.set_message_handler(websocketpp::lib::bind(&WebStompServer::onMessage, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
}

void webstomppp::WebStompServer::Run(uint16_t port)
{
	_ws_server.listen(port);
	_ws_server.start_accept();
	try {
		_ws_server.run();
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
	}
}

void webstomppp::WebStompServer::RegisterMessageHandler(const char* destination, webstomppp::callback_func callback)
{
	this->msg_handler_map.insert(std::make_pair(destination, callback));
}

void webstomppp::WebStompServer::RegisterSubscribeHandler(const char* destination, webstomppp::callback_func callback)
{
	this->sub_handler_map.insert(std::make_pair(destination, callback));
}

void webstomppp::WebStompServer::RegisterUnsubscribeHandler(const char* destination, webstomppp::callback_func callback)
{
	this->unsub_handler_map.insert(std::make_pair(destination, callback));
}

void webstomppp::WebStompServer::SendToUser(uint64_t session_id, const char* destination, const char* content, const char* content_type, const char* raw_addition_header)
{
	auto& sub_id = hdl_sub_id_map[id_hdl_map[session_id]][destination];
	std::stringstream ss;
	StompFrameHeader header;
	ss << raw_addition_header;
	while (true) {
		std::string line;
		std::getline(ss, line);

		if (line == "") {
			break;
		}

		auto tmp = line.find(':');
		std::string key = line.substr(0, tmp);
		std::string value = line.substr(tmp + 1);

		header.emplace(std::make_pair(key, value));
	}

	auto frame = StompMessageFrame(destination, sub_id.c_str(), std::to_string(message_id_gen++).c_str(), content, content_type, &header);
	char* buf = nullptr;
	size_t len = 0;
	StompMessageFrame::toByteFrame(frame.toRawString().c_str(), buf, len);
	_ws_server.send(id_hdl_map[session_id], buf, len, websocketpp::frame::opcode::TEXT);
}

void webstomppp::WebStompServer::SendBroadcast(const char* destination, const char* content, const char* content_type, const char* raw_addition_header)
{
	std::stringstream ss;
	StompFrameHeader header;
	ss << raw_addition_header;
	while (true) {
		std::string line;
		std::getline(ss, line);

		if (line == "") {
			break;
		}

		auto tmp = line.find(':');
		std::string key = line.substr(0, tmp);
		std::string value = line.substr(tmp + 1);

		header.emplace(std::make_pair(key, value));
	}

	auto& hdls = subscribe_map[destination];
	for (auto& k : hdls) {
		auto& sub_id = hdl_sub_id_map[k][destination];

		auto frame = StompMessageFrame(destination, sub_id.c_str(), std::to_string(message_id_gen++).c_str(), content, content_type, &header);
		char* buf = nullptr;
		size_t len = 0;
		StompMessageFrame::toByteFrame(frame.toRawString().c_str(), buf, len);
		_ws_server.send(k, buf, len, websocketpp::frame::opcode::TEXT);
	}
}

void webstomppp::WebStompServer::onSocketOpen(websocketpp::connection_hdl hdl)
{
	auto sid = session_id_gen++;
	id_hdl_map.insert(std::make_pair(sid, hdl));
	hdl_id_map.insert(std::make_pair(hdl, sid));
	this->onWebSocketOpen(sid);
}

void webstomppp::WebStompServer::onSocketClose(websocketpp::connection_hdl hdl)
{
	auto sid = hdl_id_map[hdl];
	this->onWebSocketClose(sid);
	id_hdl_map.erase(sid);
	hdl_id_map.erase(hdl);

	if (hdl_sub_id_map.count(hdl)) {
		hdl_sub_id_map.erase(hdl);
		for (auto& kv : subscribe_map) {
			auto& v = kv.second;

			if (v.count(hdl)) {
				v.erase(hdl);
			}
		}
	}
}

void webstomppp::WebStompServer::onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
	switch (msg->get_opcode())
	{
	case websocketpp::frame::opcode::TEXT:
	{
		StompFrame stomp_msg(msg->get_payload().c_str());
		switch (stomp_msg.type)
		{
		case StompCommandType::CONNECT: case StompCommandType::STOMP:
		{
			auto& version = stomp_msg.header["accept-version"];

			auto frame = StompConnectedFrame(version.c_str(), std::to_string(hdl_id_map[hdl]).c_str());
			char* buf = nullptr;
			size_t len = 0;
			StompMessageFrame::toByteFrame(frame.toRawString().c_str(), buf, len);
			_ws_server.send(hdl, buf, len, websocketpp::frame::opcode::TEXT);
		}
		break;
		case StompCommandType::SEND:
		{
			auto& des = stomp_msg.header["destination"];
			auto& body = stomp_msg.body;
			auto it = msg_handler_map.find(des);
			if (it != msg_handler_map.end()) {
				(it->second)(StompCallbackMsg(stomp_msg.header, body.c_str(), hdl_id_map[hdl], StompCommandType::SEND));
				auto header = stomp_msg.header;
				header.erase("destination");
				header.erase("content-type");

				auto frame = StompMessageFrame(des.c_str(), hdl_sub_id_map[hdl][des].c_str(), std::to_string(message_id_gen++).c_str(), body.c_str(), stomp_msg.header["content-type"].c_str(), &header);
				char* buf = nullptr;
				size_t len = 0;
				StompMessageFrame::toByteFrame(frame.toRawString().c_str(), buf, len);
				_ws_server.send(hdl, buf, len, websocketpp::frame::opcode::TEXT);
			}
		}
		break;
		case StompCommandType::SUBSCRIBE:
		{
			auto& des = stomp_msg.header["destination"];
			auto& sub_id = stomp_msg.header["id"];
			auto& ack_mode = stomp_msg.header["ack"];
			auto& body = stomp_msg.body;

			auto set_it = subscribe_map.find(des);
			if (set_it == subscribe_map.end()) {
				std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> s{ hdl };
				subscribe_map.insert(std::make_pair(des, s));
			}
			else {
				set_it->second.insert(hdl);
			}

			auto map_it = hdl_sub_id_map.find(hdl);
			if (map_it == hdl_sub_id_map.end()) {
				std::unordered_map<std::string, std::string> um{ {des, sub_id} };
				hdl_sub_id_map.insert(std::make_pair(hdl, um));
			}
			else {
				map_it->second.insert(std::make_pair(des, sub_id));
			}

			auto it = sub_handler_map.find(des);
			if (it != sub_handler_map.end()) {
				(it->second)(StompCallbackMsg(stomp_msg.header, body.c_str(), hdl_id_map[hdl], StompCommandType::SUBSCRIBE));
			}
		}
		break;
		case StompCommandType::UNSUBSCRIBE:
		{
			auto& sub_id = stomp_msg.header["id"];
			auto& des = hdl_sub_id_map[hdl][sub_id];
			auto& body = stomp_msg.body;

			subscribe_map[des].erase(hdl);
			if (!subscribe_map[des].size()) {
				subscribe_map.erase(des);
			}
			
			hdl_sub_id_map[hdl].erase(des);
			if (!hdl_sub_id_map[hdl].size()) {
				hdl_sub_id_map.erase(hdl);
			}

			auto it = unsub_handler_map.find(des);
			if (it != unsub_handler_map.end()) {
				(it->second)(StompCallbackMsg(stomp_msg.header, body.c_str(), hdl_id_map[hdl], StompCommandType::UNSUBSCRIBE));
			}
		}
		break;
		case StompCommandType::ACK:
		{

		}
		break;
		case StompCommandType::NACK:
		{

		}
		break;
		case StompCommandType::DISCONNECT:
		{
			auto& rpt_id = stomp_msg.header["receipt"];

			StompReceiptFrame frame(rpt_id.c_str());
			char* buf = nullptr;
			size_t len = 0;
			StompReceiptFrame::toByteFrame(frame.toRawString().c_str(), buf, len);
			_ws_server.send(hdl, buf, len, websocketpp::frame::opcode::TEXT);
			this->onDisConnected();
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
