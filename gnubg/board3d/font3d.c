/*
* font3d.c
* by Jon Kinsey, 2005
*
* Draw 3d numbers using luxi font and freetype library
*
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of version 2 of the GNU General Public License as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* $Id$
*/

#include "inc3d.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#define FONT_VERA "fonts/Vera.ttf"
#define FONT_VERA_SERIF_BOLD "fonts/VeraSeBd.ttf"

struct _OGLFont
{
	unsigned int glyphs;
	int advance;
	int kern[10][10];
	float scale;
	float heightRatio;
	float height;
};

typedef struct _Point
{
	double data[3];
} Point;

typedef struct _Contour
{
	GArray *conPoints;
} Contour;

typedef struct _Vectoriser
{
	GArray *contours;
	unsigned int numPoints;
} Vectoriser;

typedef struct _Tesselation
{
	GArray *tessPoints;
	GLenum meshType;
} Tesselation;

typedef struct _Mesh
{
	GArray *tesselations;
} Mesh;

static int CreateOGLFont(FT_Library ftLib, OGLFont *pFont, const char *pPath, int pointSize, float size, float heightRatio);
static void PopulateVectoriser(Vectoriser* pVect, const FT_Outline* pOutline);
static void TidyMemory(const Vectoriser* pVect, const Mesh* pMesh);
static void PopulateContour(GArray *contour, const FT_Vector* points, const char* pointTags, int numberOfPoints);
static void PopulateMesh(const Vectoriser* pVect, Mesh* pMesh);
static int MakeGlyph(const FT_Outline* pOutline, unsigned int displayList);

#define FONT_PITCH 24
#define FONT_SIZE (base_unit / 20.0f)
#define FONT_HEIGHT_RATIO 1.f

#define CUBE_FONT_PITCH 34
#define CUBE_FONT_SIZE (base_unit / 24.0f)
#define CUBE_FONT_HEIGHT_RATIO 1.25f

int BuildFont3d(BoardData3d* bd3d)
{
	FT_Library ftLib;
        char *file;
	if (FT_Init_FreeType(&ftLib))
		return 0;

	free(bd3d->numberFont);
	bd3d->numberFont = (OGLFont*)malloc(sizeof(OGLFont));
	file = g_build_filename(PKGDATADIR, FONT_VERA, NULL);
	if (!CreateOGLFont(ftLib, bd3d->numberFont, file, FONT_PITCH, FONT_SIZE, FONT_HEIGHT_RATIO))
	{
		g_print("Failed to create font %s\n", file);
		return 0;
	}
	free(file);

	free(bd3d->cubeFont);
	bd3d->cubeFont = (OGLFont*)malloc(sizeof(OGLFont));
	file = g_build_filename(PKGDATADIR, FONT_VERA_SERIF_BOLD, NULL);
	if (!CreateOGLFont(ftLib, bd3d->cubeFont, file, CUBE_FONT_PITCH, CUBE_FONT_SIZE, CUBE_FONT_HEIGHT_RATIO))
	{
		g_print("Failed to create font %s\n", file);
		return 0;
	}
	free(file);

	return !FT_Done_FreeType(ftLib);
}

static int CreateOGLFont(FT_Library ftLib, OGLFont *pFont, const char *pPath, int pointSize, float scale, float heightRatio)
{
	unsigned int i, j;
	FT_Face face;

	memset(pFont, 0, sizeof(OGLFont));
	pFont->scale = scale;
	pFont->heightRatio = heightRatio;

	if (FT_New_Face(ftLib, pPath, 0, &face))
		return 0;

	if (FT_Set_Char_Size(face, 0, pointSize * 64 /* 26.6 fractional points */, 0, 0))
		return 0;

	{	/* Find height of "1" */
		unsigned int glyphIndex = FT_Get_Char_Index(face, '1');
		if (!glyphIndex)
			return 0;

		if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_HINTING) ||
			(face->glyph->format != ft_glyph_format_outline))
			return 0;

		pFont->height = (float)face->glyph->metrics.height * scale * heightRatio / 64;
		pFont->advance = face->glyph->advance.x;
	}

	/* Create digit glyphs */
	pFont->glyphs = glGenLists(10);

	for (i = 0; i < 10; i++)
	{
		unsigned int glyphIndex = FT_Get_Char_Index(face, '0' + i);
		if (!glyphIndex)
			return 0;

		if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_NO_HINTING))
			return 0;
		assert(face->glyph->format == ft_glyph_format_outline);

		if (!MakeGlyph(&face->glyph->outline, pFont->glyphs + i))
			return 0;

		/* Calculate kerning table */
		for (j = 0; j < 10; j++)
		{
			FT_Vector kernAdvance;
			unsigned int nextGlyphIndex = FT_Get_Char_Index(face, '0' + j);
			if (!nextGlyphIndex || FT_Get_Kerning(face, glyphIndex, nextGlyphIndex, ft_kerning_unfitted, &kernAdvance))
				return 0;
			pFont->kern[i][j] = kernAdvance.x;
		}
	}

	return !FT_Done_Face(face);
}

static int MakeGlyph(const FT_Outline* pOutline, unsigned int displayList)
{
	Vectoriser vect;
	Mesh mesh;
	unsigned int index, point;

	PopulateVectoriser(&vect, pOutline);

	if ((vect.contours->len < 1) || (vect.numPoints < 3))
		return 0;

	glNewList(displayList, GL_COMPILE);

	/* Solid font */
	PopulateMesh(&vect, &mesh);

	for (index = 0; index < mesh.tesselations->len; index++)
	{
		Tesselation* subMesh = &g_array_index(mesh.tesselations, Tesselation, index);

		glBegin(subMesh->meshType);
		for (point = 0; point < subMesh->tessPoints->len; point++)
		{
			Point* pPoint = &g_array_index(subMesh->tessPoints, Point, point);

			assert(pPoint->data[2] == 0);
			glVertex2f((float)pPoint->data[0] / 64.0f, (float)pPoint->data[1] / 64.0f);
		}
		glEnd();
	}

	/* Outline font - anti-alias edges */
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	for (index = 0; index < vect.contours->len; index++)
	{
		Contour* contour = &g_array_index(vect.contours, Contour, index);

		glBegin( GL_LINE_LOOP);
		for (point = 0; point < contour->conPoints->len; point++)
		{
			Point* pPoint = &g_array_index(contour->conPoints, Point, point);
			glVertex2f((float)pPoint->data[0] / 64.0f, (float)pPoint->data[1] / 64.0f);
		}
		glEnd();
	}
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);

	glEndList();

	TidyMemory(&vect, &mesh);

	return 1;
}

static int GetKern(const OGLFont *pFont, char cur, char next)
{
	if (next)
		return pFont->kern[cur - '0'][next - '0'];
	else
		return 0;
}

static float getTextLen3d(const OGLFont *pFont, const char* str)
{
	int len = 0;
	while (*str)
	{
		len += pFont->advance + GetKern(pFont, str[0], str[1]);
		str++;
	}
	return (len / 64.0f) * pFont->scale;
}

static void RenderString3d(const OGLFont *pFont, const char* str)
{
	glScalef(pFont->scale, pFont->scale * pFont->heightRatio, 1.f);

	while (*str)
	{
		int offset = *str - '0';
		/* Draw character */
		glCallList(pFont->glyphs + (unsigned int)offset);

		/* Move on to next place */
		glTranslatef((pFont->advance + GetKern(pFont, str[0], str[1])) / 64.0f, 0.f, 0.f);

		str++;
	}
}

static GList *combineList;

static void PopulateVectoriser(Vectoriser* pVect, const FT_Outline* pOutline)
{
	int startIndex = 0;
	int endIndex;
	int contourLength;
	int contourIndex;

	pVect->contours = g_array_new(FALSE, FALSE, sizeof(Contour));
	pVect->numPoints = 0;

	for (contourIndex = 0; contourIndex < pOutline->n_contours; contourIndex++)
	{
		Contour newContour;
		FT_Vector* pointList = &pOutline->points[startIndex];
		char* tagList = &pOutline->tags[startIndex];

		endIndex = pOutline->contours[contourIndex];
		contourLength = (endIndex - startIndex) + 1;

		newContour.conPoints = g_array_new(FALSE, FALSE, sizeof(Point));
		PopulateContour(newContour.conPoints, pointList, tagList, contourLength);
		pVect->numPoints += newContour.conPoints->len;
		g_array_append_val(pVect->contours, newContour);

		startIndex = endIndex + 1;
	}
}

static void AddPoint(GArray *points, double x, double y)
{
	if (points->len > 0)
	{	/* Ignore duplicate contour points */
		Point* point = &g_array_index(points, Point, points->len - 1);
		/* See if point is duplicated (often happens when tesselating)
		   so suppress lint error 777 (testing floats for equality) */
		if (/*lint --e(777)*/point->data[0] == x && point->data[1] == y)
			return;
	}

	{
		Point newPoint;
		newPoint.data[0] = x;
		newPoint.data[1] = y;
		newPoint.data[2] = 0;

		g_array_append_val(points, newPoint);
	}
}

#define BEZIER_STEPS 5
#define BEZIER_STEP_SIZE 0.2f
static double controlPoints[4][2]; /* 2D array storing values of de Casteljau algorithm */

static void evaluateQuadraticCurve(GArray *points)
{
	int i;
	for (i = 0; i <= BEZIER_STEPS; i++)
	{
		double bezierValues[2][2];

		float t = i * BEZIER_STEP_SIZE;

		bezierValues[0][0] = (1.0f - t) * controlPoints[0][0] + t * controlPoints[1][0];
		bezierValues[0][1] = (1.0f - t) * controlPoints[0][1] + t * controlPoints[1][1];

		bezierValues[1][0] = (1.0f - t) * controlPoints[1][0] + t * controlPoints[2][0];
		bezierValues[1][1] = (1.0f - t) * controlPoints[1][1] + t * controlPoints[2][1];

		bezierValues[0][0] = (1.0f - t) * bezierValues[0][0] + t * bezierValues[1][0];
		bezierValues[0][1] = (1.0f - t) * bezierValues[0][1] + t * bezierValues[1][1];

		AddPoint(points, bezierValues[0][0], bezierValues[0][1]);
	}
}

static void PopulateContour(GArray *contour, const FT_Vector* points, const char* pointTags, int numberOfPoints)
{
	int pointIndex;

	for (pointIndex = 0; pointIndex < numberOfPoints; pointIndex++)
	{
		char pointTag = pointTags[pointIndex];

		if (pointTag == FT_Curve_Tag_On || numberOfPoints < 2)
		{
			AddPoint(contour, (double)points[pointIndex].x, (double)points[pointIndex].y);
		}
		else
		{
			int previousPointIndex = (pointIndex == 0) ? numberOfPoints - 1 : pointIndex - 1;
			int nextPointIndex = (pointIndex == numberOfPoints - 1) ? 0 : pointIndex + 1;
			Point controlPoint, previousPoint, nextPoint;
			controlPoint.data[0] = points[pointIndex].x;
			controlPoint.data[1] = points[pointIndex].y;
			previousPoint.data[0] = points[previousPointIndex].x;
			previousPoint.data[1] = points[previousPointIndex].y;
			nextPoint.data[0] = points[nextPointIndex].x;
			nextPoint.data[1] = points[nextPointIndex].y;

			assert(pointTag == FT_Curve_Tag_Conic);	/* Only this main type supported */

			while (pointTags[nextPointIndex] == FT_Curve_Tag_Conic)
			{
				nextPoint.data[0] = (controlPoint.data[0] + nextPoint.data[0]) * 0.5f;
				nextPoint.data[1] = (controlPoint.data[1] + nextPoint.data[1]) * 0.5f;

				controlPoints[0][0] = previousPoint.data[0];	controlPoints[0][1] = previousPoint.data[1];
				controlPoints[1][0] = controlPoint.data[0];		controlPoints[1][1] = controlPoint.data[1];
				controlPoints[2][0] = nextPoint.data[0];		controlPoints[2][1] = nextPoint.data[1];
				
				evaluateQuadraticCurve(contour);

				pointIndex++;
				previousPoint = nextPoint;
				controlPoint.data[0] = points[pointIndex].x;
				controlPoint.data[1] = points[pointIndex].y;
				nextPointIndex = (pointIndex == numberOfPoints - 1) ? 0 : pointIndex + 1;
				nextPoint.data[0] = points[nextPointIndex].x;
				nextPoint.data[1] = points[nextPointIndex].y;
			}
			
			controlPoints[0][0] = previousPoint.data[0];	controlPoints[0][1] = previousPoint.data[1];
			controlPoints[1][0] = controlPoint.data[0];		controlPoints[1][1] = controlPoint.data[1];
			controlPoints[2][0] = nextPoint.data[0];		controlPoints[2][1] = nextPoint.data[1];
			
			evaluateQuadraticCurve(contour);
		}
	}
}

/* Unfortunately the glu library doesn't define this callback type precisely
 so it may well cause problems on different platforms / opengl implementations */
#if WIN32
/* Need to set the callback calling convention for windows */
#define TESS_CALLBACK APIENTRY
#else
#define TESS_CALLBACK
#endif

#ifdef __GNUC__
#define GLUFUN(X) (_GLUfuncptr)X
#else
#define GLUFUN(X) X
#endif

static Tesselation curTess;
static void TESS_CALLBACK tcbError(GLenum error)
{	/* This is very unlikely to happen... */
	g_print("Tesselation error! (%d)\n", error);
}

static void TESS_CALLBACK tcbVertex(void* data, Mesh* notused)
{
	double* vertex = (double*)data;
	Point newPoint;

	newPoint.data[0] = vertex[0];
	newPoint.data[1] = vertex[1];
	newPoint.data[2] = vertex[2];

	g_array_append_val(curTess.tessPoints, newPoint);
}

static void TESS_CALLBACK tcbCombine(const double coords[3], void* notused/*vertex_data*/[4], GLfloat notused2/*weight*/[4], void** outData)
{
	/* Just return vertex position (colours etc. not required) */
	Point *newEle = (Point*)malloc(sizeof(Point));
	assert(newEle);
	memcpy(newEle->data, coords, sizeof(double[3]));

	combineList = g_list_append(combineList, newEle);
	*outData = newEle;
}

static void TESS_CALLBACK tcbBegin(GLenum type, Mesh* notused)
{
	curTess.tessPoints = g_array_new(FALSE, FALSE, sizeof(Point));
	curTess.meshType = type;
}

static void TESS_CALLBACK tcbEnd(/*lint -e{818}*/Mesh* pMesh)	/* Declaring pMesh as constant isn't useful as pointer member is modified (error 818) */
{
	g_array_append_val(pMesh->tesselations, curTess);
}

static void PopulateMesh(const Vectoriser* pVect, Mesh* pMesh)
{
	unsigned int c, p;
	GLUtesselator* tobj = gluNewTess();

	combineList = NULL;

	pMesh->tesselations = g_array_new(FALSE, FALSE, sizeof(Tesselation));

	gluTessCallback( tobj, GLU_TESS_BEGIN_DATA, GLUFUN(tcbBegin));
	gluTessCallback( tobj, GLU_TESS_VERTEX_DATA, GLUFUN(tcbVertex));
	gluTessCallback( tobj, GLU_TESS_COMBINE, GLUFUN(tcbCombine));
	gluTessCallback( tobj, GLU_TESS_END_DATA, GLUFUN(tcbEnd));
	gluTessCallback( tobj, GLU_TESS_ERROR, GLUFUN(tcbError));

	gluTessProperty( tobj, GLU_TESS_WINDING_RULE, (double)GLU_TESS_WINDING_ODD);

	gluTessNormal( tobj, 0.0, 0.0, 1.0);

	gluTessBeginPolygon( tobj, pMesh);

	for(c = 0; c < pVect->contours->len; ++c)
	{
		Contour* contour = &g_array_index(pVect->contours, Contour, c);

		gluTessBeginContour( tobj);

		for(p = 0; p < contour->conPoints->len; p++)
		{
			Point* point = &g_array_index(contour->conPoints, Point, p);
			gluTessVertex(tobj, point->data, point->data);
		}

		gluTessEndContour( tobj);
	}

	gluTessEndPolygon( tobj);

	gluDeleteTess( tobj);
}

static void TidyMemory(const Vectoriser* pVect, const Mesh* pMesh)
{
	GList *pl;
	unsigned int c, i;
	
	for (c = 0; c < pVect->contours->len; c++)
	{
		Contour* contour = &g_array_index(pVect->contours, Contour, c);
		g_array_free(contour->conPoints, TRUE);
	}
	g_array_free(pVect->contours, TRUE);
	for (i = 0; i < pMesh->tesselations->len; i++)
	{
		Tesselation* subMesh = &g_array_index(pMesh->tesselations, Tesselation, i);
		g_array_free(subMesh->tessPoints, TRUE);
	}
	g_array_free(pMesh->tesselations, TRUE);

	for (pl = g_list_first(combineList); pl; pl = g_list_next(pl))
	{
		free(pl->data);
	}
	g_list_free(combineList);
}

extern void glPrintPointNumbers(const BoardData3d* bd3d, const char *text)
{
	/* Align horizontally */
	glTranslatef(-getTextLen3d(bd3d->numberFont, text) / 2.0f, 0.f, 0.f);
	RenderString3d(bd3d->numberFont, text);
}

extern void glPrintCube(const BoardData3d* bd3d, const char *text)
{
	/* Align horizontally and vertically */
	glTranslatef(-getTextLen3d(bd3d->cubeFont, text) / 2.0f, -bd3d->cubeFont->height / 2.0f, 0.f);
	RenderString3d(bd3d->cubeFont, text);
}

extern void glPrintNumbersRA(const BoardData3d* bd3d, const char *text)
{
	/* Right align */
	glTranslatef(-getTextLen3d(bd3d->numberFont, text), 0.f, 0.f);
	RenderString3d(bd3d->numberFont, text);
}

extern float GetFontHeight3d(const OGLFont *font)
{
	return font->height;
}
