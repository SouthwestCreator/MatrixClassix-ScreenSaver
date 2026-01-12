#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>
#include "resource.h" 

// --- Modifiable Globals ---
int g_TimerInterval = 30;
int g_FontSizeScroll = 20;
int g_FontSizeDecode = 70;
int g_TrailLength = 15;
int g_DecodeDuration = 6000;

COLORREF g_ColorHead = RGB(180, 255, 180);
COLORREF g_ColorBody = RGB(0, 255, 0);
COLORREF g_ColorDecode = RGB(255, 50, 50);
wchar_t g_DecodeText[256] = L"Your Text Here";
int g_UserSpeed = 5;

// --- Logic Globals ---
enum Phase { SCROLLING, MERGING, DECODING, SEPARATING };
struct TrailPart { double x, y; wchar_t ch; };
struct CharDrop {
    double x, y;
    double targetX, targetY;
    int speed;
    wchar_t ch;
    bool isDecodeChar;
    std::vector<TrailPart> trail;
    int glitchTimer;
};

std::vector<CharDrop> drops;
int screenWidth, screenHeight;
Phase currentPhase = SCROLLING;
DWORD phaseStartTime;
HFONT hFontScroll = NULL;
HFONT hFontDecode = NULL;
POINT lastMousePos = { -1, -1 };

// --- Registry Functions ---
void SaveSettings() {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\MatrixSaver", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, L"DecodeText", 0, REG_SZ, (BYTE*)g_DecodeText, (lstrlen(g_DecodeText) + 1) * sizeof(wchar_t));
        RegSetValueEx(hKey, L"MatrixColor", 0, REG_DWORD, (BYTE*)&g_ColorBody, sizeof(DWORD));
        RegSetValueEx(hKey, L"DecodeColor", 0, REG_DWORD, (BYTE*)&g_ColorDecode, sizeof(DWORD));
        RegSetValueEx(hKey, L"Speed", 0, REG_DWORD, (BYTE*)&g_UserSpeed, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

void LoadSettings() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\MatrixSaver", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwSize = sizeof(g_DecodeText);
        RegQueryValueEx(hKey, L"DecodeText", NULL, NULL, (BYTE*)g_DecodeText, &dwSize);
        DWORD dwColSize = sizeof(DWORD);
        RegQueryValueEx(hKey, L"MatrixColor", NULL, NULL, (BYTE*)&g_ColorBody, &dwColSize);
        RegQueryValueEx(hKey, L"DecodeColor", NULL, NULL, (BYTE*)&g_ColorDecode, &dwColSize);
        DWORD dwSpeedSize = sizeof(DWORD);
        RegQueryValueEx(hKey, L"Speed", NULL, NULL, (BYTE*)&g_UserSpeed, &dwSpeedSize);
        RegCloseKey(hKey);
    }
}

// --- Settings Dialog Logic ---
INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDC_DECODE_TEXT, g_DecodeText);
        SetDlgItemInt(hDlg, IDC_SCROLL_SPEED, g_UserSpeed, FALSE);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            GetDlgItemText(hDlg, IDC_DECODE_TEXT, g_DecodeText, 256);
            g_UserSpeed = GetDlgItemInt(hDlg, IDC_SCROLL_SPEED, NULL, FALSE);
            SaveSettings();
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDC_COLOR_MATRIX || LOWORD(wParam) == IDC_COLOR_DECODE) {
            CHOOSECOLOR cc = { 0 };
            static COLORREF acrCustClr[16];
            cc.lStructSize = sizeof(cc);
            cc.hwndOwner = hDlg;
            cc.lpCustColors = (LPDWORD)acrCustClr;
            cc.rgbResult = (LOWORD(wParam) == IDC_COLOR_MATRIX) ? g_ColorBody : g_ColorDecode;
            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
            if (ChooseColor(&cc)) {
                if (LOWORD(wParam) == IDC_COLOR_MATRIX) g_ColorBody = cc.rgbResult;
                else g_ColorDecode = cc.rgbResult;
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// --- Animation logic ---
void InitDrops() {
    int cols = screenWidth / g_FontSizeScroll;
    if (cols <= 0) cols = 1;
    drops.clear();
    for (int i = 0; i < cols * 2; ++i) {
        CharDrop drop;
        drop.x = (double)((i % cols) * g_FontSizeScroll);
        drop.y = (double)(rand() % screenHeight);
        drop.speed = (rand() % 3 + g_UserSpeed);
        drop.ch = (wchar_t)(33 + rand() % 94);
        drop.isDecodeChar = false;
        drop.glitchTimer = 0;
        drop.targetX = 0; drop.targetY = 0;
        drops.push_back(drop);
    }
}

void StartMerging() {
    currentPhase = MERGING;
    phaseStartTime = GetTickCount();
    int textLength = (int)wcslen(g_DecodeText);
    double centerX = (double)(screenWidth - (int)(g_FontSizeDecode * textLength * 0.6)) / 2.0;
    double centerY = (double)(screenHeight - g_FontSizeDecode) / 2.0;
    for (size_t i = 0; i < drops.size(); ++i) {
        if (i < (size_t)textLength) {
            drops[i].targetX = centerX + (i * (g_FontSizeDecode * 0.6));
            drops[i].targetY = centerY;
            drops[i].isDecodeChar = true;
            drops[i].glitchTimer = rand() % 50;
        }
        else {
            drops[i].targetX = (double)(rand() % screenWidth);
            drops[i].targetY = (double)(rand() % screenHeight);
            drops[i].isDecodeChar = false;
        }
    }
}

void DrawFrame(HDC hdc) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
    HGDIOBJ hOldObj = SelectObject(hdcMem, hbmMem);
    RECT rect = { 0, 0, screenWidth, screenHeight };
    FillRect(hdcMem, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetBkMode(hdcMem, TRANSPARENT);

    for (auto& drop : drops) {
        if (currentPhase == MERGING || currentPhase == SEPARATING) {
            drop.x += (drop.targetX - drop.x) * 0.1;
            drop.y += (drop.targetY - drop.y) * 0.1;
            drop.trail.clear();
        }
        else {
            if (!drop.isDecodeChar || currentPhase == SCROLLING) {
                drop.y += drop.speed;
                if (drop.y > screenHeight + (g_TrailLength * g_FontSizeScroll))
                    drop.y = -(double)g_FontSizeScroll;
                if (drop.trail.empty() || abs(drop.y - drop.trail[0].y) >= g_FontSizeScroll) {
                    TrailPart p = { drop.x, drop.y, drop.ch };
                    drop.trail.insert(drop.trail.begin(), p);
                    if (drop.trail.size() > (size_t)g_TrailLength) drop.trail.pop_back();
                    drop.ch = (wchar_t)(33 + rand() % 94);
                }
            }
        }
        if (drop.isDecodeChar && (currentPhase == MERGING || currentPhase == DECODING)) {
            SelectObject(hdcMem, hFontDecode);
            wchar_t displayChar = g_DecodeText[(&drop - &drops[0])];
            if (currentPhase == MERGING || (currentPhase == DECODING && drop.glitchTimer > 0)) {
                displayChar = (wchar_t)(33 + rand() % 94);
                if (currentPhase == DECODING) drop.glitchTimer--;
                SetTextColor(hdcMem, g_ColorBody);
            }
            else {
                SetTextColor(hdcMem, g_ColorDecode);
            }
            TextOut(hdcMem, (int)drop.x, (int)drop.y, &displayChar, 1);
        }
        else {
            SelectObject(hdcMem, hFontScroll);
            for (size_t i = 0; i < drop.trail.size(); ++i) {
                int intensity = 255 - (int)(i * (255 / g_TrailLength));
                if (intensity < 30) intensity = 30;
                SetTextColor(hdcMem, RGB((GetRValue(g_ColorBody) * intensity) / 255, (GetGValue(g_ColorBody) * intensity) / 255, (GetBValue(g_ColorBody) * intensity) / 255));
                TextOut(hdcMem, (int)drop.trail[i].x, (int)drop.trail[i].y, &drop.trail[i].ch, 1);
            }
            SetTextColor(hdcMem, g_ColorHead);
            TextOut(hdcMem, (int)drop.x, (int)drop.y, &drop.ch, 1);
        }
    }
    BitBlt(hdc, 0, 0, screenWidth, screenHeight, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOldObj); DeleteObject(hbmMem); DeleteDC(hdcMem);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        hFontScroll = CreateFont(g_FontSizeScroll, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, FIXED_PITCH, L"Consolas");
        hFontDecode = CreateFont(g_FontSizeDecode, 0, 0, 0, FW_EXTRABOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, FIXED_PITCH, L"Consolas");
        SetTimer(hwnd, 1, g_TimerInterval, NULL);
        phaseStartTime = GetTickCount();
        break;
    case WM_TIMER: {
        DWORD elapsed = GetTickCount() - phaseStartTime;
        if (currentPhase == SCROLLING && elapsed > 8000) StartMerging();
        else if (currentPhase == MERGING && elapsed > 3000) { currentPhase = DECODING; phaseStartTime = GetTickCount(); }
        else if (currentPhase == DECODING && elapsed > (DWORD)g_DecodeDuration) {
            currentPhase = SEPARATING; phaseStartTime = GetTickCount();
            for (auto& d : drops) { d.targetX = (double)(rand() % screenWidth); d.targetY = (double)(rand() % screenHeight); d.isDecodeChar = false; }
        }
        else if (currentPhase == SEPARATING && elapsed > 3000) { currentPhase = SCROLLING; phaseStartTime = GetTickCount(); }
        InvalidateRect(hwnd, NULL, FALSE);
    } break;
    case WM_PAINT: { PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); DrawFrame(hdc); EndPaint(hwnd, &ps); } break;
    case WM_MOUSEMOVE: {
        POINT pt; GetCursorPos(&pt);
        if (lastMousePos.x == -1) lastMousePos = pt;
        else if (abs(lastMousePos.x - pt.x) > 10 || abs(lastMousePos.y - pt.y) > 10) PostQuitMessage(0);
    } break;
    case WM_KEYDOWN: case WM_LBUTTONDOWN: PostQuitMessage(0); break;
    case WM_DESTROY:
        if (hFontScroll) DeleteObject(hFontScroll);
        if (hFontDecode) DeleteObject(hFontDecode);
        KillTimer(hwnd, 1); PostQuitMessage(0);
        break;
    default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// --- Main Entry Point (The Fix) ---
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    LoadSettings();

    // 1. Get full command line (always populated by Windows)
    std::wstring cmd = GetCommandLineW();

    // 2. Standardize to lowercase
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::towlower);

    // 3. Search for Configure Mode
    if (cmd.find(L"/c") != std::wstring::npos) {
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, SettingsDlgProc);
        return 0;
    }

    // 4. Handle Preview Mode (Exit early)
    if (cmd.find(L"/p") != std::wstring::npos) {
        return 0;
    }

    // 5. Default to Animation (Run/Test Mode)
    srand((unsigned)time(NULL));
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = NULL;
    wc.lpszClassName = L"MatrixSaver";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    if (!RegisterClass(&wc)) return 0;

    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST, L"MatrixSaver", L"Matrix",
        WS_POPUP | WS_VISIBLE, 0, 0,
        screenWidth, screenHeight, NULL, NULL, hInstance, NULL);

    InitDrops();
    ShowCursor(FALSE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}