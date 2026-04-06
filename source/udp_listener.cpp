// Receives voice packets from 8bit via UDP
#include "udp_listener.h"
#include "audio_buffer.h"
#include "steam_voice.h"
#include "debug_log.h"

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#endif

static std::thread g_listenerThread;
static std::atomic<bool> g_running(false);
static SOCKET g_socket = INVALID_SOCKET;
static int g_packetCount = 0;
#include <unordered_map>
#include <mutex>
static std::mutex g_steamidMutex;
static std::unordered_map<int, uint64_t> g_keyToSteamid;

std::string GetSteamID64ForKey(int key) {
    std::lock_guard<std::mutex> lock(g_steamidMutex);
    auto it = g_keyToSteamid.find(key);
    if (it == g_keyToSteamid.end()) return "";
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu",
        (unsigned long long)it->second);
    return std::string(buf);
}

static void ListenerLoop(int port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        WDEBUG("[Whisper] WSAStartup failed\n");
        return;
    }
#endif

    g_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_socket == INVALID_SOCKET) {
        WDEBUG("[Whisper] Failed to create socket\n");
        return;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_socket, (sockaddr*)&addr, sizeof(addr)) < 0) {
        WDEBUG("[Whisper] Failed to bind port %d\n", port);
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
        return;
    }

    WDEBUG("[Whisper] UDP socket bound on port %d\n", port);

    char buf[65535];
    while (g_running) {
        sockaddr_in from;
        socklen_t fromLen = sizeof(from);
        int len = recvfrom(
            g_socket, buf, sizeof(buf), 0,
            (sockaddr*)&from, &fromLen
        );
        if (len <= 0 || !g_running) continue;

        g_packetCount++;

        // Extract steamid from first 8 bytes, use lower 32
        // bits as buffer key (unique per player)
        int userid = 0;
        if (len >= 8) {
            userid = *(int*)((uint8_t*)buf);
        }

        // Store key→steamid64 mapping
        if (len >= 8) {
            uint64_t sid = *(uint64_t*)buf;
            std::lock_guard<std::mutex> lock(g_steamidMutex);
            g_keyToSteamid[userid] = sid;
        }

        auto pcm = DecodeSteamVoice(
            (const uint8_t*)buf, len
        );

        if (g_packetCount <= 5) {
            uint64_t steamid64 = 0;
            if (len >= 8) steamid64 = *(uint64_t*)buf;
            WDEBUG("[Whisper] Packet #%d len=%d key=%d "
                "steamid=%llu decoded=%d\n",
                g_packetCount, len, userid,
                (unsigned long long)steamid64, (int)pcm.size());
        }

        if (!pcm.empty()) {
            GetAudioBuffer().Append(
                userid, pcm.data(), (int)pcm.size()
            );
        }
    }

    closesocket(g_socket);
    g_socket = INVALID_SOCKET;

#ifdef _WIN32
    WSACleanup();
#endif
}

void StartUDPListener(int port) {
    if (g_running) return;
    g_running = true;
    g_listenerThread = std::thread(ListenerLoop, port);
}

void StopUDPListener() {
    g_running = false;
    if (g_socket != INVALID_SOCKET) {
        closesocket(g_socket);
    }
    if (g_listenerThread.joinable()) {
        g_listenerThread.join();
    }
}
