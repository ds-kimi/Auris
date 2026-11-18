// Locates model files whose exact name varies between sherpa-onnx exports
#pragma once
#include <string>

// First file in dir named <prefix>*.int8.onnx, else <prefix>*.onnx, else ""
std::string FindOnnxFile(const std::string& dir, const std::string& prefix);
