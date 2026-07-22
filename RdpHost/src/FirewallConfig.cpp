#include "rdphost/FirewallConfig.h"

#include "rdphost/WinUtils.h"

#include <windows.h>

#include <string>

namespace rdphost {
namespace {

OperationResult RunHidden(const std::wstring& commandLine) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    std::wstring mutableCmd = commandLine;
    if (!CreateProcessW(
            nullptr,
            mutableCmd.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        return WinUtils::LastErrorAsResult(L"CreateProcess(netsh)");
    }

    WaitForSingleObject(pi.hProcess, 15000);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return {
        code == 0,
        code == 0 ? L"netsh ok" : (L"netsh exit " + std::to_wstring(code))};
}

}

OperationResult FirewallConfig::EnsureRdpAllowRules() {
    RunHidden(
        L"netsh advfirewall firewall delete rule name=\"Remote Desktop\"");

    OperationResult tcp = RunHidden(
        L"netsh advfirewall firewall add rule name=\"Remote Desktop\" "
        L"dir=in protocol=tcp localport=3389 profile=any action=allow");
    if (!tcp.ok) {
        return tcp;
    }

    OperationResult udp = RunHidden(
        L"netsh advfirewall firewall add rule name=\"Remote Desktop\" "
        L"dir=in protocol=udp localport=3389 profile=any action=allow");
    if (!udp.ok) {
        return udp;
    }

    return {true, L"Firewall allow rules for 3389 set"};
}

}
