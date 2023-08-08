//  נדיר יעקב Nadir Yaakov
//Created by Ran Dror on April 2018
//Copyright (c) Ran Dror. All right reserved.
//Code can not be copied and/or distributed without the express permission of author
//

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h> 
#include <stdarg.h>
#include "GL/glut.h" 
#include "glm.h"
#include "MatrixAlgebraLib.h"

//////////////////////////////////////////////////////////////////////
// Grphics Pipeline section
//////////////////////////////////////////////////////////////////////
#define WIN_SIZE 500
#define TEXTURE_SIZE 512
#define CAMERA_DISTANCE_FROM_AXIS_CENTER 10

typedef struct {
	GLfloat point3D[4];
	GLfloat normal[4];
	GLfloat TextureCoordinates[2];
	GLfloat point3DeyeCoordinates[4];
	GLfloat NormalEyeCoordinates[4];
	GLfloat pointScreen[4];
	GLfloat PixelValue;
} Vertex;

enum ProjectionTypeEnum { ORTHOGRAPHIC = 1, PERSPECTIVE };
enum DisplayTypeEnum { FACE_VERTEXES = 11, FACE_COLOR, LIGHTING_FLAT, LIGHTING_GOURARD, LIGHTING_PHONG, TEXTURE, TEXTURE_LIGHTING_PHONG };
enum DisplayNormalEnum { DISPLAY_NORMAL_YES = 21, DISPLAY_NORMAL_NO };

typedef struct {
	GLfloat ModelMinVec[3]; //(left, bottom, near) of a model.
	GLfloat ModelMaxVec[3]; //(right, top, far) of a model.
	GLfloat CameraPos[3];
	GLfloat ModelScale;
	GLfloat ModelTranslateVector[3];
	enum ProjectionTypeEnum ProjectionType;
	enum DisplayTypeEnum DisplayType;
	enum DisplayNormalEnum DisplayNormals;
	GLfloat Lighting_Diffuse;
	GLfloat Lighting_Specular;
	GLfloat Lighting_Ambient;
	GLfloat Lighting_sHininess;
	GLfloat LightPosition[3];
} GuiParamsForYou;

GuiParamsForYou GlobalGuiParamsForYou;

//written for you
void setPixel(GLint x, GLint y, GLfloat r, GLfloat g, GLfloat b);

//you should write
void ModelProcessing();
void VertexProcessing(Vertex* v);
void FaceProcessing(Vertex* v1, Vertex* v2, Vertex* v3, GLfloat FaceColor[3]);
GLfloat LightingEquation(GLfloat point[3], GLfloat PointNormal[3], GLfloat LightPos[3], GLfloat Kd, GLfloat Ks, GLfloat Ka, GLfloat n);
void DrawLineDDA(GLint x1, GLint y1, GLint x2, GLint y2, GLfloat r, GLfloat g, GLfloat b);


GLMmodel* model_ptr;
void ClearColorBuffer();
void DisplayColorBuffer();



void GraphicsPipeline()
{
	static GLuint i;
	static GLMgroup* group;
	static GLMtriangle* triangle;
	Vertex v1, v2, v3;
	GLfloat FaceColor[3];

	//calling ModelProcessing every time refreshing screen
	ModelProcessing();

	//call VertexProcessing for every vertrx
	//and then call FaceProcessing for every face
	group = model_ptr->groups;
	srand(0);
	while (group) {
		for (i = 0; i < group->numtriangles; i++) {
			triangle = &(model_ptr->triangles[group->triangles[i]]);

			MatrixCopy(v1.point3D, &model_ptr->vertices[3 * triangle->vindices[0]], 3);
			v1.point3D[3] = 1;
			MatrixCopy(v1.normal, &model_ptr->normals[3 * triangle->nindices[0]], 3);
			v1.normal[3] = 1;
			if (model_ptr->numtexcoords != 0)
				MatrixCopy(v1.TextureCoordinates, &model_ptr->texcoords[2 * triangle->tindices[0]], 2);
			else {
				v1.TextureCoordinates[0] = -1; v1.TextureCoordinates[1] = -1;
			}
			VertexProcessing(&v1);

			MatrixCopy(v2.point3D, &model_ptr->vertices[3 * triangle->vindices[1]], 3);
			v2.point3D[3] = 1;
			MatrixCopy(v2.normal, &model_ptr->normals[3 * triangle->nindices[1]], 3);
			v2.normal[3] = 1;
			if (model_ptr->numtexcoords != 0)
				MatrixCopy(v2.TextureCoordinates, &model_ptr->texcoords[2 * triangle->tindices[1]], 2);
			else {
				v2.TextureCoordinates[0] = -1; v2.TextureCoordinates[1] = -1;
			}
			VertexProcessing(&v2);

			MatrixCopy(v3.point3D, &model_ptr->vertices[3 * triangle->vindices[2]], 3);
			v3.point3D[3] = 1;
			MatrixCopy(v3.normal, &model_ptr->normals[3 * triangle->nindices[2]], 3);
			v3.normal[3] = 1;
			if (model_ptr->numtexcoords != 0)
				MatrixCopy(v3.TextureCoordinates, &model_ptr->texcoords[2 * triangle->tindices[2]], 2);
			else {
				v3.TextureCoordinates[0] = -1; v3.TextureCoordinates[1] = -1;
			}
			VertexProcessing(&v3);

			FaceColor[0] = (GLfloat)rand() / ((GLfloat)RAND_MAX + 1);
			FaceColor[1] = (GLfloat)rand() / ((GLfloat)RAND_MAX + 1);
			FaceColor[2] = (GLfloat)rand() / ((GLfloat)RAND_MAX + 1);
			FaceProcessing(&v1, &v2, &v3, FaceColor);
		}
		group = group->next;
	}

	DisplayColorBuffer();
}

GLfloat Zbuffer[WIN_SIZE][WIN_SIZE];
GLubyte  TextureImage[TEXTURE_SIZE][TEXTURE_SIZE][3];
GLfloat Mmodeling[16];
GLfloat Mlookat[16];
GLfloat Mprojection[16];
GLfloat Mviewport[16];

void ModelProcessing()
{
	float left, right, bottom, top, far, near;
	float leftGlobal, rightGlobal, bottomGlobal, topGlobal, farGlobal, nearGlobal;
	GLfloat scale[16];
	GLfloat translate[16];
	GLfloat eye[16];
	GLfloat rotate[16];
	GLfloat center[3] = { 0,0,0 };	// look at (0,0,0)
	GLfloat up[3] = { 0,1,0 };
	GLfloat w[3];
	GLfloat u[3];
	GLfloat v[3];
	float ratio, difference, move[3];
	int i, sum;

	M4x4identity(translate);
	M4x4identity(scale);
	M4x4identity(eye);

	left = -1;
	right = 1;
	bottom = -1;
	top = 1;

	far = CAMERA_DISTANCE_FROM_AXIS_CENTER + 1;
	near = CAMERA_DISTANCE_FROM_AXIS_CENTER - 1;


	leftGlobal = GlobalGuiParamsForYou.ModelMinVec[0];
	bottomGlobal = GlobalGuiParamsForYou.ModelMinVec[1];
	nearGlobal = GlobalGuiParamsForYou.ModelMinVec[2];

	rightGlobal = GlobalGuiParamsForYou.ModelMaxVec[0];
	topGlobal = GlobalGuiParamsForYou.ModelMaxVec[1];
	farGlobal = GlobalGuiParamsForYou.ModelMaxVec[2];

	// ex2-3-extra: calculating model scaling and translating transformation matrix
	//////////////////////////////////////////////////////////////////////////////////
	M4x4identity(Mmodeling);
	M4x4identity(translate);
	M4x4identity(scale);

	difference = rightGlobal - leftGlobal;
	sum = rightGlobal + leftGlobal;
	if (sum != 0)
		move[0] = -(sum / 2);
	if (topGlobal - bottomGlobal > difference)
		difference = topGlobal - bottomGlobal;
	sum = topGlobal + bottomGlobal;
	if (sum != 0)
		move[1] = -(sum / 2);
	if (farGlobal - nearGlobal > difference)
		difference = farGlobal - nearGlobal;
	sum = farGlobal + nearGlobal;
	if (sum != 0)
		move[2] = -(sum / 2);
	ratio = (2.0 / difference);


	// ex2-3: calculating translate transformation matrix
	/////////////////////////////////////////////////////////////////////////////////

	translate[12] = GlobalGuiParamsForYou.ModelTranslateVector[0]; // move in x
	translate[13] = GlobalGuiParamsForYou.ModelTranslateVector[1]; // move in y
	translate[14] = GlobalGuiParamsForYou.ModelTranslateVector[2]; // move in z
	// ex2-3: calculating scale transformation matrix
	//////////////////////////////////////////////////////////////////////////////////


	scale[0] = GlobalGuiParamsForYou.ModelScale * ratio; // scale x
	scale[5] = GlobalGuiParamsForYou.ModelScale * ratio; // scale y
	scale[10] = GlobalGuiParamsForYou.ModelScale * ratio; // scale z

	M4multiplyM4(Mmodeling, translate, scale);

	// ex2-4: calculating lookat transformation matrix
	//////////////////////////////////////////////////////////////////////////////////
	M4x4identity(Mlookat);
	M4x4identity(eye);
	M4x4identity(rotate);
	eye[12] = -GlobalGuiParamsForYou.CameraPos[0];
	eye[13] = -GlobalGuiParamsForYou.CameraPos[1];
	eye[14] = -GlobalGuiParamsForYou.CameraPos[2];


	/*
	w = (eyepos - centerpos) / normalize(eyepos - centerpos)
	u = (cross(up, w) / normalize(cross(up, w))
	v = cross(w, u)
	*/

	Vminus(w, GlobalGuiParamsForYou.CameraPos, center, 3);
	V3Normalize(w);

	V3cross(u, up, w);
	V3Normalize(u);
	V3cross(v, w, u);

	rotate[0] = u[0];
	rotate[1] = v[0];
	rotate[2] = w[0];

	rotate[4] = u[1];
	rotate[5] = v[1];
	rotate[6] = w[1];

	rotate[8] = u[2];
	rotate[9] = v[2];
	rotate[10] = w[2];


	M4multiplyM4(Mlookat, rotate, eye);

	// ex2-2: calculating Orthographic or Perspective projection transformation matrix
	//////////////////////////////////////////////////////////////////////////////////
	M4x4identity(Mprojection);


	if (GlobalGuiParamsForYou.ProjectionType == ORTHOGRAPHIC)
	{
		Mprojection[0] = 2.0 / (right - left);
		Mprojection[5] = 2.0 / (top - bottom);
		Mprojection[10] = 2.0 / (far - near);
		Mprojection[15] = 1;

		Mprojection[12] = -(left + right) / 2.0;
		Mprojection[13] = -(bottom + top) / 2.0;
		Mprojection[14] = -(far + near) / 2.0;
		Mprojection[15] = 1;
	}
	else
	{
		Mprojection[0] = (2.0 * near) / (right - left);
		Mprojection[5] = (2.0 * near) / (top - bottom);
		Mprojection[8] = ((right + left) / (right - left));
		Mprojection[9] = (top + bottom) / (top - bottom);
		Mprojection[10] = -((far + near) / (far - near));
		Mprojection[11] = -1;
		Mprojection[14] = -((2.0 * far * near) / (far - near));
		Mprojection[15] = 0;
	}


	// ex2-2: calculating viewport transformation matrix
	//////////////////////////////////////////////////////////////////////////////////
	M4x4identity(Mviewport);


	Mviewport[12] = WIN_SIZE / 2.0;
	Mviewport[13] = WIN_SIZE / 2.0;
	Mviewport[14] = 0.5;

	Mviewport[0] = WIN_SIZE / 2.0;
	Mviewport[5] = WIN_SIZE / 2.0;
	Mviewport[10] = 0.5;

	// ex3: clearing color and Z-buffer
	//////////////////////////////////////////////////////////////////////////////////
	ClearColorBuffer(); // setting color buffer to background color
	//add here clearing z-buffer
	for (int i = 0; i < WIN_SIZE; i++)
		for (int j = 0; j < WIN_SIZE; j++)
			Zbuffer[i][j] = 1;

}


void VertexProcessing(Vertex* v)
{
	GLfloat point3DafterModelingTrans[4];
	GLfloat temp1[4], temp2[4];
	GLfloat point3D_plusNormal_screen[4];
	GLfloat Mmodeling3x3[9], Mlookat3x3[9];



	// ex2-3: modeling transformation v->point3D --> point3DafterModelingTrans
	//////////////////////////////////////////////////////////////////////////////////
	//MatrixCopy(point3DafterModelingTrans, v->point3D, 4);
	M4multiplyV4(point3DafterModelingTrans, Mmodeling, v->point3D);


	// ex2-4: lookat transformation point3DafterModelingTrans --> v->point3DeyeCoordinates
	//////////////////////////////////////////////////////////////////////////////////
	//MatrixCopy(v->point3DeyeCoordinates, point3DafterModelingTrans, 4);
	M4multiplyV4(v->point3DeyeCoordinates, Mlookat, point3DafterModelingTrans);

	// ex2-2: transformation from eye coordinates to screen coordinates v->point3DeyeCoordinates --> v->pointScreen
	//////////////////////////////////////////////////////////////////////////////////

	M4multiplyV4(temp1, Mprojection, v->point3DeyeCoordinates);
	M4multiplyV4(v->pointScreen, Mviewport, temp1);





	// ex2-5: transformation normal from object coordinates to eye coordinates v->normal --> v->NormalEyeCoordinates
	//////////////////////////////////////////////////////////////////////////////////
	M3fromM4(Mmodeling3x3, Mmodeling);
	M3fromM4(Mlookat3x3, Mlookat);
	M3multiplyV3(temp1, Mmodeling3x3, v->normal);
	M3multiplyV3(v->NormalEyeCoordinates, Mlookat3x3, temp1);
	V3Normalize(v->NormalEyeCoordinates);
	v->NormalEyeCoordinates[3] = 1;

	// ex2-5: drawing normals 
	//////////////////////////////////////////////////////////////////////////////////
	if (GlobalGuiParamsForYou.DisplayNormals == DISPLAY_NORMAL_YES) {
		V4HomogeneousDivide(v->point3DeyeCoordinates);
		VscalarMultiply(temp1, v->NormalEyeCoordinates, 0.05, 3);
		Vplus(temp2, v->point3DeyeCoordinates, temp1, 4);
		temp2[3] = 1;
		M4multiplyV4(temp1, Mprojection, temp2);
		M4multiplyV4(point3D_plusNormal_screen, Mviewport, temp1);
		V4HomogeneousDivide(point3D_plusNormal_screen);
		V4HomogeneousDivide(v->pointScreen);
		DrawLineDDA(round(v->pointScreen[0]), round(v->pointScreen[1]), round(point3D_plusNormal_screen[0]), round(point3D_plusNormal_screen[1]), 0, 0, 1);
	}

	// ex3: calculating lighting for vertex
	//////////////////////////////////////////////////////////////////////////////////
	v->PixelValue = LightingEquation(v->point3DeyeCoordinates, v->NormalEyeCoordinates, GlobalGuiParamsForYou.LightPosition,
		GlobalGuiParamsForYou.Lighting_Diffuse, GlobalGuiParamsForYou.Lighting_Specular, GlobalGuiParamsForYou.Lighting_Ambient,
		GlobalGuiParamsForYou.Lighting_sHininess);
}



void FaceProcessing(Vertex* v1, Vertex* v2, Vertex* v3, GLfloat FaceColor[3])
{

	V4HomogeneousDivide(v1->pointScreen);
	V4HomogeneousDivide(v2->pointScreen);
	V4HomogeneousDivide(v3->pointScreen);

	if (GlobalGuiParamsForYou.DisplayType == FACE_VERTEXES)
	{

		DrawLineDDA(round(v1->pointScreen[0]), round(v1->pointScreen[1]), round(v2->pointScreen[0]), round(v2->pointScreen[1]), 1, 1, 1);
		DrawLineDDA(round(v2->pointScreen[0]), round(v2->pointScreen[1]), round(v3->pointScreen[0]), round(v3->pointScreen[1]), 1, 1, 1);
		DrawLineDDA(round(v3->pointScreen[0]), round(v3->pointScreen[1]), round(v1->pointScreen[0]), round(v1->pointScreen[1]), 1, 1, 1);

	}
	else {
		//ex3: Barycentric Coordinates and lighting
		//////////////////////////////////////////////////////////////////////////////////
		GLfloat alpha, beta, gamma, left, right, top, bottom;
		GLfloat a1, b1, c1, a2, b2, c2, a3, b3, c3;
		GLfloat x1, y1, x2, y2, x3, y3;
		GLfloat flat, gourad,phong;
		GLfloat depth;
		int x, y;

		//Find rectangle surrounding the triangle 
		left = min(v1->pointScreen[0], v2->pointScreen[0]);
		left = min(left, v3->pointScreen[0]);
		right = max(v1->pointScreen[0], v2->pointScreen[0]);
		right = max(right, v3->pointScreen[0]);

		top = max(v1->pointScreen[1], v2->pointScreen[1]);
		top = max(top, v3->pointScreen[1]);
		bottom = min(v1->pointScreen[1], v2->pointScreen[1]);
		bottom = min(bottom, v3->pointScreen[1]);


		x1 = round(v1->pointScreen[0]);
		y1 = round(v1->pointScreen[1]);
		x2 = round(v2->pointScreen[0]);
		y2 = round(v2->pointScreen[1]);
		x3 = round(v3->pointScreen[0]);
		y3 = round(v3->pointScreen[1]);

		//A = y2 - y1 
		//B = x1 - x2
		//C = (y1 * x2) - (y2 * x1)

		a1 = y1 - y2;
		b1 = x2 - x1;
		c1 = (y2 * x1) - (y1 * x2);

		a2 = y2 - y3;
		b2 = x3 - x2;
		c2 = (y3 * x2) - (y2 * x3);

		a3 = y3 - y1;
		b3 = x1 - x3;
		c3 = (y1 * x3) - (y3 * x1);

		for (x = left; x <= right && x >= 0; x++)
			for (y = bottom; y <= top && y >= 0; y++)
			{
				alpha = (a1 * x + b1 * y + c1) / (a1 * x3 + b1 * y3 + c1);
				beta = (a2 * x + b2 * y + c2) / (a2 * x1 + b2 * y1 + c2);
				gamma = (a3 * x + b3 * y + c3) / (a3 * x2 + b3 * y2 + c3);

				if ((alpha >= 0 && alpha <= 1) && (beta >= 0 && beta <= 1) && (gamma >= 0 && gamma <= 1))
				{
					depth = (alpha * v3->pointScreen[2]) + (beta * v1->pointScreen[2]) + (gamma * v2->pointScreen[2]);
					if (depth < Zbuffer[x][y])
					{
						Zbuffer[x][y] = depth;
						if (GlobalGuiParamsForYou.DisplayType == FACE_COLOR)
						{
							setPixel(x, y, FaceColor[0], FaceColor[1], FaceColor[2]);
						}
						else if (GlobalGuiParamsForYou.DisplayType == LIGHTING_GOURARD)
						{
							gourad = (alpha * v3->PixelValue) + (beta * v1->PixelValue) + (gamma * v2->PixelValue);
							setPixel(x, y, gourad, gourad, gourad);
						}
						else if (GlobalGuiParamsForYou.DisplayType == LIGHTING_FLAT)
						{
							flat = (v1->PixelValue + v2->PixelValue + v3->PixelValue) / 3.0;
							setPixel(x, y, flat, flat, flat);
						}

					}
				}
			}
	}
}



void DrawLineDDA(GLint x1, GLint y1, GLint x2, GLint y2, GLfloat r, GLfloat g, GLfloat b)
{
	float dx, dy, x, y, a, roundx, roundy, temp;
	dx = (x2 - x1);
	dy = (y2 - y1);


	if (dy < -dx)	//swap points
	{
		temp = x1;
		x1 = x2;
		x2 = temp;
		temp = y1;
		y1 = y2;
		y2 = temp;
		dx = (x2 - x1);
		dy = (y2 - y1);
	}
	if (dx <= dy)//second alg
	{
		a = dx / dy;
		x = x1;
		for (y = y1; y < y2; y++)
		{
			roundx = (x > 0.0) ? x + 0.5 : x - 0.5;		//round x
			setPixel(roundx, y, r, g, b);
			x += a;
		}
	}
	if (dy < dx)	//first alg;
	{
		a = dy / dx;
		y = y1;
		for (x = x1; x < x2; x++)
		{
			roundy = (y > 0.0) ? y + 0.5 : y - 0.5;		//round y
			setPixel(x, roundy, r, g, b);
			y += a;
		}
	}

}




GLfloat LightingEquation(GLfloat point[3], GLfloat PointNormal[3], GLfloat LightPos[3], GLfloat Kd, GLfloat Ks, GLfloat Ka, GLfloat n)
{
	//ex3: calculate lighting equation
	//////////////////////////////////////////////////////////////////////////////////
	GLfloat L[3], V[3], N[3], R[3], iRet;
	GLfloat lighting;
	int i;

	for (int i = 0; i < 3; i++)
	{
		N[i] = PointNormal[i];
		L[i] = LightPos[i] - point[i];
		V[i] = PointNormal[i] - point[i];

	}
	//V3Normalize(N);
	V3Normalize(L);
	V3Normalize(V);
	for (int i = 0; i < 3; i++)
	{
		//R = 2 * (N . L) * N - L
		//R[i] = (2 * (V3dot(N, L) * N[i]) - L[i]);
		R[i] = L[i]-(2 * (V3dot(N, L) * N[i]) );
	}

	//V3Normalize(R);									//normalize R
	lighting = max(Ks * pow(V3dot(V, R), n),0) + max(Kd * V3dot(N, L),0) + max(Ka,0);
	return lighting;
}





//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GUI section
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//function declerations
void InitGuiGlobalParams();
void drawingCB(void);
void reshapeCB(int width, int height);
void keyboardCB(unsigned char key, int x, int y);
void keyboardSpecialCB(int key, int x, int y);
void MouseClickCB(int button, int state, int x, int y);
void MouseMotionCB(int x, int y);
void menuCB(int value);
void TerminationErrorFunc(char* ErrorString);
void LoadModelFile();
void DisplayColorBuffer();
void drawstr(char* FontName, int FontSize, GLuint x, GLuint y, char* format, ...);
void TerminationErrorFunc(char* ErrorString);
GLubyte* readBMP(char* imagepath, int* width, int* height);

enum FileNumberEnum { TEAPOT = 100, TEDDY, PUMPKIN, COW, SIMPLE_PYRAMID, FIRST_EXAMPLE, SIMPLE_3D_EXAMPLE, SPHERE, TRIANGLE, Z_BUFFER_EXAMPLE, TEXTURE_BOX, TEXTURE_TRIANGLE, TEXTURE_BARREL, TEXTURE_MOON, TEXTURE_SHEEP };

typedef struct {
	enum FileNumberEnum FileNum;
	GLfloat CameraRaduis;
	GLint   CameraAnleHorizontal;
	GLint   CameraAnleVertical;
	GLint   MouseLastPos[2];
} GuiCalculations;

GuiCalculations GlobalGuiCalculations;

GLuint ColorBuffer[WIN_SIZE][WIN_SIZE][3];

int main(int argc, char** argv)
{
	GLint submenu1_id, submenu2_id, submenu3_id, submenu4_id;

	//initizlizing GLUT
	glutInit(&argc, argv);

	//initizlizing GUI globals
	InitGuiGlobalParams();

	//initializing window
	glutInitWindowSize(WIN_SIZE, WIN_SIZE);
	glutInitWindowPosition(900, 100);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("Computer Graphics HW");

	//registering callbacks
	glutDisplayFunc(drawingCB);
	glutReshapeFunc(reshapeCB);
	glutKeyboardFunc(keyboardCB);
	glutSpecialFunc(keyboardSpecialCB);
	glutMouseFunc(MouseClickCB);
	glutMotionFunc(MouseMotionCB);

	//registering and creating menu
	submenu1_id = glutCreateMenu(menuCB);
	glutAddMenuEntry("open teapot.obj", TEAPOT);
	glutAddMenuEntry("open teddy.obj", TEDDY);
	glutAddMenuEntry("open pumpkin.obj", PUMPKIN);
	glutAddMenuEntry("open cow.obj", COW);
	glutAddMenuEntry("open Simple3Dexample.obj", SIMPLE_3D_EXAMPLE);
	glutAddMenuEntry("open SimplePyramid.obj", SIMPLE_PYRAMID);
	glutAddMenuEntry("open sphere.obj", SPHERE);
	glutAddMenuEntry("open triangle.obj", TRIANGLE);
	glutAddMenuEntry("open FirstExample.obj", FIRST_EXAMPLE);
	glutAddMenuEntry("open ZbufferExample.obj", Z_BUFFER_EXAMPLE);
	glutAddMenuEntry("open TriangleTexture.obj", TEXTURE_TRIANGLE);
	glutAddMenuEntry("open box.obj", TEXTURE_BOX);
	glutAddMenuEntry("open barrel.obj", TEXTURE_BARREL);
	glutAddMenuEntry("open moon.obj", TEXTURE_MOON);
	glutAddMenuEntry("open sheep.obj", TEXTURE_SHEEP);
	submenu2_id = glutCreateMenu(menuCB);
	glutAddMenuEntry("Orthographic", ORTHOGRAPHIC);
	glutAddMenuEntry("Perspective", PERSPECTIVE);
	submenu3_id = glutCreateMenu(menuCB);
	glutAddMenuEntry("Face Vertexes", FACE_VERTEXES);
	glutAddMenuEntry("Face Color", FACE_COLOR);
	glutAddMenuEntry("Lighting Flat", LIGHTING_FLAT);
	glutAddMenuEntry("Lighting Gourard", LIGHTING_GOURARD);
	glutAddMenuEntry("Lighting Phong", LIGHTING_PHONG);
	glutAddMenuEntry("Texture", TEXTURE);
	glutAddMenuEntry("Texture lighting Phong", TEXTURE_LIGHTING_PHONG);
	submenu4_id = glutCreateMenu(menuCB);
	glutAddMenuEntry("Yes", DISPLAY_NORMAL_YES);
	glutAddMenuEntry("No", DISPLAY_NORMAL_NO);
	glutCreateMenu(menuCB);
	glutAddSubMenu("Open Model File", submenu1_id);
	glutAddSubMenu("Projection Type", submenu2_id);
	glutAddSubMenu("Display type", submenu3_id);
	glutAddSubMenu("Display Normals", submenu4_id);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	LoadModelFile();

	//starting main loop
	glutMainLoop();
}

void drawingCB(void)
{
	GLenum er;

	char DisplayString1[200], DisplayString2[200];

	//clearing the background
	glClearColor(0, 0, 0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//initializing modelview transformation matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	GraphicsPipeline();

	glColor3f(0, 1, 0);
	sprintf(DisplayString1, "Scale:%.1f , Translate: (%.1f,%.1f,%.1f), Camera angles:(%d,%d) position:(%.1f,%.1f,%.1f) ", GlobalGuiParamsForYou.ModelScale, GlobalGuiParamsForYou.ModelTranslateVector[0], GlobalGuiParamsForYou.ModelTranslateVector[1], GlobalGuiParamsForYou.ModelTranslateVector[2], GlobalGuiCalculations.CameraAnleHorizontal, GlobalGuiCalculations.CameraAnleVertical, GlobalGuiParamsForYou.CameraPos[0], GlobalGuiParamsForYou.CameraPos[1], GlobalGuiParamsForYou.CameraPos[2]);
	drawstr("helvetica", 12, 15, 25, DisplayString1);
	sprintf(DisplayString2, "Lighting reflection - Diffuse:%1.2f, Specular:%1.2f, Ambient:%1.2f, sHininess:%1.2f", GlobalGuiParamsForYou.Lighting_Diffuse, GlobalGuiParamsForYou.Lighting_Specular, GlobalGuiParamsForYou.Lighting_Ambient, GlobalGuiParamsForYou.Lighting_sHininess);
	drawstr("helvetica", 12, 15, 10, DisplayString2);

	//swapping buffers and displaying
	glutSwapBuffers();

	//check for errors
	er = glGetError();  //get errors. 0 for no error, find the error codes in: https://www.opengl.org/wiki/OpenGL_Error
	if (er) printf("error: %d\n", er);
}


void LoadModelFile()
{
	int width, height;
	GLubyte* ImageData;

	if (model_ptr) {
		glmDelete(model_ptr);
		model_ptr = 0;
	}

	switch (GlobalGuiCalculations.FileNum) {
	case TEAPOT:
		model_ptr = glmReadOBJ("teapot.obj");
		break;
	case TEDDY:
		model_ptr = glmReadOBJ("teddy.obj");
		break;
	case PUMPKIN:
		model_ptr = glmReadOBJ("pumpkin.obj");
		break;
	case COW:
		model_ptr = glmReadOBJ("cow.obj");
		break;
	case SIMPLE_PYRAMID:
		model_ptr = glmReadOBJ("SimplePyramid.obj");
		break;
	case FIRST_EXAMPLE:
		model_ptr = glmReadOBJ("FirstExample.obj");
		break;
	case SIMPLE_3D_EXAMPLE:
		model_ptr = glmReadOBJ("Simple3Dexample.obj");
		break;
	case SPHERE:
		model_ptr = glmReadOBJ("sphere.obj");
		break;
	case TRIANGLE:
		model_ptr = glmReadOBJ("triangle.obj");
		break;
	case Z_BUFFER_EXAMPLE:
		model_ptr = glmReadOBJ("ZbufferExample.obj");
		break;
	case TEXTURE_TRIANGLE:
		model_ptr = glmReadOBJ("TriangleTexture.obj");
		ImageData = readBMP("TriangleTexture.bmp", &width, &height);
		if (width != TEXTURE_SIZE || height != TEXTURE_SIZE)
			TerminationErrorFunc("Invalid texture size");
		memcpy(TextureImage, ImageData, TEXTURE_SIZE * TEXTURE_SIZE * 3);
		free(ImageData);
		break;
	case TEXTURE_BOX:
		model_ptr = glmReadOBJ("box.obj");
		ImageData = readBMP("box.bmp", &width, &height);
		if (width != TEXTURE_SIZE || height != TEXTURE_SIZE)
			TerminationErrorFunc("Invalid texture size");
		memcpy(TextureImage, ImageData, TEXTURE_SIZE * TEXTURE_SIZE * 3);
		free(ImageData);
		break;
	case TEXTURE_BARREL:
		model_ptr = glmReadOBJ("barrel.obj");
		ImageData = readBMP("barrel.bmp", &width, &height);
		if (width != TEXTURE_SIZE || height != TEXTURE_SIZE)
			TerminationErrorFunc("Invalid texture size");
		memcpy(TextureImage, ImageData, TEXTURE_SIZE * TEXTURE_SIZE * 3);
		free(ImageData);
		break;
	case TEXTURE_MOON:
		model_ptr = glmReadOBJ("moon.obj");
		ImageData = readBMP("moon.bmp", &width, &height);
		if (width != TEXTURE_SIZE || height != TEXTURE_SIZE)
			TerminationErrorFunc("Invalid texture size");
		memcpy(TextureImage, ImageData, TEXTURE_SIZE * TEXTURE_SIZE * 3);
		free(ImageData);
		break;
	case TEXTURE_SHEEP:
		model_ptr = glmReadOBJ("sheep.obj");
		ImageData = readBMP("sheep.bmp", &width, &height);
		if (width != TEXTURE_SIZE || height != TEXTURE_SIZE)
			TerminationErrorFunc("Invalid texture size");
		memcpy(TextureImage, ImageData, TEXTURE_SIZE * TEXTURE_SIZE * 3);
		free(ImageData);
		break;
	default:
		TerminationErrorFunc("File number not valid");
		break;
	}

	if (!model_ptr)
		TerminationErrorFunc("can not load 3D model");
	//glmUnitize(model_ptr);  //"unitize" a model by translating it

	//to the origin and scaling it to fit in a unit cube around
	//the origin
	glmFacetNormals(model_ptr);  //adding facet normals
	glmVertexNormals(model_ptr, 90.0);  //adding vertex normals

	glmBoundingBox(model_ptr, GlobalGuiParamsForYou.ModelMinVec, GlobalGuiParamsForYou.ModelMaxVec);
}

void ClearColorBuffer()
{
	GLuint x, y;
	for (y = 0; y < WIN_SIZE; y++) {
		for (x = 0; x < WIN_SIZE; x++) {
			ColorBuffer[y][x][0] = 0;
			ColorBuffer[y][x][1] = 0;
			ColorBuffer[y][x][2] = 0;
		}
	}
}

void setPixel(GLint x, GLint y, GLfloat r, GLfloat g, GLfloat b)
{
	if (x >= 0 && x < WIN_SIZE && y >= 0 && y < WIN_SIZE) {
		ColorBuffer[y][x][0] = round(r * 255);
		ColorBuffer[y][x][1] = round(g * 255);
		ColorBuffer[y][x][2] = round(b * 255);
	}
}

void DisplayColorBuffer()
{
	GLuint x, y;
	glBegin(GL_POINTS);
	for (y = 0; y < WIN_SIZE; y++) {
		for (x = 0; x < WIN_SIZE; x++) {
			glColor3ub(min(255, ColorBuffer[y][x][0]), min(255, ColorBuffer[y][x][1]), min(255, ColorBuffer[y][x][2]));
			glVertex2f(x + 0.5, y + 0.5);   // The 0.5 is to target pixel
		}
	}
	glEnd();
}


void InitGuiGlobalParams()
{
	GlobalGuiCalculations.FileNum = TEAPOT;
	GlobalGuiCalculations.CameraRaduis = CAMERA_DISTANCE_FROM_AXIS_CENTER;
	GlobalGuiCalculations.CameraAnleHorizontal = 0;
	GlobalGuiCalculations.CameraAnleVertical = 0;

	GlobalGuiParamsForYou.CameraPos[0] = 0;
	GlobalGuiParamsForYou.CameraPos[1] = 0;
	GlobalGuiParamsForYou.CameraPos[2] = GlobalGuiCalculations.CameraRaduis;

	GlobalGuiParamsForYou.ModelScale = 1;

	GlobalGuiParamsForYou.ModelTranslateVector[0] = 0;
	GlobalGuiParamsForYou.ModelTranslateVector[1] = 0;
	GlobalGuiParamsForYou.ModelTranslateVector[2] = 0;
	GlobalGuiParamsForYou.DisplayType = FACE_VERTEXES;
	GlobalGuiParamsForYou.ProjectionType = ORTHOGRAPHIC;
	GlobalGuiParamsForYou.DisplayNormals = DISPLAY_NORMAL_NO;
	GlobalGuiParamsForYou.Lighting_Diffuse = 0.75;
	GlobalGuiParamsForYou.Lighting_Specular = 0.2;
	GlobalGuiParamsForYou.Lighting_Ambient = 0.2;
	GlobalGuiParamsForYou.Lighting_sHininess = 40;
	GlobalGuiParamsForYou.LightPosition[0] = 10;
	GlobalGuiParamsForYou.LightPosition[1] = 5;
	GlobalGuiParamsForYou.LightPosition[2] = 0;

}


void reshapeCB(int width, int height)
{
	if (width != WIN_SIZE || height != WIN_SIZE)
	{
		glutReshapeWindow(WIN_SIZE, WIN_SIZE);
	}

	//update viewport
	glViewport(0, 0, width, height);

	//clear the transformation matrices (load identity)
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//projection
	gluOrtho2D(0, WIN_SIZE, 0, WIN_SIZE);
}


void keyboardCB(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
		exit(0);
		break;
	case '+':
		GlobalGuiParamsForYou.ModelScale += 0.1;
		glutPostRedisplay();
		break;
	case '-':
		GlobalGuiParamsForYou.ModelScale -= 0.1;
		glutPostRedisplay();
		break;
	case 'd':
	case 'D':
		GlobalGuiParamsForYou.Lighting_Diffuse += 0.05;
		glutPostRedisplay();
		break;
	case 'c':
	case 'C':
		GlobalGuiParamsForYou.Lighting_Diffuse -= 0.05;
		glutPostRedisplay();
		break;
	case 's':
	case 'S':
		GlobalGuiParamsForYou.Lighting_Specular += 0.05;
		glutPostRedisplay();
		break;
	case 'x':
	case 'X':
		GlobalGuiParamsForYou.Lighting_Specular -= 0.05;
		glutPostRedisplay();
		break;
	case 'a':
	case 'A':
		GlobalGuiParamsForYou.Lighting_Ambient += 0.05;
		glutPostRedisplay();
		break;
	case 'z':
	case 'Z':
		GlobalGuiParamsForYou.Lighting_Ambient -= 0.05;
		glutPostRedisplay();
		break;
	case 'h':
	case 'H':
		GlobalGuiParamsForYou.Lighting_sHininess += 1;
		glutPostRedisplay();
		break;
	case 'n':
	case 'N':
		GlobalGuiParamsForYou.Lighting_sHininess -= 1;
		glutPostRedisplay();
		break;
	default:
		printf("Key not valid (language shold be english)\n");
	}
}


void keyboardSpecialCB(int key, int x, int y)
{
	switch (key) {
	case GLUT_KEY_LEFT:
		GlobalGuiParamsForYou.ModelTranslateVector[0] -= 0.1;
		glutPostRedisplay();
		break;
	case GLUT_KEY_RIGHT:
		GlobalGuiParamsForYou.ModelTranslateVector[0] += 0.1;
		glutPostRedisplay();
		break;
	case GLUT_KEY_DOWN:
		GlobalGuiParamsForYou.ModelTranslateVector[2] -= 0.1;
		glutPostRedisplay();
		break;
	case GLUT_KEY_UP:
		GlobalGuiParamsForYou.ModelTranslateVector[2] += 0.1;
		glutPostRedisplay();
		break;
	}
}


void MouseClickCB(int button, int state, int x, int y)
{
	GlobalGuiCalculations.MouseLastPos[0] = x;
	GlobalGuiCalculations.MouseLastPos[1] = y;
}

void MouseMotionCB(int x, int y)
{
	GlobalGuiCalculations.CameraAnleHorizontal += (x - GlobalGuiCalculations.MouseLastPos[0]) / 40;
	GlobalGuiCalculations.CameraAnleVertical -= (y - GlobalGuiCalculations.MouseLastPos[1]) / 40;

	if (GlobalGuiCalculations.CameraAnleVertical > 30)
		GlobalGuiCalculations.CameraAnleVertical = 30;
	if (GlobalGuiCalculations.CameraAnleVertical < -30)
		GlobalGuiCalculations.CameraAnleVertical = -30;

	GlobalGuiCalculations.CameraAnleHorizontal = (GlobalGuiCalculations.CameraAnleHorizontal + 360) % 360;
	//	GlobalGuiCalculations.CameraAnleVertical   = (GlobalGuiCalculations.CameraAnleVertical   + 360) % 360;

	GlobalGuiParamsForYou.CameraPos[0] = GlobalGuiCalculations.CameraRaduis * sin((float)(GlobalGuiCalculations.CameraAnleVertical + 90) * M_PI / 180) * cos((float)(GlobalGuiCalculations.CameraAnleHorizontal + 90) * M_PI / 180);
	GlobalGuiParamsForYou.CameraPos[2] = GlobalGuiCalculations.CameraRaduis * sin((float)(GlobalGuiCalculations.CameraAnleVertical + 90) * M_PI / 180) * sin((float)(GlobalGuiCalculations.CameraAnleHorizontal + 90) * M_PI / 180);
	GlobalGuiParamsForYou.CameraPos[1] = GlobalGuiCalculations.CameraRaduis * cos((float)(GlobalGuiCalculations.CameraAnleVertical + 90) * M_PI / 180);
	glutPostRedisplay();
}

void menuCB(int value)
{
	switch (value) {
	case ORTHOGRAPHIC:
	case PERSPECTIVE:
		GlobalGuiParamsForYou.ProjectionType = value;
		glutPostRedisplay();
		break;
	case FACE_VERTEXES:
	case FACE_COLOR:
	case LIGHTING_FLAT:
	case LIGHTING_GOURARD:
	case LIGHTING_PHONG:
		GlobalGuiParamsForYou.DisplayType = value;
		glutPostRedisplay();
		break;
	case TEXTURE:
		GlobalGuiParamsForYou.DisplayType = value;
		glutPostRedisplay();
		break;
	case TEXTURE_LIGHTING_PHONG:
		GlobalGuiParamsForYou.DisplayType = value;
		glutPostRedisplay();
		break;
	case DISPLAY_NORMAL_YES:
	case DISPLAY_NORMAL_NO:
		GlobalGuiParamsForYou.DisplayNormals = value;
		glutPostRedisplay();
		break;
	default:
		GlobalGuiCalculations.FileNum = value;
		LoadModelFile();
		glutPostRedisplay();
	}
}


void drawstr(char* FontName, int FontSize, GLuint x, GLuint y, char* format, ...)
{
	va_list args;
	char buffer[255], * s;

	GLvoid* font_style = GLUT_BITMAP_TIMES_ROMAN_10;

	font_style = GLUT_BITMAP_HELVETICA_10;
	if (strcmp(FontName, "helvetica") == 0) {
		if (FontSize == 12)
			font_style = GLUT_BITMAP_HELVETICA_12;
		else if (FontSize == 18)
			font_style = GLUT_BITMAP_HELVETICA_18;
	}
	else if (strcmp(FontName, "times roman") == 0) {
		font_style = GLUT_BITMAP_TIMES_ROMAN_10;
		if (FontSize == 24)
			font_style = GLUT_BITMAP_TIMES_ROMAN_24;
	}
	else if (strcmp(FontName, "8x13") == 0) {
		font_style = GLUT_BITMAP_8_BY_13;
	}
	else if (strcmp(FontName, "9x15") == 0) {
		font_style = GLUT_BITMAP_9_BY_15;
	}

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	glRasterPos2i(x, y);
	for (s = buffer; *s; s++)
		glutBitmapCharacter(font_style, *s);
}


void TerminationErrorFunc(char* ErrorString)
{
	char string[256];
	printf(ErrorString);
	fgets(string, 256, stdin);

	exit(0);
}


// Function to load bmp file
// buffer for the image is allocated in this function, you should free this buffer
GLubyte* readBMP(char* imagepath, int* width, int* height)
{
	unsigned char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int imageSize;   // = width*height*3
	unsigned char* data;
	unsigned char tmp;
	int i;

	// Open the file
	FILE* file = fopen(imagepath, "rb");
	if (!file) {
		TerminationErrorFunc("Image could not be opened\n");
	}

	if (fread(header, 1, 54, file) != 54) { // If not 54 bytes read : problem
		TerminationErrorFunc("Not a correct BMP file\n");
	}

	if (header[0] != 'B' || header[1] != 'M') {
		TerminationErrorFunc("Not a correct BMP file\n");
	}

	// Read ints from the byte array
	dataPos = *(int*)&(header[0x0A]);
	imageSize = *(int*)&(header[0x22]);
	*width = *(int*)&(header[0x12]);
	*height = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize == 0)
		imageSize = *width * *height * 3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos == 0)
		dataPos = 54; // The BMP header is done that way

	// Create a buffer
	data = malloc(imageSize * sizeof(GLubyte));

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);


	//swap the r and b values to get RGB (bitmap is BGR)
	for (i = 0; i < *width * *height; i++)
	{
		tmp = data[i * 3];
		data[i * 3] = data[i * 3 + 2];
		data[i * 3 + 2] = tmp;
	}


	//Everything is in memory now, the file can be closed
	fclose(file);

	return data;
}
