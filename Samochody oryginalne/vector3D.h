

#define _VEKTOR3D__H

class Vector3
{
public:
	float x, y, z;
	Vector3();
	Vector3(float _x, float _y, float _z);
	Vector3 operator* (float value);

	Vector3 operator/ (float value);
	Vector3 operator+=(Vector3 v);    // dodanie vec+vec
	Vector3 operator+(Vector3 v);     // dodanie vec+vec
	Vector3 operator+=(float constant_value);    // dodanie vec+liczba
	Vector3 operator+(float constant_value);     // dodanie vec+liczba
	Vector3 operator-=(Vector3 v);    // dodanie vec+vec
	Vector3 operator-(Vector3 v);     // odejmowanie vec-vec
	Vector3 operator*(Vector3 v);     // iloczyn __vectorowy
	Vector3 operator- ();              // odejmowanie vec-vec
	bool operator==(Vector3 v2); // porownanie
	Vector3 rotation(float steering_angle, float vn0, float vn1, float vn2);
	Vector3 znorm();
	Vector3 znorm2D();
	float operator^(Vector3 v);        // iloczyn scalarny
	float length(); // length aktualnego __vectora    
};

Vector3 normal_vector(Vector3 A, Vector3 B, Vector3 C);  // normal_vector do trianglea ABC
float angle_between_vectors2D(Vector3 Wa, Vector3 Wb);  // zwraca kat pomiedzy __vectorami
bool point_of_intersection2D(float *x, float *y, float xA, float yA, float xB, float yB,
	float xC, float yC, float xD, float yD);
