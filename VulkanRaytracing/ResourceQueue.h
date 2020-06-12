#pragma once
#include <vector>
#include <functional>

using VoidFunction = std::function<void()>;

namespace vkut {

	class ResourceQueue
	{
	public:

		void push(const VoidFunction &destroyFunc);
		void popAll();

	private:
		
		std::vector<VoidFunction> queue;
	};
}


