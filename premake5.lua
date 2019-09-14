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

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "LibRiiEditor"
	location "LibRiiEditor"
	kind "StaticLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/source/**.h",
		"%{prj.name}/source/**.hpp",
		"%{prj.name}/source/**.cpp",
		"%{prj.name}/source/**.c"
	}

	includedirs
	{
		"LibRiiEditor/source",
		"ThirdParty/source",
		"oishii"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"RII_PLATFORM_WINDOWS",
			"_LIB"
		}

	filter "configurations:Debug"
		defines "DEBUG"
		symbols "On"
	
	filter "configurations:Release"
		defines "NDEBUG"
		optimize "On"
	
	filter "configurations:Dist"
		defines "NDEBUG"
		optimize "On"

project "TestEditor"
	location "TestEditor"
	kind "ConsoleApp"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/source/**.h",
		"%{prj.name}/source/**.hpp",
		"%{prj.name}/source/**.cpp",
		"%{prj.name}/source/**.c"
	}

	includedirs
	{
		"LibRiiEditor/source",
		"ThirdParty/source",
		"oishii",
	}

	

	defines
	{
		"RII_PLATFORM_WINDOWS",
		"_LIB"
	}
	links
	{
		"LibRiiEditor",
		"ThirdParty",
		"ThirdParty/glfw/lib-vc2017/glfw3dll.lib",
		"opengl32.lib"
	}
	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

	postbuildcommands {
  		--"{COPY} ThirdParty/glfw/lib-vc2017glfw3.dll %{cfg.targetdir}"
	}

	filter "files:*.cpp"
		language "C++"
	filter "files:*.c"
		language "C"

	filter "configurations:Debug"
		defines "DEBUG"
		symbols "On"
	
	filter "configurations:Release"
		defines "NDEBUG"
		optimize "On"
	
	filter "configurations:Dist"
		defines "NDEBUG"
		optimize "On"

project "ThirdParty"
	location "ThirdParty"
	kind "StaticLib"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/source/**.h",
		"%{prj.name}/source/**.hpp",
		"%{prj.name}/source/**.cpp",
		"%{prj.name}/source/**.c"
	}

	includedirs
	{
		"oishii",
		"ThirdParty/source/"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

		defines
		{
			"RII_PLATFORM_WINDOWS",
			"_LIB"
		}

	filter "configurations:Debug"
		defines "DEBUG"
		symbols "On"
	
	filter "configurations:Release"
		defines "NDEBUG"
		optimize "On"
	
	filter "configurations:Dist"
		defines "NDEBUG"
		optimize "On"

