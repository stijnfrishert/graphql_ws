
#include <graphql_ws/socket.hpp>

#include <cassert>
#include <sstream>
#include <stdexcept>

#include <ixwebsocket/IXWebSocketMessageType.h>
#include <ixwebsocket/IXWebSocketSendInfo.h>

namespace graphql_ws {
	namespace {
		constexpr std::string_view DoubleAckErrorMessage = "Received graphql-ws connection_ack twice";
	}

	Socket::Socket(std::string_view address)
	{
		ackReceived.emplace();

		socket.setUrl("ws://" + std::string{ address });
		socket.addSubProtocol("graphql-transport-ws");
		socket.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
			process(*msg);
		});

		socket.start();

		ackReceived->get_future().get();
	}

	Socket::~Socket()
	{
		socket.stop();
		socket.setOnMessageCallback(nullptr);
	}

	Connection Socket::send(std::string_view query, const QueryArgs& args, std::function<void(const nlohmann::json&)> onReceive)
	{
		const std::string idStr = std::to_string(nextUniqueId++);

		if (onReceive)
		{
			std::unique_lock<std::mutex> lock(callbacksMutex);
			replyCallbacks.emplace(idStr, onReceive);
		}

		const nlohmann::json payload{ { "id", idStr }, { "query", query }, { "variables", args } };
		send({ { "type", "subscribe" }, { "id", idStr }, { "payload", payload } });

		return Connection{ [this, idStr] {
			send({ { "type", "complete" }, { "id", idStr } });
			std::unique_lock<std::mutex> lock(callbacksMutex);
			if (auto it = replyCallbacks.find(idStr); it != replyCallbacks.end())
				replyCallbacks.erase(it);
		} };
	}

	void Socket::send(const nlohmann::json& json)
	{
		const auto msg = json.dump();
		const ix::WebSocketSendInfo info = socket.send(msg);
		assert(info.success);
	}

	void Socket::process(const ix::WebSocketMessage& msg)
	{
		switch (msg.type)
		{
			case ix::WebSocketMessageType::Open:
				send({ { "type", "connection_init" } });
				break;
			case ix::WebSocketMessageType::Close:
				break;
			case ix::WebSocketMessageType::Error:
				if (ackReceived)
					ackReceived->set_exception(std::make_exception_ptr(std::runtime_error(msg.errorInfo.reason)));
				else if (onProtocolError)
					onProtocolError(msg.errorInfo.reason);
				break;
			case ix::WebSocketMessageType::Message:
				processMessage(nlohmann::json::parse(msg.str));
				break;
			case ix::WebSocketMessageType::Ping:
				// We've been pinged. I assume the websocket implementation we use automatically pongs back for us.
				// We don't do anything.
				break;
			case ix::WebSocketMessageType::Pong:
				// This should never happen, because we don't ping the connection currently
				assert(false);
				break;
			case ix::WebSocketMessageType::Fragment:
				// I don't know what fragments are, so I'm just going to assert false for
				// now and handle this when it happens
				assert(false);
				break;
		}
	}

	void Socket::processMessage(const nlohmann::json& msg)
	{
		assert(msg.is_object());
		assert(msg.contains("type") && msg["type"].is_string());

		const auto& type = msg["type"].get<std::string>();
		if (type == "connection_ack")
		{
			if (ackReceived)
			{
				ackReceived->set_value();
				ackReceived.reset();
			}
			else if (onProtocolError)
				onProtocolError(DoubleAckErrorMessage);
			else
				throw std::runtime_error(std::string{ DoubleAckErrorMessage });
		}
		else if (type == "ping")
		{
			// The graphql-ws protocol pinged us. We just send back a pong.
			// I'm not sure why this exists, because the WebSocket protocol itself has a ping/pong
			// mechanism, but whatever. We're good citizens, let's just pong back.
			send({ { "type", "pong" } });
		}
		else if (type == "pong")
		{
			// We never send pings, so this is some sort of heartbeat pong from the server
			//
			// Well...whatever. Do you thing, servertje
		}
		else if (type == "next")
		{
			assert(msg.contains("id"));
			assert(msg.contains("payload"));

			std::unique_lock<std::mutex> lock(callbacksMutex);
			if (auto it = replyCallbacks.find(msg["id"].get<std::string>()); it != replyCallbacks.end())
				it->second(msg["payload"]);
		}
		else if (type == "error")
		{
			assert(msg.contains("id"));
			assert(msg.contains("payload"));

			std::unique_lock<std::mutex> lock(callbacksMutex);
			if (auto it = replyCallbacks.find(msg["id"].get<std::string>()); it != replyCallbacks.end())
				replyCallbacks.erase(it);
		}
		else if (type == "complete")
		{
			assert(msg.contains("id"));

			std::unique_lock<std::mutex> lock(callbacksMutex);
			if (auto it = replyCallbacks.find(msg["id"].get<std::string>()); it != replyCallbacks.end())
				replyCallbacks.erase(it);
		}
		else
		{
			// Unhandled graphql-ws message type
			std::ostringstream stream;
			throw std::runtime_error("Unknown graphql-ws message type: " + type);
		}
	}
}   // namespace graphql_ws