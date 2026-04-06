-- Whisper configuration
-- Edit these values to customize transcription behavior

-- Path to the whisper model file (relative to garrysmod/)
-- Available models: tiny, base, small, medium, large
-- Add ".en" suffix for english-only (faster): ggml-tiny.en.bin
-- Multilingual (auto-detect language): ggml-tiny.bin
WHISPER_MODEL = "garrysmod/data/whisper/ggml-tiny.bin"

-- Local UDP port for receiving 8bit voice broadcasts
WHISPER_PORT = 4000

-- Number of CPU threads for transcription
WHISPER_THREADS = 4

-- Show whisper progress during transcription
WHISPER_PRINT_PROGRESS = false

-- Show segment timestamps in output
WHISPER_PRINT_TIMESTAMPS = false

-- Force single segment output (recommended for voice chat)
WHISPER_SINGLE_SEGMENT = true

-- Disable context from previous transcriptions
WHISPER_NO_CONTEXT = true

-- Enable debug prints in console
WHISPER_DEBUG = false

-- Language for transcription
-- "auto" = auto-detect (multilingual models only)
-- "en", "fr", "es", "de", "it", "pt", "ru", "ja", "zh", etc.
-- Ignored for .en models (always english)
WHISPER_LANGUAGE = "fr"

-- Apply config to the whisper module
local function getLanguage(path)
    if string.find(path, "%.en%.bin$") then return "en" end
    return WHISPER_LANGUAGE
end

function ApplyWhisperConfig()
    whisper.SetConfig({
        language = getLanguage(WHISPER_MODEL),
        n_threads = WHISPER_THREADS,
        print_progress = WHISPER_PRINT_PROGRESS,
        print_timestamps = WHISPER_PRINT_TIMESTAMPS,
        single_segment = WHISPER_SINGLE_SEGMENT,
        no_context = WHISPER_NO_CONTEXT,
    })
    whisper.Debug(WHISPER_DEBUG)
end
