#include "ResourceQueue.h"

namespace vkut {
	void ResourceQueue::push(const VoidFunction &destroyFunc)
	{
		queue.push_back(destroyFunc);
	}

	void ResourceQueue::popAll()
	{
		while(!queue.empty())
		{
			queue[queue.size() - 1U]();
			queue.pop_back();
		}		
	}
}

