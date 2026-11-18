#include "onnx_recognizer_config.h"
#ifdef AURIS_WITH_ONNX
#include <cstring>

void FillRecognizerConfig(const WhisperConfig& cfg, const OnnxModelPaths& paths,
                          SherpaOnnxOfflineRecognizerConfig& rc) {
    memset(&rc, 0, sizeof(rc));
    rc.feat_config.sample_rate = 16000;
    // Parakeet was trained on 128 mel bins; the others on 80
    rc.feat_config.feature_dim = cfg.onnx_model_type == "parakeet" ? 128 : 80;
    if (cfg.onnx_model_type == "moonshine") {
        rc.model_config.moonshine.encoder = paths.encoder.c_str();
        rc.model_config.moonshine.merged_decoder = paths.decoder.c_str();
    } else {
        rc.model_config.transducer.encoder = paths.encoder.c_str();
        rc.model_config.transducer.decoder = paths.decoder.c_str();
        rc.model_config.transducer.joiner = paths.joiner.c_str();
        // NeMo TDT needs its own decoder loop; icefall is detected from metadata
        if (cfg.onnx_model_type == "parakeet") rc.model_config.model_type = "nemo_transducer";
    }
    rc.model_config.tokens = paths.tokens.c_str();
    rc.model_config.num_threads = cfg.n_threads;
    rc.model_config.provider = "cpu";
    rc.decoding_method = "greedy_search";
}
#endif
