#include <windows.h>
#include <stdio.h>
#include <string.h>

#define __VERSION__ 0.1

#define LOG(fmt, ...) do{char _buffer[16380];sprintf_s(_buffer,16380, fmt, __VA_ARGS__);OutputDebugString(_buffer);}while(0);

#define IDI_TRAY 101
#define ID_TRAY_MENU 500

#define ID_ADD 900
#define ID_RELOAD 901
#define ID_ABOUT 910
#define ID_EXIT 911
#define ID_CHECKED 912

#define ID_OFFSET 1000

#define CHECK   1 
#define UNCHECK 2 

#define WM_TRAY (WM_USER + 100)  
#define WM_TASKBAR_CREATED RegisterWindowMessage(TEXT("Hoststray"))  

#define APP_TITLE    TEXT("hoststray v0.1")  
#define APP_TIP     TEXT("����һ�����޸�ϵͳhosts��С����\r\n��Ŀ��ַ��https://github.com/LaoQi/hoststray.git")

#define HOSTS_MAX 255
#define HOSTS_FILE "C:\\Windows\\System32\\drivers\\etc\\hosts"
#define HOSTS_FILE_BACKUP "C:\\Windows\\System32\\drivers\\etc\\hosts.bak"

//ֻ֧��ipv4
char AddrChar[13] = "0123456789.:";

typedef struct AddressNode {
    char addr[20];
    char domain[255];
    char title[512];
    BOOL is_checked;
    int id;
} ANode;

static ANode Hosts[HOSTS_MAX];
static size_t hosts_top = 0;

BOOL VerfiyAddr(const char* addr) {
    size_t length = strlen(addr);
    size_t clen = strlen(AddrChar);
    for (size_t i = 0; i < length; i++) {
        BOOL is_verfiy = FALSE;
        for (size_t j = 0; j < clen; j++) {
            if (addr[i] == AddrChar[j]) {
                is_verfiy = TRUE;
                break;
            }
        }
        if (!is_verfiy) {
            return FALSE;
        }
    }
    return TRUE;
}

int ProcessHostsLine(const char* line) {
    size_t length = strlen(line);
    if (length < 5) {
        return 0;
    }
    size_t i = 0;
    BOOL is_checked;
    BOOL addr_ok = FALSE;
    BOOL domain_ok = FALSE;
    char addr[20];
    size_t addr_cur = 0;
    size_t domain_cur = 0;
    char domain[255];
    if (line[0] == '#') {
        is_checked = FALSE;
    } else {
        is_checked = TRUE;
    }
    for (i = 0; i < length; i++) {
        char ch = line[i];
        if (ch == '#') {
            continue;
        }
        if ((ch == ' ' || ch == '\t') && !addr_ok && addr_cur > 0) {
            addr[addr_cur] = '\0';

            if (!VerfiyAddr(addr)) {
                return 0;
            }
            addr_ok = TRUE;
        } else if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
            continue;
        } else if (!addr_ok) {
            addr[addr_cur++] = ch;
        } else if (addr_ok) {
            domain[domain_cur++] = ch;
        }
    }
    if (domain_cur > 0) {
        domain[domain_cur] = '\0';
        domain_ok = TRUE;
    }
    if (addr_ok && domain_ok) {
        strcpy_s(Hosts[hosts_top].addr, sizeof(addr) + 1, addr);
        strcpy_s(Hosts[hosts_top].domain, sizeof(domain) + 1, domain);
        sprintf_s(Hosts[hosts_top].title, 512, "%s - %s", addr, domain);
        Hosts[hosts_top].is_checked = is_checked;
        Hosts[hosts_top].id = hosts_top + ID_OFFSET;
        LOG("%s\n", Hosts[hosts_top].title);
        hosts_top++;
    }
    return 0;
}

int LoadHosts() {
    hosts_top = 0;
    FILE * fp;
    errno_t err;
    if ((err = fopen_s(&fp, HOSTS_FILE, "r")) != 0) 
    {
        LOG("error!");
        return -1;
    }
    char line[1024];
    while (!feof(fp)) {
        if (NULL != fgets(line, 10240, fp)) {
            ProcessHostsLine(line);
        }       
    }
    return fclose(fp);
}

int DumpHosts() {
    FILE * fp;
    errno_t err;
    if ((err = fopen_s(&fp, HOSTS_FILE, "w")) != 0) {
        LOG("error!");
        return -1;
    }
    for (size_t i = 0; i < hosts_top; i++) {
        if (Hosts[i].is_checked) {
            fprintf_s(fp, "%s %s\n", Hosts[i].addr, Hosts[i].domain);
        } else {
            fprintf_s(fp, "# %s %s\n", Hosts[i].addr, Hosts[i].domain);
        }
    }
    return fclose(fp);
}

int BackupHosts() {
    if (access(HOSTS_FILE_BACKUP, 0)) {
        LOG("alread backup \n");
        return 0;
    }
    FILE * sourcefile;
    FILE * desfile;

    char c;
    errno_t err;
    if ((err = fopen_s(&sourcefile, HOSTS_FILE, "r")) != 0) {
        LOG("%s read error\n", HOSTS_FILE);
        return -1;
    }
    if ((err = fopen_s(&desfile, HOSTS_FILE_BACKUP, "w")) != 0) {
        LOG("%s read error\n", HOSTS_FILE_BACKUP);
        fclose(sourcefile);
        return -1;
    }
    while ((c = fgetc(sourcefile)) != EOF) {
        fputc(fgetc(sourcefile), desfile);
    }
    fclose(sourcefile);
    fclose(desfile);
    return 0;
}

NOTIFYICONDATA nid; 

void InitTray(HINSTANCE hInstance, HWND hWnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = IDI_TRAY;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    nid.uCallbackMessage = WM_TRAY;
    //nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TRAY));
    HMODULE shell32 = LoadLibraryEx("shell32.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
    nid.hIcon = LoadIcon(shell32, MAKEINTRESOURCE(14));
    lstrcpy(nid.szTip, APP_TITLE);
    Shell_NotifyIcon(NIM_ADD, &nid);
}

HBITMAP WINAPI CreateMenuBitmap(int type) {
    // Create a DC compatible with the desktop window's DC. 

    HWND hwndDesktop = GetDesktopWindow();
    HDC hdcDesktop = GetDC(hwndDesktop);
    HDC hdcMem = CreateCompatibleDC(hdcDesktop);

    // Determine the required bitmap size. 

    SIZE size = { GetSystemMetrics(SM_CXMENUCHECK),
        GetSystemMetrics(SM_CYMENUCHECK) };

    // Create a monochrome bitmap and select it. 

    HBITMAP hbm = CreateBitmap(size.cx/2, size.cy/2, 1, 1, NULL);
    HBITMAP hbmOld = SelectObject(hdcMem, hbm);

    // Erase the background and call the drawing function. 

    if (type == CHECK) {
        PatBlt(hdcMem, 0, 0, size.cx / 2, size.cy / 2, BLACKNESS);
    } else {
        PatBlt(hdcMem, 0, 0, size.cx / 2, size.cy / 2, WHITENESS);
    }
    //(*lpfnDraw)(hdcMem, size);

    // Clean up. 

    SelectObject(hdcMem, hbmOld);
    DeleteDC(hdcMem);
    ReleaseDC(hwndDesktop, hdcDesktop);
    return hbm;
}


BOOL ShowPopupMenu(HWND hWnd, POINT *curpos) {
    HMENU hMenu = CreatePopupMenu();

    HBITMAP hbmpCheck;   // handle to checked bitmap    
    hbmpCheck = CreateMenuBitmap(CHECK);
 
    // @TODO ����
    //AppendMenu(hMenu, MF_STRING, ID_ADD, TEXT("����"));
    AppendMenu(hMenu, MF_STRING, ID_RELOAD, TEXT("���¼���"));
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    for (size_t i = 0; i < hosts_top; i++) {
        AppendMenu(hMenu, MF_STRING, Hosts[i].id, Hosts[i].title);
        SetMenuItemBitmaps(hMenu, Hosts[i].id, MF_BYCOMMAND, NULL, hbmpCheck);
        if (Hosts[i].is_checked) {
            CheckMenuItem(hMenu, Hosts[i].id, MF_BYCOMMAND | MF_CHECKED);
        }
    }
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    //AppendMenu(hMenu, MF_STRING, ID_ABOUT, TEXT("����"));
    AppendMenu(hMenu, MF_STRING, ID_EXIT, TEXT("�˳�"));
    //EnableMenuItem(hMenu, ID_ABOUT, MF_GRAYED);
    //SetMenuItemBitmaps(hMenu, 1001, MF_BYCOMMAND, NULL, hbmpCheck);
    {
        /* Display the context menu */
        WORD cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, curpos->x, curpos->y, 0, hWnd, NULL);
        /*Send callback message to the application handle (window) */
        SendMessage(hWnd, ID_TRAY_MENU, cmd, 0);
    }
    DestroyMenu(hMenu);

    return 0;
}

void DispatchMenu(HWND hWnd, int cmd) {
    size_t cur = cmd - ID_OFFSET;
    if (cur > hosts_top || cur > HOSTS_MAX) {
        return;
    }
    Hosts[cur].is_checked = !Hosts[cur].is_checked;
    DumpHosts();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TRAY:
            switch (lParam) {
                case WM_RBUTTONDOWN:
                {
                    POINT pt; GetCursorPos(&pt);
                    SetForegroundWindow(hWnd); 
                    ShowPopupMenu(hWnd, &pt);
                }
                break;
                case WM_LBUTTONDOWN:
                    MessageBox(hWnd, APP_TIP, APP_TITLE, MB_OK);
                    break;
                case WM_LBUTTONDBLCLK:
                    break;
            }
            break;
        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;
        case ID_TRAY_MENU:
        {
            int cmd = (int)wParam;
            switch (cmd) {
                case ID_RELOAD:
                    LoadHosts();
                    break;
                case ID_ADD:
                    MessageBox(hWnd, TEXT("û��"), APP_TITLE, MB_OK);
                    break;
                case ID_ABOUT:
                    MessageBox(hWnd, APP_TIP, APP_TITLE, MB_OK);
                    break;
                case ID_EXIT:
                    PostMessage(hWnd, WM_DESTROY, NULL, NULL);
                    break;
                default:
                    DispatchMenu(hWnd, cmd);
                    break;
            }
        }
        break;
    }
    if (uMsg == WM_TASKBAR_CREATED) {
        Shell_NotifyIcon(NIM_ADD, &nid);
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int iCmdShow) {
    HWND hWnd;
    MSG msg;
    WNDCLASS wc = { 0 };
    wc.style = NULL;
    wc.hIcon = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = APP_TITLE;
    wc.hCursor = NULL;

    if (!RegisterClass(&wc)) return 0;

    hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, APP_TITLE, APP_TITLE, WS_POPUP, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    BackupHosts();
    LoadHosts();
    InitTray(hInstance, hWnd); 

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}