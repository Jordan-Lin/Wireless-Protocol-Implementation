#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Menu.hpp"
#include "Globals.hpp"
#include "Session.hpp"
#include "OpenFileDialog.h"
#include "wchar.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance, LPSTR lspszCmdParam, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
VOID packectizeFile(LPCSTR fileTitle);
VOID setProtocol();
VOID displayReceivedFile(LPCSTR fileTitle);
VOID updateSendStatus(LPCSTR status);
VOID updateRecieveStatus(LPCSTR status);
VOID updateACKRecieveStatus();
VOID updateACKSendStatus();
VOID updatePacketSendStatus();
VOID updatePacketRecieveStatus();
VOID resetProtocol();
VOID updateBadPacketStatus();
VOID updateAll();
VOID updateTimer();
LPSTR HandleDroppedFiles(HWND hWnd, WPARAM wParam);
VOID OpenFile();
std::string ExePath();



#endif // !APPLICATION_HPP
