#include <stdio.h>
#include "quaternion.h"

#define PI 3.1415926

struct ObjectState
{
	Vector3 vPos;             // polozenie obiektu (œrodka geometrycznego obiektu) 
	quaternion qOrient;       // orientacja (polozenie katowe)
	Vector3 vV, vA;            // predkosc, przyspiesznie liniowe
	Vector3 vV_ang, vA_ang;   // predkosc i przyspieszenie liniowe
	float steering_angle;               // kat skretu kol w radianach (w lewo - dodatni)
};

// Klasa opisuj¹ca obiekty ruchome
class MovableObject
{
public:
	int iID;                  // identyfikator obiektu

	ObjectState state;

	float F, Fb;               // si³y dzia³aj¹ce na obiekt: F - pchajaca do przodu, Fb - w prawo
	float breaking_factor;    // stopieñ hamowania Fh_max = friction*Fy*ham
	float Fy;                 // si³a nacisku na podstawê pojazdu - gdy obiekt styka siê z pod³o¿em (od niej zale¿y si³a hamowania)
	float steer_wheel_speed;      // prêdkoœæ krêcenia kierownic¹
	bool if_keep_steer_wheel;       // czy kierownica jest trzymana (prêdkoœæ mo¿e byæ zerowa, a kierownica trzymana w pewnym po³o¿eniu nie wraca do po³. zerowego)

	float mass_own;				  // masa obiektu	
	float length, width, height; // rozmiary w kierunku lokalnych osi x,y,z
	float clearance;           // wysokoœæ na której znajduje siê podstawa obiektu
	float front_axis_dist;           // odleg³oœæ od przedniej osi do przedniego zderzaka 
	float back_axis_dist;             // odleg³oœæ od tylniej osi do tylniego zderzaka	
	float steer_wheel_ret_speed;    // prêdkoœæ powrotu kierownicy po puszczeniu

public:
	MovableObject();          // konstruktor
	~MovableObject();
	void ChangeState(ObjectState state);          // zmiana stateu obiektu
	ObjectState State();        // metoda zwracajaca state obiektu
	void Simulation(float dt);  // symulacja ruchu obiektu w oparciu o biezacy state, przylozone sily
	// oraz czas dzialania sil. Efektem symulacji jest nowy state obiektu 
	void DrawObject();			   // odrysowanie obiektu					
};

// Klasa opisuj¹ca env, po którym poruszaj¹ siê obiekty
class Environment
{
public:
	float **height_map;          // wysokoœci naro¿ników oraz œrodków pól
	float ***d;            // wartoœci wyrazu wolnego z równania p³aszczyzny dla ka¿dego trójk¹ta
	Vector3 ***Norm;       // normalne do p³aszczyzn trójk¹tów
	float field_size;    // length boku kwadratowego pola na mapie
	long number_of_rows, number_of_columns; // liczba wierszy i kolumn mapy (kwadratów na wysokoœæ i szerokoœæ)     
	Environment();
	~Environment();
	float DistFromGround(float x, float z);      // okreœlanie wysokoœci dla punktu o wsp. (x,z) 
	void Draw();	                      // odrysowywanie envu   
	void DrawInitialisation();               // tworzenie listy wyœwietlania
	int ReadMap(char filename[128]);
};
