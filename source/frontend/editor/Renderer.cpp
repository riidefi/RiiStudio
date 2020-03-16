#include "Renderer.hpp"

#include <core/3d/gl.hpp>

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui/imgui.h>
#include <array>

#include "renderer/ShaderProgram.hpp"
#include "renderer/ShaderCache.hpp"
#include "renderer/UBOBuilder.hpp"

#include <core/3d/aabb.hpp>


#ifndef _WIN32
#include <SDL.h>
#include <SDL_opengles2.h>

#endif

namespace riistudio::frontend {

struct SceneTree
{
	struct Node : public lib3d::IObserver
	{
		const lib3d::Material& mat;
		const lib3d::Polygon& poly;
		const lib3d::Bone& bone;
		u8 priority;

		ShaderProgram& shader;

		// Only used later
		mutable u32 idx_ofs = 0;
		mutable u32 idx_size = 0;
		mutable u32 mtx_id = 0;

		Node(const lib3d::Material& m, const lib3d::Polygon& p, const lib3d::Bone& b, u8 prio, ShaderProgram& prog)
			: mat(m), poly(p), bone(b), priority(prio), shader(prog)
		{}

		void update() override {
			const auto shader_sources = mat.generateShaders();
			shader = ShaderCache::compile(shader_sources.first, shader_sources.second);
		}

		bool isTranslucent() const
		{
			return mat.isXluPass();
		}

	};


	std::vector<std::unique_ptr<Node>> opaque, translucent;


	void gatherBoneRecursive(u64 boneId, const kpi::FolderData& bones,
		const kpi::FolderData& mats, const kpi::FolderData& polys)
	{
		const auto& pBone = bones.at<lib3d::Bone>(boneId);

		const u64 nDisplay = pBone.getNumDisplays();
		for (u64 i = 0; i < nDisplay; ++i)
		{
			const auto display = pBone.getDisplay(i);
			const auto& mat = mats.at<lib3d::Material>(display.matId);
			
			const auto shader_sources = mat.generateShaders();
			ShaderProgram& shader = ShaderCache::compile(shader_sources.first, shader_sources.second);
			const Node node { mats.at<lib3d::Material>(display.matId),
				polys.at<lib3d::Polygon>(display.polyId), pBone, display.prio, shader };

			auto& nodebuf = node.isTranslucent() ? translucent : opaque;

			nodebuf.push_back(std::make_unique<Node>(node));
			nodebuf.back()->mat.observers.push_back(nodebuf.back().get());
		}

		for (u64 i = 0; i < pBone.getNumChildren(); ++i)
			gatherBoneRecursive(pBone.getChild(i), bones, mats, polys);
	}

	void gather(const kpi::IDocumentNode& root)
	{
		const auto* pMats = root.getFolder<lib3d::Material>();
		if (pMats == nullptr)
			return;
		const auto* pPolys = root.getFolder<lib3d::Polygon>();
		if (pPolys == nullptr)
			return;

		// We cannot proceed without bones, as we attach all render instructions to them.
		const auto pBones = root.getFolder<lib3d::Bone>();
		if (pBones == nullptr)
			return;

		// Assumes root at zero
		if (pBones->size() == 0)
			return;
		gatherBoneRecursive(0, *pBones, *pMats, *pPolys);
	}

	// TODO: Z Sort
};


struct SceneState
{
	DelegatedUBOBuilder mUboBuilder;
	SceneTree mTree;

	~SceneState()
	{
		for (const auto& tex : mTextures)
			glDeleteTextures(1, &tex.id);
		mTextures.clear();
	}

	void buildBuffers()
	{

		// TODO -- nodes may be of incompatible type..
		for (const auto& node : mTree.opaque)
		{
			node->idx_ofs = static_cast<u32>(mVbo.mIndices.size());
			node->poly.propogate(mVbo);
			node->idx_size = static_cast<u32>(mVbo.mIndices.size()) - node->idx_ofs;
		}

		mVbo.build();
		glBindVertexArray(0);

	}

	std::map<std::string, u32> texIdMap;

	void buildTextures(const kpi::IDocumentNode& root)
	{
		for (const auto& tex : mTextures)
			glDeleteTextures(1, &tex.id);
		mTextures.clear();
		texIdMap.clear();

		const auto textures = root.getFolder<lib3d::Texture>();

		mTextures.resize(textures->size());
		std::vector<u8> data(1024 * 1024 * 4);
		for (int i = 0; i < textures->size(); ++i)
		{
			const auto& tex = textures->at<lib3d::Texture>(i);

			// TODO: Wrapping mode, filtering, mipmaps
			glGenTextures(1, &mTextures[i].id);
			const auto texId = mTextures[i].id;

			texIdMap[tex.getName()] = texId;
			glBindTexture(GL_TEXTURE_2D, texId);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			tex.decode(data, false);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.getWidth(), tex.getHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
			// TODO
			// glGenerateMipmap(GL_TEXTURE_2D);
		}
	}

	void gather(const kpi::IDocumentNode& model, const kpi::IDocumentNode& texture, bool buf=true, bool tex=true)
	{
		bones = model.getFolder<lib3d::Bone>();
		mTree.gather(model);
		if (buf) buildBuffers();
		if (tex) buildTextures(texture);

		//	const auto mats = root.getFolder<lib3d::Material>();
		//	const auto mat = mats->at<lib3d::Material>(0);
		//	const auto compiled = mat->generateShaders();
	}
	const kpi::FolderData* bones;
	glm::mat4 computeMdlMtx(const lib3d::SRT3& srt)
	{
		glm::mat4 mdl(1.0f);

		// TODO
		// mdl = glm::rotate(mdl, (glm::mat4)srt.rotation);
		mdl = glm::translate(mdl, srt.translation);
		mdl = glm::scale(mdl, srt.scale);

		return mdl;
	}
	glm::mat4 computeBoneMdl(u32 id)
	{
		glm::mat4 mdl(1.0f);

		auto& bone = bones->at<lib3d::Bone>(id);
		// const auto parent = bone->getParent();
		//if (parent >= 0 && parent != id)
		//	mdl = computeBoneMdl(parent);

		mdl = computeMdlMtx(bone.getSRT());
		return mdl;
	}

	glm::mat4 computeBoneMdl(const lib3d::Bone& bone)
	{
		glm::mat4 mdl(1.0f);

		//	const auto parent = bone.getParent();
		//	if (parent >= 0)
		//		mdl = computeBoneMdl(parent);

		mdl = computeMdlMtx(bone.getSRT());
		return mdl;
	}
	void build(const glm::mat4& view, const glm::mat4& proj, riistudio::core::AABB& bound)
	{
		// TODO
		bound.m_minBounds = { 0.0f, 0.0f, 0.0f };
		bound.m_maxBounds = { 0.0f, 0.0f, 0.0f };

		for (const auto& node : mTree.opaque)
		{
			auto mdl = computeBoneMdl(node->bone);

			auto nmax = mdl * glm::vec4(node->poly.getBounds().max, 0.0f);
			auto nmin = mdl * glm::vec4(node->poly.getBounds().min, 0.0f);

			riistudio::core::AABB newBound{ nmin, nmax };
			bound.expandBound(newBound);
		}

		// const f32 dist = glm::distance(bound.m_minBounds, bound.m_maxBounds);


		mUboBuilder.clear();
		u32 i = 0;
		for (const auto& node : mTree.opaque)
		{
			auto mdl = computeBoneMdl(node->bone);

			node->mat.generateUniforms(mUboBuilder, mdl, view, proj, node->shader.getId(), texIdMap);
			node->mtx_id = i;
			++i;
		}


		mUboBuilder.submit();
	}

	glm::mat4 scaleMatrix{ 1.0f };

	void draw()
	{
		glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		MegaState state;

		for (const auto& node : mTree.opaque)
		{
			node->mat.setMegaState(state);
			glEnable(GL_BLEND);
			glBlendFunc(state.blendSrcFactor, state.blendDstFactor);
			glBlendEquation(state.blendMode);
			if (state.cullMode == -1)
			{
				glDisable(GL_CULL_FACE);
			}
			else
			{
				glEnable(GL_CULL_FACE);
				glCullFace(state.cullMode);
			}
			glFrontFace(state.frontFace);
			// TODO: depthWrite, depthCompare
			assert(mVbo.VAO && node->idx_size >= 0 && node->idx_size % 3 == 0);
			glUseProgram(node->shader.getId());
			mUboBuilder.use(node->mtx_id);
			glBindVertexArray(mVbo.VAO);

			node->mat.genSamplUniforms(node->shader.getId(), texIdMap);
			// printf("Draw: index (size=%u, ofs=%u)\n", node->idx_size, node->idx_ofs * 4);
			glDrawElements(GL_TRIANGLES, node->idx_size, GL_UNSIGNED_INT, (void*)(node->idx_ofs * 4));
			// if (glGetError() != GL_NO_ERROR) exit(1);
		}

		glBindVertexArray(0);
	}

	VBOBuilder mVbo;
	struct Texture
	{
		u32 id;
		// std::vector<u8> data;
		//	u32 width;
		//	u32 height;
	};
	std::vector<Texture> mTextures;
};

void Renderer::prepare(const kpi::IDocumentNode& model, const kpi::IDocumentNode& texture, bool buf, bool tex)
{
	mState->gather(model, texture, buf, tex);
}

static void cb(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	printf("%s\n", message);
}
Renderer::Renderer()
{
#ifdef _WIN32
	glDebugMessageCallback(cb, 0);
#endif

	mState = std::make_unique<SceneState>();
}
Renderer::~Renderer() {}

void Renderer::render(u32 width, u32 height, bool& showCursor)
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Camera"))
		{
			ImGui::SliderFloat("Mouse Speed", &mouseSpeed, 0.0f, .2f);

			ImGui::SliderFloat("Camera Speed", &speed, 1.0f, 10000.f);
			ImGui::SliderFloat("FOV", &fov, 1.0f, 180.0f);

			ImGui::InputFloat("Near Plane", &cmin);
			ImGui::InputFloat("Far Plane", &cmax);

			ImGui::DragFloat("X", &eye.x, .01f, -10, 30);
			ImGui::DragFloat("Y", &eye.y, .01f, -10, 30);
			ImGui::DragFloat("Z", &eye.z, .01f, -10, 30);

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Rendering"))
		{
			ImGui::Checkbox("Render", &rend);
#ifdef _WIN32
			ImGui::Checkbox("Wireframe", &wireframe);
#endif
			ImGui::EndMenu();
		}
		static int combo_choice = 0;
		ImGui::Combo("Shading", &combo_choice, "Emulated Shaders\0Normals\0TODO");

		ImGui::Combo("Controls", &combo_choice_cam, "WASD // FPS\0WASD // Plane\0");

#ifdef _WIN32
		ImGui::Checkbox("Wireframe", &wireframe);
#endif
		ImGui::EndMenuBar();
	}

	// horizontal angle : toward -Z
	float horizontalAngle = 3.14f;
	// vertical angle : 0, look at the horizon
	float verticalAngle = 0.0f;

	// TODO: Enable for plane mode
	// glm::vec3 up = glm::cross(right, direction);
	glm::vec3 up = { 0, 1, 0 };

	bool mouseDown = ImGui::IsAnyMouseDown();
	bool ctrlPress = ImGui::IsKeyPressed(341); // GLFW_KEY_LEFT_CONTROL


	if (ImGui::IsWindowFocused())
	{
		if (ctrlPress)
		{
			if (inCtrl)
			{
				inCtrl = false;
				showCursor = true;
			}
			else
			{
				inCtrl = true;
				showCursor = false;
			}
		}
		else if (!inCtrl)
		{
			showCursor = !mouseDown;
		}
		float deltaTime = 1.0f / ImGui::GetIO().Framerate;

		static float xpos = 0;
		static float ypos = 0;

		if (!showCursor)
		{
			auto pos = ImGui::GetMousePos();

			xpos = pos.x;
			ypos = pos.y;
		}
		horizontalAngle += mouseSpeed * deltaTime * float(width - xpos);
		verticalAngle += mouseSpeed * deltaTime * float(height - ypos);

		// Direction : Spherical coordinates to Cartesian coordinates conversion
		direction = glm::vec3(
			cos(verticalAngle) * sin(horizontalAngle),
			sin(verticalAngle),
			cos(verticalAngle) * cos(horizontalAngle)
		);
		glm::vec3 mvmt_dir{ direction.x, combo_choice_cam == 0 ? 0.0f : direction.y, direction.z };
		if (combo_choice_cam == 0)
			mvmt_dir = glm::normalize(mvmt_dir);
		glm::vec3 right = glm::vec3(
			sin(horizontalAngle - 3.14f / 2.0f),
			0,
			cos(horizontalAngle - 3.14f / 2.0f)
		);

#ifdef RII_BACKEND_GLFW
		if (ImGui::IsKeyDown('W'))
#else
		const Uint8* keys = SDL_GetKeyboardState(NULL);

		if (keys[SDL_SCANCODE_W])
#endif
			eye += mvmt_dir * deltaTime * speed;
#ifdef RII_BACKEND_GLFW
		if (ImGui::IsKeyDown('S'))
#else
		if (keys[SDL_SCANCODE_S])
#endif
			eye -= mvmt_dir * deltaTime * speed;
#ifdef RII_BACKEND_GLFW
		if (ImGui::IsKeyDown('A'))
#else
		if (keys[SDL_SCANCODE_A])
#endif
			eye -= right * deltaTime * speed;
#ifdef RII_BACKEND_GLFW
		if (ImGui::IsKeyDown('D'))
#else
		if (keys[SDL_SCANCODE_D])
#endif
			eye += right * deltaTime * speed;

		if ((ImGui::IsKeyDown(' ') && combo_choice_cam == 0) || ImGui::IsKeyDown('E'))
			eye += up * deltaTime * speed;
		if ((ImGui::IsKeyDown(340) && combo_choice_cam == 0) || ImGui::IsKeyDown('Q')) // GLFW_KEY_LEFT_SHIFT
			eye -= up * deltaTime * speed;
	}
	else // if (inCtrl)
	{
		inCtrl = false;
		showCursor = true;
	}


	if (!rend) return;

#ifdef _WIN32
	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif


	glm::mat4 projMtx = glm::perspective(glm::radians(fov),
		static_cast<f32>(width) / static_cast<f32>(height), cmin, cmax);
	glm::mat4 viewMtx = glm::lookAt(
		eye,
		eye + direction,
		up
	);

	riistudio::core::AABB bound;

	mState->build(projMtx, viewMtx, bound);


	const f32 dist = glm::distance(bound.m_minBounds, bound.m_maxBounds);
	if (speed == 0.0f)
		speed = dist / 10.0f;
	//cmin = std::min(std::min(bound.m_minBounds.x, bound.m_minBounds.y), bound.m_minBounds.z) * 100;
	//cmax = std::max(std::max(bound.m_maxBounds.x, bound.m_maxBounds.y), bound.m_maxBounds.z) * 100;
	if (eye == glm::vec3{ 0.0f })
	{
		const auto min = bound.m_minBounds;
		const auto max = bound.m_maxBounds;

		eye.x = (min.x + max.x) / 2.0f;
		eye.y = (min.y + max.y) / 2.0f;
		eye.z = (min.z + max.z) / 2.0f;
	}

	mState->draw();
}

}
