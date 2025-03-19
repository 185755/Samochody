#include <gl\gl.h>
#include <gl\glu.h>

#ifndef _VEKTOR3D__H 
#include "vector3D.h"
#endif

enum GLDisplayListNames
{
	Wall1 = 1,
	Wall2 = 2,
	Floor = 3,
	Cube = 4,
	Auto = 5,
	EnvironmentMap = 10

};


struct ViewParams
{
	// Parametry widoku:
	Vector3 cam_direct_1,                  // kierunek patrzenia
		cam_pos_1,                           // po³o¿enie kamery
		cam_vertical_1,                          // kierunek pionu kamery        
		cam_direct_2,                      // to samo dla widoku z góry
		cam_pos_2,
		cam_vertical_2,
		cam_direct, cam_pos, cam_vertical;
	bool tracking;                             // tryb œledzenia obiektu przez kamerê
	bool top_view;                          // tryb widoku z góry
	float cam_distance;                            // cam_distance widoku z kamery
	float cam_angle;                            // obrót kamery góra-dó³
	float cam_distance_1, cam_angle_1, cam_distance_2, cam_angle_2,
		cam_distance_3, cam_angle_3;
	float zoom;
};

int GraphicsInitialisation(HDC g_context);
void DrawScene();
void WindowResize(int cx, int cy);
void EndOfGraphics();

BOOL SetWindowPixelFormat(HDC hDC);
BOOL CreateViewGLContext(HDC hDC);
GLvoid BuildFont(HDC hDC);
GLvoid glPrint(const char *fmt, ...);