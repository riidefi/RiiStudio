#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

class SelectionManager
{
public:
	void prepareKeys(const std::vector<std::string>& entries)
	{
		for (const auto& e : entries)
			mMap[e] = {};
	}
	SelectionManager(const std::vector<std::string>& entries = {})
	{
		prepareKeys(entries);
	}
	bool isSelected(const std::string& key, std::size_t index) const
	{
		const auto& vec = mMap.at(key);
		return std::find(vec.begin(), vec.end(), index) != vec.end();
	}
	bool select(const std::string& key, std::size_t index)
	{
		if (isSelected(key, index)) return true;

		mMap[key].push_back(index);
		return false;
	}
	bool deselect(const std::string& key, std::size_t index)
	{
		auto it = std::find(mMap[key].begin(), mMap[key].end(), index);

		if (it == mMap[key].end()) return false;
		mMap[key].erase(it);
		return true;
	}
	std::size_t clearSelection(const std::string& key)
	{
		std::size_t before = mMap.at(key).size();
		mMap.at(key).clear();
		return before;
	}

private:

	std::size_t getIndexOfSelect(const std::string& key, std::size_t index) const
	{
		const auto& vec = mMap.at(key);
		return std::find(vec.begin(), vec.end(),
			index) - vec.begin();
	}

	// New selection is per-folder of collection
	std::map<std::string, std::vector<std::size_t>> mMap;
};
