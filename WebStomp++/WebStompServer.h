#pragma once

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <unordered_map>
#include <map>
#include <set>
#include <functional>
#include <vector>

#include "WebStompType.h"

namespace webstomppp {
	using server = websocketpp::server<websocketpp::config::asio>;

	class WebStompServer
	{
	public:
		void Init();
		void Run(uint16_t port);
		void RegisterMessageHandler(const char* destination, webstomppp::callback_func callback);
		void RegisterSubscribeHandler(const char* destination, webstomppp::callback_func callback);
		void RegisterUnsubscribeHandler(const char* destination, webstomppp::callback_func callback);
		void SendToUser(uint64_t session_id, const char* destination, const char* content, const char* content_type = "text/plain", const char* raw_addition_header = nullptr);
		void SendBroadcast(const char* destination, const char* content, const char* content_type = "text/plain", const char* raw_addition_header = nullptr);
		virtual void onConnected(uint64_t session_id) {};
		virtual void onDisConnected(uint64_t session_id) {};
		virtual void onWebSocketOpen(uint64_t session_id) {};
		virtual void onWebSocketClose(uint64_t session_id) {};
	private:
		uint64_t session_id_gen{ 0 };
		uint64_t message_id_gen{ 0 };
		server _ws_server;
		std::unordered_map<std::string, std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>> subscribe_map;
		std::map<websocketpp::connection_hdl, std::unordered_map<std::string, std::string>, std::owner_less<websocketpp::connection_hdl>> hdl_sub_id_map;
		std::unordered_map<std::string, webstomppp::callback_func> sub_handler_map;
		std::unordered_map<std::string, webstomppp::callback_func> unsub_handler_map;
		std::unordered_map<std::string, webstomppp::callback_func> msg_handler_map;
		std::map<websocketpp::connection_hdl, uint64_t, std::owner_less<websocketpp::connection_hdl>> hdl_id_map;
		std::unordered_map<uint64_t, websocketpp::connection_hdl> id_hdl_map;

		void onSocketOpen(websocketpp::connection_hdl hdl);
		void onSocketClose(websocketpp::connection_hdl hdl);
		void onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg);
	};
}

