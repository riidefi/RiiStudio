#include "editor_window.hpp"

#include <regex>
#include <random>

#include <vendor/fa5/IconsFontAwesome5.h>
#include "kit/Image.hpp"
#include "Renderer.hpp"
#include "kit/Viewport.hpp"

#include <plugins/gc/Export/Material.hpp>

#ifdef _WIN32
#include <vendor/glfw/glfw3.h>
#endif

#include <algorithm>

namespace riistudio::frontend {

struct GenericCollectionOutliner : public StudioWindow
{
	GenericCollectionOutliner(kpi::IDocumentNode& host) : StudioWindow("Outliner"), mHost(host)
	{}

	struct ImTFilter : public ImGuiTextFilter
	{
		bool test(const std::string& str) const noexcept
		{
			return PassFilter(str.c_str());
		}
	};
	struct RegexFilter
	{
		bool test(const std::string& str) const noexcept
		{
			try {
				std::regex match(mFilt);
				return std::regex_search(str, match);
			}
			catch (std::exception e)
			{
				return false;
			}
		}
		void Draw()
		{
#ifdef _WIN32
			char buf[128]{};
			memcpy_s(buf, 128, mFilt.c_str(), mFilt.length());
			ImGui::InputText(ICON_FA_SEARCH " (regex)", buf, 128);
			mFilt = buf;
#endif
		}

		std::string mFilt;
	};
	using TFilter = RegexFilter;

	struct TConfig
	{
		static std::string res_name_plural()
		{
			return "Unknowns";
		}
		static std::string res_name_singular()
		{
			return "Unknown";
		}
		static std::string res_icon_plural()
		{
			return "(?)";
		}
		static std::string res_icon_singular()
		{
			return "(?)";
		}
	};

	//! @brief Return the number of resources in the source that pass the filter.
	//!
	std::size_t calcNumFiltered(const kpi::FolderData& sampler, const TFilter* filter = nullptr) const noexcept
	{
		// If no data, empty
		if (sampler.size() == 0)
			return 0;

		// If we don't have a filter, everything is included.
		if (!filter)
			return sampler.size();

		std::size_t nPass = 0;

		for (u32 i = 0; i < sampler.size(); ++i)
			if (filter->test(sampler[i]->getName().c_str()))
				++nPass;

		return nPass;
	}

	//! @brief Format the title in the "<header> (<number of resources>)" format.
	//!
	std::string formatTitle(const kpi::FolderData& sampler, const TFilter* filter = nullptr) const noexcept
	{
		return "";
		//	const auto name = GetRich(sampler.getType());
		//	return std::string(std::string(name.icon.icon_plural) + "  " +
		//		std::string(name.exposedName) + "s (" + std::to_string(calcNumFiltered(sampler, filter)) + ")");
	}

	void drawFolder(kpi::FolderData& sampler, const kpi::IDocumentNode& host, const std::string& key) noexcept
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (!ImGui::TreeNode(formatTitle(sampler, &mFilter).c_str()))
			return;


		//	if (ImGui::Button("Shuffle")) {
		//		static std::default_random_engine rng{};
		//		std::shuffle(std::begin(sampler), std::end(sampler), rng);
		//	}

		// A filter tree for multi selection. Prevents inclusion of unfiltered data with SHIFT clicks.
		std::vector<int> filtered;

		int justSelectedId = -1;
		// Relative to filter vector.
		std::size_t justSelectedFilteredIdx = -1;
		// Necessary to filter out clicks on already selected items.
		bool justSelectedAlreadySelected = false;
		// Prevent resetting when SHIFT is unpressed with arrow keys.
		bool thereWasAClick = false;

		// Draw the tree
		for (int i = 0; i < sampler.size(); ++i)
		{
			auto& nodeAt = sampler[i];
			const std::string cur_name = nodeAt->getName();

			if (nodeAt->children.empty() && !mFilter.test(cur_name)) {
				continue;
			}

			filtered.push_back(i);

			// Whether or not this node is already selected.
			// Selections from other windows will carry over.
			bool curNodeSelected = sampler.isSelected(i);

			bool bSelected = ImGui::Selectable(std::to_string(i).c_str(), curNodeSelected);
			ImGui::SameLine();

			thereWasAClick = ImGui::IsItemClicked();
			bool focused = ImGui::IsItemFocused();


			// std::string cur_name = "TODO";
			//	std::string(GetRich(sampler.getType()).icon.icon_singular) + " " +
			//	name;
			

			if (ImGui::TreeNodeEx(std::to_string(i).c_str(), ImGuiTreeNodeFlags_DefaultOpen | (nodeAt->children.empty() ? ImGuiTreeNodeFlags_Leaf : 0), cur_name.c_str()))
			{
				// NodeDrawer::drawNode(*node.get());

				drawRecursive(*nodeAt.get());

				// If there waas a click above, we need to ignore the focus below.
				// Assume only one item can be clicked.
				if (thereWasAClick || (focused && justSelectedId == -1 && !curNodeSelected))
				{
					// If it was already selected, we may need to reprocess for ctrl clicks followed by shift clicks
					justSelectedAlreadySelected = curNodeSelected;
					justSelectedId = i;
					justSelectedFilteredIdx = filtered.size() - 1;
				}


				ImGui::TreePop();
			}
		}
		ImGui::TreePop();

		// If nothing new was selected, no new processing needs to occur.
		if (justSelectedId == -1)
			return;

		// Currently, nothing for ctrl + shift or ctrl + art.
		// If both are pressed, SHIFT takes priority.
		const ImGuiIO& io = ImGui::GetIO();
		bool shiftPressed = io.KeyShift;
		bool ctrlPressed = io.KeyCtrl;

		if (shiftPressed)
		{
			// Transform last selection index into filtered space.
			std::size_t lastSelectedFilteredIdx = -1;
			for (std::size_t i = 0; i < filtered.size(); ++i)
			{
				if (filtered[i] == sampler.getActiveSelection())
					lastSelectedFilteredIdx = i;
			}
			if (lastSelectedFilteredIdx == -1)
			{
				// If our last selection is no longer in filtered space, we can just treat it as a control key press.
				shiftPressed = false;
				ctrlPressed = true;
			}
			else
			{
#undef min
#undef max
				// Iteration must occur in filtered space to prevent selection of occluded nodes.
				const std::size_t a = std::min(justSelectedFilteredIdx, lastSelectedFilteredIdx);
				const std::size_t b = std::max(justSelectedFilteredIdx, lastSelectedFilteredIdx);

				for (std::size_t i = a; i <= b; ++i)
					sampler.select(filtered[i]);
			}
		}

		// If the control key was pressed, we add to the selection if necessary.
		if (ctrlPressed && !shiftPressed)
		{
			// If already selected, no action necessary - just for id update.
			if (!justSelectedAlreadySelected)
				sampler.select(justSelectedId);
			else if (thereWasAClick)
				sampler.deselect(justSelectedId);
		}
		else if (!ctrlPressed && !shiftPressed && (thereWasAClick || justSelectedId != sampler.getActiveSelection()))
		{
			// Replace selection
			sampler.clearSelection();
			sampler.select(justSelectedId);
		}

		// Set our last selection index, for shift clicks.
		sampler.setActiveSelection(justSelectedId);
	}

	void drawRecursive(kpi::IDocumentNode& host) noexcept
	{
		for (auto& folder : host.children)
			drawFolder(folder.second, host, folder.first);
	}
	void draw_() noexcept override
	{
		mFilter.Draw();
		drawRecursive(mHost);
	}

private:
	kpi::IDocumentNode& mHost;
	TFilter mFilter;
};

struct TexImgPreview : public StudioWindow
{
	TexImgPreview(const kpi::IDocumentNode& host)
		: StudioWindow("Texture Preview"), mHost(host)
	{
		setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize);
	}
	void draw_() override
	{
		auto* osamp = mHost.getFolder<lib3d::Texture>();
		if (!osamp) return;

		assert(osamp);
		auto& samp = *osamp;

		int width = samp.at<lib3d::Texture>(samp.getActiveSelection()).getWidth();
		ImGui::InputInt("width", &width);
		const_cast<lib3d::Texture&>(samp.at<lib3d::Texture>(samp.getActiveSelection())).setWidth(width);

		if (lastTexId != samp.getActiveSelection())
			setFromImage(samp.at<lib3d::Texture>(samp.getActiveSelection()));

		mImg.draw();
	}
	void setFromImage(const lib3d::Texture& tex)
	{
		mImg.setFromImage(tex);
	}

	const kpi::IDocumentNode& mHost;
	int lastTexId = -1;
	ImagePreview mImg;
};
struct RenderTest : public StudioWindow
{
	RenderTest(const kpi::IDocumentNode& host)
		: StudioWindow("Viewport"), mHost(host)
	{
		setWindowFlag(ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar);

		const auto* models = host.getFolder<lib3d::Model>();

		if (!models)
			return;

		if (models->size() == 0)
			return;

		mRenderer.prepare(models->at<kpi::IDocumentNode>(0), host);
	}
	void draw_() override
	{

		auto bounds = ImGui::GetWindowSize();

		if (mViewport.begin(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y)))
		{
			auto* parent = dynamic_cast<EditorWindow*>(mParent);
			static bool showCursor = false; // TODO
			mRenderer.render(static_cast<u32>(bounds.x), static_cast<u32>(bounds.y), showCursor);

#ifdef RII_BACKEND_GLFW
			if (mpGlfwWindow != nullptr)
				glfwSetInputMode(mpGlfwWindow, GLFW_CURSOR, showCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
#endif
			mViewport.end();
		}
	}
	Viewport mViewport;
	Renderer mRenderer;
	const kpi::IDocumentNode& mHost;
};

struct HistoryList : public StudioWindow
{
	HistoryList(kpi::History& host, kpi::IDocumentNode& root)
		: StudioWindow("History"), mHost(host), mRoot(root)
	{
	}
	void draw_() override
	{
		if (ImGui::Button("Commit " ICON_FA_SAVE)) {
			mHost.commit(mRoot);
		}
		ImGui::SameLine();
		if (ImGui::Button("Undo " ICON_FA_UNDO)) {
			mHost.undo(mRoot);
		}
		ImGui::SameLine();
		if (ImGui::Button("Redo " ICON_FA_REDO)) {
			mHost.redo(mRoot);
		}
		ImGui::BeginChild("Record List");
		for (std::size_t i = 0; i < mHost.size(); ++i) {
			ImGui::Text("(%s) History #%u", i == mHost.cursor() ? "X" : " ", static_cast<u32>(i));
		}
		ImGui::EndChild();
	}

	kpi::History& mHost;
	kpi::IDocumentNode& mRoot;
};

struct CullMode
{
	bool front, back;

	CullMode()
		: front(true), back(false)
	{}
	CullMode(bool f, bool b)
		: front(f), back(b)
	{}
	CullMode(libcube::gx::CullMode c)
	{
		set(c);
	}

	void set(libcube::gx::CullMode c) noexcept
	{
		switch (c)
		{
		case libcube::gx::CullMode::All:
			front = back = false;
			break;
		case libcube::gx::CullMode::None:
			front = back = true;
			break;
		case libcube::gx::CullMode::Front:
			front = false;
			back = true;
			break;
		case libcube::gx::CullMode::Back:
			front = true;
			back = false;
			break;
		}
	}
	libcube::gx::CullMode get() const noexcept
	{
		if (front && back)
			return libcube::gx::CullMode::None;

		if (!front && !back)
			return libcube::gx::CullMode::All;

		return front ? libcube::gx::CullMode::Back : libcube::gx::CullMode::Front;
	}

	void draw()
	{
		ImGui::Text("Show sides of faces:");
		ImGui::Checkbox("Front", &front);
		ImGui::Checkbox("Back", &back);
	}
};
struct MatEditor : public StudioWindow
{
	MatEditor(kpi::History& history, kpi::IDocumentNode& root)
		: StudioWindow("MatEditor"), mHistory(history), mRoot(root)
	{
		auto& mdls = *root.getFolder<lib3d::Model>();
		auto& mdl = mdls[0];
		mMats = mdl->getFolder<libcube::IGCMaterial>();
		mImgs = root.getFolder<lib3d::Texture>();
	}

	enum class Tabs {
		DisplaySurface,
		Colors,
		Texture,
		SwapTable,
		Stage,
		Fog,
		PE,

		_Num
	};

	struct TabNames {
		static const char* get(Tabs tab) {
			switch (tab) {
				case Tabs::DisplaySurface:	 return ICON_FA_GHOST " Surface Visibility";
				case Tabs::Colors:			 return ICON_FA_PAINT_BRUSH " Colors";
				case Tabs::Texture:			 return ICON_FA_IMAGES " Textures";
				case Tabs::SwapTable:		 return ICON_FA_SWATCHBOOK " Swap Tables";
				case Tabs::Stage:			 return ICON_FA_NETWORK_WIRED " Stage";
				case Tabs::Fog:				 return ICON_FA_GHOST " Fog";
				case Tabs::PE:				 return ICON_FA_GHOST " PE";
				default:					 return "?";
			}
		}
	};

	template<void(*PredFunc)(void)>
	struct Predicate
	{
		~Predicate() { PredFunc(); }
	};
	template<typename ETabs, typename _TabNames>
	class TabGui {
	public:
		TabGui() = default;
		~TabGui() = default;

		template<typename T>
		void forEachTab(T action)
		{
			for (int i = 0; i < static_cast<int>(ETabs::_Num); ++i) {
				action(static_cast<ETabs>(i));
			}
		}

		bool drawLeft() {
#if 0
			Predicate<ImGui::EndChild> g;

			if (!ImGui::BeginChild("left pane", ImVec2(150, 0), true))
				return false;

			forEachTab([&](ETabs tab) {
				if (ImGui::Selectable(_TabNames::get(tab), mCurrent == tab))
					mCurrent = tab;
			});
#else
			if (ImGui::BeginTabBar("Pane")) {
				forEachTab([&](ETabs tab) {
					bool sel = mCurrent == tab;
					if (ImGui::BeginTabItem(_TabNames::get(tab))) {
						mCurrent = tab;
						ImGui::EndTabItem();
					}
				});
				ImGui::EndTabBar();
			}
#endif
			return true;
		}

		ETabs getTab() const { return mCurrent; }
		

	private:
		ETabs mCurrent = static_cast<ETabs>(0);
	};

	TabGui<Tabs, TabNames> mTabGui;

	void draw_() override
	{

		struct ConditionalActive {
			ConditionalActive(bool pred) : bPred(pred) {
				if (!bPred) {
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				}
			}
			~ConditionalActive() {
				if (!bPred) {
					ImGui::PopItemFlag();
					ImGui::PopStyleVar();
				}
			}
			bool bPred = false;
		};
		if (mMats == nullptr) {
			return;
		}
		auto& matfolders = *mMats;

		std::vector<libcube::IGCMaterial*> mats;
		for (std::size_t i = 0; i < mMats->size(); ++i) {
			if (mMats->isSelected(i)) mats.push_back(&mMats->at<libcube::IGCMaterial>(i));
		}
		if (mats.empty()) {
			ImGui::Text("No material is selected");
			return;
		}
		auto* active = dynamic_cast<libcube::IGCMaterial*>(matfolders[mMats->getActiveSelection()].get());
		ImGui::Text("%s %s (%u)", active->getName().c_str(), mats.size() > 1 ? "..." : "", static_cast<u32>(mats.size()));
		mTabGui.drawLeft();
		// ImGui::SameLine();
		ImGui::BeginGroup();
		{
			auto property = [&](auto compare_fn, auto set_fn, auto ref) {
				if (!compare_fn(ref)) {
					for (auto* mat : mats) {
						set_fn(mat, ref);
					}
					mHistory.commit(mRoot);
				}
			};
#define AUTO_PROP(type, ref) \
	property([&](auto m) { return m == active->getMaterialData().type; }, \
			 [&](libcube::IGCMaterial* mat, auto m) { mat->getMaterialData().type = m; }, \
			 ref);


			auto& matData = active->getMaterialData();

			switch (mTabGui.getTab()) {
			case Tabs::DisplaySurface:
			{
				
				// if (before != static_cast<int>(active->getMaterialData().cullMode)) {
				// 	for (auto* mat : mats) {
				// 		mat->getMaterialData().cullMode = static_cast<libcube::gx::CullMode>(before);
				// 	}
				// 	mHistory.commit(mRoot);
				// }

				bool oldDisplaySurfaceUi = false;

				if (oldDisplaySurfaceUi)
				{
					int before = static_cast<int>(active->getMaterialData().cullMode);
					ImGui::Combo("Cull mode", &before, "None\0Front\0Back\0All");
					property([&](libcube::gx::CullMode m) { return m == active->getMaterialData().cullMode; },
						[&](libcube::IGCMaterial* mat, libcube::gx::CullMode m) { mat->getMaterialData().cullMode = m; },
						static_cast<libcube::gx::CullMode>(before));
				}
				else
				{
					int before = static_cast<int>(active->getMaterialData().cullMode);
					CullMode cm(active->getMaterialData().cullMode);

					cm.draw();

					before = static_cast<int>(cm.get());
					AUTO_PROP(cullMode, static_cast<libcube::gx::CullMode>(before));
				}
			}
				break;
			case Tabs::Texture:
			{
				if (ImGui::BeginTabBar("Textures")) {
					for (std::size_t i = 0; i < matData.texGens.nElements; ++i) {
						auto& tg = matData.texGens[i];
						auto& tm = matData.texMatrices[i]; // TODO: Proper lookup
						auto& samp = matData.samplers[i];

						if (ImGui::BeginTabItem((std::string("Texture ") + std::to_string(i)).c_str())) {
							if (ImGui::CollapsingHeader("Image", ImGuiTreeNodeFlags_DefaultOpen)) {
								if (ImGui::BeginCombo("Name", samp->mTexture.c_str())) {
									for (const auto& tex : *mImgs) {
										bool selected = tex->getName() == samp->mTexture;
										if (ImGui::Selectable(tex->getName().c_str(), selected)) {
											AUTO_PROP(samplers[i]->mTexture, tex->getName());
										}
										if (selected) ImGui::SetItemDefaultFocus();
									}

									ImGui::EndCombo();
								}

								lib3d::Texture* curImg = nullptr;

								for (std::size_t j = 0; j < mImgs->size(); ++j) {
									auto& it = mImgs->at<lib3d::Texture>(j);
									if (it.getName() == samp->mTexture) {
										curImg = &it;
									}
								}

								if (curImg == nullptr) {
									ImGui::Text("No valid image.");
								} else {
									if (mLastImg != curImg->getName()) {
										mImg.setFromImage(*curImg);
										mLastImg = curImg->getName();
									}
									mImg.draw(128.0f * (static_cast<f32>(curImg->getWidth()) / static_cast<f32>(curImg->getHeight())), 128.0f);
								}
							}
							if (ImGui::CollapsingHeader("Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
								if (ImGui::BeginTabBar("Mapping")) {
									if (ImGui::BeginTabItem("Standard")) {
										ImGui::EndTabItem();
									}
									if (ImGui::BeginTabItem("Advanced")) {
										if (ImGui::CollapsingHeader("Texture Coordinate Generator", ImGuiTreeNodeFlags_DefaultOpen)) {
											int basefunc = 0;
											int mtxtype = 0;
											int lightid = 0;

											switch (tg.func) {
											case libcube::gx::TexGenType::Matrix2x4:
												basefunc = 0;
												mtxtype = 0;
												break;
											case libcube::gx::TexGenType::Matrix3x4:
												basefunc = 0;
												mtxtype = 1;
												break;
											case libcube::gx::TexGenType::Bump0:
											case libcube::gx::TexGenType::Bump1:
											case libcube::gx::TexGenType::Bump2:
											case libcube::gx::TexGenType::Bump3:
											case libcube::gx::TexGenType::Bump4:
											case libcube::gx::TexGenType::Bump5:
											case libcube::gx::TexGenType::Bump6:
											case libcube::gx::TexGenType::Bump7:
												basefunc = 1;
												lightid = static_cast<int>(tg.func) - static_cast<int>(libcube::gx::TexGenType::Bump0);
												break;
											case libcube::gx::TexGenType::SRTG:
												basefunc = 2;
												break;
											}

											ImGui::Combo("Function", &basefunc, "Standard Texture Matrix\0Bump Mapping: Use vertex lighting calculation result.\0SRTG: Map R(ed) and G(reen) components of a color channel to U/V coordinates");
											{
												ConditionalActive g(basefunc == 0);
												ImGui::Combo("Matrix Size", &mtxtype, "UV Matrix: 2x4\0UVW Matrix: 3x4");
												bool identitymatrix = tg.matrix == libcube::gx::TexMatrix::Identity;
												int texmatrixid = 0;
												const int rawtgmatrix = static_cast<int>(tg.matrix);
												if (rawtgmatrix >= static_cast<int>(libcube::gx::TexMatrix::TexMatrix0) && rawtgmatrix <= static_cast<int>(libcube::gx::TexMatrix::TexMatrix7)) {
													texmatrixid = rawtgmatrix - static_cast<int>(libcube::gx::TexMatrix::TexMatrix0);
												}
												ImGui::Checkbox("Identity Matrix", &identitymatrix);
												ImGui::SameLine();
												{
													ConditionalActive g2(!identitymatrix);
													ImGui::SliderInt("Matrix ID", &texmatrixid, 0, 7);
												}
												libcube::gx::TexMatrix newtexmatrix = identitymatrix ? libcube::gx::TexMatrix::Identity :
													static_cast<libcube::gx::TexMatrix>(static_cast<int>(libcube::gx::TexMatrix::TexMatrix0) + texmatrixid);
												AUTO_PROP(texGens[i].matrix, newtexmatrix);
											}
											{
												ConditionalActive g(basefunc == 1);
												ImGui::SliderInt("Hardware light ID", &lightid, 0, 7);
											}

											libcube::gx::TexGenType newfunc = libcube::gx::TexGenType::Matrix2x4;
											switch (basefunc) {
											case 0:
												newfunc = mtxtype ? libcube::gx::TexGenType::Matrix3x4 : libcube::gx::TexGenType::Matrix2x4;
												break;
											case 1:
												newfunc = static_cast<libcube::gx::TexGenType>(static_cast<int>(libcube::gx::TexGenType::Bump0) + lightid);
												break;
											case 2:
												newfunc = libcube::gx::TexGenType::SRTG;
												break;
											}
											AUTO_PROP(texGens[i].func, newfunc);

											int src = static_cast<int>(tg.sourceParam);
											ImGui::Combo("Source data", &src, "Position\0Normal\0Binormal\0Tangent\0UV 0\0UV 1\0UV 2\0UV 3\0UV 4\0UV 5\0UV 6\0UV 7\0Bump UV0\0Bump UV1\0Bump UV2\0Bump UV3\0Bump UV4\0Bump UV5\0Bump UV6\0Color Channel 0\0Color Channel 1");
											AUTO_PROP(texGens[i].sourceParam, static_cast<libcube::gx::TexGenSrc>(src));	
										}
										if (ImGui::CollapsingHeader("Texture Coordinate Generator", ImGuiTreeNodeFlags_DefaultOpen)) {
											// TODO: Effect matrix
											int xfmodel = static_cast<int>(tm->transformModel);
											ImGui::Combo("Transform Model", &xfmodel, " Default\0 Maya\0 3DS Max\0 Softimage XSI");
											AUTO_PROP(texMatrices[i]->transformModel, static_cast<libcube::GCMaterialData::CommonTransformModel>(xfmodel));
											// TODO: Not all backends support all modes..
											int mapMethod = 0;
											switch (tm->method) {
											default:
											case libcube::GCMaterialData::CommonMappingMethod::Standard:
												mapMethod = 0;
												break;
											case libcube::GCMaterialData::CommonMappingMethod::EnvironmentMapping:
												mapMethod = 1;
												break;
											case libcube::GCMaterialData::CommonMappingMethod::ViewProjectionMapping:
												mapMethod = 2;
												break;
											case libcube::GCMaterialData::CommonMappingMethod::ProjectionMapping:
												mapMethod = 3;
												break;
											case libcube::GCMaterialData::CommonMappingMethod::EnvironmentLightMapping:
												mapMethod = 4;
												break;
											case libcube::GCMaterialData::CommonMappingMethod::EnvironmentSpecularMapping:
												mapMethod = 5;
												break;
											case libcube::GCMaterialData::CommonMappingMethod::ManualEnvironmentMapping:
												mapMethod = 6;
												break;
											}
											ImGui::Combo("Mapping method", &mapMethod, "Standard Mapping\0Environment Mapping\0View Projection Mapping\0Manual Projection Mapping\0Environment Light Mapping\0Environment Specular Mapping\0Manual Environment Mapping");
											libcube::GCMaterialData::CommonMappingMethod newMapMethod = libcube::GCMaterialData::CommonMappingMethod::Standard;
											using cmm = libcube::GCMaterialData::CommonMappingMethod;
											switch (mapMethod) {
											default:
											case 0:
												newMapMethod = cmm::Standard;
												break;
											case 1:
												newMapMethod = cmm::EnvironmentMapping;
												break;
											case 2:
												newMapMethod = cmm::ViewProjectionMapping;
												break;
											case 3:
												newMapMethod = cmm::ProjectionMapping;
												break;
											case 4:
												newMapMethod = cmm::EnvironmentLightMapping;
												break;
											case 5:
												newMapMethod = cmm::EnvironmentSpecularMapping;
												break;
											case 6:
												newMapMethod = cmm::ManualEnvironmentMapping;
												break;
											}
											AUTO_PROP(texMatrices[i]->method, newMapMethod);

											int mod = static_cast<int>(tm->option);
											ImGui::Combo("Option", &mod, "Standard\0J3D Basic: Don't remap into texture space (Keep -1..1 not 0...1)\0J3D Old: Keep translation column.");
											AUTO_PROP(texMatrices[i]->option, static_cast<libcube::GCMaterialData::CommonMappingOption>(mod));
										}
										ImGui::EndTabItem();
									}
									ImGui::EndTabBar();
								}
								if (ImGui::CollapsingHeader("Transformation", ImGuiTreeNodeFlags_DefaultOpen)) {
									auto s = tm->scale;
									auto r = tm->rotate;
									auto t = tm->translate;
									ImGui::DragFloat2("Scale", &s.x);
									ImGui::DragFloat("Rotate", &r);
									ImGui::DragFloat2("Translate", &t.x);
									AUTO_PROP(texMatrices[i]->scale, s);
									AUTO_PROP(texMatrices[i]->rotate, r);
									AUTO_PROP(texMatrices[i]->translate, t);
								}
							}
							if (ImGui::CollapsingHeader("Tiling", ImGuiTreeNodeFlags_DefaultOpen)) {
								int sTile = static_cast<int>(samp->mWrapU);
								ImGui::Combo("U tiling", &sTile, "Clamp\0Repeat\0Mirror");
								int tTile = static_cast<int>(samp->mWrapV);
								ImGui::Combo("V tiling", &tTile, "Clamp\0Repeat\0Mirror");
								AUTO_PROP(samplers[i]->mWrapU, static_cast<libcube::gx::TextureWrapMode>(sTile));
								AUTO_PROP(samplers[i]->mWrapV, static_cast<libcube::gx::TextureWrapMode>(tTile));
							}
							if (ImGui::CollapsingHeader("Filtering", ImGuiTreeNodeFlags_DefaultOpen)) {
								int magBase = static_cast<int>(samp->mMagFilter);

								int minBase = 0;
								int minMipBase = 0;
								bool mip = false;

								switch (samp->mMinFilter) {
								case libcube::gx::TextureFilter::near_mip_near:
									mip = true;
								case libcube::gx::TextureFilter::near:
									minBase = minMipBase = static_cast<int>(libcube::gx::TextureFilter::near);
									break;
								case libcube::gx::TextureFilter::lin_mip_lin:
									mip = true;
								case libcube::gx::TextureFilter::linear:
									minBase = minMipBase = static_cast<int>(libcube::gx::TextureFilter::linear);
									break;
								case libcube::gx::TextureFilter::near_mip_lin:
									mip = true;
									minBase = static_cast<int>(libcube::gx::TextureFilter::near);
									minMipBase = static_cast<int>(libcube::gx::TextureFilter::linear);
									break;
								case libcube::gx::TextureFilter::lin_mip_near:
									mip = true;
									minBase = static_cast<int>(libcube::gx::TextureFilter::linear);
									minMipBase = static_cast<int>(libcube::gx::TextureFilter::near);
									break;
								}

								const char* linNear = "Nearest (no interpolation/pixelated)\0Linear (interpolated/blurry)";

								ImGui::Combo("Interpolation when scaled up", &magBase, linNear);
								AUTO_PROP(samplers[i]->mMagFilter, static_cast<libcube::gx::TextureFilter>(magBase));
								ImGui::Combo("Interpolation when scaled down", &minBase, linNear);

								ImGui::Checkbox("Use mipmap", &mip);

								{
									struct ConditionalActive g(mip);

									if (ImGui::CollapsingHeader("Mipmapping", ImGuiTreeNodeFlags_DefaultOpen)) {
										ImGui::Combo("Interpolation type", &minMipBase, linNear);

										bool mipBiasClamp = samp->bBiasClamp;
										ImGui::Checkbox("Bias clamp", &mipBiasClamp);
										AUTO_PROP(samplers[i]->bBiasClamp, mipBiasClamp);

										bool edgelod = samp->bEdgeLod;
										ImGui::Checkbox("Edge LOD", &edgelod);
										AUTO_PROP(samplers[i]->bEdgeLod, edgelod);

										float lodbias = samp->mLodBias;
										ImGui::SliderFloat("LOD bias", &lodbias, -4.0f, 3.99f);
										AUTO_PROP(samplers[i]->mLodBias, lodbias);

										int maxaniso = 0;
										switch (samp->mMaxAniso) {
										case libcube::gx::AnisotropyLevel::x1:
											maxaniso = 1;
											break;
										case libcube::gx::AnisotropyLevel::x2:
											maxaniso = 2;
											break;
										case libcube::gx::AnisotropyLevel::x4:
											maxaniso = 4;
											break;
										}
										ImGui::SliderInt("Anisotropic filtering level", &maxaniso, 1, 4);
										libcube::gx::AnisotropyLevel alvl = libcube::gx::AnisotropyLevel::x1;
										switch (maxaniso) {
										case 1:
											alvl = libcube::gx::AnisotropyLevel::x1;
											break;
										case 2:
											alvl = libcube::gx::AnisotropyLevel::x2;
											break;
										case 3:
										case 4:
											alvl = libcube::gx::AnisotropyLevel::x4;
											break;
										}
										AUTO_PROP(samplers[i]->mMaxAniso, alvl);
									}
								}

								libcube::gx::TextureFilter computedMin = libcube::gx::TextureFilter::near;
								if (!mip) {
									computedMin = static_cast<libcube::gx::TextureFilter>(minBase);
								} else {
									bool baseLin = static_cast<libcube::gx::TextureFilter>(minBase) == libcube::gx::TextureFilter::linear;
									if (static_cast<libcube::gx::TextureFilter>(minMipBase) == libcube::gx::TextureFilter::linear) {
										computedMin = baseLin ? libcube::gx::TextureFilter::lin_mip_lin : libcube::gx::TextureFilter::near_mip_lin;
									} else {
										computedMin = baseLin ? libcube::gx::TextureFilter::lin_mip_near : libcube::gx::TextureFilter::near_mip_near;
									}
								}

								AUTO_PROP(samplers[i]->mMinFilter, computedMin);
							}
							ImGui::EndTabItem();
						}
					}
					ImGui::EndTabBar();
				}
			}
				break;
			case Tabs::SwapTable:
			{
				ImGui::BeginColumns("swap", 4);
				int sel = 0;
				for (int j = 0; j < 4; ++j) {
					ImGui::Combo("R", &sel, "R\0G\0\0B\0A");
					ImGui::NextColumn();
					ImGui::Combo("G", &sel, "R\0G\0\0B\0A");
					ImGui::NextColumn();
					ImGui::Combo("B", &sel, "R\0G\0\0B\0A");
					ImGui::NextColumn();
					ImGui::Combo("A", &sel, "R\0G\0\0B\0A");
					ImGui::NextColumn();
				}
				ImGui::EndColumns();
			}
				break;
			case Tabs::Colors:
			{
				//if (ImGui::BeginTabBar("Type")) {
				//	if (ImGui::BeginTabItem("TEV Color Registers")) {
						libcube::gx::ColorF32 clr;


						if (ImGui::CollapsingHeader("TEV Color Registers", ImGuiTreeNodeFlags_DefaultOpen)) {

							// TODO: Is CPREV default state accessible?
							for (std::size_t i = 0; i < 4; ++i)
							{
								clr = matData.tevColors[i];
								ImGui::ColorEdit4((std::string("Color Register ") + std::to_string(i)).c_str(), clr);
								clr.clamp(-1024.0f / 255.0f, 1023.0f / 255.0f);
								AUTO_PROP(tevColors[i], static_cast<libcube::gx::ColorS10>(clr));
							}
						}

						if (ImGui::CollapsingHeader("TEV Constant Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
							for (std::size_t i = 0; i < 4; ++i)
							{
								clr = matData.tevKonstColors[i];
								ImGui::ColorEdit4((std::string("Constant Register ") + std::to_string(i)).c_str(), clr);
								clr.clamp(0.0f, 1.0f);
								AUTO_PROP(tevKonstColors[i], static_cast<libcube::gx::Color>(clr));
							}
						}

				//		ImGui::EndTabItem();
				//	}
				//	if (ImGui::BeginTabItem("TEV Constant Colors")) {
				//		ImGui::EndTabItem();
				//	}
				//	ImGui::EndTabBar();
				//	}
			}
				break;
			case Tabs::Stage:
			{
				if (ImGui::BeginTabBar("Stages")) {
					for (std::size_t i = 0; i < matData.shader.mStages.size(); ++i) {
						auto& stage = matData.shader.mStages[i];

						if (ImGui::BeginTabItem((std::string("Stage ") + std::to_string(i)).c_str())) {
							if (ImGui::CollapsingHeader("Stage Setting", ImGuiTreeNodeFlags_DefaultOpen)) {
								// RasColor
								// TODO: Better selection here
								int texid = stage.texMap;
								ImGui::InputInt("TexId", &texid);
								AUTO_PROP(shader.mStages[i].texMap, texid);
								AUTO_PROP(shader.mStages[i].texCoord, texid);

								if (stage.texCoord != stage.texMap) {
									ImGui::Text("TODO: TexCoord != TexMap: Not valid");
								}
								if (stage.texCoord >= matData.texGens.size()) {
									ImGui::Text("No valid image.");
								}
								else {
									lib3d::Texture* curImg = nullptr;

									for (std::size_t j = 0; j < mImgs->size(); ++j) {
										auto& it = mImgs->at<lib3d::Texture>(j);
										if (it.getName() == matData.samplers[stage.texMap]->mTexture) {
											curImg = &it;
										}
									}
									if (matData.samplers[stage.texCoord]->mTexture != mLastImg) {
										mImg.setFromImage(*curImg);
										mLastImg = curImg->getName();
									}
									mImg.draw(128.0f * (static_cast<f32>(curImg->getWidth()) / static_cast<f32>(curImg->getHeight())), 128.0f);
								}
							}
							if (ImGui::CollapsingHeader("Color Stage", ImGuiTreeNodeFlags_DefaultOpen)) {
								// TODO: Only add for now..
								if (stage.colorStage.formula == libcube::gx::TevColorOp::add) {
									ImGui::PushItemWidth(200);
									ImGui::Text("[");
									ImGui::SameLine();
									int a = static_cast<int>(stage.colorStage.a);
									int b = static_cast<int>(stage.colorStage.b);
									int c = static_cast<int>(stage.colorStage.c);
									int d = static_cast<int>(stage.colorStage.d);
									bool clamp = stage.colorStage.clamp;
									int dst = static_cast<int>(stage.colorStage.out);
									const char* colorOpt = "Register 3 Color\0Register 3 Alpha\0Register 0 Color\0Register 0 Alpha\0Register 1 Color\0Register 1 Alpha\0Register 2 Color\0Register 2 Alpha\0Texture Color\0Texture Alpha\0Raster Color\0Raster Alpha\0 1.0\0 0.5\0 Constant Color Selection\0 0.0";
									ImGui::Combo("##D", &d, colorOpt);
									ImGui::SameLine();
									ImGui::Text("{(1 - ");
									{
										ConditionalActive g(false);
										ImGui::SameLine();
										ImGui::Combo("##C_", &c, colorOpt);
									}
									ImGui::SameLine();
									ImGui::Text(") * ");
									ImGui::SameLine();
									ImGui::Combo("##A", &a, colorOpt);
									ImGui::SameLine();
									ImGui::Text(" + ");
									ImGui::SameLine();
									ImGui::Combo("##C", &c, colorOpt);
									ImGui::SameLine();
									ImGui::Text(" * ");
									ImGui::SameLine();
									ImGui::Combo("##B", &b, colorOpt);
									ImGui::SameLine();
									ImGui::Text(" } ");
									//ImGui::SameLine();
									//ImGui::Combo("##Bias", &bias, "+ 0.0\0+")

									ImGui::Checkbox("Clamp calculation to 0-255", &clamp);
									ImGui::Combo("Calculation Result Output Destionation", &dst, "Register 3\0Register 0\0Register 1\0Register 2");
									ImGui::PopItemWidth();
								}
							}
							ImGui::EndTabItem();
						}
					}
					ImGui::EndTabBar();
				}
			}
				break;
			case Tabs::Fog:
				break;
			case Tabs::PE: {
				if (ImGui::CollapsingHeader("Alpha Comparison", ImGuiTreeNodeFlags_DefaultOpen)) {
					const char* compStr = "Always do not pass.\0<\0==\0<=\0>\0!=\0>=\0Always pass.";
					ImGui::PushItemWidth(100);

					{
						ImGui::Text("( Pixel Alpha");
						ImGui::SameLine();
						int leftAlpha = static_cast<int>(active->getMaterialData().alphaCompare.compLeft);
						ImGui::Combo("##l", &leftAlpha, compStr);
						AUTO_PROP(alphaCompare.compLeft, static_cast<libcube::gx::Comparison>(leftAlpha));

						int leftRef = static_cast<int>(active->getMaterialData().alphaCompare.refLeft);
						ImGui::SameLine();
						ImGui::SliderInt("##lr", &leftRef, 0, 255);
						AUTO_PROP(alphaCompare.refLeft, leftRef);
						ImGui::SameLine();
						ImGui::Text(")");
					}
					{
						int op = static_cast<int>(active->getMaterialData().alphaCompare.op);
						ImGui::Combo("##o", &op, "&&\0||\0!=\0==\0");
						AUTO_PROP(alphaCompare.op, static_cast<libcube::gx::AlphaOp>(op));
					}
					{
						ImGui::Text("( Pixel Alpha");
						ImGui::SameLine();
						int rightAlpha = static_cast<int>(active->getMaterialData().alphaCompare.compRight);
						ImGui::Combo("##r", &rightAlpha, compStr);
						AUTO_PROP(alphaCompare.compRight, static_cast<libcube::gx::Comparison>(rightAlpha));
						
						int rightRef = static_cast<int>(active->getMaterialData().alphaCompare.refRight);
						ImGui::SameLine();
						ImGui::SliderInt("##rr", &rightRef, 0, 255);
						AUTO_PROP(alphaCompare.refRight, rightRef);

						ImGui::SameLine();
						ImGui::Text(")");
					}
					ImGui::PopItemWidth();
				}
				if (ImGui::CollapsingHeader("Z Buffer", ImGuiTreeNodeFlags_DefaultOpen)) {
					bool zcmp = active->getMaterialData().zMode.compare;
					ImGui::Checkbox("Compare Z Values", &zcmp);
					AUTO_PROP(zMode.compare, zcmp);

					{
						ConditionalActive g(zcmp);

						ImGui::Indent(30.0f);

						bool zearly = active->getMaterialData().earlyZComparison;
						ImGui::Checkbox("Compare Before Texture Processing", &zearly);
						AUTO_PROP(earlyZComparison, zearly);

						bool zwrite = active->getMaterialData().zMode.update;
						ImGui::Checkbox("Write to Z Buffer", &zwrite);
						AUTO_PROP(zMode.update, zwrite);

						int zcond = static_cast<int>(active->getMaterialData().zMode.function);
						ImGui::Combo("Condition", &zcond, "Never draw.\0Pixel Z < EFB Z\0Pixel Z == EFB Z\0Pixel Z <= EFB Z\0Pixel Z > EFB Z\0Pixel Z != EFB Z\0Pixel Z >= EFB Z\0 Always draw.");
						AUTO_PROP(zMode.function, static_cast<libcube::gx::Comparison>(zcond));

						ImGui::Unindent(30.0f);
					}
					
				}
				if (ImGui::CollapsingHeader("Blending", ImGuiTreeNodeFlags_DefaultOpen)) {
					ImGui::PushItemWidth(200);
					int btype = static_cast<int>(matData.blendMode.type);
					ImGui::Combo("Type", &btype, "Do not blend.\0Blending\0Logical Operations\0Subtract from Frame Buffer");
					AUTO_PROP(blendMode.type, static_cast<libcube::gx::BlendModeType>(btype));

					{

						ConditionalActive g(btype == static_cast<int>(libcube::gx::BlendModeType::blend));
						ImGui::Text("Blend calculation");

						const char* blendOpts = " 0\0 1\0 EFB Color\0 1 - EFB Color\0 Pixel Alpha\0 1 - Pixel Alpha\0 EFB Alpha\0 1 - EFB Alpha";
						const char* blendOptsDst = " 0\0 1\0 Pixel Color\0 1 - Pixel Color\0 Pixel Alpha\0 1 - Pixel Alpha\0 EFB Alpha\0 1 - EFB Alpha";
						ImGui::Text("( Pixel Color * ");

						int srcFact = static_cast<int>(matData.blendMode.source);
						ImGui::SameLine();
						ImGui::Combo("##Src", &srcFact, blendOpts);
						AUTO_PROP(blendMode.source, static_cast<libcube::gx::BlendModeFactor>(srcFact));

						ImGui::SameLine();
						ImGui::Text(") + ( EFB Color * ");

						int dstFact = static_cast<int>(matData.blendMode.dest);
						ImGui::SameLine();
						ImGui::Combo("##Dst", &dstFact, blendOptsDst);
						AUTO_PROP(blendMode.dest, static_cast<libcube::gx::BlendModeFactor>(dstFact));

						ImGui::SameLine();
						ImGui::Text(")");
					}
					{
						ConditionalActive g(btype == static_cast<int>(libcube::gx::BlendModeType::logic));
						ImGui::Text("Logical Operations");
					}
					ImGui::PopItemWidth();
				}
			}
				break;
			default:
				break;
			}
		}
		ImGui::EndGroup();
	}

	kpi::History& mHistory;
	kpi::IDocumentNode& mRoot;
	kpi::FolderData* mMats = nullptr;
	kpi::FolderData* mImgs = nullptr;


	ImagePreview mImg; // In mat sampler
	std::string mLastImg = "";
};

EditorWindow::EditorWindow(std::unique_ptr<kpi::IDocumentNode> state, const std::string& path)
	: StudioWindow(path.substr(path.rfind("\\") + 1), true), mState(std::move(state)), mFilePath(path)
{
	mHistory.commit(*mState.get());

	attachWindow(std::make_unique<MatEditor>(mHistory, *mState.get()));
	attachWindow(std::make_unique<HistoryList>(mHistory, *mState.get()));
	attachWindow(std::make_unique<GenericCollectionOutliner>(*mState.get()));
	attachWindow(std::make_unique<TexImgPreview>(*mState.get()));
	attachWindow(std::make_unique<RenderTest>(*mState.get()));
}
void EditorWindow::draw_()
{
#ifdef _WIN32
	if (ImGui::GetIO().KeyCtrl) {
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z))) {
			mHistory.undo(*mState.get());
		} else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Y))) {
			mHistory.redo(*mState.get());
		}
	}
#endif
	auto* parent = mParent;

	// if (!parent) return;

	
	//	if (!showCursor)
	//	{
	//		parent->hideMouse();
	//	}
	//	else
	//	{
	//		parent->showMouse();
	//	}
}
}
