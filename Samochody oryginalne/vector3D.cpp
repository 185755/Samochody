/*********************************************************************************
	Operacje na __vectorach 3- wymiarowych + kilka przydatnych funkcji graficznych
	**********************************************************************************/
#include <math.h>
#include "vector3D.h"

float pii = 3.14159265358979;

Vector3::Vector3(float x, float y, float z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

Vector3::Vector3()
{
	x = 0;
	y = 0;
	z = 0;
}

Vector3 Vector3::operator* (float constant_value)  // mnozenie przez wartosc
{
	return Vector3(x*constant_value, y*constant_value, z*constant_value);
}


Vector3 Vector3::operator/ (float constant_value)			// operator* is used to scale a Vector3D by a value. This value multiplies the Vector3D's x, y and z.
{
	if (constant_value != 0)
		return Vector3(x / constant_value, y / constant_value, z / constant_value);
	else return Vector3(x, y, z);

}

Vector3 Vector3::operator+ (float constant_value)  // dodanie vec+vec
{
	return Vector3(x + constant_value, y + constant_value, z + constant_value);
};


Vector3 Vector3::operator+=(float constant_value)  // dodanie vec+vec
{
	x += constant_value;
	y += constant_value;
	z += constant_value;
	return *this;
};

Vector3 Vector3:: operator+= (Vector3 ww)			// operator+= is used to add another Vector3D to this Vector3D.
{
	x += ww.x;
	y += ww.y;
	z += ww.z;
	return *this;
}



Vector3 Vector3::operator+ (Vector3 ww)  // dodanie vec+vec
{
	return Vector3(x + ww.x, y + ww.y, z + ww.z);
};


Vector3 Vector3::operator-=(Vector3 ww)  // dodanie vec+vec
{
	x -= ww.x;
	y -= ww.y;
	z -= ww.z;
	return *this;
};

Vector3 Vector3::operator- (Vector3 ww)  // dodanie vec+vec
{
	return Vector3(x - ww.x, y - ww.y, z - ww.z);
};

Vector3 Vector3::operator- ()             // znak (-)
{
	return Vector3(-x, -y, -z);
};

Vector3 Vector3::operator*(Vector3 ww)  // iloczyn __vectorowy
{
	Vector3 w;
	w.x = y*ww.z - z*ww.y;
	w.y = z*ww.x - x*ww.z;
	w.z = x*ww.y - y*ww.x;
	return w;
}

bool Vector3::operator==(Vector3 v2)
{
	return (x == v2.x) && (y == v2.y) && (z == v2.z);
}

Vector3 Vector3::rotation(float steering_angle, float vn0, float vn1, float vn2)
{
	float s = sin(steering_angle), c = cos(steering_angle);

	Vector3 w;
	w.x = x*(vn0*vn0 + c*(1 - vn0*vn0)) + y*(vn0*vn1*(1 - c) - s*vn2) + z*(vn0*vn2*(1 - c) + s*vn1);
	w.y = x*(vn1*vn0*(1 - c) + s*vn2) + y*(vn1*vn1 + c*(1 - vn1*vn1)) + z*(vn1*vn2*(1 - c) - s*vn0);
	w.z = x*(vn2*vn0*(1 - c) - s*vn1) + y*(vn2*vn1*(1 - c) + s*vn0) + z*(vn2*vn2 + c*(1 - vn2*vn2));

	return w;
}

Vector3 Vector3::znorm()
{
	float d = length();
	if (d > 0)
		return Vector3(x / d, y / d, z / d);
	else return Vector3(0, 0, 0);
}

Vector3 Vector3::znorm2D()
{
	float d = sqrt(x*x + y*y);
	if (d > 0)
		return Vector3(x / d, y / d, 0);
	else return Vector3(0, 0, 0);
}

float Vector3::operator^(Vector3 w) // iloczyn skalarny
{
	return w.x*x + w.y*y + w.z*z;
}

float Vector3::length() // length aktualnego __vectora
{
	return sqrt(x*x + y*y + z*z);
}

// Funkcja obliczajaca normal_vector do plaszczyzny wyznaczanej przez triangle ABC
// zgodnie z ruchem wskazowek zegara:
Vector3 normal_vector(Vector3 A, Vector3 B, Vector3 C)
{
	Vector3 w1 = B - A, w2 = C - A;
	return (w1*w2).znorm();
}



// zwraca kat pomiedzy __vectorami w zakresie <0,2pi) w kierunku przeciwnym
// do ruchu wskazowek zegara. Zakladam, ze Wa.z = Wb.z = 0
float angle_between_vectors2D(Vector3 Wa, Vector3 Wb)
{

	Vector3 vector_prod = Wa.znorm2D() * Wb.znorm2D();  // iloczyn __vectorowy __vectorow o jednostkowej lengthi
	float sin_angle = vector_prod.z;        // problem w tym, ze sin(steering_angle) == sin(pi-steering_angle)   
	if (sin_angle == 0)
	{
		if (Wa.znorm2D() == Wb.znorm2D()) return 0;
		if (Wa.znorm2D() == -Wb.znorm2D()) return pii;
	}
	// dlatego trzeba  jeszcze sprawdzic czy to kat rozwarty
	Vector3 wso_n = Wa.znorm2D() + Wb.znorm2D();
	Vector3 vector_prod_1 = wso_n.znorm2D() * Wb.znorm2D();
	bool obtuse_angle = (vector_prod_1.length() > sqrt(2.0) / 2);

	float angle = asin(fabs(sin_angle));
	if (obtuse_angle) angle = pii - angle;
	if (sin_angle < 0) angle = 2 * pii - angle;

	return angle;
}

/*
   wyznaczanie punktu przeciecia sie 2 odcinkow AB i CD lub ich przedluzen
   zwraca 1 jesli odcinki sie przecinaja
   */
bool point_of_intersection2D(float *x, float *y, float xA, float yA, float xB, float yB,
	float xC, float yC, float xD, float yD)
{
	float a1, b1, c1, a2, b2, c2;  // rownanie prostej: ax+by+c=0 
	a1 = (yB - yA); b1 = -(xB - xA); c1 = (xB - xA)*yA - (yB - yA)*xA;
	a2 = (yD - yC); b2 = -(xD - xC); c2 = (xD - xC)*yC - (yD - yC)*xC;
	float denominator = a1*b2 - a2*b1;
	if (denominator == 0) { *x = 0; *y = 0; return 0; }  // odcinki rownolegle
	float xx = (c2*b1 - c1*b2) / denominator,
		yy = (c1*a2 - c2*a1) / denominator;
	*x = xx; *y = yy;
	if ((((xx > xA) && (xx < xB)) || ((xx < xA) && (xx > xB)) ||
		((yy > yA) && (yy < yB)) || ((yy < yA) && (yy > yB))) &&
		(((xx > xC) && (xx < xD)) || ((xx < xC) && (xx > xD)) ||
		((yy > yC) && (yy < yD)) || ((yy < yC) && (yy > yD)))
		)
		return 1;
	else
		return 0;
}
