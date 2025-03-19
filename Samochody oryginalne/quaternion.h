
#ifndef _QUATMATHVECT__H
#define _QUATMATHVECT__H

#define M_PI 3.14159265358979323846  
#define M_RAD 180/M_PI

#include <math.h>

#ifndef _VEKTOR3D__H 
#include "vector3D.h"
#endif


struct quaternion
{
	float x, y, z, w;

	quaternion(float x1, float y1, float z1, float w1);
	quaternion();

	Vector3 AsixAngle();        // zamiana quaterniona na reprezentacjê k¹towo-osiow¹
	Vector3 rotate_vector(Vector3 w);      // obrót __vectora z u¿yciem quaterniona obrotu

	quaternion operator*(quaternion q); // iloczyn vectorowy
	quaternion operator~ ();
	quaternion operator+=(quaternion q);  // dodanie vec+vec
	quaternion operator+ (quaternion q);  // dodanie vec+vec
	quaternion operator- (quaternion q);  // odejmowanie vec-vec
	quaternion n(); // licz normal_vector z obecnego quaternionu 
	float l(); // length kwateriona 
	quaternion operator* (float value); // mnozenie razy skalar
	quaternion operator/ (float value); // dzielenie przez skalar


};

quaternion AsixToQuat(Vector3 v, float angle);

#endif
