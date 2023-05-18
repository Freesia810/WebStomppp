#include "WebStompType.h"
#include <sstream>
#include <string>


webstomppp::StompFrame::StompFrame(const char* raw_str)
{
	std::stringstream ss;
	ss << raw_str;

	std::string command;
	std::getline(ss, command);

	if (command == "MESSAGE") {
		type = StompCommandType::MESSAGE;
	}
	else if (command == "ERROR") {
		type = StompCommandType::ERROR_FRAME;
	}
	else if (command == "CONNECTED") {
		type = StompCommandType::CONNECTED;
	}
	else if (command == "RECEIPT") {
		type = StompCommandType::RECEIPT;
	}
	else if (command == "CONNECT") {
		type = StompCommandType::CONNECT;
	}
	else if (command == "DISCONNECT") {
		type = StompCommandType::DISCONNECT;
	}
	else if (command == "SEND") {
		type = StompCommandType::SEND;
	}
	else if (command == "STOMP") {
		type = StompCommandType::STOMP;
	}
	else if (command == "SUBSCRIBE") {
		type = StompCommandType::SUBSCRIBE;
	}
	else if (command == "UNSUBSCRIBE") {
		type = StompCommandType::UNSUBSCRIBE;
	}
	else if (command == "ACK") {
		type = StompCommandType::ACK;
	}
	else if (command == "NACK") {
		type = StompCommandType::NACK;
	}
	else {
		type = StompCommandType::UNKNOWN;
	}

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

	ss >> body;
}


void webstomppp::StompFrame::toByteFrame(const char* raw_str, char*& buf, size_t& len)
{
	std::string str = std::string(raw_str);
	buf = new char[str.size() + 1];
	strcpy_s(buf, str.size() + 1, str.c_str());
	buf[str.size()] = '\0';
	len = str.size() + 1;
}

void webstomppp::StompFrame::toRawString(char*& raw_str)
{
	std::stringstream ss;
	switch (type)
	{
	case StompCommandType::CONNECT:
		ss << "CONNECT\n";
		break;
	case StompCommandType::CONNECTED:
		ss << "CONNECTED\n";
		break;
	case StompCommandType::SEND:
		ss << "SEND\n";
		break;
	case StompCommandType::SUBSCRIBE:
		ss << "SUBSCRIBE\n";
		break;
	case StompCommandType::UNSUBSCRIBE:
		ss << "UNSUBSCRIBE\n";
		break;
	case StompCommandType::ACK:
		ss << "ACK\n";
		break;
	case StompCommandType::NACK:
		ss << "NACK\n";
		break;
	case StompCommandType::DISCONNECT:
		ss << "DISCONNECT\n";
		break;
	case StompCommandType::MESSAGE:
		ss << "MESSAGE\n";
		break;
	case StompCommandType::RECEIPT:
		ss << "RECEIPT\n";
		break;
	case StompCommandType::ERROR_FRAME:
		ss << "ERROR\n";
		break;
	default:
		break;
	}

	for (auto& kv : _raw_header) {
		ss << kv.first << ":" << kv.second << "\n";
	}

	ss << "\n" << body;

	raw_str = new char[ss.str().size() + 1];
	strcpy_s(raw_str, ss.str().size() + 1, ss.str().c_str());
}

webstomppp::StompCallbackMsg::StompCallbackMsg(StompFrameHeader& header, const char* body, uint64_t session_id, StompCommandType type)
{
	this->session_id = session_id;
	this->type = type;
	this->body = body;
	std::stringstream ss;
	for (auto& kv : header) {
		ss << kv.first << ":" << kv.second << "\n";
	}
	std::string str = ss.str();
	strcpy_s(this->header_raw, 1024, str.c_str());
	strcpy_s(this->header_raw, 1023, str.c_str());
}

webstomppp::StompMessageFrame::StompMessageFrame(const char* destination, const char* subscription, const char* message_id, const char* content, const char* content_type, StompFrameHeader* user_defined_header)
{
	type = StompCommandType::MESSAGE;
	_raw_header.emplace_back(StompHeaderKeyValue("subscription", subscription));
	_raw_header.emplace_back(StompHeaderKeyValue("message-id", message_id));
	_raw_header.emplace_back(StompHeaderKeyValue("destination", destination));
	_raw_header.emplace_back(StompHeaderKeyValue("content-type", content_type));
	auto len = strlen(content);
	if (!len) {
		_raw_header.emplace_back(StompHeaderKeyValue("content-length", std::to_string(len)));
		if (user_defined_header->count("content-length")) {
			user_defined_header->erase("content-length");
		}
	}

	if (user_defined_header != nullptr) {
		for (auto& kv : *user_defined_header) {
			_raw_header.emplace_back(StompHeaderKeyValue(kv.first, kv.second));
		}
	}

	body = content;
}
