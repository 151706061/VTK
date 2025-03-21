// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGaussianKernel
 * @brief   a spherical Gaussian interpolation kernel
 *
 *
 * vtkGaussianKernel is an interpolation kernel that simply returns the
 * weights for all points found in the sphere defined by radius R. The
 * weights are computed as: exp(-(s*r/R)^2) where r is the distance from the
 * point to be interpolated to a neighboring point within R. The sharpness s
 * simply affects the rate of fall off of the Gaussian. (A more general
 * Gaussian kernel is available from vtkEllipsoidalGaussianKernel.)
 *
 * @warning
 * The weights are normalized sp that SUM(Wi) = 1. If a neighbor point p
 * precisely lies on the point to be interpolated, then the interpolated
 * point takes on the values associated with p.
 *
 * @sa
 * vtkPointInterpolator vtkInterpolationKernel vtkEllipsoidalGaussianKernel
 * vtkVoronoiKernel vtkSPHKernel vtkShepardKernel
 */

#ifndef vtkGaussianKernel_h
#define vtkGaussianKernel_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkGeneralizedKernel.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkDoubleArray;

class VTKFILTERSPOINTS_EXPORT vtkGaussianKernel : public vtkGeneralizedKernel
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkGaussianKernel* New();
  vtkTypeMacro(vtkGaussianKernel, vtkGeneralizedKernel);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Initialize the kernel. Overload the superclass to set up internal
   * computational values.
   */
  void Initialize(vtkAbstractPointLocator* loc, vtkDataSet* ds, vtkPointData* pd) override;

  // Reuse any superclass signatures that we don't override.
  using vtkGeneralizedKernel::ComputeWeights;

  /**
   * Given a point x, a list of basis points pIds, and a probability
   * weighting function prob, compute interpolation weights associated with
   * these basis points.  Note that basis points list pIds, the probability
   * weighting prob, and the weights array are provided by the caller of the
   * method, and may be dynamically resized as necessary. The method returns
   * the number of weights (pIds may be resized in some cases). Typically
   * this method is called after ComputeBasis(), although advanced users can
   * invoke ComputeWeights() and provide the interpolation basis points pIds
   * directly. The probably weighting prob are numbers 0<=prob<=1 which are
   * multiplied against the interpolation weights before normalization. They
   * are estimates of local confidence of weights. The prob may be nullptr in
   * which all probabilities are considered =1.
   */
  vtkIdType ComputeWeights(
    double x[3], vtkIdList* pIds, vtkDoubleArray* prob, vtkDoubleArray* weights) override;

  ///@{
  /**
   * Set / Get the sharpness (i.e., falloff) of the Gaussian. By default
   * Sharpness=2. As the sharpness increases the effects of distant points
   * are reduced.
   */
  vtkSetClampMacro(Sharpness, double, 1, VTK_FLOAT_MAX);
  vtkGetMacro(Sharpness, double);
  ///@}

protected:
  vtkGaussianKernel();
  ~vtkGaussianKernel() override;

  double Sharpness;

  // Internal structure to reduce computation
  double F2;

private:
  vtkGaussianKernel(const vtkGaussianKernel&) = delete;
  void operator=(const vtkGaussianKernel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
