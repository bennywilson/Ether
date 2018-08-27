//===================================================================================================
// kbVector.h
//
//
// 2016-2018 kbEngine 2.0
//===================================================================================================
#ifndef __KBVECTOR_H_
#define __KBVECTOR_H_

/**
 *	kbVec2i
 */
class kbVec2i {
public:
	kbVec2i() { }
	kbVec2i( const int inX, const int inY ) : x( inX ), y( inY ) { }
	void Set( const int inX, const int inY ) { x = inX, y = inY; }

	int x, y;
};

/**
 *	kbVec2
 */
class kbVec2 {
public:
	kbVec2() { }
	kbVec2( const float initX, const float initY ) { x = initX, y = initY; }

	void	Set( const float initX, const float initY ) { x = initX, y = initY; }

	kbVec2	operator +( const kbVec2 & rhs ) const { return kbVec2( x + rhs.x, y + rhs.y ); }
	void	operator +=( const kbVec2 & rhs ) { x += rhs.x, y += rhs.y; }

	kbVec2	operator -( const kbVec2 & rhs ) const { return kbVec2( x - rhs.x, y - rhs.y ); }
	void	operator -=( const kbVec2 & rhs ) { x -= rhs.x, y -= rhs.y; }

	void	operator *=(const float rhs ) { x *= rhs, y *= rhs; }

	bool Compare( const kbVec2 & op2, const float epsilon = 0.0001f ) const { return fabs( x - op2.x ) < epsilon && fabs( y - op2.y ) < epsilon; }

	const float operator[]( const int index ) const { return ( &x )[index]; }
	float & operator[]( const int index ) { return ( &x )[index]; }
	float * ToFloat() const { return ( float * ) this; }

	void Rotate( const float angle )
	{
		const float rad = angle * ( kbPI / 180.f );
		const float cosAngle = cos( rad );
		const float sinAngle = sin( rad );

		float newX = ( x * cosAngle ) + ( y * sinAngle );
		float newY = ( x * -sinAngle ) + ( y * cosAngle );

		x = newX;
		y = newY;
		Normalize();
	}

	float Length()
	{
		return sqrt( x * x + y * y );
	}

	float LengthSqr()
	{
		return ( x * x + y * y);
	}

	void Normalize()
	{
		float vectorLength = Length();
		float invLength = 1.0f / vectorLength;
		x *= invLength;
		y *= invLength;
	}

	float x,y;
};

kbVec2 operator *( const kbVec2 & op1, const float op2 );

/**
 *	kbVec3i
 */
class kbVec3i {
public:
	kbVec3i( const int inX, const int inY, const int inZ ) : x( inX ), y( inY ), z( inZ ) { }
	int x, y, z;
};

/**
 *	kbVec3
 */
class kbVec3 {
public:

	kbVec3() { }
	kbVec3( const float initX, const float initY, const float initZ ) { x = initX, y = initY, z = initZ; }

	void Set( const float initX, const float initY, const float initZ ) { x = initX, y = initY, z = initZ; }

	kbVec3	operator +(const kbVec3 & rhs) const { return kbVec3(x + rhs.x, y + rhs.y, z + rhs.z); }
	void operator +=(const kbVec3 & rhs) { x += rhs.x, y += rhs.y, z += rhs.z; }
	kbVec3 operator +( const float rhs ) const { return kbVec3( x + rhs, y + rhs, z + rhs ); }

	kbVec3	operator -( const kbVec3 & rhs ) const { return kbVec3(x - rhs.x, y - rhs.y, z - rhs.z); }
	void operator -=( const kbVec3 & rhs ) { x -= rhs.x, y -= rhs.y, z -= rhs.z; }
	kbVec3 operator -( const float rhs ) const { return kbVec3( x - rhs, y - rhs, z - rhs ); }
	kbVec3 operator -() const { return kbVec3( -x, -y, -z ); }

	kbVec3 operator *( const class kbMat4 & ) const;
	kbVec3 operator *( const float op2 ) const { return kbVec3( x * op2, y * op2, z * op2 ); }

	void operator *=( const float op ) { x *= op, y *= op, z *= op; }

	void operator /=( const float rhs )  { x /= rhs, y /= rhs, z /= rhs; }
	kbVec3 operator /( const float rhs ) const { return kbVec3( x / rhs, y / rhs, z / rhs ); };

	kbVec3 operator / ( const kbVec3 & rhs ) const { return kbVec3( x / rhs.x, y / rhs.y, z / rhs.z ); }

	bool Compare( const kbVec3 & op2, float epsilon = 0.0001f ) const { return fabs( x - op2.x ) < epsilon && fabs( y - op2.y ) < epsilon && fabs( z - op2.z ) < epsilon; }

	float Dot( const kbVec3 & rhs ) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
	kbVec3 Cross( const kbVec3 & op2 ) const { return kbVec3( ( y * op2.z ) - ( z * op2.y ), ( z * op2.x ) - ( x * op2.z ), ( x * op2.y ) - ( y * op2.x ) ); }

	void MultiplyComponents( const kbVec3 & op2 ) { x *= op2.x, y *= op2.y, z *= op2.z; }
	void AddComponents( const kbVec3 & op2 ) { x += op2.x, y += op2.y, z += op2.z; }

	float Length() const { return sqrt( LengthSqr() ); }
	float LengthSqr() const { return ( x * x + y * y + z * z ); }

	float Normalize() { float len = Length(); float invLength = 1.f / len; x *= invLength; y *= invLength; z *= invLength; return len; }
	kbVec3 Normalized() const { kbVec3 returnVec = *this; returnVec.Normalize(); return returnVec; }
	
	const float operator[]( const int index ) const { return ( &x )[index]; }
	float & operator[]( const int index ) { return ( &x )[index]; }
	float * ToFloat() const { return ( float * ) this; }

	kbVec3 ToVecXZ() const { return kbVec3( x, 0.0f, z ); }

	float x, y, z;

public:
	static const kbVec3 zero;
	static const kbVec3 right;
	static const kbVec3 up;
    static const kbVec3 down;
	static const kbVec3 forward;
	static const kbVec3 one;
};

kbVec3 operator *( const float op1, const kbVec3 & op2 ); 


/**
 * kbVec4
 */
class kbVec4
{
public:
	kbVec4() { }

	kbVec4( const kbVec3 & inVec ) { x = inVec.x, y = inVec.y, z = inVec.z, w = 1.0f; }
	kbVec4( const kbVec3 & inVec, float inW ) { x = inVec.x, y = inVec.y, z = inVec.z, w = inW; }
	kbVec4( const float initX, const float initY, const float initZ, const float initW ) { x = initX, y = initY, z = initZ, w = initW; }

	void Set( const float inX, const float inY, const float inZ, const float inW ) { x = inX, y = inY, z = inZ, w = inW; }

	kbVec4 operator +( const kbVec4 & op2 ) const { return kbVec4( x + op2.x, y + op2.y, z + op2.z, w + op2.w ); }
	void operator +=( const kbVec4 & op2 ) { x += op2.x; y += op2.y; z += op2.z; w += op2.w; }
	kbVec4	operator -( const kbVec4 & rhs ) const { return kbVec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w ); }

	kbVec4 operator *( const float op2 ) const { return kbVec4( x * op2, y * op2, z * op2, w * op2 ); }
	void operator *=( const float op2 ) { x *= op2; y *= op2, z *= op2, w *= op2; }

	kbVec4 TransformPoint(const kbMat4 & op2, bool bDivideByW = false) const;

	kbVec4 operator /( const float op2 ) const { return kbVec4( x / op2, y / op2, z / op2, w / op2 ); }
	void operator /=( const float op2 ) { x /= op2; y /= op2, z /= op2, w /= op2; }

	const float operator[]( const int index ) const { return ( &x )[index]; }
	float & operator[]( const int index ) { return ( &x )[index]; }

	const kbVec3 & ToVec3() const { return *(kbVec3*)this; }
	kbVec3 & ToVec3() { return *(kbVec3*)this; }

	union {
		struct {
			float x, y, z, w;
		};

		struct {
			float r, g, b, a;
		};
	};

public:
	const static kbVec4 right;
	const static kbVec4 up;
	const static kbVec4 forward;
};

kbVec4 operator *( const float op1, const kbVec4 & op2 ); 

/**
 * kbColor
 */
class kbColor : public kbVec4 {
public:
	kbColor() { }
	kbColor( const float inX, const float inY, const float inZ, const float inW ) :
	  kbVec4( inX, inY, inZ, inW ) { }

	kbColor( const kbVec4 & inVec ) :
		kbVec4( inVec.r, inVec.g, inVec.b, inVec.a ) { }

	const static kbColor red;
	const static kbColor green;
	const static kbColor blue;
	const static kbColor yellow;
	const static kbColor white;
	const static kbColor black;
};

/**
 *	kbMat4
 */
class kbMat4 {
public:

	kbMat4() { }

	explicit kbMat4( const kbVec4 & xAxis, const kbVec4 & yAxis, const kbVec4 & zAxis, const kbVec4 & wAxis );
    explicit kbMat4( const class kbQuat & rotation, const kbVec3 & position );

	void Set( const kbVec4 & xAxis, const kbVec4 & yAxis, const kbVec4 & zAxis, const kbVec4 & wAxis ) {
		mat[0] = xAxis;
		mat[1] = yAxis;
		mat[2] = zAxis;
		mat[3] = wAxis;
	}

	void MakeIdentity() {
		mat[0].x = 1.0f;
		mat[0].y = 0.0f;
		mat[0].z = 0.0f;
		mat[0].w = 0.0f;

		mat[1].x = 0.0f;
		mat[1].y = 1.0f;
		mat[1].z = 0.0f;
		mat[1].w = 0.0f;

		mat[2].x = 0.0f;
		mat[2].y = 0.0f;
		mat[2].z = 1.0f;
		mat[2].w = 0.0f;

		mat[3].x = 0.0f;
		mat[3].y = 0.0f;
		mat[3].z = 0.0f;
		mat[3].w = 1.0f;
	}

	void MakeScale( const kbVec3 & scale ) {
		mat[0].x = scale.x;
		mat[0].y = 0.0f;
		mat[0].z = 0.0f;
		mat[0].w = 0.0f;

		mat[1].x = 0.0f;
		mat[1].y = scale.y;
		mat[1].z = 0.0f;
		mat[1].w = 0.0f;

		mat[2].x = 0.0f;
		mat[2].y = 0.0f;
		mat[2].z = scale.z;
		mat[2].w = 0.0f;

		mat[3].x = 0.0f;
		mat[3].y = 0.0f;
		mat[3].z = 0.0f;
		mat[3].w = 1.0f;
	}

	void LookAt( const kbVec3 & eye, const kbVec3 & at, const kbVec3 & up ) {
		kbVec3 z_axis = at - eye;
		z_axis.Normalize();
		
		kbVec3 x_axis = up.Cross(z_axis);
		x_axis.Normalize();
		
		kbVec3 y_axis = z_axis.Cross(x_axis);
		y_axis.Normalize();

		MakeIdentity();

		mat[0][0] = x_axis.x;
		mat[1][0] = x_axis.y;
		mat[2][0] = x_axis.z;
		mat[3][0] = -(x_axis.Dot(eye));

		mat[0][1] = y_axis.x;
		mat[1][1] = y_axis.y;
		mat[2][1] = y_axis.z;
		mat[3][1] = -(y_axis.Dot(eye));

		mat[0][2] = z_axis.x;
		mat[1][2] = z_axis.y;
		mat[2][2] = z_axis.z;
		mat[3][2] = -(z_axis.Dot(eye));
	}

	void CreatePerspectiveMatrix( const float fov, const float aspect, const float zn, const float zf ) {
		const float yscale = 1.f /tanf(fov / 2.0f );
		const float xscale = yscale / aspect;

		MakeIdentity();

		mat[0][0] = xscale;
		mat[1][1] = yscale;
		mat[2][2] = zf / (zf - zn);
		mat[3][2] = -zn * zf / (zf - zn);
		mat[2][3] = 1;
		mat[3][3] = 0;
	}

	kbMat4 & TransposeSelf() {
		kbMat4 transposedMat;
		transposedMat[0][0] = mat[0][0];
		transposedMat[0][1] = mat[1][0];
		transposedMat[0][2] = mat[2][0];
		transposedMat[0][3] = mat[3][0];
		transposedMat[1][0] = mat[0][1];
		transposedMat[1][1] = mat[1][1];
		transposedMat[1][2] = mat[2][1];
		transposedMat[1][3] = mat[3][1];
		transposedMat[2][0] = mat[0][2];
		transposedMat[2][1] = mat[1][2];
		transposedMat[2][2] = mat[2][2];
		transposedMat[2][3] = mat[3][2];
		transposedMat[3][0] = mat[0][3];
		transposedMat[3][1] = mat[1][3];
		transposedMat[3][2] = mat[2][3];
		transposedMat[3][3] = mat[3][3];

		*this = transposedMat;

		return *this;
	}

	kbMat4 & TransposeUpper() {
		kbMat4 transposedMat;
		transposedMat[0][0] = mat[0][0];
		transposedMat[0][1] = mat[1][0];
		transposedMat[0][2] = mat[2][0];
		transposedMat[0][3] = mat[3][0];
		transposedMat[1][0] = mat[0][1];
		transposedMat[1][1] = mat[1][1];
		transposedMat[1][2] = mat[2][1];
		transposedMat[1][3] = mat[3][1];
		transposedMat[2][0] = mat[0][2];
		transposedMat[2][1] = mat[1][2];
		transposedMat[2][2] = mat[2][2];
		transposedMat[2][3] = mat[3][2];

		transposedMat[3][0] = transposedMat[3][1] = transposedMat[3][2] = 0;
		transposedMat[3][3] = 1;

		*this = transposedMat;

		return *this;
	}
	
	void InverseProjection()
	{
		kbMat4 TempMatrix(*this);

		float OneOverUpperDet = 1.f / ((mat[0][0] * mat[1][1]) - (mat[0][1] * mat[1][0]));
		mat[0][0] = TempMatrix[1][1] * OneOverUpperDet;
		mat[1][1] = TempMatrix[0][0] * OneOverUpperDet;

		float OneOverLowerDet = 1.f / ((mat[2][2] * mat[3][3]) - (mat[2][3] * mat[3][2]));
		mat[2][2] = TempMatrix[3][3] * OneOverLowerDet;
		mat[3][3] = TempMatrix[2][2] * OneOverLowerDet;
		mat[2][3] = -TempMatrix[2][3] * OneOverLowerDet;
		mat[3][2] = -TempMatrix[3][2] * OneOverLowerDet;
	}

	void InvertFast()
	{
		kbVec3 Trans(-mat[3][0], -mat[3][1], -mat[3][2]);

		mat[3][0] = mat[3][1] = mat[3][2] = 0;
		TransposeUpper();
	
		Trans = Trans * *this;
		mat[3][0] = Trans.x;
		mat[3][1] = Trans.y;
		mat[3][2] = Trans.z;
	}

	kbVec4 & operator[]( const int index ) { return mat[ index ]; }

	const kbVec4 & operator[]( const int index ) const { return mat[ index ]; }

	void operator *=( const kbMat4 & op2 ) {
		kbMat4 tempMatrix = *this;

		mat[0][0] = (tempMatrix[0][0] * op2[0][0]) + (tempMatrix[0][1] * op2[1][0]) + (tempMatrix[0][2] * op2[2][0]) + (tempMatrix[0][3] * op2[3][0]);
		mat[1][0] = (tempMatrix[1][0] * op2[0][0]) + (tempMatrix[1][1] * op2[1][0]) + (tempMatrix[1][2] * op2[2][0]) + (tempMatrix[1][3] * op2[3][0]);
		mat[2][0] = (tempMatrix[2][0] * op2[0][0]) + (tempMatrix[2][1] * op2[1][0]) + (tempMatrix[2][2] * op2[2][0]) + (tempMatrix[2][3] * op2[3][0]);
		mat[3][0] = (tempMatrix[3][0] * op2[0][0]) + (tempMatrix[3][1] * op2[1][0]) + (tempMatrix[3][2] * op2[2][0]) + (tempMatrix[3][3] * op2[3][0]);
	
		mat[0][1] = (tempMatrix[0][0] * op2[0][1]) + (tempMatrix[0][1] * op2[1][1]) + (tempMatrix[0][2] * op2[2][1]) + (tempMatrix[0][3] * op2[3][1]);
		mat[1][1] = (tempMatrix[1][0] * op2[0][1]) + (tempMatrix[1][1] * op2[1][1]) + (tempMatrix[1][2] * op2[2][1]) + (tempMatrix[1][3] * op2[3][1]);
		mat[2][1] = (tempMatrix[2][0] * op2[0][1]) + (tempMatrix[2][1] * op2[1][1]) + (tempMatrix[2][2] * op2[2][1]) + (tempMatrix[2][3] * op2[3][1]);
		mat[3][1] = (tempMatrix[3][0] * op2[0][1]) + (tempMatrix[3][1] * op2[1][1]) + (tempMatrix[3][2] * op2[2][1]) + (tempMatrix[3][3] * op2[3][1]);

		mat[0][2] = (tempMatrix[0][0] * op2[0][2]) + (tempMatrix[0][1]  * op2[1][2]) + (tempMatrix[0][2] * op2[2][2]) + (tempMatrix[0][3] * op2[3][2]);
		mat[1][2] = (tempMatrix[1][0] * op2[0][2]) + (tempMatrix[1][1]  * op2[1][2]) + (tempMatrix[1][2] * op2[2][2]) + (tempMatrix[1][3] * op2[3][2]);
		mat[2][2] = (tempMatrix[2][0] * op2[0][2]) + (tempMatrix[2][1]  * op2[1][2]) + (tempMatrix[2][2] * op2[2][2]) + (tempMatrix[2][3] * op2[3][2]);
		mat[3][2] = (tempMatrix[3][0] * op2[0][2]) + (tempMatrix[3][1]  * op2[1][2]) + (tempMatrix[3][2] * op2[2][2]) + (tempMatrix[3][3] * op2[3][2]);

		mat[0][3] = (tempMatrix[0][0] * op2[0][3]) + (tempMatrix[0][1] * op2[1][3]) + (tempMatrix[0][2] * op2[2][3]) + (tempMatrix[0][3] * op2[3][3]);
		mat[1][3] = (tempMatrix[1][0] * op2[0][3]) + (tempMatrix[1][1] * op2[1][3]) + (tempMatrix[1][2] * op2[2][3]) + (tempMatrix[1][3] * op2[3][3]);
		mat[2][3] = (tempMatrix[2][0] * op2[0][3]) + (tempMatrix[2][1] * op2[1][3]) + (tempMatrix[2][2] * op2[2][3]) + (tempMatrix[2][3] * op2[3][3]);
		mat[3][3] = (tempMatrix[3][0] * op2[0][3]) + (tempMatrix[3][1] * op2[1][3]) + (tempMatrix[3][2] * op2[2][3]) + (tempMatrix[3][3] * op2[3][3]);
	}

	kbMat4 operator*( const kbMat4 & MatrixOperand ) const {
		kbMat4 tempMatrix;

		tempMatrix[0][0] = (mat[0][0] * MatrixOperand[0][0]) + (mat[0][1] * MatrixOperand[1][0]) + (mat[0][2] * MatrixOperand[2][0]) + (mat[0][3] * MatrixOperand[3][0]);
		tempMatrix[1][0] = (mat[1][0] * MatrixOperand[0][0]) + (mat[1][1] * MatrixOperand[1][0]) + (mat[1][2] * MatrixOperand[2][0]) + (mat[1][3] * MatrixOperand[3][0]);
		tempMatrix[2][0] = (mat[2][0] * MatrixOperand[0][0]) + (mat[2][1] * MatrixOperand[1][0]) + (mat[2][2] * MatrixOperand[2][0]) + (mat[2][3] * MatrixOperand[3][0]);
		tempMatrix[3][0] = (mat[3][0] * MatrixOperand[0][0]) + (mat[3][1] * MatrixOperand[1][0]) + (mat[3][2] * MatrixOperand[2][0]) + (mat[3][3] * MatrixOperand[3][0]);
	
		tempMatrix[0][1] = (mat[0][0] * MatrixOperand[0][1]) + (mat[0][1] * MatrixOperand[1][1]) + (mat[0][2] * MatrixOperand[2][1]) + (mat[0][3] * MatrixOperand[3][1]);
		tempMatrix[1][1] = (mat[1][0] * MatrixOperand[0][1]) + (mat[1][1] * MatrixOperand[1][1]) + (mat[1][2] * MatrixOperand[2][1]) + (mat[1][3] * MatrixOperand[3][1]);
		tempMatrix[2][1] = (mat[2][0] * MatrixOperand[0][1]) + (mat[2][1] * MatrixOperand[1][1]) + (mat[2][2] * MatrixOperand[2][1]) + (mat[2][3] * MatrixOperand[3][1]);
		tempMatrix[3][1] = (mat[3][0] * MatrixOperand[0][1]) + (mat[3][1] * MatrixOperand[1][1]) + (mat[3][2] * MatrixOperand[2][1]) + (mat[3][3] * MatrixOperand[3][1]);

		tempMatrix[0][2] = (mat[0][0] * MatrixOperand[0][2]) + (mat[0][1]  * MatrixOperand[1][2]) + (mat[0][2] * MatrixOperand[2][2]) + (mat[0][3] * MatrixOperand[3][2]);
		tempMatrix[1][2] = (mat[1][0] * MatrixOperand[0][2]) + (mat[1][1]  * MatrixOperand[1][2]) + (mat[1][2] * MatrixOperand[2][2]) + (mat[1][3] * MatrixOperand[3][2]);
		tempMatrix[2][2] = (mat[2][0] * MatrixOperand[0][2]) + (mat[2][1]  * MatrixOperand[1][2]) + (mat[2][2] * MatrixOperand[2][2]) + (mat[2][3] * MatrixOperand[3][2]);
		tempMatrix[3][2] = (mat[3][0] * MatrixOperand[0][2]) + (mat[3][1]  * MatrixOperand[1][2]) + (mat[3][2] * MatrixOperand[2][2]) + (mat[3][3] * MatrixOperand[3][2]);

		tempMatrix[0][3] = (mat[0][0] * MatrixOperand[0][3]) + (mat[0][1] * MatrixOperand[1][3]) + (mat[0][2] * MatrixOperand[2][3]) + (mat[0][3] * MatrixOperand[3][3]);
		tempMatrix[1][3] = (mat[1][0] * MatrixOperand[0][3]) + (mat[1][1] * MatrixOperand[1][3]) + (mat[1][2] * MatrixOperand[2][3]) + (mat[1][3] * MatrixOperand[3][3]);
		tempMatrix[2][3] = (mat[2][0] * MatrixOperand[0][3]) + (mat[2][1] * MatrixOperand[1][3]) + (mat[2][2] * MatrixOperand[2][3]) + (mat[2][3] * MatrixOperand[3][3]);
		tempMatrix[3][3] = (mat[3][0] * MatrixOperand[0][3]) + (mat[3][1] * MatrixOperand[1][3]) + (mat[3][2] * MatrixOperand[2][3]) + (mat[3][3] * MatrixOperand[3][3]);

		return tempMatrix;
	}

	void OrthoLH( const float width, const float height, const float zn, const float zf ) {
		MakeIdentity();

		mat[0].x = 2.f / width;
		mat[1].y = 2.f / height;
		mat[2].z = 1.f / (zf - zn);
		mat[3].z = zn / (zn - zf);
	}

	kbVec3 TransformPoint( const kbVec3 Point ) const;

	// Retrieve clip plane from projection matrices.  See // http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf

	void GetLeftClipPlane( class kbPlane & ClipPlane );
	void GetRightClipPlane( class kbPlane & ClipPlane );
	void GetTopClipPlane( class kbPlane & ClipPlane );
	void GetBottomClipPlane( class kbPlane & ClipPlane );
	void GetNearClipPlane( class kbPlane & ClipPlane );
	void GetFarClipPlane ( class kbPlane & ClipPlane );

private:
	kbVec4 mat[ 4 ];

public:
	const static kbMat4 identity;

};

#endif