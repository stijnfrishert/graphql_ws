#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>

#include <graphql_ws/connection.hpp>
#include <graphql_ws/socket.hpp>
#include <nlohmann/json.hpp>

namespace graphql_ws {
	class Context
	{
	public:
		Context(std::string_view address);

		Connection sendAsync(std::string_view query, const QueryArgs& args, std::function<void(const nlohmann::json&)> onReceive);
		std::optional<nlohmann::json> sendSync(std::string_view query, const QueryArgs& args);

	public:
		Socket socket;

		std::function<void(const std::string_view, const QueryArgs&, const QueryErrors&)> onQueryError;
	};
}   // namespace graphql_ws