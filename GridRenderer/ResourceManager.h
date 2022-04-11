#pragma once

typedef size_t ResourceIndex;

class ResourceManager
{
protected:

public:
	ResourceManager() = default;
	virtual ~ResourceManager() = default;
	ResourceManager(const ResourceManager& other) = delete;
	ResourceManager& operator=(const ResourceManager& other) = delete;
	ResourceManager(ResourceManager&& other) = default;
	ResourceManager& operator=(ResourceManager&& other) = default;
};