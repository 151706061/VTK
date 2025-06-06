// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractImageInterpolator
 * @brief   interpolate data values from images
 *
 * vtkAbstractImageInterpolator provides an abstract interface for
 * interpolating image data.  You specify the data set you want to
 * interpolate values from, then call Interpolate(x,y,z) to interpolate
 * the data.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 * @sa
 * vtkImageReslice vtkImageInterpolator vtkImageSincInterpolator
 */

#ifndef vtkAbstractImageInterpolator_h
#define vtkAbstractImageInterpolator_h

#include "vtkImagingCoreModule.h" // For export macro
#include "vtkObject.h"
#include "vtkWrappingHints.h"

enum vtkImageBorderMode : int
{
  VTK_IMAGE_BORDER_CLAMP = 0,
  VTK_IMAGE_BORDER_REPEAT = 1,
  VTK_IMAGE_BORDER_MIRROR = 2,
};

VTK_ABI_NAMESPACE_BEGIN
class vtkDataObject;
class vtkImageData;
class vtkDataArray;
struct vtkInterpolationInfo;
struct vtkInterpolationWeights;

class VTKIMAGINGCORE_EXPORT VTK_MARSHALAUTO vtkAbstractImageInterpolator : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractImageInterpolator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initialize the interpolator with the data that you wish to interpolate.
   */
  virtual void Initialize(vtkDataObject* data);

  /**
   * Release any data stored by the interpolator.
   */
  virtual void ReleaseData();

  /**
   * Copy the interpolator.  It is possible to duplicate an interpolator
   * by calling NewInstance() followed by DeepCopy().
   */
  void DeepCopy(vtkAbstractImageInterpolator* obj);

  /**
   * Update the interpolator.  If the interpolator has been modified by
   * a Set method since Initialize() was called, you must call this method
   * to update the interpolator before you can use it.
   */
  virtual void Update();

  /**
   * Get the result of interpolating the specified component of the input
   * data, which should be set to zero if there is only one component.
   * If the point is not within the bounds of the data set, then OutValue
   * will be returned.  This method is primarily meant for use by the
   * wrapper languages.
   */
  double Interpolate(double x, double y, double z, int component);

  /**
   * Sample the input data. This is an inline method that calls the
   * function that performs the appropriate interpolation for the
   * data type.  If the point is not within the bounds of the data set,
   * then the return value is false, and each component will be set to
   * the OutValue.
   */
  bool Interpolate(const double point[3], double* value);

  /**
   * The value to return when the point is out of bounds.
   */
  void SetOutValue(double outValue);
  double GetOutValue() { return this->OutValue; }

  /**
   * The tolerance to apply when checking whether a point is out of bounds.
   * This is a fractional distance relative to the voxel size, so a tolerance
   * of 1 expands the bounds by one voxel.
   */
  void SetTolerance(double tol);
  double GetTolerance() { return this->Tolerance; }

  /**
   * This method specifies which component of the input will be interpolated,
   * or if ComponentCount is also set, it specifies the first component.
   * When the interpolation is performed, it will be clamped to the number
   * of available components.
   */
  void SetComponentOffset(int offset);
  int GetComponentOffset() { return this->ComponentOffset; }

  /**
   * This method specifies the number of components to extract.  The default
   * value is -1, which extracts all available components.  When the
   * interpolation is performed, this will be clamped to the number of
   * available components.
   */
  void SetComponentCount(int count);
  int GetComponentCount() { return this->ComponentCount; }

  /**
   * Compute the number of output components based on the ComponentOffset,
   * ComponentCount, and the number of components in the input data.
   */
  int ComputeNumberOfComponents(int inputComponents);

  /**
   * Get the number of components that will be returned when Interpolate()
   * is called.  This is only valid after initialization.  Before then, use
   * ComputeNumberOfComponents instead.
   */
  int GetNumberOfComponents();

  ///@{
  /**
   * A version of Interpolate that takes structured coords instead of data
   * coords.  Structured coords are the data coords after subtracting the
   * Origin and dividing by the Spacing.
   */
  void InterpolateIJK(const double point[3], double* value);
  void InterpolateIJK(const float point[3], float* value);
  ///@}

  ///@{
  /**
   * Check an x,y,z point to see if it is within the bounds for the
   * structured coords of the image.  This is meant to be called prior
   * to InterpolateIJK.  The bounds that are checked against are the input
   * image extent plus the tolerance.
   */
  bool CheckBoundsIJK(const double x[3]);
  bool CheckBoundsIJK(const float x[3]);
  ///@}

  ///@{
  /**
   * The border mode (default: clamp).  This controls how out-of-bounds
   * lookups are handled, i.e. how data will be extrapolated beyond the
   * bounds of the image.  The default is to clamp the lookup point to the
   * bounds.  The other modes wrap around to the opposite boundary, or
   * mirror the image at the boundary.
   */
  void SetBorderMode(vtkImageBorderMode mode);
  void SetBorderModeToClamp() { this->SetBorderMode(VTK_IMAGE_BORDER_CLAMP); }
  void SetBorderModeToRepeat() { this->SetBorderMode(VTK_IMAGE_BORDER_REPEAT); }
  void SetBorderModeToMirror() { this->SetBorderMode(VTK_IMAGE_BORDER_MIRROR); }
  vtkImageBorderMode GetBorderMode() { return this->BorderMode; }
  const char* GetBorderModeAsString();
  ///@}

  /**
   * Enable sliding window for separable kernels.
   * When this is enabled, the interpolator will cache partial sums in
   * in order to accelerate the computation.  It only makes sense to do
   * this if the interpolator is used by calling InterpolateRow() while
   * incrementing first the Y, and then the Z index with every call.
   */
  void SetSlidingWindow(bool x);
  void SlidingWindowOn() { this->SetSlidingWindow(true); }
  void SlidingWindowOff() { this->SetSlidingWindow(false); }
  bool GetSlidingWindow() { return this->SlidingWindow; }

  /**
   * Get the support size for use in computing update extents.  If the data
   * will be sampled on a regular grid, then pass a matrix describing the
   * structured coordinate transformation between the output and the input.
   * Otherwise, pass nullptr as the matrix to retrieve the full kernel size.
   */
  virtual void ComputeSupportSize(const double matrix[16], int support[3]) = 0;

  /**
   * True if the interpolation is separable, which means that the weights
   * can be precomputed in order to accelerate the interpolation.  Any
   * interpolator which is separable will implement the methods
   * PrecomputeWeightsForExtent and InterpolateRow
   */
  virtual bool IsSeparable() = 0;

  ///@{
  /**
   * If the data is going to be sampled on a regular grid, then the
   * interpolation weights can be precomputed.  A matrix must be supplied
   * that provides a transformation between the provided extent and the
   * structured coordinates of the input.  This matrix must perform only
   * permutation, scale, and translation, i.e. each of the three columns
   * must have only one non-zero value.  A checkExtent is provided that can
   * be used to check which indices in the extent map to out-of-bounds
   * coordinates in the input data.
   */
  virtual void PrecomputeWeightsForExtent(const double matrix[16], const int extent[6],
    int checkExtent[6], vtkInterpolationWeights*& weights);
  virtual void PrecomputeWeightsForExtent(const float matrix[16], const int extent[6],
    int checkExtent[6], vtkInterpolationWeights*& weights);
  ///@}

  /**
   * Free the weights that were provided by PrecomputeWeightsForExtent.
   */
  virtual void FreePrecomputedWeights(vtkInterpolationWeights*& weights);

  ///@{
  /**
   * Get a row of samples, using the weights that were precomputed
   * by PrecomputeWeightsForExtent.  Note that each sample may have
   * multiple components.  It is possible to select which components
   * will be returned by setting the ComponentOffset and ComponentCount.
   */
  void InterpolateRow(
    vtkInterpolationWeights*& weights, int xIdx, int yIdx, int zIdx, double* value, int n);
  void InterpolateRow(
    vtkInterpolationWeights*& weights, int xIdx, int yIdx, int zIdx, float* value, int n);
  ///@}

  ///@{
  /**
   * Get the spacing of the data being interpolated.
   */
  vtkGetVector3Macro(Spacing, double);
  ///@}

  ///@{
  /**
   * Get the direction of the data being interpolated.
   */
  vtkGetVectorMacro(Direction, double, 9);
  ///@}

  ///@{
  /**
   * Get the origin of the data being interpolated.
   */
  vtkGetVector3Macro(Origin, double);
  ///@}

  ///@{
  /**
   * Get the extent of the data being interpolated.
   */
  vtkGetVector6Macro(Extent, int);
  ///@}

protected:
  vtkAbstractImageInterpolator();
  ~vtkAbstractImageInterpolator() override;

  /**
   * Subclass-specific updates.
   */
  virtual void InternalUpdate() = 0;

  /**
   * Subclass-specific copy.
   */
  virtual void InternalDeepCopy(vtkAbstractImageInterpolator* obj) = 0;

  /**
   * Convert XYZ coordinate to IJK continuous index
   */
  void CoordinateToIJK(const double point[3], double ijk[3]);

  ///@{
  /**
   * Get the interpolation functions.
   */
  virtual void GetInterpolationFunc(
    void (**doublefunc)(vtkInterpolationInfo*, const double[3], double*));
  virtual void GetInterpolationFunc(
    void (**floatfunc)(vtkInterpolationInfo*, const float[3], float*));
  ///@}

  ///@{
  /**
   * Get the row interpolation functions.
   */
  virtual void GetRowInterpolationFunc(
    void (**doublefunc)(vtkInterpolationWeights*, int, int, int, double*, int));
  virtual void GetRowInterpolationFunc(
    void (**floatfunc)(vtkInterpolationWeights*, int, int, int, float*, int));
  ///@}

  ///@{
  /**
   * Get the sliding window interpolation functions.
   */
  virtual void GetSlidingWindowFunc(
    void (**doublefunc)(vtkInterpolationWeights*, int, int, int, double*, int));
  virtual void GetSlidingWindowFunc(
    void (**floatfunc)(vtkInterpolationWeights*, int, int, int, float*, int));
  ///@}

  vtkDataArray* Scalars;
  double StructuredBoundsDouble[6];
  float StructuredBoundsFloat[6];
  int Extent[6];
  double Spacing[3];
  double Direction[9];
  double InverseDirection[9];
  double Origin[3];
  double OutValue;
  double Tolerance;
  vtkImageBorderMode BorderMode;
  int ComponentOffset;
  int ComponentCount;
  bool UseDirection;
  bool SlidingWindow;

  // information needed by the interpolator funcs
  vtkInterpolationInfo* InterpolationInfo;

  void (*InterpolationFuncDouble)(
    vtkInterpolationInfo* info, const double point[3], double* outPtr);
  void (*InterpolationFuncFloat)(vtkInterpolationInfo* info, const float point[3], float* outPtr);

  void (*RowInterpolationFuncDouble)(
    vtkInterpolationWeights* weights, int idX, int idY, int idZ, double* outPtr, int n);
  void (*RowInterpolationFuncFloat)(
    vtkInterpolationWeights* weights, int idX, int idY, int idZ, float* outPtr, int n);

private:
  vtkAbstractImageInterpolator(const vtkAbstractImageInterpolator&) = delete;
  void operator=(const vtkAbstractImageInterpolator&) = delete;
};

inline void vtkAbstractImageInterpolator::InterpolateIJK(const double point[3], double* value)
{
  this->InterpolationFuncDouble(this->InterpolationInfo, point, value);
}

inline void vtkAbstractImageInterpolator::InterpolateIJK(const float point[3], float* value)
{
  this->InterpolationFuncFloat(this->InterpolationInfo, point, value);
}

inline bool vtkAbstractImageInterpolator::CheckBoundsIJK(const double x[3])
{
  const double* bounds = this->StructuredBoundsDouble;
  return !((x[0] < bounds[0]) || (x[0] > bounds[1]) || (x[1] < bounds[2]) || (x[1] > bounds[3]) ||
    (x[2] < bounds[4]) || (x[2] > bounds[5]));
}

inline bool vtkAbstractImageInterpolator::CheckBoundsIJK(const float x[3])
{
  const float* bounds = this->StructuredBoundsFloat;
  return !((x[0] < bounds[0]) || (x[0] > bounds[1]) || (x[1] < bounds[2]) || (x[1] > bounds[3]) ||
    (x[2] < bounds[4]) || (x[2] > bounds[5]));
}

inline void vtkAbstractImageInterpolator::InterpolateRow(
  vtkInterpolationWeights*& weights, int xIdx, int yIdx, int zIdx, double* value, int n)
{
  this->RowInterpolationFuncDouble(weights, xIdx, yIdx, zIdx, value, n);
}

inline void vtkAbstractImageInterpolator::InterpolateRow(
  vtkInterpolationWeights*& weights, int xIdx, int yIdx, int zIdx, float* value, int n)
{
  this->RowInterpolationFuncFloat(weights, xIdx, yIdx, zIdx, value, n);
}

VTK_ABI_NAMESPACE_END
#endif
