#include "Application.hpp"
/* ------------------------------------------------------------------------------------------
--  SOURCE FILE: Application.cpp
--
--  PROGRAM:     Wireless Protocol
--
--  FUNCTIONS:
--               WinMain
--				 WndProc
--				 packectizeFile
--				 setProtocol
--				 displayReceivedFile
--				 updateSendStatus
--			     updateRecieveStatus
--				 updateACKRecieveStatus
--				 updateACKSendStatus
--				 updatePacketSendStatus
--				 updatePacketRecieveStatus
--				 resetProtocol
--				 updateBadPacketStatus
--				 updateAll
--				 updateTimer
--				 HandleDroppedFiles
--				 OpenFile
--				 ExePath
------------------------------------------------------------------------------------------------*/

static TCHAR Name[] = TEXT("Final Project - Team Marx");
static TCHAR fileTitle[] = TEXT("Your File");
static TCHAR timeElapsed[] = TEXT("Time Elapsed: ");
static TCHAR badPacketTitle[] = TEXT("Bad Packets: ");
static TCHAR ackCountTitle[] = TEXT("ACK Send Count: ");
static TCHAR ackCount2Title[] = TEXT("ACK Recieved Count: ");
static TCHAR packSendCountTitle[] = TEXT("Packets Sent: ");
static TCHAR packRecieveCountTitle[] = TEXT("Packets Recieved: ");
static TCHAR statusTitle[] = TEXT("Status: ");
CHAR* fileDrop;
std::vector<char> displayVector;		// Container for displaying text to gui
std::atomic<int> ackCountR = 0;			// ACK recieving counter
std::atomic<int> ackCountS = 0;			// ACK sending counter
std::atomic<int> packetSentCount = 0;	// Packets sent counter
std::atomic<int> packetRecievedCount = 0;	// Packets (error free) recieved counter
std::atomic<int> badPacketCount = 0;	// Invalid packets recieved counter
std::atomic<int> timeSeconds = 0;		// Seconds in timer
std::atomic<int> timeMinutes = 0;		// Minutes in timer

HANDLE hFile;
TCHAR* fileName = NULL;
DWORD fileSize = 0;

std::thread protocolthread;		// Thread that runs the protocol

OpenFileDialog* openFileDialog1 = new OpenFileDialog();

DWORD dwBytesRead, dwBytesWritten, dwPos;

LPTSTR	lpszCommName = TEXT("com1");
COMMCONFIG	cc;
HWND hwndEdit;		// Your file window handle
HWND hwndReceive;	// Recieved file window handle
HWND hwndEdit2;		// Timer window handle
HWND hwndEditAck;	// ACK Send count window handle
HWND hwndEditAck2;	// ACK recieve count window handle
HWND hwndButtonConnect;	// Connect button window handle
HWND hwndStatus;	// Sending status widnow handle
HWND hwndStatus2;	// Recieveing status window handle
HANDLE threadEvent;	// Thread event, signaled on end of protocol thread
HWND hwndBER;		// Error detection counter window handle
HWND hwndNumPacketSent;	// Packets successfully sent widnow handle
HWND hwndEditPacketsReceived;	// Packets recieved window handle
HINSTANCE hInstance;

/*-----------------------------------------------------------------------------------------------
FUNCTION:     ExePath

DESCRIPTION:   Gets the directory path of the program. 

RETURNS:      (std::string) Path to program as a string.

-----------------------------------------------------------------------------------------------*/
std::string ExePath() {
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     WinMain

DESCRIPTION:  Packetizes data to the protocol specification.

RETURNS:      (int) Exit code.

PARAMS:       (HINSTANCE hInst, HINSTANCE hprevInstance, LPSTR lspszCmdParam, int nCmdShow)

NOTES:		  Main function that starts the program. 
			  Setups the main window wnad begins that message loop.
-----------------------------------------------------------------------------------------------*/
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hprevInstance,
	LPSTR lspszCmdParam, int nCmdShow)
{
	HWND hwnd;
	MSG Msg;
	WNDCLASSEX Wcl;
	UINT_PTR ID_TIMER = NULL;
	
	hInstance = hInst;

	Wcl.cbSize = sizeof(WNDCLASSEX);
	Wcl.style = CS_HREDRAW | CS_VREDRAW;
	Wcl.hIcon = LoadIcon(NULL, IDI_APPLICATION); // large icon 
	Wcl.hIconSm = NULL; // use small version of large icon
	Wcl.hCursor = LoadCursor(NULL, IDC_ARROW);  // cursor style

	Wcl.lpfnWndProc = WndProc;
	Wcl.hInstance = hInst;
	Wcl.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH); //white background
	Wcl.lpszClassName = Name;

	Wcl.lpszMenuName = TEXT("MYMENU"); // The menu Class
	Wcl.cbClsExtra = 0;      // no extra memory needed
	Wcl.cbWndExtra = 0;

	if (!RegisterClassEx(&Wcl))
		return 0;
	//main window
	hwnd = CreateWindow(Name, Name, WS_OVERLAPPEDWINDOW, 10, 10,
		900, 700, NULL, NULL, hInst, NULL);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);
	threadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);	// Event to check if protocol thread is finished.

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return Msg.wParam;
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     WndProc

DESCRIPTION:  Packetizes data to the protocol specification.

RETURNS:      LRESULT

PARAMS:       (HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)

NOTES:		  Win32 Message function 
-----------------------------------------------------------------------------------------------*/
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message,
	WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_DROPFILES:	// Drop file into gui
		HandleDroppedFiles(hwnd, wParam);
		OpenFile();
		SetCurrentDirectory(ExePath().c_str());
		break;
	case WM_CREATE: {	
		//open file contents
		hwndEdit = CreateWindow(TEXT("Edit"), fileTitle, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
			40, 50, 350, 450, hwnd, NULL,
			GetModuleHandle(NULL), NULL);
		DragAcceptFiles(hwnd, TRUE);
		
		//receive contents
		 hwndReceive = CreateWindow(TEXT("Edit"), TEXT("received file"), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
			420, 50, 350, 450, hwnd, NULL,
			GetModuleHandle(NULL), NULL);

		//status report time elapsed
		hwndEdit2 = CreateWindow(TEXT("Edit"), timeElapsed, WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
			420, 550, 200, 20, hwnd, NULL, 
			GetModuleHandle(NULL), NULL);


		//Bad Packets Detected
		hwndBER = CreateWindow(TEXT("Edit"), badPacketTitle, WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
			620, 550, 200, 20, hwnd, NULL,
			GetModuleHandle(NULL), NULL);

		//Ack Counter 1
		hwndEditAck = CreateWindow(TEXT("Edit"), ackCountTitle, WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
			420, 570, 200, 20, hwnd, NULL,
			GetModuleHandle(NULL), NULL);

		//Ack Counter 2
		hwndEditAck2 = CreateWindow(TEXT("Edit"), ackCount2Title, WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
			620, 570, 200, 20, hwnd, NULL,
			GetModuleHandle(NULL), NULL);

		//Number of Packets sent
		hwndNumPacketSent = CreateWindow(TEXT("Edit"), packSendCountTitle, WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
			420, 590, 200, 20, hwnd, NULL,
			GetModuleHandle(NULL), NULL);

		//Number of Packets received 
		hwndEditPacketsReceived = CreateWindow(TEXT("Edit"), packRecieveCountTitle, WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
			620, 590, 200, 20, hwnd, NULL,
			GetModuleHandle(NULL), NULL);

		//status box send
		hwndStatus = CreateWindow(TEXT("Edit"), statusTitle, WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
			40, 515, 350, 20, hwnd, NULL,
			GetModuleHandle(NULL), NULL);

		//status box recieve
		hwndStatus2 = CreateWindow(TEXT("Edit"), statusTitle, WS_CHILD | WS_VISIBLE | ES_READONLY | WS_BORDER,
			420, 515, 350, 20, hwnd, NULL,
			GetModuleHandle(NULL), NULL);

		//open file button
		HWND hwndButtonOpen = CreateWindow(
			TEXT("BUTTON"),  // Predefined class; Unicode assumed 
			TEXT("Open File"),      // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_LEFT | BS_DEFPUSHBUTTON,  // Styles 
			40,         // x position 
			550,         // y position 
			80,        // Button width
			40,        // Button height
			hwnd,     // Parent window
			(HMENU)OPENBTN,
			(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
			NULL);      // Pointer not needed.

						//open file button
			hwndButtonConnect = CreateWindow(
			TEXT("BUTTON"),  // Predefined class; Unicode assumed 
			TEXT("Connect"),      // Button text
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_LEFT | BS_DEFPUSHBUTTON,  // Styles 
			125,         // x position 
			550,         // y position 
			85,        // Button width
			40,        // Button height
			hwnd,     // Parent window
			(HMENU)CONNECTBTN,
			(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
			NULL);      // Pointer not needed.

		HWND hwndButtonSendFile = CreateWindow(
			TEXT("BUTTON"),  // Predefined class; Unicode assumed 
			TEXT("Send File"),      // Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_LEFT | BS_DEFPUSHBUTTON,  // Styles 
			215,         // x position 
			550,         // y position 
			80,        // Button width
			40,        // Button height
			hwnd,     // Parent window
			(HMENU)SENDBTN,
			(HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE),
			NULL);      // Pointer not needed.
		}
		break;
	case WM_COMMAND:
		switch (wParam) {

		case IDM_ABOUT:
			MessageBox(0, TEXT(" - Team Marx  - \n Brody McCrone \n Jacob McPhail \n Jordin Lin \n Mark Tattrie"), _T("About"), MB_OK | MB_ICONINFORMATION);

			break;

		case OPENBTN:
			// Dialog to open a file into program
			SetWindowText(hwndEdit, '\0');

			openFileDialog1->FilterIndex = 1;
			openFileDialog1->Filter = TEXT("txt files (*.txt) | *.txt");
			openFileDialog1->InitialDir = _T("C:\\Windows\\");
			openFileDialog1->Title = _T("Open Text File");
			openFileDialog1->Flags = OF_READ;
			if (openFileDialog1->ShowDialog())
			{
				fileName = openFileDialog1->FileName;
				OpenFile();
			}
			SetCurrentDirectory(ExePath().c_str());
			break;

		case SENDBTN:
			// Current opened file is to be sent
			if (connection == false) {
				MessageBox(NULL, TEXT("Need a connection to send"), TEXT("Cannot Send"), MB_OK);
			} else if (fileName == NULL) {
				MessageBox(NULL, TEXT("No file opened to send"), TEXT("Cannot Send"), MB_OK);
			} else {
				packectizeFile(fileName);
				SetWindowText(hwndStatus, "Status: Sending..." );
			}
			break;
		case CONNECTBTN:
			// Connect or disconnect
			Button_Enable(hwndButtonConnect, FALSE);
			std::thread protocolStartup = std::thread(setProtocol);	// Setup prorgram for connection
			protocolStartup.detach();
			SetTimer(hwnd, IDT_TIMER, 1000, NULL);
			break;
		}
	case WM_TIMER:
		switch (wParam) {
		case IDT_TIMER:
			timeSeconds++;
			if (timeSeconds >= 60) {
				timeSeconds = 0;
				timeMinutes++;
			}
			updateAll();
			if (connection == false) {
				KillTimer(hwnd, IDT_TIMER);
			}
		}
		break;
	case WM_DESTROY:	// Terminate program
		KillTimer(hwnd, IDT_TIMER);
		PostQuitMessage(0);
		if (connection) {
			connection = false;
			WaitForSingleObject(threadEvent, 5000);
		}
		break;
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     packectizeFile

DESCRIPTION:  Reads file and splits it into packets. Then puts packets into buffer to be sent

RETURNS:      VOID

PARAMS:		 (LPCSTR filePath) Path to file

NOTES:		 Opens file that is loaded into the program and reads it in block of 1024.
			 First adds a DC1 packet for transfer start, then generates packets until the
			 end of file. Once at the end it will fill the last packt with null characters,
			 or create a packet with only nulls if the last is full. The null characters
			 signal the end of the file to the reciever.
-----------------------------------------------------------------------------------------------*/
VOID packectizeFile(LPCSTR filePath) {
	if (filePath != NULL) {
		char* outbuffer = new char[1027];
		std::ifstream sendFile(filePath);
		if (!sendFile.is_open()) {
			SetWindowText(hwndStatus, "ERROR: Cannot open file to send");
			return;
		}

		// Create DC1 packet
		char* dcbuf = new char[1024];
		for (int i = 0; i < 1024; ++i) {
			dcbuf[i] = DC1;
		}

		char* packet = generatePacket(dcbuf);
		sendingBuffer.emplace(packet);

		// Read file and create packets from the blocks of data
		// Add nulls at the last packet
		while (1) {
			outbuffer = new CHAR[DATA_SIZE];
			CHAR data[DATA_SIZE];
			sendFile.read(data, DATA_SIZE);
			if (sendFile.gcount() < DATA_SIZE) {
				for (std::streamsize i = sendFile.gcount(); i < DATA_SIZE; i++) {
					data[i] = NULL;
				}
				char* packet = generatePacket(data);
				sendingBuffer.emplace(packet);
				break;
			}
			char* packet = generatePacket(data);
			sendingBuffer.emplace(packet);
		}

		sendFile.close();
		SetEvent(sendEvent); // Tell protocol that a file is ready to send
	}
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     setProtocol

DESCRIPTION:  Starts or ends the protocol thread / connection

RETURNS:      VOID

NOTES:		  On function call if there is a connection running then tell protocol to finish
			  and wait for event back saying it's done. If there is no connection then reset
			  all stats and create a new protocol thread to start a new connection.
-----------------------------------------------------------------------------------------------*/
VOID setProtocol() {
	if (connection) {
		connection = false;
		WaitForSingleObject(threadEvent, INFINITE);
		SetWindowText(hwndButtonConnect, "Connect");
	} else {
		ackCountR = 0;
		ackCountS = 0;
		packetSentCount = 0;
		packetRecievedCount = 0;
		badPacketCount = 0;
		timeSeconds = 0;
		timeMinutes = 0;
		connection = true;
		ResetEvent(threadEvent);
		protocolthread = std::thread(run);
		protocolthread.detach();
		SetWindowText(hwndButtonConnect, "Disconnect");
	}
	updateAll();
	Button_Enable(hwndButtonConnect, TRUE);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     resetProtocol

DESCRIPTION:  Ends connection attempt from protocol thread

RETURNS:      VOID

NOTES:		 When Protocol gets an idel sequence timeout, then is will call this function and
			 will tell itself to end the protocol thread.
-----------------------------------------------------------------------------------------------*/
VOID resetProtocol() {
	Button_Enable(hwndButtonConnect, FALSE);
	connection = false;
	SetWindowText(hwndButtonConnect, "Connect");
	updateAll();
	Button_Enable(hwndButtonConnect, TRUE);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     displayReceivedFile

DESCRIPTION:  Reads a file recieved and displays it.

RETURNS:      VOID

PARAMS:		 (LPCSTR filePath) Path to file

NOTES:		 When a file has be fully sent to program, it will read it and display its
			 contents in the gui.
-----------------------------------------------------------------------------------------------*/
VOID displayReceivedFile(LPCSTR filePath) {
	std::wifstream infile;
	FILE * fp = fopen(filePath, "r");
	char wc;
	displayVector.clear();
	if (fp != NULL) {
		wc = getc(fp);
		displayVector.push_back(wc);
		while (wc != EOF) {
			wc = getc(fp);
			displayVector.push_back(wc);
		}
	}
	std::string text = "";
	for (auto& x : displayVector) {
		text += x;
	}
	text[text.size() - 1] = NULL;
	LPCSTR lpStr = text.c_str();
	SetWindowText(hwndReceive, lpStr);

	fclose(fp);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     OpenFile

DESCRIPTION:  Reads an open file and displays it.

RETURNS:      VOID

NOTES:		  On user opening or draging file into program, it will be read and displayed in
			  the gui.
-----------------------------------------------------------------------------------------------*/
VOID OpenFile() {
	FILE * fp = fopen(fileName, "r");
	int x = errno;
	char wc;
	displayVector.clear();
	if (fp != NULL) {
		wc = getc(fp);
		displayVector.push_back(wc);
		while (wc != EOF) {
			wc = getc(fp);
			displayVector.push_back(wc);
		}
	}
	std::string text = "";
	for (auto& x : displayVector) {
		text += x;
	}
	text[text.size() - 1] = NULL;
	LPCSTR lpStr = text.c_str();
	SetWindowText(hwndEdit, lpStr);

	fseek(fp, 0, SEEK_SET);
	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fclose(fp);
	
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     HandleDroppedFiles

DESCRIPTION:  Gets file path of the file that was droped into the program.

RETURNS:      (LPSTR) Path to file

PARAMS:		  (HWND hWnd, WPARAM wParam) 

NOTES:		 When a file has be fully sent to program, it will read it and display its
			 contents in the gui.
-----------------------------------------------------------------------------------------------*/
LPSTR HandleDroppedFiles(HWND hWnd, WPARAM wParam) {
	fileDrop = new CHAR[200];
	DragQueryFile((HDROP)wParam, 0, fileDrop, 200);
	fileName = new CHAR[200];
	strcpy(fileName, fileDrop);
	return (TEXT(fileDrop));
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     updateTimer 

DESCRIPTION:  Updates the Timer in the gui.

RETURNS:      VOID

NOTES:		  Updates the elasped time of the connection.
-----------------------------------------------------------------------------------------------*/
VOID updateTimer() {
	wchar_t tempBuffer[1024];
	InvalidateRect(hwndEdit2, NULL, true);
	if (timeSeconds < 10) {
		wsprintfW(tempBuffer, L"Time Elapsed: %d : 0%d", timeMinutes, timeSeconds);
	} else {
		wsprintfW(tempBuffer, L"Time Elapsed: %d : %d", timeMinutes, timeSeconds);
	}
	SetWindowTextW(hwndEdit2, tempBuffer);
	UpdateWindow(hwndEdit2);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:	  updateAll

DESCRIPTION:  Updates all gui windows.

RETURNS:      VOID

NOTES:		  Calls all update functions to for all windows in the gui.
-----------------------------------------------------------------------------------------------*/
VOID updateAll() {
	updateTimer();
	updateACKSendStatus();
	updateACKRecieveStatus();
	updatePacketSendStatus();
	updatePacketRecieveStatus();
	updateBadPacketStatus();
	//updateProgress();
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     updateSendStatus

DESCRIPTION:  Updates send status

RETURNS:      VOID

PARAMS:		  (LPCSTR status) Pointer to text to set

NOTES:		  Changes the text in the window to the text given.
-----------------------------------------------------------------------------------------------*/
VOID updateSendStatus(LPCSTR status) {
	SetWindowText(hwndStatus, status);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:     updateRecieveStatus

DESCRIPTION:  Updates recieve status

RETURNS:      VOID

PARAMS:		  (LPCSTR status) Pointer to text to set

NOTES:		  Changes the text in the window to the text given.
-----------------------------------------------------------------------------------------------*/
VOID updateRecieveStatus(LPCSTR status) {
	SetWindowText(hwndStatus2, status);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:	  updateACKSendStatus

DESCRIPTION:  Updates the Counter for Acks Sent

RETURNS:      VOID

NOTES:		  Updates the counter to match the current value.
-----------------------------------------------------------------------------------------------*/
VOID updateACKSendStatus() {
	InvalidateRect(hwndEditAck, NULL, true);
	wchar_t tempBuffer[1024];
	wsprintfW(tempBuffer, L"ACK Send Count: %d", ackCountS);
	SetWindowTextW(hwndEditAck, tempBuffer);
	UpdateWindow(hwndEditAck);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:	  updateACKRecieveStatus

DESCRIPTION:  Updates the Counter for Acks recieved

RETURNS:      VOID

NOTES:		  Updates the counter to match the current value.
-----------------------------------------------------------------------------------------------*/
VOID updateACKRecieveStatus() {
	InvalidateRect(hwndEditAck2, NULL, true);
	wchar_t tempBuffer[1024];
	wsprintfW(tempBuffer, L"ACK Recieved Count: %d", ackCountR);
	SetWindowTextW(hwndEditAck2, tempBuffer);
	UpdateWindow(hwndEditAck2);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:	  updatePacketSendStatus

DESCRIPTION:  Updates the Counter for packets sent

RETURNS:      VOID

NOTES:		  Updates the counter to match the current value.
-----------------------------------------------------------------------------------------------*/
VOID updatePacketSendStatus() {
	InvalidateRect(hwndNumPacketSent, NULL, true);
	wchar_t tempBuffer[1024];
	wsprintfW(tempBuffer, L"Packets Sent:  %d", packetSentCount);
	SetWindowTextW(hwndNumPacketSent, tempBuffer);
	UpdateWindow(hwndNumPacketSent);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:	  updatePacketRecieveStatus

DESCRIPTION:  Updates the counter for packets recieved

RETURNS:      VOID

NOTES:		  Updates the counter to match the current value.
-----------------------------------------------------------------------------------------------*/
VOID updatePacketRecieveStatus() {
	InvalidateRect(hwndEditPacketsReceived, NULL, true);
	wchar_t tempBuffer[1024];
	wsprintfW(tempBuffer, L"Packets Recieved:   %d", packetRecievedCount);
	SetWindowTextW(hwndEditPacketsReceived, tempBuffer);
	UpdateWindow(hwndEditPacketsReceived);
}

/*-----------------------------------------------------------------------------------------------
FUNCTION:	  updateBadPacketStatus

DESCRIPTION:  Updates the counter for bad packets recieved

RETURNS:      VOID

NOTES:		  Updates the counter to match the current value.
-----------------------------------------------------------------------------------------------*/
VOID updateBadPacketStatus() {
	InvalidateRect(hwndBER, NULL, true);
	wchar_t tempBuffer[1024];
	wsprintfW(tempBuffer, L"Bad Packets: %d", badPacketCount);
	SetWindowTextW(hwndBER, tempBuffer);
	UpdateWindow(hwndBER);
}


