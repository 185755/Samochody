/****************************************************
	Cooperation in cyberspace - The base program
	The main module
****************************************************/

#include <windows.h>
#include <math.h>
#include <time.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <iterator> 
#include <map>

#include "objects.h"
#include "graphics.h"
#include "net.h"
using namespace std;

FILE* f = fopen("wwc_log.txt", "w"); // plik do zapisu informacji testowych


MovableObject* my_car;               // obiekt przypisany do tej aplikacji
Environment env;

map<int, MovableObject*> other_cars;

float avg_cycle_time;                // sredni czas pomiedzy dwoma kolejnymi cyklami symulacji i wyswietlania
long time_of_cycle, number_of_cyc;   // zmienne pomocnicze potrzebne do obliczania avg_cycle_time
long time_start = clock();

multicast_net* multi_reciv;          // wsk do obiektu zajmujacego sie odbiorem komunikatow
multicast_net* multi_send;           //   -||-  wysylaniem komunikatow

HANDLE threadReciv;                  // uchwyt w�tku odbioru komunikat�w
HWND main_window;                    // uchwyt do g��wnego okna programu 
CRITICAL_SECTION m_cs;               // do synchronizacji w�tk�w

bool if_SHIFT_pressed = false;
bool if_ID_visible = true;           // czy rysowac nr ID przy ka�dym obiekcie
bool if_mouse_control = false;       // sterowanie za pomoc� klawisza myszki
int mouse_cursor_x = 0, mouse_cursor_y = 0;     // po�o�enie kursora myszy

extern ViewParams viewpar;           // ustawienia widoku zdefiniowane w grafice

long duration_of_day = 800;         // czas trwania dnia w [s]

struct Frame                                      // g��wna struktura s�u��ca do przesy�ania informacji
{
	int iID;                                      // identyfikator obiektu, kt�rego 
	int type;                                     // typ ramki: informacja o stateie, informacja o zamkni�ciu, komunikat tekstowy, ... 
	ObjectState state;                            // po�o�enie, pr�dko��: �rodka masy + k�towe, ...

	long sending_time;                            // tzw. znacznik czasu potrzebny np. do obliczenia op�nienia
	int iID_receiver;                             // nr ID odbiorcy wiadomo�ci, je�li skierowana jest tylko do niego
	int swap_request;
};

int FindClosestCar()
{
	int closestID = -1;
	float minDistance = FLT_MAX;

	FILE* f = fopen("wwc_log.txt", "a");
	if (!f) return closestID;

	EnterCriticalSection(&m_cs);

	for (auto& car : other_cars)
	{
		if (car.second != nullptr)
		{
			float distance = (car.second->state.vPos - my_car->state.vPos).length();
			fprintf(f, "FindClosestCar: Checking car ID = %d, distance = %.2f\n", car.first, distance);

			if (distance < minDistance)
			{
				minDistance = distance;
				closestID = car.first;
			}
		}
	}

	LeaveCriticalSection(&m_cs);

	if (closestID != -1)
	{
		fprintf(f, "FindClosestCar: Closest car found -> ID = %d, Distance = %.2f\n", closestID, minDistance);
	}
	else
	{
		fprintf(f, "FindClosestCar: No car found.\n");
	}

	fclose(f);
	return closestID;
}



void RequestSwap()
{
	FILE* f = fopen("wwc_log.txt", "a");
	if (!f) return;

	int closestID = FindClosestCar();

	if (closestID != -1)
	{
		Frame frame = {}; // Initialize frame
		frame.iID = my_car->iID; // Sender ID
		frame.type = 3; // Frame type for vehicle swapping
		frame.swap_request = 1; // Request swap
		frame.iID_receiver = closestID; // Closest car ID

		fprintf(f, "RequestSwap: Preparing to send swap request -> iID = %d, iID_receiver = %d\n", frame.iID, frame.iID_receiver);

		if (multi_send->send((char*)&frame, sizeof(Frame)) > 0)
		{
			fprintf(f, "RequestSwap: Swap request successfully sent.\n");
		}
		else
		{
			fprintf(f, "RequestSwap: Failed to send swap request.\n");
		}
	}
	else
	{
		fprintf(f, "RequestSwap: No cars nearby. Cannot send swap request.\n");
	}

	fclose(f);
}


void ConfirmSwap(int targetID)
{
	FILE* f = fopen("wwc_log.txt", "a");
	if (!f) return;

	fprintf(f, "ConfirmSwap: Wchodzę do funkcji. Target ID = %d\n", targetID);

	Frame frame = {}; // Zerowanie struktury ramki
	frame.iID = my_car->iID; // ID nadawcy
	frame.type = 3; // Typ ramki zamiany pojazdów
	frame.swap_request = 2; // Potwierdzenie zamiany
	frame.iID_receiver = targetID; // ID odbiorcy

	fprintf(f, "ConfirmSwap: Przygotowano ramkę -> iID = %d, iID_receiver = %d, type = %d, swap_request = %d\n",
		frame.iID, frame.iID_receiver, frame.type, frame.swap_request);

	if (multi_send->send((char*)&frame, sizeof(Frame)) > 0)
	{
		fprintf(f, "ConfirmSwap: Ramka została poprawnie wysłana.\n");
	}
	else
	{
		fprintf(f, "ConfirmSwap: Błąd wysyłania ramki.\n");
	}

	fclose(f);
}




void HandleSwap(Frame frame)
{
	// Otwórz plik do zapisu logów
	FILE* f = fopen("wwc_log.txt", "a");
	if (!f) return;

	// Logujemy odebraną ramkę
	fprintf(f, "HandleSwap: Otrzymano ramkę -> swap_request = %d, iID = %d, iID_receiver = %d\n",
		frame.swap_request, frame.iID, frame.iID_receiver);

	EnterCriticalSection(&m_cs); // Blokujemy sekcję krytyczną

	// Obsługa żądania zamiany
	if (frame.swap_request == 1)
	{
		fprintf(f, "HandleSwap: Przetwarzam żądanie zamiany od pojazdu %d\n", frame.iID);

		// Sprawdzamy, czy pojazd docelowy istnieje
		if (other_cars.find(frame.iID) == other_cars.end() || other_cars[frame.iID] == nullptr)
		{
			fprintf(f, "HandleSwap: Błąd -> Pojazd docelowy nie istnieje.\n");
			LeaveCriticalSection(&m_cs);
			fclose(f);
			return;
		}

		// Wysyłamy wiadomość do głównego okna (do WndProc)
		fprintf(f, "HandleSwap: Wysyłam WM_USER + 2 do głównego wątku -> iID = %d\n", frame.iID);
		FILE* f = fopen("wwc_log.txt", "a");
		if (f)
		{
			fprintf(f, "HandleSwap: main_window handle = %p\n", main_window);
			fclose(f);
		}
		PostMessage(main_window, WM_USER + 2, frame.iID, 0); // Przekazanie do WndProc
	}
	// Obsługa potwierdzenia zamiany
	else if (frame.swap_request == 2)
	{
		fprintf(f, "HandleSwap: Otrzymano potwierdzenie zamiany od pojazdu %d\n", frame.iID);

		// Weryfikacja, czy pojazd docelowy istnieje
		if (other_cars.find(frame.iID) == other_cars.end() || other_cars[frame.iID] == nullptr)
		{
			fprintf(f, "HandleSwap: Błąd -> Pojazd docelowy nie istnieje.\n");
			LeaveCriticalSection(&m_cs);
			fclose(f);
			return;
		}

		if (my_car == nullptr)
		{
			fprintf(f, "HandleSwap: Błąd -> Brak przypisanego pojazdu użytkownika.\n");
			LeaveCriticalSection(&m_cs);
			fclose(f);
			return;
		}

		// Wymiana pojazdów
		MovableObject* my_car_old = my_car;
		my_car = other_cars[frame.iID];
		other_cars[frame.iID] = my_car_old;

		fprintf(f, "HandleSwap: Zamiana zakończona -> my_car->iID = %d, other_cars[%d]->iID = %d\n",
			my_car->iID, frame.iID, other_cars[frame.iID]->iID);

		// Wysyłamy potwierdzenie do głównego wątku
		PostMessage(main_window, WM_USER + 1, 0, (LPARAM)"Pojazdy zostały zamienione.");
	}
	// Obsługa synchronizacji zamiany
	else if (frame.swap_request == 3)
	{
		fprintf(f, "HandleSwap: Synchronizacja zamiany z pojazdem %d\n", frame.iID);

		// Sprawdzamy, czy pojazd docelowy istnieje
		if (other_cars.find(frame.iID) == other_cars.end() || other_cars[frame.iID] == nullptr)
		{
			fprintf(f, "HandleSwap: Błąd -> Pojazd docelowy nie istnieje.\n");
			LeaveCriticalSection(&m_cs);
			fclose(f);
			return;
		}

		if (my_car == nullptr)
		{
			fprintf(f, "HandleSwap: Błąd -> Brak przypisanego pojazdu użytkownika.\n");
			LeaveCriticalSection(&m_cs);
			fclose(f);
			return;
		}

		// Synchronizacja zamiany pojazdów
		MovableObject* temp = my_car;
		my_car = other_cars[frame.iID];
		other_cars[frame.iID] = temp;

		fprintf(f, "HandleSwap: Synchronizacja zakończona -> my_car->iID = %d, other_cars[%d]->iID = %d\n",
			my_car->iID, frame.iID, other_cars[frame.iID]->iID);

		// Wysyłamy potwierdzenie zakończenia synchronizacji
		PostMessage(main_window, WM_USER + 1, 0, (LPARAM)"Synchronizacja zamiany zakończona.");
	}

	LeaveCriticalSection(&m_cs); // Odblokowujemy sekcję krytyczną
	fclose(f); // Zamykamy plik logów
}





//******************************************
// Funkcja obs�ugi w�tku odbioru komunikat�w 
// UWAGA!  Odbierane s� te� komunikaty z w�asnej aplikacji by por�wna� obraz ekstrapolowany do rzeczywistego.
DWORD WINAPI ReceiveThreadFun(void* ptr)
{
	multicast_net* pmt_net = (multicast_net*)ptr;
	Frame frame;

	while (1)
	{
		int frame_size = pmt_net->reciv((char*)&frame, sizeof(Frame));
		ObjectState state = frame.state;

		EnterCriticalSection(&m_cs);

		if (frame.iID != my_car->iID)  // je�li to nie m�j w�asny obiekt
		{
			if ((other_cars.size() == 0) || (other_cars[frame.iID] == NULL))
			{
				MovableObject* ob = new MovableObject();
				ob->iID = frame.iID;
				other_cars[frame.iID] = ob;
				fprintf(f, "Zarejestrowano nowy pojazd: ID = %d\n", frame.iID);
			}
			other_cars[frame.iID]->ChangeState(state);

			if (frame.type == 3)  // nowy typ ramki dla zamiany pojazd�w
			{
				HandleSwap(frame);
			}
		}

		LeaveCriticalSection(&m_cs);
	}
	return 1;
}
// *****************************************************************
// ****    Wszystko co trzeba zrobi� podczas uruchamiania aplikacji
// ****    poza grafik�   
void InteractionInitialisation()
{
	DWORD dwThreadId;

	my_car = new MovableObject();    // tworzenie wlasnego obiektu

	time_of_cycle = clock();             // pomiar aktualnego czasu

	// obiekty sieciowe typu multicast (z podaniem adresu WZR oraz numeru portu)
	multi_reciv = new multicast_net("224.12.12.10", 10001);      // obiekt do odbioru ramek sieciowych
	multi_send = new multicast_net("224.12.12.10", 10001);       // obiekt do wysy�ania ramek

	// uruchomienie w�tku obs�uguj�cego odbi�r komunikat�w:
	threadReciv = CreateThread(
		NULL,                        // no security attributes
		0,                           // use default stack size
		ReceiveThreadFun,                // thread function
		(void*)multi_reciv,               // argument to thread function
		NULL,                        // use default creation flags
		&dwThreadId);                // returns the thread identifier
	SetThreadPriority(threadReciv, THREAD_PRIORITY_HIGHEST);

	printf("start interakcji\n");
}


// *****************************************************************
// ****    Wszystko co trzeba zrobi� w ka�dym cyklu dzia�ania 
// ****    aplikacji poza grafik� 
void VirtualWorldCycle()
{
	number_of_cyc++;

	if (number_of_cyc % 50 == 0)          // je�li licznik cykli przekroczy� pewn� warto��, to
	{                              // nale�y na nowo obliczy� �redni czas cyklu avg_cycle_time
		char text[256];
		long prev_time = time_of_cycle;
		time_of_cycle = clock();
		float fFps = (50 * CLOCKS_PER_SEC) / (float)(time_of_cycle - prev_time);
		if (fFps != 0) avg_cycle_time = 1.0 / fFps; else avg_cycle_time = 1;

		sprintf(text, "WWC-lab 2024/25 temat 1 (%0.0f fps  %0.2fms) ", fFps, 1000.0 / fFps);

		SetWindowText(main_window, text); // wy�wietlenie aktualnej ilo�ci klatek/s w pasku okna			
	}

	my_car->Simulation(avg_cycle_time);                    // symulacja w�asnego obiektu

	Frame frame;
	frame.state = my_car->State();               // state w�asnego obiektu 
	frame.iID = my_car->iID;

	multi_send->send((char*)&frame, sizeof(Frame));  // wys�anie komunikatu do pozosta�ych aplikacji
}

// *****************************************************************
// ****    Wszystko co trzeba zrobi� podczas zamykania aplikacji
// ****    poza grafik� 
void EndOfInteraction()
{
	fprintf(f, "Koniec interakcji\n");
	fclose(f);
}



//deklaracja funkcji obslugi okna
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HDC g_context = NULL;        // uchwyt contextu graficznego



//funkcja Main - dla Windows
int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{

	//Initilize the critical section
	InitializeCriticalSection(&m_cs);

	MSG message;		  //innymi slowy "komunikat"
	WNDCLASS main_class; //klasa g��wnego okna aplikacji

	static char class_name[] = "Klasa_Podstawowa";

	//Definiujemy klase g��wnego okna aplikacji
	//Okreslamy tu wlasciwosci okna, szczegoly wygladu oraz
	//adres funkcji przetwarzajacej komunikaty
	main_class.style = CS_HREDRAW | CS_VREDRAW;
	main_class.lpfnWndProc = WndProc; //adres funkcji realizuj�cej przetwarzanie meldunk�w 
	main_class.cbClsExtra = 0;
	main_class.cbWndExtra = 0;
	main_class.hInstance = hInstance; //identyfikator procesu przekazany przez MS Windows podczas uruchamiania programu
	main_class.hIcon = 0;
	main_class.hCursor = LoadCursor(0, IDC_ARROW);
	main_class.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	main_class.lpszMenuName = "Menu";
	main_class.lpszClassName = class_name;

	//teraz rejestrujemy klas� okna g��wnego
	RegisterClass(&main_class);

	main_window = CreateWindow(class_name, "WWC-lab 2024/25 temat 1", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		100, 50, 950, 650, NULL, NULL, hInstance, NULL);



	ShowWindow(main_window, nCmdShow);

	//odswiezamy zawartosc okna
	UpdateWindow(main_window);

	// pobranie komunikatu z kolejki je�li funkcja PeekMessage zwraca warto�� inn� ni� FALSE,
	// w przeciwnym wypadku symulacja wirtualnego �wiata wraz z wizualizacj�
	ZeroMemory(&message, sizeof(message));
	while (message.message != WM_QUIT)
	{
		if (PeekMessage(&message, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		else
		{
			VirtualWorldCycle();    // Cykl wirtualnego �wiata
			InvalidateRect(main_window, NULL, FALSE);
		}
	}

	return (int)message.wParam;
}

/********************************************************************
FUNKCJA OKNA realizujaca przetwarzanie meldunk�w kierowanych do okna aplikacji*/
LRESULT CALLBACK WndProc(HWND main_window, UINT message_code, WPARAM wParam, LPARAM lParam)
{

	switch (message_code)
	{
	case WM_CREATE:  //message wysy�any w momencie tworzenia okna
	{

		g_context = GetDC(main_window);

		srand((unsigned)time(NULL));
		int result = GraphicsInitialisation(g_context);
		if (result == 0)
		{
			printf("nie udalo sie otworzyc okna graficznego\n");
			//exit(1);
		}

		InteractionInitialisation();

		SetTimer(main_window, 1, 10, NULL);

		return 0;
	}


	case WM_USER + 1:  // Wy�wietl MessageBox
	{
		char* message = (char*)lParam;
		MessageBox(main_window, message, "Zamiana pojazd�w", MB_OK);
		break;
	}
	case WM_USER + 2:  // Zapytanie o zamianę
	{
		int targetID = (int)wParam;

		FILE* f = fopen("wwc_log.txt", "a");
		if (f)
		{
			fprintf(f, "WndProc: Otrzymano wiadomość WM_USER + 2. Target ID = %d\n", targetID);
			fclose(f);
		}

		if (MessageBox(main_window, "Czy chcesz zamienić się pojazdem?", "Zamiana pojazdów", MB_YESNO) == IDYES)
		{
			ConfirmSwap(targetID); // Wywołanie potwierdzenia zamiany
		}
		else
		{
			f = fopen("wwc_log.txt", "a");
			if (f)
			{
				fprintf(f, "WndProc: Odrzucono zamianę pojazdów. Target ID = %d\n", targetID);
				fclose(f);
			}
		}
		break;
	}




	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC context;
		context = BeginPaint(main_window, &paint);

		DrawScene();
		SwapBuffers(context);

		EndPaint(main_window, &paint);

		return 0;
	}

	case WM_TIMER:

		return 0;

	case WM_SIZE:
	{
		int cx = LOWORD(lParam);
		int cy = HIWORD(lParam);

		WindowResize(cx, cy);

		return 0;
	}

	case WM_DESTROY: //obowi�zkowa obs�uga meldunku o zamkni�ciu okna

		EndOfInteraction();
		EndOfGraphics();

		ReleaseDC(main_window, g_context);
		KillTimer(main_window, 1);

		//LPDWORD lpExitCode;
		DWORD ExitCode;
		GetExitCodeThread(threadReciv, &ExitCode);
		TerminateThread(threadReciv, ExitCode);
		//ExitThread(ExitCode);

		//Sleep(1000);

		other_cars.clear();


		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN: //reakcja na lewy przycisk myszki
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (if_mouse_control)
			my_car->F = 30.0;        // si�a pchaj�ca do przodu
		break;
	}
	case WM_RBUTTONDOWN: //reakcja na prawy przycisk myszki
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (if_mouse_control)
			my_car->F = -5.0;        // si�a pchaj�ca do tylu
		break;
	}
	case WM_MBUTTONDOWN: //reakcja na �rodkowy przycisk myszki : uaktywnienie/dezaktywacja sterwania myszkowego
	{
		if_mouse_control = 1 - if_mouse_control;
		if (if_mouse_control) my_car->if_keep_steer_wheel = true;
		else my_car->if_keep_steer_wheel = false;

		mouse_cursor_x = LOWORD(lParam);
		mouse_cursor_y = HIWORD(lParam);
		break;
	}
	case WM_LBUTTONUP: //reakcja na puszczenie lewego przycisku myszki
	{
		if (if_mouse_control)
			my_car->F = 0.0;        // si�a pchaj�ca do przodu
		break;
	}
	case WM_RBUTTONUP: //reakcja na puszczenie lewy przycisk myszki
	{
		if (if_mouse_control)
			my_car->F = 0.0;        // si�a pchaj�ca do przodu
		break;
	}
	case WM_MOUSEMOVE:
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		if (if_mouse_control)
		{
			float wheel_angle = (float)(mouse_cursor_x - x) / 20;
			if (wheel_angle > 60) wheel_angle = 60;
			if (wheel_angle < -60) wheel_angle = -60;
			my_car->state.steering_angle = PI * wheel_angle / 180;
			//my_car->steer_wheel_speed = (float)(mouse_cursor_x - x) / 20;
		}
		break;
	}
	case WM_KEYDOWN:
	{

		switch (LOWORD(wParam))
		{
		case VK_SHIFT:
		{
			if_SHIFT_pressed = 1;
			break;
		}
		case VK_SPACE:
		{
			my_car->breaking_factor = 1.0;       // stopie� hamowania (reszta zale�y od si�y docisku i wsp. tarcia)
			break;                       // 1.0 to maksymalny stopie� (np. zablokowanie k�)
		}
		case VK_UP:
		{
			my_car->F = 100.0;        // si�a pchaj�ca do przodu
			break;
		}
		case VK_DOWN:
		{
			my_car->F = -70.0;
			break;
		}
		case VK_LEFT:
		{
			if (my_car->steer_wheel_speed < 0) {
				my_car->steer_wheel_speed = 0;
				my_car->if_keep_steer_wheel = true;
			}
			else {
				if (if_SHIFT_pressed) my_car->steer_wheel_speed = 0.5;
				else my_car->steer_wheel_speed = 0.25 / 8;
			}

			break;
		}
		case VK_RIGHT:
		{
			if (my_car->steer_wheel_speed > 0) {
				my_car->steer_wheel_speed = 0;
				my_car->if_keep_steer_wheel = true;
			}
			else {
				if (if_SHIFT_pressed) my_car->steer_wheel_speed = -0.5;
				else my_car->steer_wheel_speed = -0.25 / 8;
			}
			break;
		}
		case 'I':   // wypisywanie nr ID
		{
			if_ID_visible = 1 - if_ID_visible;
			break;
		}
		case 'W':   // cam_distance widoku
		{
			//cam_pos = cam_pos - cam_direct*0.3;
			if (viewpar.cam_distance > 0.5) viewpar.cam_distance /= 1.2;
			else viewpar.cam_distance = 0;
			break;
		}
		case 'S':   // przybli�enie widoku
		{
			//cam_pos = cam_pos + cam_direct*0.3; 
			if (viewpar.cam_distance > 0) viewpar.cam_distance *= 1.2;
			else viewpar.cam_distance = 0.5;
			break;
		}
		case 'Q':   // widok z g�ry
		{
			if (viewpar.tracking) break;
			viewpar.top_view = 1 - viewpar.top_view;
			if (viewpar.top_view)
			{
				viewpar.cam_pos_1 = viewpar.cam_pos; viewpar.cam_direct_1 = viewpar.cam_direct; viewpar.cam_vertical_1 = viewpar.cam_vertical;
				viewpar.cam_distance_1 = viewpar.cam_distance; viewpar.cam_angle_1 = viewpar.cam_angle;
				viewpar.cam_pos = viewpar.cam_pos_2; viewpar.cam_direct = viewpar.cam_direct_2; viewpar.cam_vertical = viewpar.cam_vertical_2;
				viewpar.cam_distance = viewpar.cam_distance_2; viewpar.cam_angle = viewpar.cam_angle_2;
			}
			else
			{
				viewpar.cam_pos_2 = viewpar.cam_pos; viewpar.cam_direct_2 = viewpar.cam_direct; viewpar.cam_vertical_2 = viewpar.cam_vertical;
				viewpar.cam_distance_2 = viewpar.cam_distance; viewpar.cam_angle_2 = viewpar.cam_angle;
				viewpar.cam_pos = viewpar.cam_pos_1; viewpar.cam_direct = viewpar.cam_direct_1; viewpar.cam_vertical = viewpar.cam_vertical_1;
				viewpar.cam_distance = viewpar.cam_distance_1; viewpar.cam_angle = viewpar.cam_angle_1;
			}
			break;
		}
		case 'E':   // obr�t kamery ku g�rze (wzgl�dem lokalnej osi z)
		{
			viewpar.cam_angle += PI * 5 / 180;
			break;
		}
		case 'D':   // obr�t kamery ku do�owi (wzgl�dem lokalnej osi z)
		{
			viewpar.cam_angle -= PI * 5 / 180;
			break;
		}
		case 'A':   // w��czanie, wy��czanie trybu �ledzenia obiektu
		{
			viewpar.tracking = 1 - viewpar.tracking;
			if (viewpar.tracking)
			{
				viewpar.cam_distance = viewpar.cam_distance_3; viewpar.cam_angle = viewpar.cam_angle_3;
			}
			else
			{
				viewpar.cam_distance_3 = viewpar.cam_distance; viewpar.cam_angle_3 = viewpar.cam_angle;
				viewpar.top_view = 0;
				viewpar.cam_pos = viewpar.cam_pos_1; viewpar.cam_direct = viewpar.cam_direct_1; viewpar.cam_vertical = viewpar.cam_vertical_1;
				viewpar.cam_distance = viewpar.cam_distance_1; viewpar.cam_angle = viewpar.cam_angle_1;
			}
			break;
		}
		case 'Z':   // zoom - zmniejszenie k�ta widzenia
		{
			viewpar.zoom /= 1.1;
			RECT rc;
			GetClientRect(main_window, &rc);
			WindowResize(rc.right - rc.left, rc.bottom - rc.top);
			break;
		}
		case 'X':   // zoom - zwi�kszenie k�ta widzenia
		{
			viewpar.zoom *= 1.1;
			RECT rc;
			GetClientRect(main_window, &rc);
			WindowResize(rc.right - rc.left, rc.bottom - rc.top);
			break;
		}
		case 'T':  // zamiana pojazd�w z najbli�szym pojazdem
		{
			RequestSwap();
			break;
		}
		case VK_F1:  // wywolanie systemu pomocy
		{
			char lan[1024], lan_bie[1024];
			//GetSystemDirectory(lan_sys,1024);
			GetCurrentDirectory(1024, lan_bie);
			strcpy(lan, "C:\\Program Files\\Internet Explorer\\iexplore ");
			strcat(lan, lan_bie);
			strcat(lan, "\\pomoc.htm");
			int wyni = WinExec(lan, SW_NORMAL);
			if (wyni < 32)  // proba uruchominia pomocy nie powiodla sie
			{
				strcpy(lan, "C:\\Program Files\\Mozilla Firefox\\firefox ");
				strcat(lan, lan_bie);
				strcat(lan, "\\pomoc.htm");
				wyni = WinExec(lan, SW_NORMAL);
				if (wyni < 32)
				{
					char lan_win[1024];
					GetWindowsDirectory(lan_win, 1024);
					strcat(lan_win, "\\notepad pomoc.txt ");
					wyni = WinExec(lan_win, SW_NORMAL);
				}
			}
			break;
		}
		case VK_ESCAPE:
		{
			SendMessage(main_window, WM_DESTROY, 0, 0);
			break;
		}
		} // switch po klawiszach

		break;
	}
	case WM_KEYUP:
	{
		switch (LOWORD(wParam))
		{
		case VK_SHIFT:
		{
			if_SHIFT_pressed = 0;
			break;
		}
		case VK_SPACE:
		{
			my_car->breaking_factor = 0.0;
			break;
		}
		case VK_UP:
		{
			my_car->F = 0.0;
			break;
		}
		case VK_DOWN:
		{
			my_car->F = 0.0;
			break;
		}
		case VK_LEFT:
		{
			my_car->Fb = 0.00;
			//my_car->state.steering_angle = 0;
			if (my_car->if_keep_steer_wheel) my_car->steer_wheel_speed = -0.25 / 8;
			else my_car->steer_wheel_speed = 0;
			my_car->if_keep_steer_wheel = false;
			break;
		}
		case VK_RIGHT:
		{
			my_car->Fb = 0.00;
			//my_car->state.steering_angle = 0;
			if (my_car->if_keep_steer_wheel) my_car->steer_wheel_speed = 0.25 / 8;
			else my_car->steer_wheel_speed = 0;
			my_car->if_keep_steer_wheel = false;
			break;
		}

		}

		break;
	}

	default: //statedardowa obs�uga pozosta�ych meldunk�w
		return DefWindowProc(main_window, message_code, wParam, lParam);
	}


}
