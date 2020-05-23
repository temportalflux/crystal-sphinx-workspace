#include "graphics/QueueFamilyGroup.hpp"
#include "graphics/QueueFamily.hpp"

using namespace graphics;

bool QueueFamilyGroup::hasFoundAllQueues() const
{
	return idxGraphicsQueue.has_value() && idxPresentationQueue.has_value();
}

std::set<ui32> QueueFamilyGroup::uniqueQueues() const
{
	return { idxGraphicsQueue.value(), idxPresentationQueue.value() };
}

std::vector<ui32> QueueFamilyGroup::allQueues() const
{
	return { idxGraphicsQueue.value(), idxPresentationQueue.value() };
}

bool QueueFamilyGroup::hasQueueFamily(QueueFamily type) const
{
	return this->getQueueIndex(type).has_value();
}

std::optional<ui32> QueueFamilyGroup::getQueueIndex(QueueFamily type) const
{
	switch (type)
	{
	case QueueFamily::eGraphics: return idxGraphicsQueue;
	case QueueFamily::ePresentation: return idxPresentationQueue;
	}
	return std::nullopt;
}
