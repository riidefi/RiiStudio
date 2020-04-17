-- FBX SDK linkage based on fbxc
FBX_SDK_ROOT = os.getenv("FBX_SDK_ROOT")
if not FBX_SDK_ROOT then
	printf("ERROR: Environment variable FBX_SDK_ROOT is not set.")
	printf("Set it to something like: C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2019.5\\lib\\vs2017")
	os.exit()
end
PYTHON_ROOT = "C:\\Python36"

workspace "RiiStudio"
	architecture "x64"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

function setupPreprocessor()
	filter "configurations:Debug"
		defines { "DEBUG", "BUILD_DEBUG" }
		symbols "On"
		buildoptions "/MDd"
	filter "configurations:Release"
		defines { "NDEBUG", "BUILD_RELEASE" }
		optimize "On"
		buildoptions "/MD"
	filter "configurations:Dist"
		defines { "NDEBUG", "BUILD_DIST" }
		optimize "On"
		buildoptions "/MD"
end

function setupSystem()
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"RII_PLATFORM_WINDOWS",
			"FBXSDK_SHARED",
			"RII_BACKEND_GLFW"
		}
end

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

function setupCppC()
	staticruntime "Off"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"source/%{prj.name}/**.h",
		"source/%{prj.name}/**.hpp",
		"source/%{prj.name}/**.cpp",
		"source/%{prj.name}/**.c"
	}
	excludes
	{
		"source/%{prj.name}/pybind11/**.cpp"
	}

	filter "files:*.cpp"
		language "C++"
	filter "files:*.c"
		language "C"
end

function setupStaticLib()
	kind "StaticLib"
	defines "_LIB"
end

function setupConsoleApp()
	kind "ConsoleApp"
end
function setupMainAppCli(set_links)
	includedirs
	{
		"./source",
		"./source/vendor/oishii",
		"./source/vendor",
		(FBX_SDK_ROOT .. "../../../include"),
		PYTHON_ROOT .. "./include",
		"./source/vendor/pybind11/include"
	}
	if set_links then
	links
	{
		"core",
		"vendor",
		"plugins",
		"libfbxsdk-md",
		"assimp-vc140-mt"
	}
	end


	setupConsoleApp()
	setupCppC()
	setupSystem()
	
	setupPreprocessor()

	configuration { "vs*", "Debug" }
		libdirs {
			(FBX_SDK_ROOT .. "/x64/debug"),
			PYTHON_ROOT .. "/libs/",
			"source/vendor/assimp"
		}
		
	configuration { "vs*", "Release" }
		libdirs {
			(FBX_SDK_ROOT .. "/x64/release"),
			PYTHON_ROOT .. "/libs/",
			"source/vendor/assimp"			
		}
	configuration { "vs*", "Dist" }
		libdirs {
			(FBX_SDK_ROOT .. "/x64/release"),
			PYTHON_ROOT .. "/libs/",
			"source/vendor/assimp"			
		}
	filter "configurations:Dist"
		postbuildcommands {
			"{COPY} %{cfg.targetdir}/*.exe ../dist/",
			"{COPY} %{cfg.targetdir}/*.dll ../dist/",
			"{COPY} %{cfg.targetdir}/*.ttf ../dist/fonts/"
			-- "{COPY} %{cfg.targetdir}/scripts  ../dist/scripts/"
		}
end

function setupMainApp()
	links
	{
		"core",
		"vendor",
		"plugins",
		"source/vendor/glfw/lib-vc2017/glfw3dll.lib",
		"opengl32.lib",
		"libfbxsdk.lib"
	}

	setupMainAppCli(false)
	
	fbx_dir = ""
	configuration { "vs*", "Debug" }
		fbx_dir = FBX_SDK_ROOT .. "/x64/debug"
	configuration { "vs*", "Release" }
		fbx_dir = FBX_SDK_ROOT .. "/x64/release"
	configuration { "vs*", "Dist" }
		fbx_dir = FBX_SDK_ROOT .. "/x64/release"

	postbuildcommands {
		"{COPY} ../vendor/glfw/lib-vc2017/glfw3.dll %{cfg.targetdir}",
		-- "{COPY} " .. fbx_dir .. "libfbxsdk.dll %{cfg.targetdir}",
		"{COPY} ../../fonts/* %{cfg.targetdir}",
		"{COPY} ./scripts/* %{cfg.targetdir}/scripts"
	}
end

project "frontend"
	location "source/frontend"
	setupMainApp()



project "vendor"
	location "source/vendor"

	includedirs
	{
		"source/vendor",
		"source/vendor/oishii"
	}
	setupStaticLib()
	setupCppC()


	setupSystem()
	setupPreprocessor()

project "core"
	location "source/core"
	includedirs {
		"./source",
		"source/vendor",
		"source/core",
		PYTHON_ROOT .. "./include",
		"source/vendor/pybind11/include"
	}

	setupStaticLib()
	setupCppC()
	setupSystem()
	setupPreprocessor()

project "plugins"
	location "source/plugins"
	includedirs {
		"./source",
		"source/vendor",
		"source/core",
		"source/vendor/oishii"
	}

	setupStaticLib()
	setupCppC()
	setupSystem()
	setupPreprocessor()
