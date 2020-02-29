#pragma once

#include "studio_window.hpp"

#include <core/kpi/Node.hpp>

namespace riistudio::frontend {

class EditorWindow : public StudioWindow {
public:
	EditorWindow(std::unique_ptr<kpi::IDocumentNode> state, const std::string& path);

	void draw() override;

	std::string getFilePath() { return mFilePath; }

	std::unique_ptr<kpi::IDocumentNode> mState;
	std::string mFilePath;
	kpi::History mHistory;
};

}
