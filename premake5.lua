-- FBX SDK linkage based on fbxc
FBX_SDK_ROOT = os.getenv("FBX_SDK_ROOT")
if not FBX_SDK_ROOT then
	printf("ERROR: Environment variable FBX_SDK_ROOT is not set.")
	printf("Set it to something like: C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2019.5\\lib\\vs2017")
	os.exit()
end

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
		defines "DEBUG"
		symbols "On"
	
	filter "configurations:Release"
		defines "NDEBUG"
		optimize "On"
	
	filter "configurations:Dist"
		defines "NDEBUG"
		optimize "On"
end

function setupSystem()
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"RII_PLATFORM_WINDOWS"
		}
end

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

function setupCppC()
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/source/**.h",
		"%{prj.name}/source/**.hpp",
		"%{prj.name}/source/**.cpp",
		"%{prj.name}/source/**.c"
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
function setupMainApp()
	includedirs
	{
		"LibRiiEditor/source",
		"ThirdParty/source",
		"LibCube/source",
		"oishii",
		(FBX_SDK_ROOT .. "../../../include")
	}

	links
	{
		"LibRiiEditor",
		"ThirdParty",
		"LibCube",
		"ThirdParty/glfw/lib-vc2017/glfw3dll.lib",
		"opengl32.lib",
		"libfbxsdk-md"
	}


	setupConsoleApp()
	setupCppC()
	setupSystem()
	
	postbuildcommands {
		"{COPY} ../ThirdParty/glfw/lib-vc2017/glfw3.dll %{cfg.targetdir}"
	}


	setupPreprocessor()

	configuration { "vs*", "Debug" }
		libdirs {
			(FBX_SDK_ROOT .. "/x64/debug")
		}
		
	configuration { "vs*", "Release" }
		libdirs {
			(FBX_SDK_ROOT .. "/x64/release")			
		}
	configuration { "vs*", "Dist" }
		libdirs {
			(FBX_SDK_ROOT .. "/x64/release")			
		}
end


project "LibRiiEditor"
	location "LibRiiEditor"
	includedirs {
		"LibRiiEditor/source",
		"ThirdParty/source",
		"oishii"
	}

	setupStaticLib()
	setupCppC()



	setupSystem()
	setupPreprocessor()

project "TestEditor"
	location "TestEditor"
	setupMainApp()

project "RiiStudio"
	location "RiiStudio"
	setupMainApp()




project "ThirdParty"
	location "ThirdParty"

	includedirs
	{
		"oishii",
		"ThirdParty/source/"
	}
	setupStaticLib()
	setupCppC()


	setupSystem()
	setupPreprocessor()

project "LibCube"
	location "LibCube"
	includedirs
	{
		"LibRiiEditor/source",
		"ThirdParty/source",
		"LibCube/source",
		"oishii",
		(FBX_SDK_ROOT .. "../../../include")
	}

	setupStaticLib()
	setupCppC()


	setupSystem()
	setupPreprocessor()
