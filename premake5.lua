-- Allow CI to override via --generator-version=3 for x86-64-support-sourcesdk branch
newoption({
	trigger = "generator-version",
	description = "garrysmod_common PROJECT_GENERATOR_VERSION (2 = default, 3 = x86-64-support-sourcesdk)",
	value = "2"
})
PROJECT_GENERATOR_VERSION = tonumber(_OPTIONS["generator-version"]) or 2

newoption({
	trigger = "gmcommon",
	description = "Sets the path to the garrysmod_common (https://github.com/danielga/garrysmod_common) directory",
	value = "../garrysmod_common"
})

newoption({
	trigger = "with-vulkan",
	description = "Enable the Vulkan GPU backend (off by default; CPU-only build)"
})
local WITH_VULKAN = _OPTIONS["with-vulkan"] ~= nil

local gmcommon = assert(_OPTIONS.gmcommon or os.getenv("GARRYSMOD_COMMON"),
	"you didn't provide a path to your garrysmod_common (https://github.com/danielga/garrysmod_common) directory")
include(gmcommon)

-- whisper/ggml sources, defines and optimisation flags shared by the module and auris-bench,
-- so the benchmark measures exactly the code srcds loads
local function AddWhisperCore()
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
		"vendor/whisper.cpp/ggml/src/ggml-cpu/llamafile/sgemm.cpp",
	})
	local versionFile = io.open("VERSION", "r")
	local aurisVersion = versionFile and versionFile:read("*l") or "unknown"
	if versionFile then versionFile:close() end

	defines({
		"AURIS_VERSION=\"" .. aurisVersion .. "\"",
		"GGML_USE_CPU",
		-- sgemm (llamafile) tinyBLAS kernels; upstream CMake enables them by default
		"GGML_USE_LLAMAFILE",
		-- match upstream ggml-cpu CMake for an AVX2 x86 build
		"GGML_AVX2",
		"GGML_FMA",
		"GGML_F16C",
		"WHISPER_VERSION=\"1.0.0\"",
		"GGML_VERSION=\"1.0.0\"",
		"GGML_COMMIT=\"local\"",
		"NDEBUG",
	})

	-- Optimization for whisper.cpp — flags split by toolset
	optimize("Speed")
	vectorextensions("AVX2")

	filter("toolset:msc*")
		flags({"NoBufferSecurityCheck"})
		buildoptions({"/fp:fast", "/Ox", "/GL", "/arch:AVX2"})
		linkoptions({"/LTCG"})

	filter("toolset:gcc or clang")
		defines({"_GNU_SOURCE"})
		-- -ffast-math enables -ffinite-math-only which ggml explicitly forbids
		-- (vec.h line 1069 #errors on __FINITE_MATH_ONLY__). Use the safe subset instead.
		-- -fno-stack-protector omitted: EDRs (CrowdSec, VirusTotal) flag binaries built without
		-- stack protection as exploit-facilitating, triggering false CVE-2023-4911 detections.
		buildoptions({"-fno-math-errno", "-funsafe-math-optimizations", "-fno-rounding-math", "-fno-signaling-nans", "-O3", "-flto", "-mavx2", "-mfma", "-mf16c"})
		linkoptions({"-flto"})

	-- 32-bit x86 Linux: force SSE2 scalar FP to match x86_64 behavior.
	-- Without this GCC uses x87 (80-bit extended precision) for scalar floats,
	-- causing whisper softmax/logit values to diverge from every other platform.
	filter("toolset:gcc or clang", "platforms:x86", "system:linux")
		buildoptions({"-mfpmath=sse", "-msse2"})

	filter({})
end

-- sherpa-onnx + static ONNX Runtime, 64-bit only: ORT ships no x86_32
-- Linux build, so 32-bit modules stay whisper only (asr_backend.cpp)
local function AddOnnx()
	filter("platforms:x86_64")
		defines({"AURIS_WITH_ONNX"})
		includedirs({"vendor/sherpa-onnx/include"})
		links({
			"sherpa-onnx-c-api", "sherpa-onnx-core", "kaldi-decoder-core",
			"sherpa-onnx-kaldifst-core", "sherpa-onnx-fstfar", "sherpa-onnx-fst",
			"kaldi-native-fbank-core", "kissfft-float", "ssentencepiece_core",
			"utf8proc", "onnxruntime",
		})
	filter({"platforms:x86_64", "system:windows"})
		libdirs({path.getabsolute("vendor/sherpa-onnx/win-x64/lib")})
	filter({"platforms:x86_64", "system:linux"})
		libdirs({path.getabsolute("vendor/sherpa-onnx/linux-x64/lib")})
		-- pre-C++11 std::string ABI is set workspace-wide (see CreateWorkspace)
		links({"dl", "pthread"})
	filter({})
end

local module_name = WITH_VULKAN and "auris-gpu" or "auris"
CreateWorkspace({name = module_name, abi_compatible = false, path = "projects/" .. os.target() .. "/" .. _ACTION})
	-- the sherpa-onnx prebuilt libs force the pre-C++11 std::string ABI; every
	-- project of the workspace (garrysmod_common helpers included) must match or
	-- std::string symbols such as ModuleLoader::LoadModule fail to link
	filter({"platforms:x86_64", "system:linux"})
		defines({"_GLIBCXX_USE_CXX11_ABI=0"})
	filter({})
	CreateProject({serverside = true, source_path = "source", manual_files = false})
		-- garrysmod_common only globs the top folder; sources live in subsystem folders
		files({"source/**.h", "source/**.cpp"})
		warnings("Default")
		IncludeSDKCommon()
		IncludeSDKTier0()
		IncludeSDKTier1()
		IncludeSteamAPI()
		IncludeDetouring()
		IncludeScanning()
		IncludeHelpersExtended()

		AddWhisperCore()

		-- Opus for decoding steam voice
		includedirs({"vendor/opus/include"})
		filter("platforms:x86")
			libdirs({path.getabsolute("vendor/opus/lib32")})
		filter("platforms:x86_64")
			libdirs({path.getabsolute("vendor/opus/lib64")})
		filter({})
		links({"opus"})

		AddOnnx()

		-- Vulkan GPU backend (opt-in via --with-vulkan)
		-- Without this flag the module is CPU-only and has no libvulkan dep,
		-- so it loads on headless dedis with no Vulkan loader installed.
		if WITH_VULKAN then
			local vulkan_sdk = os.getenv("VULKAN_SDK") or "C:/VulkanSDK/1.4.341.1"
			defines({"GGML_USE_VULKAN"})
			includedirs({
				"vendor/whisper.cpp/ggml/src/ggml-vulkan",
			})
			files({
				"vendor/whisper.cpp/ggml/src/ggml-vulkan/ggml-vulkan.cpp",
				"vendor/whisper.cpp/ggml/src/ggml-vulkan/ggml-vulkan-shaders.cpp",
			})

			-- Windows: Vulkan SDK headers + import lib
			filter("system:windows")
				includedirs({vulkan_sdk .. "/Include"})
				links({"vulkan-1"})

			-- premake does not support compound filter keys — split into separate filters
			filter({"system:windows", "platforms:x86"})
				libdirs({path.getabsolute("vendor/vulkan/lib32")})
				-- x86 stdcall decorates symbols with @N but the import lib has
				-- undecorated names. Map the 3 directly-linked Vulkan symbols.
				linkoptions({
					"/ALTERNATENAME:_vkCmdCopyBuffer@28=_vkCmdCopyBuffer",
					"/ALTERNATENAME:_vkGetPhysicalDeviceFeatures2@8=_vkGetPhysicalDeviceFeatures2",
					"/ALTERNATENAME:_vkGetInstanceProcAddr@8=_vkGetInstanceProcAddr",
				})

			filter({"system:windows", "platforms:x86_64"})
				libdirs({vulkan_sdk .. "/Lib"})

			-- Linux: system Vulkan loader
			-- VULKAN_HPP_TYPESAFE_CONVERSION enables implicit vk:: <-> Vk* handle conversions
			-- that ggml-vulkan.cpp relies on when mixing C and C++ Vulkan APIs.
			filter("system:linux")
				defines({"VULKAN_HPP_TYPESAFE_CONVERSION=1"})
				links({"vulkan"})

			filter({})
		end

	-- Console benchmark: same whisper build, pool-side sources without GMod deps
	project("auris-bench")
		kind("ConsoleApp")
		language("C++")
		warnings("Default")
		cppdialect("C++17")
		includedirs({"source"})
		files({
			"tools/bench/*.cpp",
			"tools/bench/*.h",
			"source/core/auris_config.cpp",
			"source/util/debug_log.cpp",
			"source/transcribe/job_limits.cpp",
			"source/transcribe/pool_limits.cpp",
			"source/vad/silero_gate.cpp",
			"source/transcribe/whisper_params.cpp",
			"source/transcribe/onnx/onnx_backend.cpp",
			"source/transcribe/onnx/onnx_model_paths.cpp",
			"source/transcribe/onnx/onnx_file_find.cpp",
			"source/transcribe/onnx/onnx_recognizer_config.cpp",
		})
		AddWhisperCore()
		AddOnnx()
		filter("system:windows")
			links({"psapi"})
		filter("system:linux")
			links({"pthread"})
		filter({})
