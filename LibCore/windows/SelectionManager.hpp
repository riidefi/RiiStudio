#pragma once

#include <vector>
#include <algorithm>

// Held by application
struct SelectionManager
{
	enum Type
	{
		None = 0, // Nothing is selected
		Effect = (1 << 0), // Particle Effect System
		Texture = (1 << 1), // Texture Image
		Material = (1 << 2) // Of a model, material vector
	};
	struct Selection
	{
		Type type;
		void* object;

		std::vector<int> mIndices;

		Selection(Type t, void* o) : type(t), object(o) {}
	};
	std::vector<Selection> mSelections;

	int getIdxOfSelect(Type type, void* obj)
	{
		return static_cast<int>(std::find_if(mSelections.begin(), mSelections.end(),
			[type, obj](const Selection& s) { return s.type == type && s.object == obj; }) - mSelections.begin());
	}

	bool isSelected(Type type, void* obj)
	{
		for (const auto& sel : mSelections)
			if (sel.object == obj && sel.type == type)
				return true;
	
		return false;
	}
};
