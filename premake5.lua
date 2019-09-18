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


project "LibRiiEditor"
	location "LibRiiEditor"
	setupStaticLib()
	setupCppC()

	includedirs {
		"LibRiiEditor/source",
		"ThirdParty/source",
		"oishii"
	}

	setupSystem()
	setupPreprocessor()

project "TestEditor"
	location "TestEditor"
	setupConsoleApp()

	setupCppC()

	includedirs {
		"LibRiiEditor/source",
		"ThirdParty/source",
		"oishii"
	}

	links
	{
		"LibRiiEditor",
		"ThirdParty",
		"LibCube",
		"ThirdParty/glfw/lib-vc2017/glfw3dll.lib",
		"opengl32.lib"
	}
	setupSystem()

	postbuildcommands {
  		--"{COPY} ThirdParty/glfw/lib-vc2017glfw3.dll %{cfg.targetdir}"
	}

	
	setupPreprocessor()

project "ThirdParty"
	location "ThirdParty"
	setupStaticLib()

	setupCppC()

	includedirs
	{
		"oishii",
		"ThirdParty/source/"
	}

	setupSystem()
	setupPreprocessor()

project "LibCube"
	location "LibCube"
	setupStaticLib()
	setupCppC()

	includedirs
	{
		"LibRiiEditor/source",
		"ThirdParty/source",
		"oishii"
	}

	setupSystem()
	setupPreprocessor()
