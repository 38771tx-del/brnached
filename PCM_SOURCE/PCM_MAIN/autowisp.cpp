// autowisp.cpp
// Port of the uploaded Python template-matching logic to C++.
// Requires Windows + OpenCV. Put PNG templates in ./letter_templates beside the EXE.

#include <windows.h>
#include <shellscalingapi.h>

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shcore.lib")

namespace fs = std::filesystem;

namespace {

constexpr double DETECTION_THRESHOLD = 0.85;
constexpr double PERFECT_MATCH_SCORE = 0.98;
constexpr int MAX_SCAN_ATTEMPTS = 10;
constexpr double SCAN_TIMEOUT_SECONDS = 2.0;
constexpr int DARK_REGION_MEAN_LIMIT = 10;
constexpr int SCAN_RETRY_DELAY_MS = 5;
constexpr int MAIN_LOOP_IDLE_MS = 1;

struct Region {
    int left;
    int top;
    int width;
    int height;
};

struct TemplateItem {
    char letter;
    cv::Mat gray;
};

const std::vector<Region> LETTER_REGIONS = {
    { 860,  200, 50, 100 },
    { 940,  200, 50, 100 },
    { 1020, 200, 50, 100 },
    { 1100, 200, 50, 100 },
};

std::atomic_bool g_scanning{ false };
std::atomic_bool g_replaceZWithY{ false };
std::atomic_bool g_debug{ false };
std::atomic_int g_keyDelayMs{ 100 }; // Python default: 0.09 + 10ms ping = 100ms.

std::vector<TemplateItem> g_templates;

void Debug(const std::string& msg) {
    if (g_debug.load()) {
        std::cout << msg << std::endl;
    }
}

fs::path GetExeDirectory() {
    wchar_t path[MAX_PATH]{};
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        return fs::current_path();
    }
    return fs::path(path).parent_path();
}

std::optional<char> LetterFromTemplateFilename(const fs::path& filePath) {
    std::string stem = filePath.stem().string();
    for (auto it = stem.rbegin(); it != stem.rend(); ++it) {
        unsigned char ch = static_cast<unsigned char>(*it);
        if (std::isalpha(ch)) {
            return static_cast<char>(std::toupper(ch));
        }
    }
    return std::nullopt;
}

bool LoadTemplates() {
    g_templates.clear();

    fs::path folder = GetExeDirectory() / "letter_templates";
    if (!fs::exists(folder) || !fs::is_directory(folder)) {
        std::cerr << "Template folder not found: " << folder.string() << std::endl;
        return false;
    }

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        fs::path path = entry.path();
        std::string filename = path.filename().string();
        std::string lower = filename;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (lower == "icon.ico" || lower == "icon.png" || lower == "background.png") {
            continue;
        }
        if (path.extension() != ".png" && path.extension() != ".PNG") {
            continue;
        }

        std::optional<char> letter = LetterFromTemplateFilename(path);
        if (!letter.has_value()) {
            continue;
        }

        cv::Mat gray = cv::imread(path.string(), cv::IMREAD_GRAYSCALE);
        if (gray.empty()) {
            Debug("Failed to load template: " + filename);
            continue;
        }

        g_templates.push_back({ *letter, gray });
    }

    if (g_templates.empty()) {
        std::cerr << "No valid templates found in letter_templates." << std::endl;
        return false;
    }

    Debug("Loaded templates: " + std::to_string(g_templates.size()));
    return true;
}

cv::Mat CaptureRegionBgra(const Region& r) {
    HDC screenDc = GetDC(nullptr);
    if (!screenDc) {
        return {};
    }

    HDC memDc = CreateCompatibleDC(screenDc);
    if (!memDc) {
        ReleaseDC(nullptr, screenDc);
        return {};
    }

    HBITMAP bitmap = CreateCompatibleBitmap(screenDc, r.width, r.height);
    if (!bitmap) {
        DeleteDC(memDc);
        ReleaseDC(nullptr, screenDc);
        return {};
    }

    HGDIOBJ oldObj = SelectObject(memDc, bitmap);
    BOOL copied = BitBlt(memDc, 0, 0, r.width, r.height, screenDc, r.left, r.top, SRCCOPY | CAPTUREBLT);

    cv::Mat bgra;
    if (copied) {
        bgra.create(r.height, r.width, CV_8UC4);

        BITMAPINFOHEADER bi{};
        bi.biSize = sizeof(BITMAPINFOHEADER);
        bi.biWidth = r.width;
        bi.biHeight = -r.height; // top-down bitmap
        bi.biPlanes = 1;
        bi.biBitCount = 32;
        bi.biCompression = BI_RGB;

        int got = GetDIBits(memDc, bitmap, 0, r.height, bgra.data, reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);
        if (got == 0) {
            bgra.release();
        }
    }

    SelectObject(memDc, oldObj);
    DeleteObject(bitmap);
    DeleteDC(memDc);
    ReleaseDC(nullptr, screenDc);
    return bgra;
}

std::pair<char, double> DetectLetterInRegion(const cv::Mat& regionBgra) {
    if (regionBgra.empty()) {
        return { '\0', 0.0 };
    }

    cv::Mat gray;
    if (regionBgra.channels() == 4) {
        cv::cvtColor(regionBgra, gray, cv::COLOR_BGRA2GRAY);
    } else if (regionBgra.channels() == 3) {
        cv::cvtColor(regionBgra, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = regionBgra;
    }

    if (cv::mean(gray)[0] < DARK_REGION_MEAN_LIMIT) {
        return { '\0', 0.0 };
    }

    char bestLetter = '\0';
    double bestScore = 0.0;

    for (const TemplateItem& item : g_templates) {
        if (item.gray.empty()) {
            continue;
        }
        if (item.gray.rows > gray.rows || item.gray.cols > gray.cols) {
            continue;
        }

        cv::Mat result;
        cv::matchTemplate(gray, item.gray, result, cv::TM_CCOEFF_NORMED);

        double maxVal = 0.0;
        cv::minMaxLoc(result, nullptr, &maxVal, nullptr, nullptr);

        if (maxVal > PERFECT_MATCH_SCORE) {
            return { item.letter, maxVal };
        }
        if (maxVal > bestScore && maxVal >= DETECTION_THRESHOLD) {
            bestScore = maxVal;
            bestLetter = item.letter;
        }
    }

    return { bestLetter, bestScore };
}

std::vector<char> ScanUntilAllFound() {
    std::vector<char> detected(LETTER_REGIONS.size(), '\0');

    const auto start = std::chrono::steady_clock::now();
    int attempts = 0;

    while (attempts < MAX_SCAN_ATTEMPTS) {
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        if (elapsed >= SCAN_TIMEOUT_SECONDS) {
            break;
        }

        bool allFound = true;

        for (size_t i = 0; i < LETTER_REGIONS.size(); ++i) {
            if (detected[i] != '\0') {
                continue;
            }

            cv::Mat region = CaptureRegionBgra(LETTER_REGIONS[i]);
            auto [letter, score] = DetectLetterInRegion(region);

            if (letter != '\0' && score >= DETECTION_THRESHOLD) {
                detected[i] = letter;
                Debug("Slot " + std::to_string(i + 1) + ": " + std::string(1, letter) +
                      " score " + std::to_string(score));
            } else {
                allFound = false;
            }
        }

        if (allFound) {
            Debug("All slots found");
            break;
        }

        ++attempts;
        Sleep(SCAN_RETRY_DELAY_MS);
    }

    return detected;
}

void PressKey(char ch) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

    SHORT vkScan = VkKeyScanA(ch);
    if (vkScan == -1) {
        return;
    }

    WORD vk = LOBYTE(vkScan);
    WORD scan = static_cast<WORD>(MapVirtualKeyA(vk, MAPVK_VK_TO_VSC));

    INPUT inputs[2]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[0].ki.wScan = scan;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vk;
    inputs[1].ki.wScan = scan;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, inputs, sizeof(INPUT));
}

void SendSequence(const std::vector<char>& detected) {
    for (char letter : detected) {
        if (letter == '\0') {
            continue;
        }

        char out = static_cast<char>(std::toupper(static_cast<unsigned char>(letter)));
        if (g_replaceZWithY.load() && out == 'Z') {
            out = 'Y';
        }

        PressKey(out);
        int delay = g_keyDelayMs.load();
        if (delay > 0) {
            Sleep(delay);
        }
    }
}

std::string SequenceToString(const std::vector<char>& detected) {
    std::string s;
    for (char c : detected) {
        if (c != '\0') {
            s.push_back(c);
        }
    }
    return s;
}

} // namespace

void StartChatScanning() {
    if (g_scanning.load()) {
        return;
    }

    if (g_templates.empty() && !LoadTemplates()) {
        std::cout << "templates_fail" << std::endl;
        return;
    }

    g_scanning.store(true);
    std::cout << "ready" << std::endl;
}

void StopChatScanning() {
    g_scanning.store(false);
}

void PerformChatScan() {
    if (!g_scanning.load()) {
        return;
    }

    try {
        std::vector<char> detected = ScanUntilAllFound();
        std::string sequence = SequenceToString(detected);

        std::cout << "ROI:[" << sequence << "]" << std::endl;

        if (!sequence.empty()) {
            SendSequence(detected);
        }
    } catch (const std::exception& e) {
        Debug(std::string("scan_error: ") + e.what());
    } catch (...) {
        Debug("scan_error: unknown");
    }
}

int main(int argc, char** argv) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--debug") {
            g_debug.store(true);
        } else if (arg == "--replace-z-with-y" || arg == "--zy") {
            g_replaceZWithY.store(true);
        } else if (arg == "--ping" && i + 1 < argc) {
            int pingMs = std::max(0, std::atoi(argv[++i]));
            g_keyDelayMs.store(90 + pingMs);
        }
    }

    StartChatScanning();

    if (!g_scanning.load()) {
        return 1;
    }

    for (;;) {
        if (GetAsyncKeyState(VK_F2) & 1) {
            StopChatScanning();
            std::cout << "stopped" << std::endl;
        }
        if (GetAsyncKeyState(VK_F1) & 1) {
            StartChatScanning();
        }
        if (GetAsyncKeyState(VK_ESCAPE) & 1) {
            break;
        }

        PerformChatScan();
        Sleep(MAIN_LOOP_IDLE_MS);
    }

    StopChatScanning();
    return 0;
}
