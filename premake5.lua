PROJECT_GENERATOR_VERSION = 2

newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "../garrysmod_common"
})

local gmcommon = assert(_OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON"),
	"you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
include(gmcommon)

CreateWorkspace({name = "whisper", abi_compatible = false, path = "projects/" .. os.target() .. "/" .. _ACTION})
	CreateProject({serverside = true, source_path = "source", manual_files = false})
		warnings("Default")
		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()
		IncludeSteamAPI()
		IncludeDetouring()
		IncludeScanning()

		-- whisper.cpp vendor files
		includedirs({
			"vendor/whisper.cpp/include",
			"vendor/whisper.cpp/ggml/include",
			"vendor/whisper.cpp/ggml/src",
			"vendor/whisper.cpp/ggml/src/ggml-cpu",
		})
		files({
			"vendor/whisper.cpp/src/whisper.cpp",
			"vendor/whisper.cpp/ggml/src/*.c",
			"vendor/whisper.cpp/ggml/src/*.cpp",
			"vendor/whisper.cpp/ggml/src/ggml-cpu/*.c",
			"vendor/whisper.cpp/ggml/src/ggml-cpu/*.cpp",
			"vendor/whisper.cpp/ggml/src/ggml-cpu/arch/x86/*.c",
			"vendor/whisper.cpp/ggml/src/ggml-cpu/arch/x86/*.cpp",
		})
		defines({
			"GGML_USE_CPU",
			"WHISPER_VERSION=\"1.0.0\"",
			"GGML_VERSION=\"1.0.0\"",
			"GGML_COMMIT=\"local\"",
			"NDEBUG",
		})

		-- Optimization for whisper.cpp
		optimize("Speed")
		vectorextensions("AVX2")
		flags({"NoBufferSecurityCheck"})
		buildoptions({"/fp:fast", "/Ox", "/GL", "/arch:AVX2"})
		linkoptions({"/LTCG"})

		-- Opus for decoding steam voice
		includedirs({"vendor/opus/include"})
		filter("platforms:x86")
			libdirs({path.getabsolute("vendor/opus/lib32")})
		filter("platforms:x86_64")
			libdirs({path.getabsolute("vendor/opus/lib64")})
		filter({})
		links({"opus"})

		-- Vulkan GPU backend (both 32 and 64 bit)
		local vulkan_sdk = os.getenv("VULKAN_SDK") or "C:/VulkanSDK/1.4.341.1"
		defines({"GGML_USE_VULKAN"})
		includedirs({
			vulkan_sdk .. "/Include",
			"vendor/whisper.cpp/ggml/src/ggml-vulkan",
		})
		files({
			"vendor/whisper.cpp/ggml/src/ggml-vulkan/ggml-vulkan.cpp",
			"vendor/whisper.cpp/ggml/src/ggml-vulkan/ggml-vulkan-shaders.cpp",
		})
		links({"vulkan-1"})
		filter("platforms:x86")
			libdirs({path.getabsolute("vendor/vulkan/lib32")})
			-- x86 stdcall decorates symbols with @N but the import lib has
			-- undecorated names. Map the 3 directly-linked Vulkan symbols.
			linkoptions({
				"/ALTERNATENAME:_vkCmdCopyBuffer@28=_vkCmdCopyBuffer",
				"/ALTERNATENAME:_vkGetPhysicalDeviceFeatures2@8=_vkGetPhysicalDeviceFeatures2",
				"/ALTERNATENAME:_vkGetInstanceProcAddr@8=_vkGetInstanceProcAddr",
			})
		filter("platforms:x86_64")
			libdirs({vulkan_sdk .. "/Lib"})
		filter({})