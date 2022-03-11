
#include <graphql_ws/connection.hpp>

namespace graphql_ws {
	Connection::Connection(std::function<void()> complete) : complete(complete)
	{}

	Connection::~Connection()
	{
		if (complete)
			complete();
	}

	void Connection::detach()
	{
		complete = nullptr;
	}
}   // namespace graphql_ws