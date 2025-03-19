/************************************************************
					 Grafika OpenGL
*************************************************************/
#include <windows.h>
#include <time.h>
#include <gl\gl.h>
#include <gl\glu.h>
#include <iterator> 
#include <map>

using namespace std;

#include "graphics.h"
#include "objects.h"

ViewParams viewpar;

extern MovableObject *my_car;               // obiekt przypisany do tej aplikacji
extern map<int, MovableObject*> other_cars;
extern CRITICAL_SECTION m_cs;

extern Environment env;
extern long duration_of_day;

int g_GLPixelIndex = 0;
HGLRC g_hGLContext = NULL;
unsigned int font_base;
extern long time_start;                 // czas od uruchomienia potrzebny np. do obliczenia po³o¿enia s³oñca
extern HWND main_window;

extern void CreateDisplayLists();		// definiujemy listy tworz¹ce labirynt
extern void DrawGlobalCoordAxes();


int GraphicsInitialisation(HDC g_context)
{

	if (SetWindowPixelFormat(g_context) == FALSE)
		return FALSE;

	if (CreateViewGLContext(g_context) == FALSE)
		return 0;
	BuildFont(g_context);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);


	CreateDisplayLists();		// definiujemy listy tworz¹ce ró¿ne elementy sceny
	env.DrawInitialisation();

	// pocz¹tkowe ustawienia widoku:
	// Parametry widoku:
	viewpar.cam_direct_1 = Vector3(10, -3, -14);   // kierunek patrzenia
	viewpar.cam_pos_1 = Vector3(-35, 6, 10);         // po³o¿enie kamery
	viewpar.cam_vertical_1 = Vector3(0, 1, 0);           // kierunek pionu kamery        
	viewpar.cam_direct_2 = Vector3(0, -1, 0.02);   // to samo dla widoku z góry
	viewpar.cam_pos_2 = Vector3(0, 100, 0);
	viewpar.cam_vertical_2 = Vector3(0, 0, -1);
	viewpar.cam_direct = viewpar.cam_direct_1;
	viewpar.cam_pos = viewpar.cam_pos_1;
	viewpar.cam_vertical = viewpar.cam_vertical_1;
	viewpar.tracking = 0;                             // tryb œledzenia obiektu przez kamerê
	viewpar.top_view = 0;                          // tryb widoku z góry
	viewpar.cam_distance = 18.0;                          // cam_distance widoku z kamery
	viewpar.cam_angle = 0;                            // obrót kamery góra-dó³
	viewpar.cam_distance_1 = viewpar.cam_distance;
	viewpar.cam_angle_1 = viewpar.cam_angle;
	viewpar.cam_distance_2 = viewpar.cam_distance;
	viewpar.cam_angle_2 = viewpar.cam_angle;
	viewpar.cam_distance_3 = viewpar.cam_distance;
	viewpar.cam_angle_3 = viewpar.cam_angle;
	viewpar.zoom = 1.0;
}


void DrawScene()
{
	GLfloat BlueSurface[] = { 0.3f, 0.0f, 0.8f, 0.5f };
	GLfloat DGreenSurface[] = { 0.2f, 0.8f, 0.25f, 0.7f };
	GLfloat RedSurface[] = { 0.8f, 0.2f, 0.1f, 0.5f };
	
	GLfloat YellowSurface[] = { 0.75f, 0.75f, 0.0f, 1.0f };
	

	GLfloat LightAmbient[] = { 0.1f, 0.1f, 0.1f, 0.1f };
	GLfloat LightDiffuse[] = { 0.2f, 0.7f, 0.7f, 0.7f };
	GLfloat LightPosition[] = { 5.0f, 5.0f, 5.0f, 0.0f };

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);		//1 sk³adowa: œwiat³o otaczaj¹ce (bezkierunkowe)
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);		//2 sk³adowa: œwiat³o rozproszone (kierunkowe)
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT0);

	glPushMatrix();
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, BlueSurface);

	

	Vector3 direction_k, vertical_k, position_k;
	if (viewpar.tracking)
	{
		direction_k = my_car->state.qOrient.rotate_vector(Vector3(1, 0, 0));
		vertical_k = my_car->state.qOrient.rotate_vector(Vector3(0, 1, 0));
		Vector3 v_cam_right = my_car->state.qOrient.rotate_vector(Vector3(0, 0, 1));

		vertical_k = vertical_k.rotation(viewpar.cam_angle, v_cam_right.x, v_cam_right.y, v_cam_right.z);
		direction_k = direction_k.rotation(viewpar.cam_angle, v_cam_right.x, v_cam_right.y, v_cam_right.z);
		position_k = my_car->state.vPos - direction_k*my_car->length * 0 +
			vertical_k.znorm()*my_car->height * 5;
		viewpar.cam_vertical = vertical_k;
		viewpar.cam_direct = direction_k;
		viewpar.cam_pos = position_k;
	}
	else
	{
		vertical_k = viewpar.cam_vertical;
		direction_k = viewpar.cam_direct;
		position_k = viewpar.cam_pos;
		Vector3 v_cam_right = (direction_k*vertical_k).znorm();
		vertical_k = vertical_k.rotation(viewpar.cam_angle / 20, v_cam_right.x, v_cam_right.y, v_cam_right.z);
		direction_k = direction_k.rotation(viewpar.cam_angle / 20, v_cam_right.x, v_cam_right.y, v_cam_right.z);
	}

	// Ustawianie widoku sceny    
	gluLookAt(position_k.x - viewpar.cam_distance*direction_k.x,
		position_k.y - viewpar.cam_distance*direction_k.y, position_k.z - viewpar.cam_distance*direction_k.z,
		position_k.x + direction_k.x, position_k.y + direction_k.y, position_k.z + direction_k.z,
		vertical_k.x, vertical_k.y, vertical_k.z);

	//glRasterPos2f(0.30,-0.27);
	//glPrint("my_car->iID = %d",my_car->iID ); 

	DrawGlobalCoordAxes();

	// s³oñce + t³o:
	int R = 50000;                // promieñ obiegu
	long x = (clock() - time_start) % (duration_of_day*CLOCKS_PER_SEC);
	float angle = (float)x / (duration_of_day*CLOCKS_PER_SEC) * 2 * 3.1416;
	//char lan[128];
	//sprintf(lan,"angle = %f\n", angle);
	//SetWindowText(main_window, lan);

	float cos_abs = fabs(cos(angle));
	float cos_sq = cos_abs*cos_abs, sin_sq = sin(angle)*sin(angle);
	float sin_angminpi = fabs(sin(angle/2 - 3.1416/2));                   // 0 gdy s³oñce na antypodach, 1 w po³udnie
	float sin_angminpi_sqr = sin_angminpi*sin_angminpi;
	float cos_ang_zachod = fabs(cos((angle - 1.5)*3));                    // 1 w momencie wschodu lub zachodu  
	glClearColor(0.35*cos_abs*sin_angminpi_sqr + 0.15*sin_sq*cos_ang_zachod, 0.85*cos_abs*sin_angminpi_sqr, 2.0*cos_abs*sin_angminpi_sqr, 0.9);  // ustawienie t³a
	GLfloat SunColor[] = { 8.0*cos_sq, 8.0 * cos_sq*cos_sq*sin_angminpi, 8.0 * cos_sq*cos_sq*sin_angminpi, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, SunColor);
	glPushMatrix();
	glTranslatef(R*cos(angle), R*(cos(angle)*0.5 - 0.2), R*sin(angle));   // ustawienie s³oñca
	//glTranslatef(R*cos(angle), 5000, R*sin(angle));
	GLUquadricObj *Qsph = gluNewQuadric();
	gluSphere(Qsph, 600.0 + 5000 * sin_sq*sin_sq, 27, 27);
	gluDeleteQuadric(Qsph);
	glPopMatrix();

	GLfloat GroundSurface[] = { 0.5*(0.4 + 0.6*sin_angminpi), 0.5*(0.2 + 0.8*sin_angminpi), 0.3*(0.1 + 0.9*sin_angminpi), 1.0f };

	//glPushMatrix();
	for (int w = -1; w < 2; w++)
		for (int k = -1; k < 2; k++)
		{
			glPushMatrix();

			glTranslatef(env.number_of_columns*env.field_size*k, 0, env.number_of_rows*env.field_size*w);

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, BlueSurface);
			glEnable(GL_BLEND);

			my_car->DrawObject();

			// Lock the Critical section
			EnterCriticalSection(&m_cs);
			for (map<int, MovableObject*>::iterator it = other_cars.begin(); it != other_cars.end(); ++it)
			{
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, DGreenSurface);
				it->second->DrawObject();
			}
			//Release the Critical section
			LeaveCriticalSection(&m_cs);

			glDisable(GL_BLEND);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, GroundSurface);

			env.Draw();
			glPopMatrix();
		}



	glPopMatrix();

	glFlush();

}

void WindowResize(int cx, int cy)
{
	GLsizei width, height;
	GLdouble aspect;
	width = cx;
	height = cy;

	if (cy == 0)
		aspect = (GLdouble)width;
	else
		aspect = (GLdouble)width / (GLdouble)height;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(35 * viewpar.zoom, aspect, 1, 100000.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDrawBuffer(GL_BACK);

	glEnable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);

}


void EndOfGraphics()
{
	if (wglGetCurrentContext() != NULL)
	{
		// dezaktualizacja contextu renderuj¹cego
		wglMakeCurrent(NULL, NULL);
	}
	if (g_hGLContext != NULL)
	{
		wglDeleteContext(g_hGLContext);
		g_hGLContext = NULL;
	}
	glDeleteLists(font_base, 96);
}

BOOL SetWindowPixelFormat(HDC hDC)
{
	PIXELFORMATDESCRIPTOR pixelDesc;

	pixelDesc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixelDesc.nVersion = 1;
	pixelDesc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_STEREO_DONTCARE;
	pixelDesc.iPixelType = PFD_TYPE_RGBA;
	pixelDesc.cColorBits = 32;
	pixelDesc.cRedBits = 8;
	pixelDesc.cRedShift = 16;
	pixelDesc.cGreenBits = 8;
	pixelDesc.cGreenShift = 8;
	pixelDesc.cBlueBits = 8;
	pixelDesc.cBlueShift = 0;
	pixelDesc.cAlphaBits = 0;
	pixelDesc.cAlphaShift = 0;
	pixelDesc.cAccumBits = 64;
	pixelDesc.cAccumRedBits = 16;
	pixelDesc.cAccumGreenBits = 16;
	pixelDesc.cAccumBlueBits = 16;
	pixelDesc.cAccumAlphaBits = 0;
	pixelDesc.cDepthBits = 32;
	pixelDesc.cStencilBits = 8;
	pixelDesc.cAuxBuffers = 0;
	pixelDesc.iLayerType = PFD_MAIN_PLANE;
	pixelDesc.bReserved = 0;
	pixelDesc.dwLayerMask = 0;
	pixelDesc.dwVisibleMask = 0;
	pixelDesc.dwDamageMask = 0;
	g_GLPixelIndex = ChoosePixelFormat(hDC, &pixelDesc);

	if (g_GLPixelIndex == 0)
	{
		g_GLPixelIndex = 1;

		if (DescribePixelFormat(hDC, g_GLPixelIndex, sizeof(PIXELFORMATDESCRIPTOR), &pixelDesc) == 0)
		{
			return FALSE;
		}
	}

	if (SetPixelFormat(hDC, g_GLPixelIndex, &pixelDesc) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}
BOOL CreateViewGLContext(HDC hDC)
{
	g_hGLContext = wglCreateContext(hDC);

	if (g_hGLContext == NULL)
	{
		return FALSE;
	}

	if (wglMakeCurrent(hDC, g_hGLContext) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

GLvoid BuildFont(HDC hDC)								// Build Our Bitmap Font
{
	HFONT	font;										// Windows Font ID
	HFONT	oldfont;									// Used For Good House Keeping

	font_base = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(-14,							// Height Of Font
		0,								// Width Of Font
		0,								// Angle Of Escapement
		0,								// Orientation Angle
		FW_NORMAL,						// Font Weight
		FALSE,							// Italic
		FALSE,							// Underline
		FALSE,							// Strikeout
		ANSI_CHARSET,					// Character Set Identifier
		OUT_TT_PRECIS,					// Output Precision
		CLIP_DEFAULT_PRECIS,			// Clipping Precision
		ANTIALIASED_QUALITY,			// Output Quality
		FF_DONTCARE | DEFAULT_PITCH,		// Family And Pitch
		"Courier New");					// Font Name

	oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, font_base);				// Builds 96 Characters Starting At Character 32
	SelectObject(hDC, oldfont);							// Selects The Font We Want
	DeleteObject(font);									// Delete The Font
}

// Napisy w OpenGL
GLvoid glPrint(const char *fmt, ...)	// Custom GL "Print" Routine
{
	char		text[256];	// Holds Our String
	va_list		ap;		// Pointer To List Of Arguments

	if (fmt == NULL)		// If There's No Text
		return;			// Do Nothing

	va_start(ap, fmt);		// Parses The String For Variables
	vsprintf(text, fmt, ap);	// And Converts Symbols To Actual Numbers
	va_end(ap);			// Results Are Stored In Text

	glPushAttrib(GL_LIST_BIT);	// Pushes The Display List Bits
	glListBase(font_base - 32);		// Sets The Base Character to 32
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();			// Pops The Display List Bits
}


void CreateDisplayLists()
{
	glNewList(Wall1, GL_COMPILE);	// GL_COMPILE - lista jest kompilowana, ale nie wykonywana

	glBegin(GL_QUADS);		// inne opcje: GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP
	// GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUAD_STRIP, GL_POLYGON
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-1.0, -1.0, 1.0);
	glVertex3f(-1.0, 1.0, 1.0);
	glVertex3f(-1.0, 1.0, -1.0);
	glVertex3f(-1.0, -1.0, -1.0);
	glEnd();
	glEndList();

	glNewList(Wall2, GL_COMPILE);
	glBegin(GL_QUADS);
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(1.0, -1.0, 1.0);
	glVertex3f(1.0, 1.0, 1.0);
	glVertex3f(1.0, 1.0, -1.0);
	glVertex3f(1.0, -1.0, -1.0);
	glEnd();
	glEndList();

	glNewList(Auto, GL_COMPILE);
	glBegin(GL_QUADS);
	// przod
	glNormal3f(0.0, 0.0, 1.0);

	glVertex3f(0, 0, 1);
	glVertex3f(0, 1, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 0, 1);

	glVertex3f(0.7, 0, 1);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0, 1);
	// tyl
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0.7, 0, 0);
	glVertex3f(0.7, 1, 0);
	glVertex3f(0, 1, 0);

	glVertex3f(0.7, 0, 0);
	glVertex3f(1.0, 0, 0);
	glVertex3f(1.0, 0.5, 0);
	glVertex3f(0.7, 0.5, 0);
	// gora
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(0, 1, 0);
	glVertex3f(0, 1, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 1, 0);

	glVertex3f(0.7, 0.5, 0);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0.5, 0);
	// dol
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(0, 0, 0);
	glVertex3f(1, 0, 0);
	glVertex3f(1, 0, 1);
	glVertex3f(0, 0, 1);
	// prawo
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f(0.7, 0.5, 0);
	glVertex3f(0.7, 0.5, 1);
	glVertex3f(0.7, 1, 1);
	glVertex3f(0.7, 1, 0);

	glVertex3f(1.0, 0.0, 0);
	glVertex3f(1.0, 0.0, 1);
	glVertex3f(1.0, 0.5, 1);
	glVertex3f(1.0, 0.5, 0);
	// lewo
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 1, 0);
	glVertex3f(0, 1, 1);
	glVertex3f(0, 0, 1);

	glEnd();
	glEndList();

}


void DrawGlobalCoordAxes(void)
{

	glColor3f(1, 0, 0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(2, 0, 0);
	glVertex3f(2, -0.25, 0.25);
	glVertex3f(2, 0.25, -0.25);
	glVertex3f(2, -0.25, -0.25);
	glVertex3f(2, 0.25, 0.25);

	glEnd();
	glColor3f(0, 1, 0);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(0.25, 2, 0);
	glVertex3f(0, 2, 0);
	glVertex3f(-0.25, 2, 0.25);
	glVertex3f(0, 2, 0);
	glVertex3f(-0.25, 2, -0.25);

	glEnd();
	glColor3f(0, 0, 1);
	glBegin(GL_LINES);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 2);
	glVertex3f(-0.25, -0.25, 2);
	glVertex3f(0.25, 0.25, 2);
	glVertex3f(-0.25, -0.25, 2);
	glVertex3f(0.25, -0.25, 2);
	glVertex3f(-0.25, 0.25, 2);
	glVertex3f(0.25, 0.25, 2);

	glEnd();

	glColor3f(1, 1, 1);
}

