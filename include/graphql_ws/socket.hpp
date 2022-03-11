#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <string_view>
#include <unordered_map>

#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

#include <graphql_ws/connection.hpp>

namespace graphql_ws {
	using QueryArgs = std::unordered_map<std::string, nlohmann::json>;
	using QueryErrors = std::vector<nlohmann::json>;

	class Socket
	{
	public:
		Socket(std::string_view address);
		~Socket();

		Connection send(std::string_view query, const QueryArgs& args, std::function<void(const nlohmann::json&)> onReceive);

	public:
		std::function<void(std::string_view)> onProtocolError;

	private:
		void send(const nlohmann::json& json);
		void process(const ix::WebSocketMessage& msg);
		void processMessage(const nlohmann::json& msg);

	private:
		std::optional<std::promise<void>> ackReceived;
		ix::WebSocket socket;
		std::uint64_t nextUniqueId = 0;

		std::mutex callbacksMutex;
		std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> replyCallbacks;
	};
}   // namespace graphql_ws