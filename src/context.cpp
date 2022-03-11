#include <graphql_ws/context.hpp>

#include <future>

namespace graphql_ws {
	Context::Context(std::string_view address) : socket(address)
	{}

	Connection Context::sendAsync(std::string_view query, const QueryArgs& args, std::function<void(const nlohmann::json&)> onReceive)
	{
		return socket.send(query, args, [this, query = std::string{ query }, args, onReceive](const nlohmann::json& payload) {
			if (payload.contains("errors"))
			{
				const auto& errors = payload["errors"].get<std::vector<nlohmann::json>>();
				if (onQueryError)
					onQueryError(query, args, errors);
				else
					throw std::runtime_error(errors.front()["message"].get<std::string>());
			}
			else
			{
				assert(payload.contains("data"));
				if (onReceive)
					onReceive(payload["data"]);
			}
		});
	}

	std::optional<nlohmann::json> Context::sendSync(std::string_view query, const QueryArgs& args)
	{
		auto promise = std::make_shared<std::promise<nlohmann::json>>();
		auto future = promise->get_future();

		// Store the handle, so that is gets out of scope and completes the action on destruction
		const auto handle = socket.send(query, args, [this, &query, &args, promise](const nlohmann::json& payload) {
			if (payload.contains("errors"))
			{
				const auto& errors = payload["errors"].get<std::vector<nlohmann::json>>();
				if (onQueryError)
				{
					onQueryError(query, args, errors);
					promise->set_value({});
				}
				else
					promise->set_exception(std::make_exception_ptr(std::runtime_error(errors.front()["message"].get<std::string>())));
			}
			else
			{
				assert(payload.contains("data"));
				promise->set_value(payload["data"]);
			}
		});

		return future.get();
	}
}   // namespace graphql_ws