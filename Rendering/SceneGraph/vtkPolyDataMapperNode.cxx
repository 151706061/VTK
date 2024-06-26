// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataMapperNode.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolygon.h"
#include "vtkProperty.h"

//============================================================================
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyDataMapperNode);

//------------------------------------------------------------------------------
vtkPolyDataMapperNode::vtkPolyDataMapperNode() = default;

//------------------------------------------------------------------------------
vtkPolyDataMapperNode::~vtkPolyDataMapperNode() = default;

//------------------------------------------------------------------------------
void vtkPolyDataMapperNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkPolyDataMapperNode::TransformPoints(
  vtkActor* act, vtkPolyData* poly, std::vector<double>& _vertices)
{
  vtkSmartPointer<vtkMatrix4x4> m = vtkSmartPointer<vtkMatrix4x4>::New();
  act->GetMatrix(m);
  bool ident = (act->GetIsIdentity() == 1);
  double inPos[4];
  inPos[3] = 1.0;
  double transPos[4];
  for (int i = 0; i < poly->GetNumberOfPoints(); i++)
  {
    double* pos = poly->GetPoints()->GetPoint(i);
    bool wasNan = false;
    int fixIndex = i - 1;
    do
    {
      wasNan = false;
      for (int j = 0; j < 3; j++)
      {
        if (vtkMath::IsNan(pos[j]))
        {
          wasNan = true;
        }
      }
      if (wasNan && fixIndex >= 0)
      {
        pos = poly->GetPoints()->GetPoint(fixIndex--);
      }
    } while (wasNan && fixIndex >= 0);
    if (ident)
    {
      _vertices.push_back(pos[0]);
      _vertices.push_back(pos[1]);
      _vertices.push_back(pos[2]);
    }
    else
    {
      inPos[0] = pos[0];
      inPos[1] = pos[1];
      inPos[2] = pos[2];
      m->MultiplyPoint(inPos, transPos);
      // alternatively use an OSPRay instance of something like that
      _vertices.push_back(transPos[0]);
      _vertices.push_back(transPos[1]);
      _vertices.push_back(transPos[2]);
    }
  }
}

namespace
{
// Helpers for MakeConnectivity()
// The CreateXIndexBuffer's were adapted from vtkOpenGLIndexBufferObject.
// Apply rule of three if made again somewhere in VTK.

//------------------------------------------------------------------------------
// Description:
// Homogenizes everything into a flat list of point indexes.
// At same time creates a reverse cell index array for obtaining cell quantities for points
void CreatePointIndexBuffer(vtkCellArray* cells, std::vector<unsigned int>& indexArray,
  std::vector<unsigned int>& reverseArray)
{
  // TODO: restore the preallocate and append to offset features I omitted
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);
  if (!cells->GetNumberOfCells())
  {
    return;
  }
  unsigned int cell_id = 0;
  for (cells->InitTraversal(); cells->GetNextCell(npts, indices);)
  {
    for (int i = 0; i < npts; ++i)
    {
      indexArray.push_back(static_cast<unsigned int>(*(indices++)));
      reverseArray.push_back(cell_id);
    }
    cell_id++;
  }
}

//------------------------------------------------------------------------------
// Description:
// Homogenizes lines into a flat list of line segments, each containing two point indexes
// At same time creates a reverse cell index array for obtaining cell quantities for points
void CreateLineIndexBuffer(vtkCellArray* cells, std::vector<unsigned int>& indexArray,
  std::vector<unsigned int>& reverseArray)
{
  // TODO: restore the preallocate and append to offset features I omitted
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);
  if (!cells->GetNumberOfCells())
  {
    return;
  }
  unsigned int cell_id = 0;
  for (cells->InitTraversal(); cells->GetNextCell(npts, indices);)
  {
    for (int i = 0; i < npts - 1; ++i)
    {
      indexArray.push_back(static_cast<unsigned int>(indices[i]));
      indexArray.push_back(static_cast<unsigned int>(indices[i + 1]));
      reverseArray.push_back(cell_id);
      reverseArray.push_back(cell_id);
    }
    cell_id++;
  }
}

//------------------------------------------------------------------------------
// Description:
// Homogenizes polygons into a flat list of line segments, each containing two point indexes.
// At same time creates a reverse cell index array for obtaining cell quantities for points
// This differs from CreateLineIndexBuffer in that it closes loops, making a segment from last point
// back to first.
void CreateTriangleLineIndexBuffer(vtkCellArray* cells, std::vector<unsigned int>& indexArray,
  std::vector<unsigned int>& reverseArray)
{
  // TODO: restore the preallocate and append to offset features I omitted
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);
  if (!cells->GetNumberOfCells())
  {
    return;
  }
  unsigned int cell_id = 0;
  for (cells->InitTraversal(); cells->GetNextCell(npts, indices);)
  {
    for (int i = 0; i < npts; ++i)
    {
      indexArray.push_back(static_cast<unsigned int>(indices[i]));
      indexArray.push_back(static_cast<unsigned int>(indices[i < npts - 1 ? i + 1 : 0]));
      reverseArray.push_back(cell_id);
      reverseArray.push_back(cell_id);
    }
    cell_id++;
  }
}

//------------------------------------------------------------------------------
// Description:
// Homogenizes polygons into a flat list of triangles, each containing three point indexes
// At same time creates a reverse cell index array for obtaining cell quantities for points
void CreateTriangleIndexBuffer(vtkCellArray* cells, vtkPoints* points,
  std::vector<unsigned int>& indexArray, std::vector<unsigned int>& reverseArray)
{
  // TODO: restore the preallocate and append to offset features I omitted
  const vtkIdType* indices(nullptr);
  vtkIdType npts(0);
  if (!cells->GetNumberOfCells())
  {
    return;
  }
  unsigned int cell_id = 0;
  // the following are only used if we have to triangulate a polygon
  // otherwise they just sit at nullptr
  vtkPolygon* polygon = nullptr;
  vtkIdList* tris = nullptr;
  vtkPoints* triPoints = nullptr;

  for (cells->InitTraversal(); cells->GetNextCell(npts, indices);)
  {
    // ignore degenerate triangles
    if (npts < 3)
    {
      cell_id++;
      continue;
    }

    // triangulate needed
    if (npts > 3)
    {
      // special case for quads, penta, hex which are common
      if (npts == 4)
      {
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[1]));
        indexArray.push_back(static_cast<unsigned int>(indices[2]));
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[2]));
        indexArray.push_back(static_cast<unsigned int>(indices[3]));
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
      }
      else if (npts == 5)
      {
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[1]));
        indexArray.push_back(static_cast<unsigned int>(indices[2]));
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[2]));
        indexArray.push_back(static_cast<unsigned int>(indices[3]));
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[3]));
        indexArray.push_back(static_cast<unsigned int>(indices[4]));
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
      }
      else if (npts == 6)
      {
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[1]));
        indexArray.push_back(static_cast<unsigned int>(indices[2]));
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[2]));
        indexArray.push_back(static_cast<unsigned int>(indices[3]));
        indexArray.push_back(static_cast<unsigned int>(indices[0]));
        indexArray.push_back(static_cast<unsigned int>(indices[3]));
        indexArray.push_back(static_cast<unsigned int>(indices[5]));
        indexArray.push_back(static_cast<unsigned int>(indices[3]));
        indexArray.push_back(static_cast<unsigned int>(indices[4]));
        indexArray.push_back(static_cast<unsigned int>(indices[5]));
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
      }
      else // 7 sided polygon or higher, do a full smart triangulation
      {
        if (!polygon)
        {
          polygon = vtkPolygon::New();
          tris = vtkIdList::New();
          triPoints = vtkPoints::New();
        }

        vtkIdType* triIndices = new vtkIdType[npts];
        triPoints->SetNumberOfPoints(npts);
        for (int i = 0; i < npts; ++i)
        {
          int idx = indices[i];
          triPoints->SetPoint(i, points->GetPoint(idx));
          triIndices[i] = i;
        }
        polygon->Initialize(npts, triIndices, triPoints);
        polygon->TriangulateLocalIds(0, tris);
        for (int j = 0; j < tris->GetNumberOfIds(); ++j)
        {
          indexArray.push_back(static_cast<unsigned int>(indices[tris->GetId(j)]));
          reverseArray.push_back(cell_id);
        }
        delete[] triIndices;
      }
    }
    else
    {
      indexArray.push_back(static_cast<unsigned int>(*(indices++)));
      indexArray.push_back(static_cast<unsigned int>(*(indices++)));
      indexArray.push_back(static_cast<unsigned int>(*(indices++)));
      reverseArray.push_back(cell_id);
      reverseArray.push_back(cell_id);
      reverseArray.push_back(cell_id);
    }

    cell_id++;
  }
  if (polygon)
  {
    polygon->Delete();
    tris->Delete();
    triPoints->Delete();
  }
}

//------------------------------------------------------------------------------
// Description:
// Homogenizes triangle strips.
// Depending on wireframeTriStrips it will produce either line segments (two indices per edge)
// or triangles (three indices per face).
// At same time creates a reverse cell index array for obtaining cell quantities for points
void CreateStripIndexBuffer(vtkCellArray* cells, std::vector<unsigned int>& indexArray,
  std::vector<unsigned int>& reverseArray, bool wireframeTriStrips)
{
  if (!cells->GetNumberOfCells())
  {
    return;
  }
  unsigned int cell_id = 0;

  const vtkIdType* pts = nullptr;
  vtkIdType npts = 0;

  size_t triCount = cells->GetNumberOfConnectivityIds() - 2 * cells->GetNumberOfCells();
  size_t targetSize = wireframeTriStrips ? 2 * (triCount * 2 + 1) : triCount * 3;
  indexArray.reserve(targetSize);

  if (wireframeTriStrips)
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      indexArray.push_back(static_cast<unsigned int>(pts[0]));
      indexArray.push_back(static_cast<unsigned int>(pts[1]));
      reverseArray.push_back(cell_id);
      reverseArray.push_back(cell_id);
      for (int j = 0; j < npts - 2; ++j)
      {
        indexArray.push_back(static_cast<unsigned int>(pts[j]));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 2]));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 1]));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 2]));
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
      }
      cell_id++;
    }
  }
  else
  {
    for (cells->InitTraversal(); cells->GetNextCell(npts, pts);)
    {
      for (int j = 0; j < npts - 2; ++j)
      {
        indexArray.push_back(static_cast<unsigned int>(pts[j]));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 1 + j % 2]));
        indexArray.push_back(static_cast<unsigned int>(pts[j + 1 + (j + 1) % 2]));
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
        reverseArray.push_back(cell_id);
      }
      cell_id++;
    }
  }
}
}

//------------------------------------------------------------------------------
void vtkPolyDataMapperNode::MakeConnectivity(
  vtkPolyData* poly, int representation, vtkPDConnectivity& conn)
{
  vtkCellArray* prims[4];
  prims[0] = poly->GetVerts();
  prims[1] = poly->GetLines();
  prims[2] = poly->GetPolys();
  prims[3] = poly->GetStrips();

  CreatePointIndexBuffer(prims[0], conn.vertex_index, conn.vertex_reverse);
  switch (representation)
  {
    case VTK_POINTS:
    {
      CreatePointIndexBuffer(prims[1], conn.line_index, conn.line_reverse);
      CreatePointIndexBuffer(prims[2], conn.triangle_index, conn.triangle_reverse);
      CreatePointIndexBuffer(prims[3], conn.strip_index, conn.strip_reverse);
      break;
    }
    case VTK_WIREFRAME:
    {
      CreateLineIndexBuffer(prims[1], conn.line_index, conn.line_reverse);
      CreateTriangleLineIndexBuffer(prims[2], conn.triangle_index, conn.triangle_reverse);
      CreateStripIndexBuffer(prims[3], conn.strip_index, conn.strip_reverse, true);
      break;
    }
    default:
    {
      CreateLineIndexBuffer(prims[1], conn.line_index, conn.line_reverse);
      CreateTriangleIndexBuffer(
        prims[2], poly->GetPoints(), conn.triangle_index, conn.triangle_reverse);
      CreateStripIndexBuffer(prims[3], conn.strip_index, conn.strip_reverse, false);
    }
  }
}
VTK_ABI_NAMESPACE_END
