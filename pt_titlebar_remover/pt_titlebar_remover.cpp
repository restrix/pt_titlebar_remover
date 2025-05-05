#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

// Used for modifying a window, like removing the title bar in this case
void ModifyWindow(HWND hwnd) {
    /* The function below is obtaining the bitmask of the style flag of the window referred to by hwnd.
       'style' contains all the enabled/disabled bits. */
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);

    /* Remove the title bar and border from the bitmask */
    style &= ~(WS_CAPTION | WS_BORDER);

    /* Retain the window style for system buttons (min/max/close) */
    style |= (WS_OVERLAPPEDWINDOW & ~WS_CAPTION);

    /* Set the updated style back to the window */
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    // The window style is now updated; force the change to take effect immediately
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    // Get the parent window size to resize the current window accordingly
    HWND parent = GetParent(hwnd);
    if (parent) {
        RECT rcClient;
        // Obtain the client area size of the parent window
        GetClientRect(parent, &rcClient);

        // Resize the child window to fit inside the parent window's client area, maximizes it till menu bar
        SetWindowPos(hwnd, nullptr,
            0, 0,
            rcClient.right - rcClient.left,
            rcClient.bottom - rcClient.top,
            SWP_NOZORDER);
    }
}

// Callback function for enumerating child windows of the target window
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
    char className[256];
    // Retrieves the class name of the window in the hwnd handle
    GetClassNameA(hwnd, className, sizeof(className));

    /* Check for the class name "DigiMDIWndClass", which is used for the Pro Tools window,
       responsible for the Windows 7 style title bar that we wish to modify. */
    if (strcmp(className, "DigiMDIWndClass") == 0) {
        ModifyWindow(hwnd);  // Apply the window style modification
    }
    return TRUE;  // Continue enumeration of other child windows
}

// Function to check whether the target child windows are present
bool HasTargetChildWindows(HWND parent) {
    bool found = false;
    /* Enumerate all child windows of the parent window. The lambda function checks
       for windows with the class name "DigiMDIWndClass" or containing "Digi" in the class name. */
    EnumChildWindows(parent, [](HWND hwnd, LPARAM lParam) -> BOOL {
        char className[256];
        // Retrieve class name of each child window
        GetClassNameA(hwnd, className, sizeof(className));

        /* If the class name matches "DigiMDIWndClass" or contains "Digi", set the flag 'found' to true
           and stop further enumeration by returning FALSE. */
        if (strcmp(className, "DigiMDIWndClass") == 0 ||
            strstr(className, "Digi") != nullptr) {
            bool* flag = reinterpret_cast<bool*>(lParam);
            *flag = true;  // Set the flag to true indicating we found the target window
            return FALSE;  // Stop further enumeration
        }
        return TRUE;  // Continue enumeration
        }, (LPARAM)&found);

    return found;  // Return whether we found the target child windows
}

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow
) {
    HWND protoolsHwnd = nullptr;

    // Continuously check for the Pro Tools window until it's found
    while (true) {
        /* Enumerate all top-level windows and check if their title contains "Pro Tools".
           If found, store the hwnd of the Pro Tools window. */
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            char title[256];
            // Retrieve the title of the current window
            GetWindowTextA(hwnd, title, sizeof(title));

            // Check if the window title contains "Pro Tools"
            if (strstr(title, "Pro Tools")) {
                *((HWND*)lParam) = hwnd;  // Store the hwnd of the Pro Tools window
                return FALSE;  // Stop enumeration once the Pro Tools window is found
            }
            return TRUE;  // Continue enumeration
            }, (LPARAM)&protoolsHwnd);

        // If Pro Tools window is found, exit the loop
        if (protoolsHwnd != nullptr)
            break;

        // Sleep for a second before retrying
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    // Wait until the target child windows (Edit/Mix) are available
    while (!HasTargetChildWindows(protoolsHwnd)) {
        // Sleep for a second before retrying
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    // Enumerate child windows of the Pro Tools window and apply modifications
    EnumChildWindows(protoolsHwnd, EnumChildProc, 0);

    // Maximize the Pro Tools window after modifications
    ShowWindow(protoolsHwnd, SW_MAXIMIZE);

    return 0;
}
