// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-FileCopyrightText: Copyright 2012 Sandia Corporation.
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkmWarpScalar
 * @brief   deform geometry with scalar data
 *
 * vtkmWarpScalar is a filter that modifies point coordinates by moving points
 * along point normals by the scalar amount times the scalar factor with viskores
 * as its backend.
 * Useful for creating carpet or x-y-z plots.
 *
 * If normals are not present in data, the Normal instance variable will
 * be used as the direction along which to warp the geometry. If normals are
 * present but you would like to use the Normal instance variable, set the
 * UseNormal boolean to true.
 *
 * If XYPlane boolean is set true, then the z-value is considered to be
 * a scalar value (still scaled by scale factor), and the displacement is
 * along the z-axis. If scalars are also present, these are copied through
 * and can be used to color the surface.
 *
 * Note that the filter passes both its point data and cell data to
 * its output, except for normals, since these are distorted by the
 * warping.
 */

#ifndef vtkmWarpScalar_h
#define vtkmWarpScalar_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // required for correct export
#include "vtkWarpScalar.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmWarpScalar : public vtkWarpScalar
{
public:
  vtkTypeMacro(vtkmWarpScalar, vtkWarpScalar);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkmWarpScalar* New();

protected:
  vtkmWarpScalar();
  ~vtkmWarpScalar() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmWarpScalar(const vtkmWarpScalar&) = delete;
  void operator=(const vtkmWarpScalar&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmWarpScalar_h
