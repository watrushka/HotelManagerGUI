#include <windows.h>
#include <commctrl.h>
#include <string>
#include "Hotel.h"
#include "Client.h"
#include "Room.h"
#include <fstream>
#include "resource.h"
#include "utils.h"  // <-- обязательно

#pragma comment(lib, "comctl32.lib")

Hotel g_hotel;
HWND hListView;
HINSTANCE g_hInst;
static bool sortPriceAsc = true;
static bool sortTypeAsc = true;
static bool sortIdAsc = true;


void updateListView(HWND hListView, Hotel& g_hotel) {
    ListView_DeleteAllItems(hListView);
    int idx = 0;

    for (auto& r : g_hotel.getRooms()) {
        wchar_t bufId[16], bufType[32], bufPrice[32];
        _snwprintf_s(bufId, _countof(bufId), _TRUNCATE, L"%d", r.getId());
        wcsncpy_s(bufType, roomTypeToString(r.getType()).c_str(), _TRUNCATE);
        _snwprintf_s(bufPrice, _countof(bufPrice), _TRUNCATE, L"%.2f", r.getPrice());

        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = idx;

        // lParam = отрицательный ID комнаты
        item.lParam = -(r.getId() + 1);

        // Если комната занята клиентом, фамилия будет отображена
        std::wstring surname = L"";
        int days = 0;
        double cost = 0;

        for (size_t i = 0; i < g_hotel.getClients().size(); i++) {
            const auto& c = g_hotel.getClients()[i];
            if (c.getRoomId() == r.getId()) {
                surname = c.getSurname();
                days = c.getDays();
                cost = c.calcCost(r.getPrice());
                // Привязываем lParam к клиенту, чтобы можно было редактировать/удалять
                item.lParam = (int)i;
                break;
            }
        }

        wchar_t bufSurname[256];
        wcsncpy_s(bufSurname, surname.c_str(), _TRUNCATE);
        item.pszText = bufSurname;
        ListView_InsertItem(hListView, &item);

        ListView_SetItemText(hListView, idx, 1, bufId);
        ListView_SetItemText(hListView, idx, 2, bufType);
        _snwprintf_s(bufPrice, _countof(bufPrice), _TRUNCATE, L"%.2f", r.getPrice());
        ListView_SetItemText(hListView, idx, 3, bufPrice);

        wchar_t bufDays[16];
        _snwprintf_s(bufDays, _countof(bufDays), _TRUNCATE, L"%d", days);
        ListView_SetItemText(hListView, idx, 4, bufDays);

        wchar_t bufCost[32];
        _snwprintf_s(bufCost, _countof(bufCost), _TRUNCATE, L"%.2f", cost);
        ListView_SetItemText(hListView, idx, 5, bufCost);

        idx++;
    }
}



// ---------------- Диалог добавления / редактирования комнаты ----------------
INT_PTR CALLBACK AddRoomDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
    {
        // Ограничение длины ввода
        SendMessage(GetDlgItem(hDlg, IDC_EDIT_ROOM_ID), EM_LIMITTEXT, 3, 0);
        SendMessage(GetDlgItem(hDlg, IDC_EDIT_ROOM_PRICE), EM_LIMITTEXT, 6, 0);

        // Инициализация комбобокса типов
        HWND combo = GetDlgItem(hDlg, IDC_COMBO_ROOM_TYPE);
        SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)L"Single");
        SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)L"Double");
        SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)L"Lux");
        SendMessage(combo, CB_SETCURSEL, 0, 0);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        {
            wchar_t buf[50];

            // ID
            GetDlgItemText(hDlg, IDC_EDIT_ROOM_ID, buf, 50);
            if (wcslen(buf) == 0) {
                MessageBox(hDlg, L"Введите ID комнаты!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            int id = _wtoi(buf);
            if (id <= 0) {
                MessageBox(hDlg, L"ID комнаты должен быть положительным числом!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            // PRICE
            GetDlgItemText(hDlg, IDC_EDIT_ROOM_PRICE, buf, 50);
            if (wcslen(buf) == 0) {
                MessageBox(hDlg, L"Введите цену!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            double price = _wtof(buf);
            if (price <= 0.0) {
                MessageBox(hDlg, L"Цена должна быть больше 0!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            // TYPE
            HWND combo = GetDlgItem(hDlg, IDC_COMBO_ROOM_TYPE);
            int typeIndex = SendMessage(combo, CB_GETCURSEL, 0, 0);
            if (typeIndex < 0) {
                MessageBox(hDlg, L"Выберите тип комнаты!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            RoomType type = (RoomType)typeIndex;

            // ADD ROOM
            if (!g_hotel.addRoom(Room(id, price, type))) {
                MessageBox(hDlg, L"Комната с таким ID уже существует!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            // Обновляем ListView после успешного добавления
            updateListView(hListView, g_hotel);

            EndDialog(hDlg, IDOK);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}



// ---------------- Диалог добавления / редактирования клиента ----------------
INT_PTR CALLBACK ClientDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static int editIndex = -1; // индекс редактируемого клиента, -1 если новый
    switch (message) {
    case WM_INITDIALOG:
    {
        editIndex = (int)lParam;
        // Ограничения на длину ввода
        SendMessage(GetDlgItem(hDlg, IDC_EDIT_SURNAME), EM_LIMITTEXT, 30, 0);
        SendMessage(GetDlgItem(hDlg, IDC_EDIT_ID), EM_LIMITTEXT, 3, 0);
        SendMessage(GetDlgItem(hDlg, IDC_EDIT_DAYS), EM_LIMITTEXT, 2, 0);

        // Если редактирование
        if (editIndex >= 0) {
            const Client& c = g_hotel.getClients()[editIndex];
            SetDlgItemText(hDlg, IDC_EDIT_SURNAME, c.getSurname().c_str());
            SetDlgItemInt(hDlg, IDC_EDIT_ID, c.getRoomId(), FALSE);
            SetDlgItemInt(hDlg, IDC_EDIT_DAYS, c.getDays(), FALSE);
        }
        else {
            editIndex = -1; // новый клиент
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        {
            wchar_t buf[50];

            // Фамилия
            GetDlgItemText(hDlg, IDC_EDIT_SURNAME, buf, 50);
            if (wcslen(buf) == 0) {
                MessageBox(hDlg, L"Введите фамилию клиента!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            std::wstring surname(buf);

            // ID комнаты
            GetDlgItemText(hDlg, IDC_EDIT_ID, buf, 50);
            if (wcslen(buf) == 0) {
                MessageBox(hDlg, L"Введите ID комнаты!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            int id = _wtoi(buf);
            if (!g_hotel.isRoomExists(id)) { // проверка существования комнаты
                MessageBox(hDlg, L"Такой комнаты не существует!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            // Кол-во дней
            GetDlgItemText(hDlg, IDC_EDIT_DAYS, buf, 50);
            if (wcslen(buf) == 0) {
                MessageBox(hDlg, L"Введите количество дней!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }
            int days = _wtoi(buf);
            if (days <= 0) {
                MessageBox(hDlg, L"Количество дней должно быть больше 0!", L"Ошибка", MB_OK | MB_ICONERROR);
                return TRUE;
            }

            // Создаем или обновляем клиента
            Client c(surname, id, days);
            if (editIndex >= 0) {
                // Редактирование клиента
                int oldRoomId = g_hotel.getClients()[editIndex].getRoomId();

                if (id != oldRoomId) { // клиент меняет комнату
                    if (!g_hotel.isRoomFree(id)) {
                        MessageBox(hDlg, L"Нельзя переместить клиента в занятую комнату!", L"Ошибка", MB_OK | MB_ICONERROR);
                        return TRUE;
                    }

                    // Освобождаем старую комнату
                    Room* oldRoom = g_hotel.getRoomById(oldRoomId);
                    if (oldRoom) oldRoom->vacate();

                    // Занимаем новую комнату
                    Room* newRoom = g_hotel.getRoomById(id);
                    if (newRoom) newRoom->occupy();
                }

                // Обновляем данные клиента
                g_hotel.updateClient(editIndex, Client(surname, id, days));
            }
            else {
                // Новый клиент
                if (!g_hotel.registerClient(surname, id, days)) {
                    MessageBox(hDlg, L"Номер занят!", L"Ошибка", MB_OK | MB_ICONERROR);
                    return TRUE;
                }
            }



            // Обновляем ListView
            updateListView(hListView, g_hotel);

            EndDialog(hDlg, IDOK);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}


// ---------------- WndProc ----------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
    {
        // ---------------- Верхняя панель ----------------

        // "Добавить комнату" — самая первая слева
        CreateWindow(L"BUTTON", L"Добавить комнату", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 10, 150, 30, hWnd, (HMENU)1, g_hInst, nullptr);

        // "Добавить Клиента" — справа от кнопки "Добавить комнату"
        CreateWindow(L"BUTTON", L"Добавить Клиента", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            170, 10, 150, 30, hWnd, (HMENU)2, g_hInst, nullptr);

        // Редактировать
        CreateWindow(L"BUTTON", L"Редактировать", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            330, 10, 120, 30, hWnd, (HMENU)3, g_hInst, nullptr);

        // Удалить
        CreateWindow(L"BUTTON", L"Удалить", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            460, 10, 100, 30, hWnd, (HMENU)4, g_hInst, nullptr);

        // Сохранить
        CreateWindow(L"BUTTON", L"Сохранить", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            570, 10, 100, 30, hWnd, (HMENU)5, g_hInst, nullptr);

        // Загрузить
        CreateWindow(L"BUTTON", L"Загрузить", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            680, 10, 100, 30, hWnd, (HMENU)6, g_hInst, nullptr);




        // ---------------- Нижняя панель (сортировка) ----------------
        CreateWindow(L"BUTTON", L"Сорт. Стоимость", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 410, 120, 30, hWnd, (HMENU)7, g_hInst, nullptr);
        CreateWindow(L"BUTTON", L"Сорт. Тип", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            140, 410, 120, 30, hWnd, (HMENU)8, g_hInst, nullptr);
        CreateWindow(L"BUTTON", L"Сорт. ID", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            270, 410, 120, 30, hWnd, (HMENU)9, g_hInst, nullptr);




        // ListView
        hListView = CreateWindow(WC_LISTVIEW, nullptr,
            WS_CHILD | WS_VISIBLE | LVS_REPORT,
            10, 50, 600, 300,
            hWnd, nullptr, g_hInst, nullptr);

        SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LV_COLUMN col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.cx = 100; // ширина колонки

        // Заголовки колонок
        const wchar_t* headers[] = { L"Фамилия", L"ID комнаты", L"Тип комнаты", L"Стоимость/день", L"Дней", L"Стоимость" };
        for (int i = 0; i < 6; i++) {
            wchar_t buf[32];
            wcsncpy_s(buf, headers[i], _TRUNCATE);
            col.pszText = buf;
            col.iSubItem = i;
            ListView_InsertColumn(hListView, i, &col);
        }

    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1: // Добавить комнату
            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ADDROOM), hWnd, AddRoomDlgProc);
            break;
        case 2: DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_REGISTER), hWnd, ClientDlgProc, -1); break; // Добавить клиента
        case 3: { // Редактировать
            int row = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (row < 0) {
                MessageBox(hWnd, L"Выберите клиента для редактирования!", L"Ошибка",
                    MB_OK | MB_ICONWARNING);
                break;
            }

            LVITEM item = {};
            item.iItem = row;
            item.mask = LVIF_PARAM;

            if (!ListView_GetItem(hListView, &item)) break;

            // Если выбрана комната
            if (item.lParam < 0) {
                MessageBox(hWnd, L"Нельзя редактировать комнату.\nВыберите клиента.",
                    L"Ошибка", MB_OK | MB_ICONERROR);
                break;
            }

            // item.lParam — индекс клиента
            DialogBoxParam(
                g_hInst,
                MAKEINTRESOURCE(IDD_REGISTER),
                hWnd,
                ClientDlgProc,
                item.lParam
            );
        }
              break;

        case 4: { // Удаление клиента или комнаты
            int row = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
            if (row >= 0) {
                LVITEM item = {};
                item.iItem = row;
                item.mask = LVIF_PARAM;

                if (!ListView_GetItem(hListView, &item)) break;

                if (item.lParam >= 0) {
                    // Удаляем клиента
                    g_hotel.removeClientByIndex(item.lParam);
                }
                else {
                    // Удаляем комнату
                    int roomId = -(item.lParam + 1);

                    // Проверяем, есть ли клиенты
                    bool hasClient = false;
                    for (const auto& c : g_hotel.getClients()) {
                        if (c.getRoomId() == roomId) {
                            hasClient = true;
                            break;
                        }
                    }

                    if (hasClient) {
                        // Сначала удаляем всех клиентов в комнате
                        for (int i = static_cast<int>(g_hotel.getClients().size()) - 1; i >= 0; i--) {
                            if (g_hotel.getClients()[i].getRoomId() == roomId)
                                g_hotel.removeClientByIndex(i);
                        }
                        MessageBox(hWnd, L"Все клиенты комнаты удалены. Нажмите еще раз для удаления самой комнаты.", L"Информация", MB_OK | MB_ICONINFORMATION);
                    }
                    else {
                        // Удаляем комнату
                        g_hotel.removeRoomById(roomId);
                    }
                }

                updateListView(hListView, g_hotel);
            }
        } break;

        case 5: { // Сохранить
            OPENFILENAME ofn = {};
            wchar_t filename[MAX_PATH] = L"";
            ofn.lStructSize = sizeof(ofn);
            ofn.lpstrFilter = L"Text Files\0*.txt\0";
            ofn.lpstrFile = filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT;

            if (GetSaveFileName(&ofn))
                g_hotel.saveToFile(filename);
        } break;

        case 6: { // Загрузить
            OPENFILENAME ofn = {};
            wchar_t filename[MAX_PATH] = L"";
            ofn.lStructSize = sizeof(ofn);
            ofn.lpstrFilter = L"Text Files\0*.txt\0";
            ofn.lpstrFile = filename;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {
                g_hotel.loadFromFile(filename);
                updateListView(hListView, g_hotel);
            }
        } break;



        case 7: // Сорт. Стоимость
            if (sortPriceAsc)
                g_hotel.sortRoomsByPriceAsc();
            else
                g_hotel.sortRoomsByPriceDesc();
            sortPriceAsc = !sortPriceAsc; // меняем направление
            updateListView(hListView, g_hotel);
            break;

        case 8: // Сорт. Тип
            if (sortTypeAsc)
                g_hotel.sortRoomsByTypeAsc();
            else
                g_hotel.sortRoomsByTypeDesc();
            sortTypeAsc = !sortTypeAsc;
            updateListView(hListView, g_hotel);
            break;

        case 9: // Сорт. ID
            if (sortIdAsc)
                g_hotel.sortRoomsByIdAsc();
            else
                g_hotel.sortRoomsByIdDesc();
            sortIdAsc = !sortIdAsc;
            updateListView(hListView, g_hotel);
            break;

        }
		break;









    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// ---------------- WinMain ----------------
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    g_hInst = hInstance;
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"HotelWin32";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    HWND hWnd = CreateWindow(L"HotelWin32", L"Hotel App",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 880, 500,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
