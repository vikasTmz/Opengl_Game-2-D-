/*
 * FTGL - OpenGL font library
 *
 * Copyright (c) 2001-2004 Henry Maddocks <ftgl@opengl.geek.nz>
 * Copyright (c) 2008 Sam Hocevar <sam@hocevar.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <iostream>

#include "FTGL/ftgl.h"

#include "FTInternals.h"
#include "FTExtrudeGlyphImpl.h"
#include "FTVectoriser.h"

#include <assert.h>


//
//  FTGLExtrudeGlyph
//


FTExtrudeGlyph::FTExtrudeGlyph(FT_GlyphSlot glyph, float depth,
                               float frontOutset, float backOutset,
                               GLint vertexCoordAttribute,
                               GLint vertexNormalAttribute,
                               GLint vertexOffsetUniform) :
    FTGlyph(new FTExtrudeGlyphImpl(glyph, depth, frontOutset, backOutset,
                                   vertexCoordAttribute,
                                   vertexNormalAttribute,
                                   vertexOffsetUniform))
{}


FTExtrudeGlyph::~FTExtrudeGlyph()
{}


const FTPoint& FTExtrudeGlyph::Render(const FTPoint& pen, int renderMode)
{
    FTExtrudeGlyphImpl *myimpl = dynamic_cast<FTExtrudeGlyphImpl *>(impl);
    return myimpl->RenderImpl(pen, renderMode);
}


//
//  FTGLExtrudeGlyphImpl
//


FTExtrudeGlyphImpl::FTExtrudeGlyphImpl(FT_GlyphSlot glyph, float _depth,
                                       float _frontOutset, float _backOutset,
                                       GLint _vertexCoordAttribute,
                                       GLint _vertexNormalAttribute,
                                       GLint _vertexOffsetUniform)
:   FTGlyphImpl(glyph),
    vectoriser(0)
{
    bBox.SetDepth(-_depth);

    if(ft_glyph_format_outline != glyph->format)
    {
        err = 0x14; // Invalid_Outline
        return;
    }

    vectoriser = new FTVectoriser(glyph);

    if((vectoriser->ContourCount() < 1) || (vectoriser->PointCount() < 3))
    {
        delete vectoriser;
        vectoriser = NULL;
        return;
    }

    hscale = glyph->face->size->metrics.x_ppem * 64;
    vscale = glyph->face->size->metrics.y_ppem * 64;
    depth = _depth;
    frontOutset = _frontOutset;
    backOutset = _backOutset;
    vertexCoordAttribute = _vertexCoordAttribute;
    vertexNormalAttribute = _vertexNormalAttribute;
    vertexOffsetUniform = _vertexOffsetUniform;

    // glewInit();
    gladLoadGL();

    // Setup buffers for GL
    glGenBuffers(3, coordVBOs);
    glGenBuffers(3, normalVBOs);

    glGenVertexArrays(3, meshVAOs);
    // glGenBuffers(3, meshIBOs);

    /* Front face */
    RenderFront();

    /* Back face */
    RenderBack();

    /* Side face */
    RenderSide();

    delete vectoriser;
    vectoriser = NULL;
}


FTExtrudeGlyphImpl::~FTExtrudeGlyphImpl()
{
    glDeleteVertexArrays(3, meshVAOs);
    glDeleteBuffers(3, coordVBOs);
    glDeleteBuffers(3, normalVBOs);
    // glDeleteBuffers(3, meshIBOs);
}


const FTPoint& FTExtrudeGlyphImpl::RenderImpl(const FTPoint& pen,
                                              int renderMode)
{
    // Offset this rendering by the given pen
    glUniform3f(vertexOffsetUniform, pen.Xf(), pen.Yf(), pen.Zf());

    if(renderMode & FTGL::RENDER_FRONT) {
        DrawVAO(0);
    }
    if(renderMode & FTGL::RENDER_BACK) {
        DrawVAO(1);
    }
    if(renderMode & FTGL::RENDER_SIDE) {
        DrawVAO(2);
    }

    return advance;
}

void FTExtrudeGlyphImpl::DrawVAO(const int meshIndex)
{
    glBindVertexArray(meshVAOs[meshIndex]);

    // If we decide to use indexed drawing rather than stuffing the array with duplicate points, this will work.
    // glDrawElements(GL_TRIANGLE_STRIP, iboSizes[0], GL_UNSIGNED_SHORT, 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, vboSizes[meshIndex]);

    glBindVertexArray(0);
}

void FTExtrudeGlyphImpl::AddVertex(
    FTVector<FTGL_FLOAT> *points, const FTPoint& point,
    FTVector<FTGL_FLOAT> *normals, const FTPoint& normal,
    const float depthOffset)
{
    points->push_back(point.Xf() / 64.0);
    points->push_back(point.Yf() / 64.0);
    points->push_back(depthOffset);

    // printf("Adding normal %f %f %f\n", normal.Xf(), normal.Yf(), normal.Zf());
    normals->push_back(normal.Xf());
    normals->push_back(normal.Yf());
    normals->push_back(normal.Zf());
}

void FTExtrudeGlyphImpl::RenderFaceToMeshIndexAtDepth(
    const int meshIndex, const FTPoint &normal, const float depthOffset)
{
    // Create vectors to hold the points and indices
    FTVector<FTGL_FLOAT> points;
    FTVector<FTGL_FLOAT> normals;
    // FTVector<GLushort> indices;

    const FTMesh *mesh = vectoriser->GetMesh();
    for(unsigned int j = 0; j < mesh->TesselationCount(); ++j)
    {
        const FTTesselation* subMesh = mesh->Tesselation(j);
        unsigned int polygonType = subMesh->PolygonType();

        // Implementation from TriangleExtractor
        switch(polygonType)
        {
            case GL_TRIANGLE_STRIP:
                AddVertex(&points, subMesh->Point(0), &normals, normal, depthOffset);
                for(unsigned int i = 0; i < subMesh->PointCount(); ++i) {
                    AddVertex(&points, subMesh->Point(i), &normals, normal, depthOffset);
                }
                AddVertex(&points, subMesh->Point(subMesh->PointCount() - 1), &normals, normal, depthOffset);
                break;
            case GL_TRIANGLES:
                assert(subMesh->PointCount() % 3 == 0);
                for(unsigned int i = 0; i < subMesh->PointCount(); i += 3)
                {
                    AddVertex(&points, subMesh->Point(i), &normals, normal, depthOffset);
                    AddVertex(&points, subMesh->Point(i), &normals, normal, depthOffset);
                    AddVertex(&points, subMesh->Point(i+1), &normals, normal, depthOffset);
                    AddVertex(&points, subMesh->Point(i+2), &normals, normal, depthOffset);
                    AddVertex(&points, subMesh->Point(i+2), &normals, normal, depthOffset);
                }
                break;
            case GL_TRIANGLE_FAN:
            {
                const FTPoint& centerPoint = subMesh->Point(0);
                AddVertex(&points, centerPoint, &normals, normal, depthOffset);

                for(unsigned int i = 1; i < subMesh->PointCount()-1; ++i)
                {
                    AddVertex(&points, centerPoint, &normals, normal, depthOffset);
                    AddVertex(&points, subMesh->Point(i), &normals, normal, depthOffset);
                    AddVertex(&points, subMesh->Point(i+1), &normals, normal, depthOffset);
                    AddVertex(&points, centerPoint, &normals, normal, depthOffset);
                }
                AddVertex(&points, centerPoint, &normals, normal, depthOffset);
                break;
            }
            default:
                assert(!"please implement...");
        }
    }

    BufferPointsToMeshIndex(&points, &normals, meshIndex);
}

void FTExtrudeGlyphImpl::BufferPointsToMeshIndex(
    const FTVector<FTGL_FLOAT> *points, const FTVector<FTGL_FLOAT> *normals, const int meshIndex) {
    // Bind the VAO
    glBindVertexArray(meshVAOs[meshIndex]);

    // Buffer the coords
    vboSizes[meshIndex] = points->size();
    glBindBuffer(GL_ARRAY_BUFFER, coordVBOs[meshIndex]);
    glBufferData(GL_ARRAY_BUFFER,
        points->size() * sizeof(FTGL_FLOAT),
        static_cast<const FTGL_FLOAT*>(points->begin()),
        GL_STATIC_DRAW);

    // Describe the points
    glEnableVertexAttribArray(vertexCoordAttribute);
    glVertexAttribPointer(
        vertexCoordAttribute, // attribute
        3,                   // number of elements per vertex, here (x,y,z)
        GL_FLOAT,            // the type of each element
        GL_FALSE,            // take our values as-is
        0,                   // no extra data between each position
        0                    // offset of first element
    );


    // Buffer the normals
    glBindBuffer(GL_ARRAY_BUFFER, normalVBOs[meshIndex]);
    glBufferData(GL_ARRAY_BUFFER,
        normals->size() * sizeof(FTGL_FLOAT),
        static_cast<const FTGL_FLOAT*>(normals->begin()),
        GL_STATIC_DRAW);

    // Describe normals
    glEnableVertexAttribArray(vertexNormalAttribute);
    glVertexAttribPointer(
        vertexNormalAttribute, // attribute
        3,                  // number of elements per vertex, here (x,y,z)
        GL_FLOAT,           // the type of each element
        GL_FALSE,           // take our values as-is
        0,                  // no extra data between each position
        0                   // offset of first element
    );

    // For a possible index array buffer implementation...
    // iboSizes[meshIndex] = indices.size();
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos + meshIndex);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER,
    //     indices.size() * sizeof(GLushort),
    //     static_cast<const GLushort*>(indices.begin()),
    //     GL_STATIC_DRAW);
    // printf("Buffered thisa many points!!! %lu \n", points.size());

    // Unbind the VAO
    glBindVertexArray(0);
}

void FTExtrudeGlyphImpl::RenderFront()
{
    vectoriser->MakeMesh(1.0, 1, frontOutset);

    const FTPoint normal = FTPoint(0.0, 0.0, 1.0).Normalise();
    const int meshIndex = 0;
    RenderFaceToMeshIndexAtDepth(meshIndex, normal, 0);
}

void FTExtrudeGlyphImpl::RenderBack()
{
    vectoriser->MakeMesh(-1.0, 2, backOutset);

    const FTPoint normal = FTPoint(0.0, 0.0, -1.0).Normalise();
    const int meshIndex = 1;
    RenderFaceToMeshIndexAtDepth(meshIndex, normal, -depth);
}

void FTExtrudeGlyphImpl::RenderSide()
{
    FTVector<FTGL_FLOAT> points;
    FTVector<FTGL_FLOAT> normals;

    const int contourFlag = vectoriser->ContourFlag();

    for(size_t c = 0; c < vectoriser->ContourCount(); ++c)
    {
        const FTContour* contour = vectoriser->Contour(c);
        const size_t n = contour->PointCount();

        if(n < 2)
        {
            continue;
        }

        // glBegin(GL_QUAD_STRIP);
        for(size_t j = 0; j <= n; ++j)
        {
            const size_t cur = (j == n) ? 0 : j;
            const size_t next = (cur == n - 1) ? 0 : cur + 1;

            const FTPoint frontPt = contour->FrontPoint(cur);
            const FTPoint nextPt = contour->FrontPoint(next);
            const FTPoint backPt = contour->BackPoint(cur);

            FTPoint normal = (FTPoint(0.f, 0.f, 1.f) ^ (frontPt - nextPt)).Normalise();
            if(normal != FTPoint(0.0f, 0.0f, 0.0f))
            {
                // glNormal3dv(static_cast<const FTGL_DOUBLE*>(normal.Normalise()));
            }

            // glTexCoord2f(frontPt.Xf() / hscale, frontPt.Yf() / vscale);

            if(contourFlag & ft_outline_reverse_fill)
            {
                if (j == 0)
                {
                    AddVertex(&points, backPt, &normals, normal, 0.0f);
                }
                // glVertex3f(backPt.Xf() / 64.0f, backPt.Yf() / 64.0f, 0.0f);
                // glVertex3f(frontPt.Xf() / 64.0f, frontPt.Yf() / 64.0f, -depth);
                AddVertex(&points, backPt, &normals, normal, 0.0f);
                AddVertex(&points, frontPt, &normals, normal, -depth);

                if (j == n)
                {
                    AddVertex(&points, frontPt, &normals, normal, -depth);
                }
            }
            else
            {
                if (j == 0)
                {
                    AddVertex(&points, backPt, &normals, normal, -depth);
                }
                // glVertex3f(backPt.Xf() / 64.0f, backPt.Yf() / 64.0f, -depth);
                // glVertex3f(frontPt.Xf() / 64.0f, frontPt.Yf() / 64.0f, 0.0f);
                AddVertex(&points, backPt, &normals, normal, -depth);
                AddVertex(&points, frontPt, &normals, normal, 0.0f);

                if (j == n)
                {
                    AddVertex(&points, frontPt, &normals, normal, 0.0f);
                }
            }
        }
        // glEnd();
    }

    const int meshIndex = 2;
    BufferPointsToMeshIndex(&points, &normals, meshIndex);
}

/*
void FTExtrudeGlyphImpl::RenderMeshUsingIndicesTo() {
    // Dumbo implementation
    switch (polygonType) {
        case GL_TRIANGLES:
            printf("TRIANGLES\n");
            break;
        case GL_TRIANGLE_STRIP:
            printf("TRIANGLE_STRIP\n");
            break;
        case GL_TRIANGLE_FAN:
            printf("TRIANGLE_FAN\n");
            break;
        default:
            printf("UNKNOWN %u", polygonType);
    }

    for(unsigned int i = 0; i < subMesh->PointCount(); ++i)
    {
        FTPoint pt = subMesh->Point(i);

        // TODO add tex coords using glVertexAttribPointer
        // glTexCoord2f(pt.Xf() / hscale,
        //              pt.Yf() / vscale);

        // glVertex3f(pt.Xf() / 64.0f,
        //            pt.Yf() / 64.0f,
        //            0.0f);
        points.push_back(pt.Xf() / 64.0f);
        points.push_back(pt.Yf() / 64.0f);
        // printf("%u Pushing point %f %f\n", meshVAOs + 0, pt.Xf() / 64.0f, pt.Yf() / 64.0f);
        points.push_back(0.0f);
        indices.push_back(i);
    }
}
*/
