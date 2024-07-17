#include <windows.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iphlpapi.h>
#include <sddl.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "comctl32.lib")

#include "./obfusheader.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditSubclassProc(HWND hwndEdit, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
std::string GenerateSN(const std::string& hwid);
std::string GetHWID();
bool VerifySN(const std::string& hwid, const std::string& sn);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L" ";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        OBF(L"Register"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 160,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    CALL(&ShowWindow, hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

HWND hwndHWID;
HWND hwndHWIDLabel;
HWND hwndSN;
HWND hwndSNLabel;
HWND hwndStatus;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        hwndHWIDLabel = CreateWindow(L"STATIC", L"HWID", WS_VISIBLE | WS_CHILD, 10, 10, 50, 20, hwnd, NULL, NULL, NULL);
        hwndHWID = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_AUTOHSCROLL, 70, 10, 200, 20, hwnd, NULL, NULL, NULL);
        hwndSNLabel = CreateWindow(L"STATIC", L"SN", WS_VISIBLE | WS_CHILD, 10, 40, 50, 20, hwnd, NULL, NULL, NULL);
        hwndSN = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 70, 40, 200, 20, hwnd, NULL, NULL, NULL);
        CreateWindow(L"BUTTON", L"Verify", WS_VISIBLE | WS_CHILD, 70, 70, 80, 30, hwnd, (HMENU)1, NULL, NULL);
        CreateWindow(L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD, 160, 70, 80, 30, hwnd, (HMENU)2, NULL, NULL);
        hwndStatus = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 10, 100, 260, 20, hwnd, NULL, NULL, NULL);

        // Automatically fetch and display the HWID
        std::string hwid = GetHWID();
        std::wstring hwidWStr(hwid.begin(), hwid.end());
        SetWindowText(hwndHWID, hwidWStr.c_str());

        (WNDPROC)CALL(&SetWindowSubclass, hwndHWID, EditSubclassProc, 0, 0);
        (WNDPROC)CALL(&SetWindowSubclass, hwndSN, EditSubclassProc, 0, 0);
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1: {
            wchar_t snBuffer[256];
            GetWindowText(hwndSN, snBuffer, 256);
            std::wstring snWStr(snBuffer);
            std::string snStr(snWStr.begin(), snWStr.end());

            wchar_t hwidBuffer[256];
            GetWindowText(hwndHWID, hwidBuffer, 256);
            std::wstring hwidWStr(hwidBuffer);
            std::string hwidStr(hwidWStr.begin(), hwidWStr.end());

            if (hwidStr.empty() || snStr.empty()) {
                // Handle empty input
                SetWindowText(hwndStatus, OBF(L"HWID or SN is empty"));
                MessageBox(hwnd, OBF(L"HWID or SN cannot be empty."), OBF(L"Error"), MB_OK | MB_ICONERROR);
                break;
            }

            bool valid = VerifySN(hwidStr, snStr);
            if (!valid) SetWindowText(hwndSN, L"");
            SetWindowText(hwndStatus, valid ? OBF(L"SN is valid") : OBF(L"SN is invalid"));

            // Debug output using MessageBox
            /*std::wstring debugMessage = L"HWID: " + hwidWStr + L"\nGenerated SN: " + std::wstring(GenerateSN(hwidStr).begin(), GenerateSN(hwidStr).end()) + L"\nInput SN: " + snWStr;
            MessageBox(hwnd, debugMessage.c_str(), L"Debug Information", MB_OK);*/
            break;
        }
        case 2:
            CALL(&RemoveWindowSubclass, hwndHWID, EditSubclassProc, 0);
            CALL(&RemoveWindowSubclass, hwndSN, EditSubclassProc, 0);
            PostQuitMessage(0);
            break;
        }
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CTLCOLORSTATIC: {
        if ((HWND)lParam == hwndHWIDLabel || (HWND)lParam == hwndSNLabel) {
            CALL(&SetBkMode, (HDC)wParam, TRANSPARENT);
            return (LRESULT)CALL(&GetStockObject, HOLLOW_BRUSH);
        }
    }
    }

    return CALL(&DefWindowProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK EditSubclassProc(HWND hwndEdit, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (uMsg) {
    case WM_KEYDOWN: {
        if ((wParam == OBF('A')) && (CALL(&GetAsyncKeyState, VK_CONTROL) & 0x8000)) {
            SendMessage(hwndEdit, EM_SETSEL, 0, -1);
            return 0;
        }
        if ((wParam == OBF('C')) && (CALL(&GetAsyncKeyState, VK_CONTROL) & 0x8000)) {
            SendMessage(hwndEdit, WM_COPY, 0, 0);
            return 0;
        }
        break;
    }
    }

    return CALL(&DefSubclassProc, hwndEdit, uMsg, wParam, lParam);
}

std::string GenerateSN(const std::string& hwid) {
    if (hwid.empty()) {
        return ""; // Return an empty SN for empty HWID
    }

    std::hash<std::string> hasher;
    size_t hash = hasher(hwid);

    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill(OBF('0')) << hash;

    std::string sn = ss.str();
    std::transform(sn.begin(), sn.end(), sn.begin(), ::toupper);

    return sn;
}


std::string GetHWID() {
    HANDLE tokenHandle = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle)) {
        return "";
    }

    DWORD tokenInfoLength = 0;
    GetTokenInformation(tokenHandle, TokenUser, NULL, 0, &tokenInfoLength);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        CloseHandle(tokenHandle);
        return "";
    }

    TOKEN_USER* tokenUser = (TOKEN_USER*)malloc(tokenInfoLength);
    if (!tokenUser) {
        CloseHandle(tokenHandle);
        return "";
    }

    if (!GetTokenInformation(tokenHandle, TokenUser, tokenUser, tokenInfoLength, &tokenInfoLength)) {
        free(tokenUser);
        CloseHandle(tokenHandle);
        return "";
    }

    LPSTR sidString = NULL;
    if (!ConvertSidToStringSidA(tokenUser->User.Sid, &sidString)) {
        free(tokenUser);
        CloseHandle(tokenHandle);
        return "";
    }

    std::string sid(sidString);
    LocalFree(sidString);
    free(tokenUser);
    CloseHandle(tokenHandle);

    return sid;
}

bool VerifySN(const std::string& hwid, const std::string& sn) {
    if (hwid.empty() || sn.empty()) return false; // Invalid inputs
    //std::string expectedSN = GenerateSN(hwid);
    std::string expectedSN = CALL(&GenerateSN, hwid);
    return inline_strcmp(sn, expectedSN);
    //return (sn == expectedSN);
}
