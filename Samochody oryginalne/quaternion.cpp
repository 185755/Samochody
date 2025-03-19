/**************************************************************************************************
	Obs³uga quaternionów

	Aby zrealizowaæ obrót nale¿y pomno¿yæ quaternion orientacji przez kwaterion obrotu
	z lewej strony:

	qOrient = qRot*qOrient

	Uwaga! Zak³adam, ¿e quaterniony qRot i qOrient s¹ d³ugoœci 1, inaczej trzeba jeszcze je normalizowaæ

	Kwaternion mo¿e tez reprezentowaæ orientacjê obiektu, jeœli za³o¿ymy jak¹œ orientacjê
	pocz¹tkow¹ np. obiekt skierowany w kierunku osi 0x, z normaln¹ zgodn¹ z 0y. Wówczas
	quaternion odpowiada obrotowi od po³o¿enia poc¿¹tkowego.
	****************************************************************************************************/
#include <stdlib.h>
#include "quaternion.h"


quaternion::quaternion(float x, float y, float z, float w)

{

	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
}


quaternion::quaternion()
{
	x = 0;
	y = 0;
	z = 0;
	w = 1;
}

// mno¿enie dwóch quaternionów
quaternion quaternion::operator*(quaternion q)
{
	double rx, ry, rz, rw;		// temp result	



	rx = x*q.w + w*q.x - z*q.y + y*q.z;
	ry = y*q.w + w*q.y - x*q.z + z*q.x;
	rz = z*q.w + w*q.z - y*q.x + x*q.y;

	rw = w*q.w - x*q.x - y*q.y - z*q.z;

	return quaternion((float)rx, (float)ry, (float)rz, (float)rw);
}

// zamiana quaterniona na reprezentacjê k¹towo-osiow¹
// (oœ jest reprezentowana __vectorem, k¹t d³ugoœci¹ __vectora) 
Vector3 quaternion::AsixAngle()
{
	Vector3 v(x, y, z);

	if (v.length() == 0) return Vector3(0, 0, 0);
	else
	{
		v = v.znorm();
		return Vector3(v.x, v.y, v.z)*2.0*acos(w);
	}
}

quaternion quaternion::operator~ ()
{
	return quaternion(-x, -y, -z, w);
}

quaternion quaternion:: operator+= (quaternion q)	// operator+= is used to add another Vector3D to this Vector3D.
{
	x += q.x;
	y += q.y;
	z += q.z;
	w += q.w;
	return *this;
}

quaternion quaternion:: operator+ (quaternion q)	// operator+= is used to add another Vector3D to this Vector3D.
{
	return quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
}

quaternion quaternion:: operator- (quaternion q)	// operator+= is used to add another Vector3D to this Vector3D.
{
	return quaternion(x - q.x, y - q.y, z - q.z, w - q.w);
}

quaternion quaternion::n() // licz normal_vector z obecnego __vectora
{
	float length;

	length = sqrt(x*x + y*y + z*z + w*w);
	if (length == 0) return quaternion(0, 0, 0, 0);
	else return quaternion(x / length, y / length, z / length, w / length);
}

float quaternion::l() // licz normal_vector z obecnego __vectora
{
	return sqrt(x*x + y*y + z*z + w*w);
}

quaternion quaternion::operator* (float value)  // mnozenie razy skalar
{
	return quaternion(x*value, y*value, z*value, w*value);
}

quaternion quaternion::operator/ (float value)  // dzielenie przez skalar
{
	return quaternion(x / value, y / value, z / value, w / value);
}



/*  obrót __vectora z u¿yciem quaterniona obrotu - do sprawdzenia
macierz obrotu:
M =  [ w^2 + x^2 - y^2 - z^2       2xy - 2wz                   2xz + 2wy
2xy + 2wz                   w^2 - x^2 + y^2 - z^2       2yz - 2wx
2xz - 2wy                   2yz + 2wx                   w^2 - x^2 - y^2 + z^2 ]
*/
Vector3 quaternion::rotate_vector(Vector3 V)
{
	Vector3 Vo;

	quaternion p = quaternion(V.x, V.y, V.z, 0), q = quaternion(x, y, z, w);
	quaternion pp = q*p*~q;
	Vo.x = pp.x; Vo.y = pp.y; Vo.z = pp.z;

	return Vo;
}

// zamiana obrotu wyra¿onego reprezentacj¹ k¹towo-osiow¹ na reprezentacjê quaternionow¹  
quaternion AsixToQuat(Vector3 v, float angle)
{
	float x, y, z, w;			// temp vars of vector
	double rad, scale;		// temp vars

	if (v.length() == 0) return quaternion();

	v = v.znorm();
	rad = angle;
	scale = sin(rad / 2.0);


	w = (float)cos(rad / 2.0);

	x = float(v.x * scale);
	y = float(v.y * scale);
	z = float(v.z * scale);

	return quaternion(x, y, z, w);
}
