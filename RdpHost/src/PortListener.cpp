#include "rdphost/PortListener.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

namespace rdphost {
namespace {

class WinsockLifetime {
public:
    WinsockLifetime() {
        WSADATA data{};
        ok_ = (WSAStartup(MAKEWORD(2, 2), &data) == 0);
    }

    ~WinsockLifetime() {
        if (ok_) {
            WSACleanup();
        }
    }

    bool Ok() const {
        return ok_;
    }

private:
    bool ok_ = false;
};

bool PortOpen(std::uint16_t port) {
    WinsockLifetime winsock;
    if (!winsock.Ok()) {
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    const int result = connect(
        sock,
        reinterpret_cast<sockaddr*>(&addr),
        sizeof(addr));
    closesocket(sock);
    return result == 0;
}
}
PortListener::PortListener(std::uint16_t port) : port_(port) {}

bool PortListener::IsListeningLocally() const {
    return PortOpen(port_);
}
OperationResult PortListener::WaitUntilListening(
    std::uint32_t timeoutMs) const {
    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline) {
        if (IsListeningLocally()) {
            OperationResult result;
            result.ok = true;
            result.message =
                L"Port " + std::to_wstring(port_) + L" is listening";
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    OperationResult result;
    result.ok = false;
    result.message =
        L"Timeout waiting for port " + std::to_wstring(port_);
    return result;
}
StatusItem PortListener::CheckStatus() const {
    StatusItem item;
    item.id = L"port";
    item.title = L"Port " + std::to_wstring(port_);
    if (IsListeningLocally()) {
        item.level = StatusLevel::Ok;
        item.detail = L"Listening on 127.0.0.1";
    } else {
        item.level = StatusLevel::Error;
        item.detail = L"Not accepting connections";
    }
    return item;
}
}