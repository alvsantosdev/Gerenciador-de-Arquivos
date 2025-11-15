#define UNICODE
#define _UNICODE
#define _WIN32_IE 0x0501
#include <stdlib.h>
#include <windows.h>
#include <wchar.h>
#include <shlobj.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h> 
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

// Define que o programa será compilado em modo UNICODE
#ifndef UNICODE
#define UNICODE
#endif

// Define o nome da classe da janela
#define NOME_CLASSE L"GERENCIADOR DE ARQUIVOS"
#define IDM_COMPARTILHAR_ARQ 101
#define IDM_ESCOLHER_DIR 9001
#define IDM_LISTVIEW 6001 // Corrigido o ID para maiúsculo
#define IDM_CRIAR_ARQUIVO 6002
#define IDM_DELETAR_ARQUIVO 6003

// variáveis globais
HMENU hMenu, hMenuBar;
HINSTANCE g_hInst = NULL;
HWND hwndList = NULL;
wchar_t currentDirectory[MAX_PATH] = L"";

// Protótipo da função CriarListView
HWND CriarListView(HWND hwndParent, HINSTANCE hInst);

void AtualizarListView(HWND hwndList, LPCWSTR dir);
BOOL CALLBACK DialogCriarArquivoProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Função para criar um novo arquivo
void CriarNovoArquivo(HWND hwndPai, LPCWSTR diretorio) {
    wchar_t nomeArquivo[256];
    if (DialogBoxW(g_hInst, MAKEINTRESOURCEW(101), hwndPai, (DLGPROC)DialogCriarArquivoProc) == IDOK) {
        GetDlgItemTextW(GetActiveWindow(), 100, nomeArquivo, 256);
        wchar_t caminhoCompleto[MAX_PATH];
        swprintf_s(caminhoCompleto, MAX_PATH, L"%s\\%s", diretorio, nomeArquivo);
        FILE* arquivo = _wfopen(caminhoCompleto, L"w");
        if (arquivo) {
            fclose(arquivo);
            AtualizarListView(hwndList, diretorio);
        } else {
            MessageBoxW(hwndPai, L"Erro ao criar o arquivo.", L"Erro", MB_OK | MB_ICONERROR);
        }
    }
}

// Dialog Procedure para a caixa de diálogo de criar arquivo
BOOL CALLBACK DialogCriarArquivoProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                EndDialog(hwndDlg, IDOK);
                return TRUE;
            } else if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }
            break;
        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

// Função para deletar o arquivo selecionado
void DeletarArquivoSelecionado(HWND hwndList, LPCWSTR diretorio) {
    int indiceSelecionado = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    if (indiceSelecionado != -1) {
        LVITEMW itemSelecionado;
        wchar_t nomeArquivo[MAX_PATH];
        itemSelecionado.mask = LVIF_TEXT;
        itemSelecionado.iItem = indiceSelecionado;
        itemSelecionado.iSubItem = 0;
        itemSelecionado.pszText = nomeArquivo;
        itemSelecionado.cchTextMax = MAX_PATH;

        if (ListView_GetItem(hwndList, &itemSelecionado)) {
            wchar_t caminhoCompleto[MAX_PATH];
            swprintf_s(caminhoCompleto, MAX_PATH, L"%s\\%s", diretorio, nomeArquivo);
            if (_wremove(caminhoCompleto) == 0) {
                AtualizarListView(hwndList, diretorio);
            } else {
                MessageBoxW(GetParent(hwndList), L"Erro ao deletar o arquivo.", L"Erro", MB_OK | MB_ICONERROR);
            }
        }
    } else {
        MessageBoxW(GetParent(hwndList), L"Nenhum arquivo selecionado para deletar.", L"Aviso", MB_OK | MB_ICONWARNING);
    }
}

void AtualizarListView(HWND hwndList, LPCWSTR dir) {
    ListView_DeleteAllItems(hwndList);

    WIN32_FIND_DATAW findFileData;
    wchar_t acharCaminho[MAX_PATH];
    HANDLE hFind;

    swprintf_s(acharCaminho, MAX_PATH, L"%s\\*", dir);

    hFind = FindFirstFileW(acharCaminho, &findFileData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(findFileData.cFileName, L".") != 0 && wcscmp(findFileData.cFileName, L"..") != 0) {
                wchar_t* nome = _wcsdup(findFileData.cFileName); // duplica string dinamicamente
                LVITEMW item = { 0 };
                item.mask = LVIF_TEXT;
                item.iItem = ListView_GetItemCount(hwndList);
                item.iSubItem = 0;
                item.pszText = nome;
                int index = ListView_InsertItem(hwndList, &item);
                free(nome); 
            }
        } while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
}

void SetView(HWND hWndListView, DWORD dwView) {
    DWORD dwStyle = GetWindowLong(hWndListView, GWL_STYLE);

    if ((dwStyle & LVS_TYPEMASK) != dwView) {
        SetWindowLong(hWndListView,
            GWL_STYLE,
            (dwStyle & ~LVS_TYPEMASK) | dwView);
    }
}

wchar_t* abirPasta(HWND hwnd) {
    BROWSEINFOW bi = { 0 };
    ITEMIDLIST* pidl;
    wchar_t szPath[MAX_PATH];

    bi.hwndOwner = hwnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = szPath;
    bi.lpszTitle = L"Selecione a pasta para exibir os arquivos";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = NULL;
    bi.lParam = 0;
    bi.iImage = 0;

    pidl = SHBrowseForFolderW(&bi);

    if (pidl != NULL) {
        if (SHGetPathFromIDListW(pidl, szPath)) {
            wchar_t* caminhoSelecionado = _wcsdup(szPath);
            CoTaskMemFree(pidl);
            return caminhoSelecionado;
        }
        CoTaskMemFree(pidl);
    }

    return NULL;
}

// Função para criar e inicializar um controle ListView
HWND CriarListView(HWND hwndParent, HINSTANCE hInst) {
    // Cria a janela do controle ListView
    HWND hwndList = CreateWindowExW(
        0,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
        0, 0, 0, 0,
        hwndParent,
        (HMENU)IDM_LISTVIEW,
        hInst,
        NULL
    );

    if (hwndList == NULL) {
        MessageBoxW(hwndParent, L"Falha ao criar o controle ListView.", L"Erro", MB_OK | MB_ICONERROR);
        return NULL; // Retorna NULL em caso de erro
    }

    // Define as colunas do ListView
    LVCOLUMNW col;
    memset(&col, 0, sizeof(LVCOLUMNW));
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    // Coluna 0: Nome
    col.cx = 200;
    col.pszText = L"Nome";
    ListView_InsertColumn(hwndList, 0, &col);

    // Coluna 1: Tipo
    col.cx = 100;
    col.pszText = L"Tipo";
    ListView_InsertColumn(hwndList, 1, &col);

    return hwndList;
}

// Função de callback para processar mensagens da janela
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    wchar_t nomeApp[] = L"Gerenciador de arquivos";
    int qtdC = wcslen(nomeApp);
    PAINTSTRUCT ps;

    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;

        case WM_PAINT:
        {
            hdc = BeginPaint(hwnd, &ps);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CREATE:
            hMenuBar = CreateMenu();
            hMenu = CreateMenu();

            // Cria o controle ListView usando a função CriarListView
            hwndList = CriarListView(hwnd, g_hInst);
            if (hwndList == NULL) {
                return -1; // Encerra a janela se o ListView não for criado
            }

            AppendMenuW(hMenu, MF_STRING, IDM_CRIAR_ARQUIVO, L"Criar Arquivo");
            AppendMenuW(hMenu, MF_STRING, IDM_DELETAR_ARQUIVO, L"Deletar Arquivo");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, IDM_ESCOLHER_DIR, L"Escolher local");
            AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hMenu, L"Arquivo");
            SetMenu(hwnd, hMenuBar);

            // Define o diretório inicial
            wcscpy_s(currentDirectory, MAX_PATH, L".");
            AtualizarListView(hwndList, currentDirectory);
            break;
        case WM_SIZE:
            if (hwndList != NULL) {
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                MoveWindow(hwndList, 0, 0, rcClient.right, rcClient.bottom, TRUE);
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_ESCOLHER_DIR: {
                    wchar_t* selecionarPasta = abirPasta(hwnd);
                    if (selecionarPasta != NULL) {
                        wcscpy_s(currentDirectory, MAX_PATH, selecionarPasta);
                        AtualizarListView(hwndList, currentDirectory);
                        free(selecionarPasta);
                    } else {
                        MessageBoxW(NULL, L"Nenhuma pasta selecionada.", L"Aviso", MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                }
                case IDM_CRIAR_ARQUIVO:
                    if (*currentDirectory != L'\0') {
                        CriarNovoArquivo(hwnd, currentDirectory);
                    } else {
                        MessageBoxW(hwnd, L"Selecione um diretório primeiro.", L"Aviso", MB_OK | MB_ICONWARNING);
                    }
                    break;
                case IDM_DELETAR_ARQUIVO:
                    if (*currentDirectory != L'\0') {
                        DeletarArquivoSelecionado(hwndList, currentDirectory);
                    } else {
                        MessageBoxW(hwnd, L"Selecione um diretório primeiro.", L"Aviso", MB_OK | MB_ICONWARNING);
                    }
                    break;
            }
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int registrarClasse(HINSTANCE hInstance, LPWSTR lpzClassName) {
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEXW));

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512));
    wc.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = lpzClassName;
    wc.hIconSm = LoadIconW(NULL, MAKEINTRESOURCEW(32512));

    if (!RegisterClassExW(&wc)) {
        DWORD error = GetLastError();
        LPWSTR lpErrorMessage = NULL;

        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpErrorMessage,
            0,
            NULL
        );

        if (lpErrorMessage) {
            MessageBoxW(NULL, lpErrorMessage, L"Erro ao Registrar Classe", MB_OK | MB_ICONERROR);
            LocalFree(lpErrorMessage);
        }
        return 0;
    }
    return 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, PSTR cmdline, int nCmdShow) {
    g_hInst = hInstance;
    // Inicializa a Common Controls Library
    InitCommonControls();

    if (!registrarClasse(hInstance, NOME_CLASSE)) {
        return EXIT_FAILURE;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES | WS_EX_RIGHTSCROLLBAR,
        NOME_CLASSE,
        L"Gerenciador",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        MessageBoxW(NULL, L"Falha ao criar janela", L"Erro", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
