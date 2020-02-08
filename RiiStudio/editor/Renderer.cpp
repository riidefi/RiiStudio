#include "Renderer.hpp"

#include <GL/gl3w.h>
#include "GLFW/glfw3.h"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui/imgui.h>
#include <array>

const char* vertexShaderSource = "#version 420\n"
"layout (location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 vertexColor;\n"
"out vec3 vcolor;\n"
"layout(std140, binding=0) uniform Matrices {\n"
"mat4 proj, view, mdl;\n"
"};\n"
"void main()\n"
"{\n"
"   gl_Position = proj * view * mdl * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"   vcolor = vertexColor;\n"
"}\0";
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 vcolor;"
"void main()\n"
"{\n"
"   FragColor = vec4(vcolor.x, vcolor.y, vcolor.z, 1.0f);\n"
"}\n\0";

void checkShaderErrors(u32 id)
{
	return;
	int success;
	char infoLog[512];
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(id, 512, NULL, infoLog);
		std::cout << "Shader compilation failed: " << infoLog << std::endl;
	}
}

u32 roundDown(u32 in, u32 align) {
	return align ? in & ~(align - 1) : in;
};
u32 roundUp(u32 in, u32 align) {
	return align ? roundDown(in + (align - 1), align) : in;
};

void Renderer::createShaderProgram()
{
	u32 vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	checkShaderErrors(vertexShader);

	u32 fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	checkShaderErrors(fragmentShader);

	mShaderProgram = glCreateProgram();
	glAttachShader(mShaderProgram, vertexShader);
	glAttachShader(mShaderProgram, fragmentShader);
	glLinkProgram(mShaderProgram);
	checkShaderErrors(mShaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	bShadersLoaded = true;
}
void Renderer::destroyShaders()
{
	if (!bShadersLoaded) return;

	glDeleteProgram(mShaderProgram);
	bShadersLoaded = false;
}


struct SceneTree
{
	struct Node
	{
		const lib3d::Material& mat;
		const lib3d::Polygon& poly;
		const lib3d::Bone& bone;
		u8 priority;

		// Only used later
		mutable u32 idx_ofs = 0;
		mutable u32 idx_size = 0;
		mutable u32 mtx_id = 0;

		bool isTranslucent() const
		{
			return mat.isXluPass();
		}

	};


	std::vector<Node> opaque, translucent;


	void gatherBoneRecursive(u64 boneId, const px::CollectionHost::CollectionHandle& bones,
		const px::CollectionHost::CollectionHandle& mats, const px::CollectionHost::CollectionHandle& polys)
	{
		const auto pBone = bones.at<lib3d::Bone>(boneId);
		if (!pBone) return;

		const u64 nDisplay = pBone->getNumDisplays();
		for (u64 i = 0; i < nDisplay; ++i)
		{
			const auto display = pBone->getDisplay(i);
			const Node node{ *mats.at<lib3d::Material>(display.matId),
				*polys.at<lib3d::Polygon>(display.polyId), *pBone, display.prio };

			if (node.isTranslucent())
				translucent.push_back(node);
			else
				opaque.push_back(node);
		}

		for (u64 i = 0; i < pBone->getNumChildren(); ++i)
			gatherBoneRecursive(pBone->getChild(i), bones, mats, polys);
	}

	void gather(const px::CollectionHost& root)
	{
		const auto pMats = root.getFolder<lib3d::Material>();
		if (!pMats.has_value())
			return;
		const auto pPolys = root.getFolder<lib3d::Polygon>();
		if (!pPolys.has_value())
			return;

		// We cannot proceed without bones, as we attach all render instructions to them.
		const auto pBones = root.getFolder<lib3d::Bone>();
		if (!pBones.has_value())
			return;

		// Assumes root at zero
		if (pBones->size() == 0)
			return;
		gatherBoneRecursive(0, pBones.value(), pMats.value(), pPolys.value());
	}

	// TODO: Z Sort
};


struct UBOBuilder
{
	UBOBuilder()
	{
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformStride);


		glGenBuffers(1, &UBO);
	}

	~UBOBuilder()
	{
		glDeleteBuffers(1, &UBO);
	}

	u32 roundUniformUp(u32 ofs) { return roundUp(ofs, uniformStride); }
	u32 roundUniformDown(u32 ofs) { return roundDown(ofs, uniformStride); }

	int getUniformAlignment() const { return uniformStride; }

private:
	int uniformStride = 0;
protected:
	u32 UBO;
};

struct MVPBuilder : public UBOBuilder
{
	// Layout in memory:
	// Contiguous M, V, P (concat these?)

	void clear() { mMatrices.clear(); }
	u32 push(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj)
	{
		mMatrices.push_back(model);
		mMatrices.push_back(view);
		mMatrices.push_back(proj);
		return (mMatrices.size() - 3) / 3;
	}
	void submit()
	{
		const u32 matrixStride = roundUniformUp(sizeof(glm::mat4));
		const u32 blocksize = matrixStride * mMatrices.size();
		void* block = alloca(blocksize);
		assert(block);

		for (int i = 0; i < mMatrices.size(); ++i)
			*reinterpret_cast<glm::mat4*>(reinterpret_cast<char*>(block) + matrixStride * i) = mMatrices[i];

		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
		glBufferData(GL_UNIFORM_BUFFER, blocksize, NULL, GL_STATIC_DRAW);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, blocksize, block);
	}
	void use(u32 idx)
	{
		const u32 matrixstride = roundUniformUp(sizeof(glm::mat4)) * 3;
		glBindBufferRange(GL_UNIFORM_BUFFER, 0, UBO, matrixstride * idx, matrixstride * 3);
	}

private:
	std::vector<glm::mat4> mMatrices;
};

struct SceneState
{
	std::vector<glm::vec3> mPositions;
	std::vector<glm::vec3> mColors;


	std::vector<u32> mIndices;

	MVPBuilder mMvpBuilder;
	SceneTree mTree;

	std::vector<lib3d::Polygon::SimpleVertex> vtxScratch;

	
	void buildBuffers()
	{
		mPositions.clear();
		mColors.clear();
		mIndices.clear();

		for (const auto& node : mTree.opaque)
		{
			lib3d::SRT3 srt = node.bone.getSRT();
			glm::mat4 mdl(1.0f);

			mdl = glm::scale(mdl, srt.scale);
			// mdl = glm::rotate(mdl, (glm::mat4)srt.rotation);
			mdl = glm::translate(mdl, srt.translation);


			vtxScratch.clear();
			node.poly.propogate(vtxScratch);
			node.idx_ofs = mIndices.size();
			u32 i = mIndices.size();
			for (const auto& v : vtxScratch)
			{
				mPositions.push_back(v.position);
				mColors.push_back(v.colors[0]);
				mIndices.push_back(i);
				++i;
			}
			node.idx_size = vtxScratch.size();
		}


		glBindVertexArray(VAO);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);


		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuf);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * 4, mIndices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, mPositionBuf);
		glBufferData(GL_ARRAY_BUFFER, mPositions.size() * 3 * 4, mPositions.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * 4, (void*)0);


		glBindBuffer(GL_ARRAY_BUFFER, mColorBuf);
		glBufferData(GL_ARRAY_BUFFER, mColors.size() * 3 * 4, mColors.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * 4, (void*)0);
	}

	void gather(const px::CollectionHost& root)
	{
		mTree.gather(root);
		buildBuffers();
	}
	void build(const glm::mat4& view, const glm::mat4& proj)
	{
		mMvpBuilder.clear();
		for (const auto& node : mTree.opaque)
		{
			lib3d::SRT3 srt = node.bone.getSRT();
			glm::mat4 mdl(1.0f);

			mdl = glm::scale(mdl, srt.scale);
			// mdl = glm::rotate(mdl, (glm::mat4)srt.rotation);
			mdl = glm::translate(mdl, srt.translation);

			node.mtx_id = mMvpBuilder.push(mdl, view, proj);
		}


		mMvpBuilder.submit();
	}

	void draw()
	{
		glClearColor(0.2f, 0.3f, 0.3f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		for (const auto& node : mTree.opaque)
		{
			mMvpBuilder.use(node.mtx_id);
			glDrawElements(GL_TRIANGLES, node.idx_size, GL_UNSIGNED_INT, (void*)(node.idx_ofs * 4));
		}
	}

	SceneState()
	{	
		glGenBuffers(1, &mPositionBuf);
		glGenBuffers(1, &mColorBuf);
		glGenBuffers(1, &mIndexBuf);

		glGenVertexArrays(1, &VAO);

		vtxScratch.resize(250);
	}
	~SceneState()
	{
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &mPositionBuf);
		glDeleteBuffers(1, &mColorBuf);
		glDeleteBuffers(1, &mIndexBuf);
	}

	u32 mPositionBuf, mColorBuf, mIndexBuf;
	u32 VAO;

};

void Renderer::prepare(const px::CollectionHost& root)
{
	mState->gather(root);
}

static void cb(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam)
{
	printf(message);
}
Renderer::Renderer()
{

	glDebugMessageCallback(cb, 0);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// glBindBufferRange(GL_UNIFORM_BUFFER, 0, UBO, 0, 3 * sizeof(glm::mat4));

	mState = std::make_unique<SceneState>();
}
Renderer::~Renderer() {}
void Renderer::render(u32 width, u32 height)
{
	if (!bShadersLoaded)
		createShaderProgram();
	ImGui::DragFloat("EyeX", &eye.x, .01f, -10, 30);
	ImGui::DragFloat("EyeY", &eye.y, .01f, -10, 30);
	ImGui::DragFloat("EyeZ", &eye.z, .01f, -10, 30);

	static bool rend = false;
	ImGui::Checkbox("Render", &rend);
	if (!rend) return;

	static bool wireframe = false;
	ImGui::Checkbox("Wireframe", &wireframe);
	if (wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// horizontal angle : toward -Z
	float horizontalAngle = 3.14f;
	// vertical angle : 0, look at the horizon
	float verticalAngle = 0.0f;
	// Initial Field of View
	float initialFoV = 45.0f;

	float speed = 3.0f; // 3 units / second
	float mouseSpeed = 0.005f;

	float deltaTime = .1f;

	const auto [xpos, ypos] = ImGui::GetMousePos();

	horizontalAngle += mouseSpeed * deltaTime * float(width - xpos);
	verticalAngle += mouseSpeed * deltaTime * float(height - ypos);
	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 direction(
		cos(verticalAngle) * sin(horizontalAngle),
		sin(verticalAngle),
		cos(verticalAngle) * cos(horizontalAngle)
	);
	glm::vec3 right = glm::vec3(
		sin(horizontalAngle - 3.14f / 2.0f),
		0,
		cos(horizontalAngle - 3.14f / 2.0f)
	);
	glm::vec3 up = glm::cross(right, direction);

	if (ImGui::IsKeyDown('W'))
		eye += direction * deltaTime * speed;
	if (ImGui::IsKeyDown('S'))
		eye -= direction * deltaTime * speed;
	if (ImGui::IsKeyDown('A'))
		eye -= right * deltaTime * speed;
	if (ImGui::IsKeyDown('D'))
		eye += right * deltaTime * speed;

	glm::mat4 projMtx = glm::perspective(glm::radians(45.0f), static_cast<f32>(width) / static_cast<f32>(height), 0.1f, 100.f);
	glm::mat4 viewMtx = glm::lookAt(
		eye,
		eye + direction,
		up
	);

	glUseProgram(mShaderProgram);

	mState->build(projMtx, viewMtx);
	mState->draw();
}
