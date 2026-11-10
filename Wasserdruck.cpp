// Wasserdruck-Rechner (Windows 11 / Win32-API, keine Abhängigkeiten)
//
// Eingaben:
//   1. Wasserdruck laut BWB (bar)
//   2. Höhe laut BWB (m über NN)
//   3. Neue Höhe (m über NN)
// Ausgabe:
//   Neuer Wasserdruck in bar (Format xx,xx)
//
// Formel: p_neu = p_BWB - (h_neu - h_BWB) * 0,0981 bar/m
//
// Compilieren (MSVC,  Developer-Eingabeaufforderung):
//   cl /W4 /EHsc /O2 Wasserdruck.cpp user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS
// oder (MinGW-w64):
//   g++ -O2 -Wall -o Wasserdruck.exe Wasserdruck.cpp -mwindows

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <wchar.h>
#include <stdio.h>

#define IDC_EDIT_DRUCK       101
#define IDC_EDIT_HOEHE_BWB   102
#define IDC_EDIT_HOEHE_NEU   103
#define IDC_EDIT_ERGEBNIS    104
#define IDC_BTN_BERECHNEN    105

static const double BAR_PRO_METER = 0.0981;

static void FontAufAlleSteuerelemente(HWND hwnd)
{
    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    for (int id = IDC_EDIT_DRUCK; id <= IDC_BTN_BERECHNEN; ++id)
    {
        HWND h = GetDlgItem(hwnd, id);
        if (h) SendMessage(h, WM_SETFONT, (WPARAM)font, TRUE);
    }
}

static bool ParseDezimal(HWND hEdit, const wchar_t* feldname,
                         double min, double max,
                         double& wert, wchar_t* fehler, size_t fehlerLen)
{
    wchar_t text[64] = L"";
    GetWindowText(hEdit, text, 64);

    wchar_t* p = text;
    while (*p == L' ' || *p == L'\t') ++p;
    wchar_t* e = p + wcslen(p);
    while (e > p && (e[-1] == L' ' || e[-1] == L'\t')) --e;
    *e = L'\0';

    if (*p == L'\0')
    {
        swprintf(fehler, fehlerLen, L"Bitte \"%s\" eingeben.", feldname);
        return false;
    }

    wchar_t buf[64] = L"";
    wcsncpy(buf, p, 63);
    for (wchar_t* q = buf; *q; ++q)
        if (*q == L',') *q = L'.';

    wchar_t* endp = nullptr;
    double v = wcstod(buf, &endp);
    if (endp == buf || *endp != L'\0')
    {
        swprintf(fehler, fehlerLen,
                 L"\"%s\": Bitte eine g\xfcltige Zahl eingeben (z. B. 4,20).",
                 feldname);
        return false;
    }
    if (v < min || v > max)
    {
        swprintf(fehler, fehlerLen,
                 L"\"%s\": Wert muss zwischen %.0f und %.0f liegen.",
                 feldname, min, max);
        return false;
    }

    wert = v;
    return true;
}

static void FormatiereBar(double v, wchar_t* out, size_t n)
{
    swprintf(out, n, L"%.2f", v);
    for (wchar_t* q = out; *q; ++q)
        if (*q == L'.') *q = L',';
}

static void Berechnen(HWND hwnd)
{
    const size_t N = 256;
    wchar_t fehler[N] = L"";

    double druck = 0.0, hoeheBwb = 0.0, hoeheNeu = 0.0;

    if (!ParseDezimal(GetDlgItem(hwnd, IDC_EDIT_DRUCK), L"Wasserdruck laut BWB (bar)",
                      0.0, 16.0, druck, fehler, N) ||
        !ParseDezimal(GetDlgItem(hwnd, IDC_EDIT_HOEHE_BWB), L"Höhe laut BWB (m)",
                      0.0, 10000.0, hoeheBwb, fehler, N) ||
        !ParseDezimal(GetDlgItem(hwnd, IDC_EDIT_HOEHE_NEU), L"Neue Höhe (m)",
                      0.0, 10000.0, hoeheNeu, fehler, N))
    {
        MessageBox(hwnd, fehler, L"Eingabefehler", MB_OK | MB_ICONWARNING);
        return;
    }

    double druckNeu = druck - (hoeheNeu - hoeheBwb) * BAR_PRO_METER;

    wchar_t druckNeuTxt[32] = L"";
    FormatiereBar(druckNeu, druckNeuTxt, 32);

    wchar_t ergebnis[N] = L"";
    swprintf(ergebnis, N, L"Neuer Wasserdruck: %s bar", druckNeuTxt);
    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_ERGEBNIS), ergebnis);

    if (druckNeu < 0.0)
    {
        MessageBox(hwnd,
                   L"Achtung: Der berechnete Druck ist negativ.\n"
                   L"Bitte Eingaben prüfen (z. B. Höhen vertauscht?).",
                   L"Warnung", MB_OK | MB_ICONWARNING);
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        const wchar_t* beschriftung[] = {
            L"Wasserdruck laut BWB (bar):",
            L"Höhe laut BWB (m):",
            L"Neue Höhe (m):",
            L"Ausgabe:"
        };
        const int editIds[] = {
            IDC_EDIT_DRUCK, IDC_EDIT_HOEHE_BWB, IDC_EDIT_HOEHE_NEU, IDC_EDIT_ERGEBNIS
        };

        int y = 20;
        for (int i = 0; i < 4; ++i)
        {
            CreateWindowW(L"STATIC", beschriftung[i],
                          WS_CHILD | WS_VISIBLE,
                          20, y, 220, 20, hwnd, NULL, NULL, NULL);
            DWORD stil = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL;
            if (i == 3) stil |= ES_READONLY;
            CreateWindowW(L"EDIT", L"",
                          stil,
                          250, y - 2, 130, 24, hwnd,
                          (HMENU)(INT_PTR)editIds[i], NULL, NULL);
            y += 35;
        }

        CreateWindowW(L"BUTTON", L"Berechnen",
                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                      250, y + 5, 130, 30, hwnd,
                      (HMENU)(INT_PTR)IDC_BTN_BERECHNEN, NULL, NULL);

        FontAufAlleSteuerelemente(hwnd);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_BTN_BERECHNEN && HIWORD(wp) == BN_CLICKED)
            Berechnen(hwnd);
        else if (LOWORD(wp) == 1 && HIWORD(wp) == 0) // Enter-Taste
            Berechnen(hwnd);
        return 0;

    case DM_GETDEFID:
        return MAKELRESULT(IDC_BTN_BERECHNEN, DC_HASDEFID);

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    SetProcessDPIAware();

    const wchar_t* KLASSE = L"WasserdruckRechner";
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = KLASSE;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(KLASSE, L"Wasserdruck-Rechner (BWB)",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                              CW_USEDEFAULT, CW_USEDEFAULT, 420, 240,
                              NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessage(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
