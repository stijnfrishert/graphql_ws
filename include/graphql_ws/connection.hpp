#pragma once

#include <functional>

namespace graphql_ws {
	class [[nodiscard]] Connection
	{
	public:
		Connection() = default;
		Connection(std::function<void()> complete);
		Connection(const Connection&) = delete;
		Connection(Connection&&) = default;

		Connection& operator=(const Connection&) = delete;
		Connection& operator=(Connection&&) = default;

		~Connection();

		void detach();

	private:
		std::function<void()> complete;
	};
}   // namespace graphql_ws