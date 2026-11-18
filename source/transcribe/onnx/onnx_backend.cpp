// The recognizer holds the ONNX sessions; ORT sessions are safe to Run from
// several threads, so workers share it and only own their stream
#include "onnx_backend.h"
#ifdef AURIS_WITH_ONNX
#include "onnx_model_paths.h"
#include "onnx_recognizer_config.h"
#include "sherpa-onnx/c-api/c-api.h"
#include <cstdio>

static const SherpaOnnxOfflineRecognizer* g_recognizer = nullptr;

bool OnnxInit(const WhisperConfig& cfg) {
    if (g_recognizer) return true;
    OnnxModelPaths paths;
    if (!ResolveOnnxModelPaths(cfg, paths)) return false;

    SherpaOnnxOfflineRecognizerConfig rc;
    FillRecognizerConfig(cfg, paths, rc);

    // the recognizer copies every path, so paths may die after this call
    g_recognizer = SherpaOnnxCreateOfflineRecognizer(&rc);
    if (!g_recognizer) {
        printf("[Auris] ERROR: sherpa-onnx could not load %s\n", cfg.onnx_model_dir.c_str());
        return false;
    }
    printf("[Auris] ONNX backend: %s (%s)\n", cfg.onnx_model_type.c_str(), cfg.onnx_model_dir.c_str());
    return true;
}

void OnnxShutdown() {
    if (g_recognizer) SherpaOnnxDestroyOfflineRecognizer(g_recognizer);
    g_recognizer = nullptr;
}

bool OnnxTranscribe(const std::vector<float>& audio, std::string& text) {
    if (!g_recognizer || audio.empty()) return false;
    const SherpaOnnxOfflineStream* stream = SherpaOnnxCreateOfflineStream(g_recognizer);
    if (!stream) return false;
    SherpaOnnxAcceptWaveformOffline(stream, 16000, audio.data(), (int32_t)audio.size());
    SherpaOnnxDecodeOfflineStream(g_recognizer, stream);
    const SherpaOnnxOfflineRecognizerResult* r = SherpaOnnxGetOfflineStreamResult(stream);
    bool ok = r && r->text;
    if (ok) text = r->text;
    if (r) SherpaOnnxDestroyOfflineRecognizerResult(r);
    SherpaOnnxDestroyOfflineStream(stream);
    return ok;
}
#else
bool OnnxInit(const WhisperConfig&) { return false; }
void OnnxShutdown() {}
bool OnnxTranscribe(const std::vector<float>&, std::string&) { return false; }
#endif
