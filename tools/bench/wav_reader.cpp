#include "wav_reader.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

struct WavFormat {
    uint16_t format = 0;
    uint16_t channels = 0;
    uint32_t rate = 0;
    uint16_t bits = 0;
};

static void ConvertSamples(const std::vector<char>& raw, const WavFormat& fmt, std::vector<float>& out) {
    if (fmt.format == 3) {
        out.resize(raw.size() / 4);
        memcpy(out.data(), raw.data(), out.size() * 4);
        return;
    }
    out.resize(raw.size() / 2);
    for (size_t i = 0; i < out.size(); ++i) {
        int16_t s;
        memcpy(&s, raw.data() + i * 2, 2);
        out[i] = s / 32768.0f;
    }
}

// Walks RIFF chunks; fmt must precede data, which every common encoder guarantees
static bool ReadChunks(FILE* f, WavFormat& fmt, std::vector<char>& raw) {
    char id[4];
    uint32_t size;
    while (fread(id, 1, 4, f) == 4 && fread(&size, 4, 1, f) == 1) {
        if (memcmp(id, "fmt ", 4) == 0 && size >= 16) {
            fread(&fmt.format, 2, 1, f);
            fread(&fmt.channels, 2, 1, f);
            fread(&fmt.rate, 4, 1, f);
            fseek(f, 6, SEEK_CUR);
            fread(&fmt.bits, 2, 1, f);
            fseek(f, size - 16, SEEK_CUR);
        } else if (memcmp(id, "data", 4) == 0) {
            raw.resize(size);
            return fread(raw.data(), 1, size, f) == size;
        } else {
            fseek(f, size + (size & 1), SEEK_CUR);
        }
    }
    return false;
}

bool ReadWav16kMono(const std::string& path, std::vector<float>& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    char riff[12];
    WavFormat fmt;
    std::vector<char> raw;
    bool ok = fread(riff, 1, 12, f) == 12 && memcmp(riff, "RIFF", 4) == 0
        && memcmp(riff + 8, "WAVE", 4) == 0 && ReadChunks(f, fmt, raw);
    fclose(f);
    bool pcm16 = fmt.format == 1 && fmt.bits == 16;
    bool f32 = fmt.format == 3 && fmt.bits == 32;
    if (!ok || fmt.channels != 1 || fmt.rate != 16000 || !(pcm16 || f32)) return false;
    ConvertSamples(raw, fmt, out);
    return true;
}
