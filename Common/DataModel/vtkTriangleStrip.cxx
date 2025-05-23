// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTriangleStrip.h"

#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkLine.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkTriangle.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTriangleStrip);

//------------------------------------------------------------------------------
vtkTriangleStrip::vtkTriangleStrip()
{
  this->Line = vtkLine::New();
  this->Triangle = vtkTriangle::New();
}

//------------------------------------------------------------------------------
vtkTriangleStrip::~vtkTriangleStrip()
{
  this->Line->Delete();
  this->Triangle->Delete();
}

//------------------------------------------------------------------------------
int vtkTriangleStrip::EvaluatePosition(const double x[3], double closestPoint[3], int& subId,
  double pcoords[3], double& minDist2, double weights[])
{
  double pc[3], dist2;
  int ignoreId, i, returnStatus, status;
  double tempWeights[3], activeWeights[3];
  double closest[3];

  pcoords[2] = 0.0;

  activeWeights[0] = 0.0;
  activeWeights[1] = 0.0;
  activeWeights[2] = 0.0;

  returnStatus = 0;
  for (minDist2 = VTK_DOUBLE_MAX, i = 0; i < this->Points->GetNumberOfPoints() - 2; i++)
  {
    weights[i] = 0.0;
    this->Triangle->Points->SetPoint(0, this->Points->GetPoint(i));
    this->Triangle->Points->SetPoint(1, this->Points->GetPoint(i + 1));
    this->Triangle->Points->SetPoint(2, this->Points->GetPoint(i + 2));
    status = this->Triangle->EvaluatePosition(x, closest, ignoreId, pc, dist2, tempWeights);
    if (status != -1 && ((dist2 < minDist2) || ((dist2 == minDist2) && (returnStatus == 0))))
    {
      returnStatus = status;
      if (closestPoint)
      {
        closestPoint[0] = closest[0];
        closestPoint[1] = closest[1];
        closestPoint[2] = closest[2];
      }
      subId = i;
      pcoords[0] = pc[0];
      pcoords[1] = pc[1];
      minDist2 = dist2;
      activeWeights[0] = tempWeights[0];
      activeWeights[1] = tempWeights[1];
      activeWeights[2] = tempWeights[2];
    }
  }

  weights[i] = 0.0;
  weights[i + 1] = 0.0;

  weights[subId] = activeWeights[0];
  weights[subId + 1] = activeWeights[1];
  weights[subId + 2] = activeWeights[2];

  return returnStatus;
}

//------------------------------------------------------------------------------
void vtkTriangleStrip::EvaluateLocation(
  int& subId, const double pcoords[3], double x[3], double* weights)
{
  static const int idx[2][3] = { { 0, 1, 2 }, { 1, 0, 2 } };
  const int order = subId % 2;

  // Efficient point access
  const auto pointsArray = vtkDoubleArray::FastDownCast(this->Points->GetData());
  if (!pointsArray)
  {
    vtkErrorMacro(<< "Points should be double type");
    return;
  }
  const double* pts = pointsArray->GetPointer(0);

  const double* pt1 = pts + 3 * (subId + idx[order][0]);
  const double* pt2 = pts + 3 * (subId + idx[order][1]);
  const double* pt3 = pts + 3 * (subId + idx[order][2]);
  const double u3 = 1.0 - pcoords[0] - pcoords[1];

  std::fill_n(weights, this->Points->GetNumberOfPoints(), 0.0);
  weights[subId] = u3;
  weights[subId + 1] = pcoords[0];
  weights[subId + 2] = pcoords[1];

  for (int i = 0; i < 3; i++)
  {
    x[i] = pt1[i] * weights[subId] + pt2[i] * weights[subId + 1] + pt3[i] * weights[subId + 2];
  }
}

//------------------------------------------------------------------------------
int vtkTriangleStrip::CellBoundary(int subId, const double pcoords[3], vtkIdList* pts)
{
  static const int idx[2][3] = { { 0, 1, 2 }, { 1, 0, 2 } };
  int order;

  order = subId % 2;

  this->Triangle->PointIds->SetId(0, this->PointIds->GetId(subId + idx[order][0]));
  this->Triangle->PointIds->SetId(1, this->PointIds->GetId(subId + idx[order][1]));
  this->Triangle->PointIds->SetId(2, this->PointIds->GetId(subId + idx[order][2]));
  return this->Triangle->CellBoundary(0, pcoords, pts);
}

//------------------------------------------------------------------------------
void vtkTriangleStrip::Contour(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* verts, vtkCellArray* lines,
  vtkCellArray* polys, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId,
  vtkCellData* outCd)
{
  int i, numTris = this->Points->GetNumberOfPoints() - 2;
  vtkDataArray* triScalars = cellScalars->NewInstance();
  triScalars->SetNumberOfComponents(cellScalars->GetNumberOfComponents());
  triScalars->SetNumberOfTuples(3);

  for (i = 0; i < numTris; i++)
  {
    this->Triangle->Points->SetPoint(0, this->Points->GetPoint(i));
    this->Triangle->Points->SetPoint(1, this->Points->GetPoint(i + 1));
    this->Triangle->Points->SetPoint(2, this->Points->GetPoint(i + 2));

    if (outPd)
    {
      this->Triangle->PointIds->SetId(0, this->PointIds->GetId(i));
      this->Triangle->PointIds->SetId(1, this->PointIds->GetId(i + 1));
      this->Triangle->PointIds->SetId(2, this->PointIds->GetId(i + 2));
    }

    triScalars->SetTuple(0, cellScalars->GetTuple(i));
    triScalars->SetTuple(1, cellScalars->GetTuple(i + 1));
    triScalars->SetTuple(2, cellScalars->GetTuple(i + 2));

    this->Triangle->Contour(
      value, triScalars, locator, verts, lines, polys, inPd, outPd, inCd, cellId, outCd);
  }
  triScalars->Delete();
}

//------------------------------------------------------------------------------
vtkCell* vtkTriangleStrip::GetEdge(int edgeId)
{
  int id1, id2;

  if (edgeId == 0)
  {
    id1 = 0;
    id2 = 1;
  }
  else if (edgeId == (this->GetNumberOfPoints() - 1))
  {
    id1 = edgeId - 1;
    id2 = edgeId;
  }
  else
  {
    id1 = edgeId - 1;
    id2 = edgeId + 1;
  }

  this->Line->PointIds->SetId(0, this->PointIds->GetId(id1));
  this->Line->PointIds->SetId(1, this->PointIds->GetId(id2));
  this->Line->Points->SetPoint(0, this->Points->GetPoint(id1));
  this->Line->Points->SetPoint(1, this->Points->GetPoint(id2));

  return this->Line;
}

//------------------------------------------------------------------------------
//
// Intersect sub-triangles
//
int vtkTriangleStrip::IntersectWithLine(const double p1[3], const double p2[3], double tol,
  double& t, double x[3], double pcoords[3], int& subId)
{
  int subTest, numTris = this->Points->GetNumberOfPoints() - 2;

  for (subId = 0; subId < numTris; subId++)
  {
    this->Triangle->Points->SetPoint(0, this->Points->GetPoint(subId));
    this->Triangle->Points->SetPoint(1, this->Points->GetPoint(subId + 1));
    this->Triangle->Points->SetPoint(2, this->Points->GetPoint(subId + 2));

    if (this->Triangle->IntersectWithLine(p1, p2, tol, t, x, pcoords, subTest))
    {
      return 1;
    }
  }

  return 0;
}

//------------------------------------------------------------------------------
int vtkTriangleStrip::TriangulateLocalIds(int vtkNotUsed(index), vtkIdList* ptIds)
{
  int numTris = this->Points->GetNumberOfPoints() - 2;
  ptIds->SetNumberOfIds(3 * numTris);
  int i, order;
  constexpr int idx[2][3] = { { 0, 1, 2 }, { 1, 0, 2 } };
  for (int subId = 0; subId < numTris; subId++)
  {
    order = subId % 2;
    for (i = 0; i < 3; i++)
    {
      ptIds->SetId(subId * 3 + i, subId + idx[order][i]);
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkTriangleStrip::Derivatives(
  int subId, const double pcoords[3], const double* values, int dim, double* derivs)
{
  this->Triangle->Points->SetPoint(0, this->Points->GetPoint(subId));
  this->Triangle->Points->SetPoint(1, this->Points->GetPoint(subId + 1));
  this->Triangle->Points->SetPoint(2, this->Points->GetPoint(subId + 2));

  this->Triangle->Derivatives(0, pcoords, values + dim * subId, dim, derivs);
}

//------------------------------------------------------------------------------
// Given a triangle strip, decompose it into (triangle) polygons. The
// polygons are appended to the end of the list of polygons.
void vtkTriangleStrip::DecomposeStrip(int npts, const vtkIdType* pts, vtkCellArray* polys)
{
  int p1, p2, p3, i;

  p1 = pts[0];
  p2 = pts[1];
  for (i = 0; i < (npts - 2); i++)
  {
    p3 = pts[i + 2];
    polys->InsertNextCell(3);
    if ((i % 2)) // flip ordering to preserve consistency
    {
      polys->InsertCellPoint(p2);
      polys->InsertCellPoint(p1);
      polys->InsertCellPoint(p3);
    }
    else
    {
      polys->InsertCellPoint(p1);
      polys->InsertCellPoint(p2);
      polys->InsertCellPoint(p3);
    }
    p1 = p2;
    p2 = p3;
  }
}

//------------------------------------------------------------------------------
void vtkTriangleStrip::Clip(double value, vtkDataArray* cellScalars,
  vtkIncrementalPointLocator* locator, vtkCellArray* tris, vtkPointData* inPd, vtkPointData* outPd,
  vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
{
  int i, numTris = this->Points->GetNumberOfPoints() - 2;
  int id1, id2, id3;
  vtkDataArray* triScalars = cellScalars->NewInstance();
  triScalars->SetNumberOfComponents(cellScalars->GetNumberOfComponents());
  triScalars->SetNumberOfTuples(3);

  for (i = 0; i < numTris; i++)
  {
    if (i % 2)
    {
      id1 = i + 2;
      id2 = i + 1;
      id3 = i;
    }
    else
    {
      id1 = i;
      id2 = i + 1;
      id3 = i + 2;
    }

    this->Triangle->Points->SetPoint(0, this->Points->GetPoint(id1));
    this->Triangle->Points->SetPoint(1, this->Points->GetPoint(id2));
    this->Triangle->Points->SetPoint(2, this->Points->GetPoint(id3));

    this->Triangle->PointIds->SetId(0, this->PointIds->GetId(id1));
    this->Triangle->PointIds->SetId(1, this->PointIds->GetId(id2));
    this->Triangle->PointIds->SetId(2, this->PointIds->GetId(id3));

    triScalars->SetTuple(0, cellScalars->GetTuple(id1));
    triScalars->SetTuple(1, cellScalars->GetTuple(id2));
    triScalars->SetTuple(2, cellScalars->GetTuple(id3));

    this->Triangle->Clip(
      value, triScalars, locator, tris, inPd, outPd, inCd, cellId, outCd, insideOut);
  }

  triScalars->Delete();
}

//------------------------------------------------------------------------------
// Return the center of the point cloud in parametric coordinates.
int vtkTriangleStrip::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.333333;
  pcoords[2] = 0.0;
  return ((this->Points->GetNumberOfPoints() - 2) / 2);
}

//------------------------------------------------------------------------------
void vtkTriangleStrip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Line:\n";
  this->Line->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Triangle:\n";
  this->Triangle->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
