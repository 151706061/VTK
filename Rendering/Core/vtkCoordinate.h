// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCoordinate
 * @brief   perform coordinate transformation, and represent position, in a variety of vtk
 * coordinate systems
 *
 * vtkCoordinate represents position in a variety of coordinate systems, and
 * converts position to other coordinate systems. It also supports relative
 * positioning, so you can create a cascade of vtkCoordinate objects (no loops
 * please!) that refer to each other. The typical usage of this object is
 * to set the coordinate system in which to represent a position (e.g.,
 * SetCoordinateSystemToNormalizedDisplay()), set the value of the coordinate
 * (e.g., SetValue()), and then invoke the appropriate method to convert to
 * another coordinate system (e.g., GetComputedWorldValue()).
 *
 * The coordinate systems in vtk are as follows:
 * <PRE>
 *   DISPLAY -             x-y pixel values in window
 *      0, 0 is the lower left of the first pixel,
 *      size, size is the upper right of the last pixel
 *   NORMALIZED DISPLAY -  x-y (0,1) normalized values
 *      0, 0 is the lower left of the first pixel,
 *      1, 1 is the upper right of the last pixel
 *   VIEWPORT -            x-y pixel values in viewport
 *      0, 0 is the lower left of the first pixel,
 *      size, size is the upper right of the last pixel
 *   NORMALIZED VIEWPORT - x-y (0,1) normalized value in viewport
 *      0, 0 is the lower left of the first pixel,
 *      1, 1 is the upper right of the last pixel
 *   VIEW -                x-y-z (-1,1) values in pose coordinates. (z is depth)
 *   POSE -                world coords translated and rotated to the camera
 *                         position and view direction
 *   WORLD -               x-y-z global coordinate values
 *   USERDEFINED -         x-y-z in User defined space
 * </PRE>
 *
 * If you cascade vtkCoordinate objects, you refer to another vtkCoordinate
 * object which in turn can refer to others, and so on. This allows you to
 * create composite groups of things like vtkActor2D that are positioned
 * relative to one another. Note that in cascaded sequences, each
 * vtkCoordinate object may be specified in different coordinate systems!
 *
 * @sa
 * vtkActor2D vtkScalarBarActor
 */

#ifndef vtkCoordinate_h
#define vtkCoordinate_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO
VTK_ABI_NAMESPACE_BEGIN
class vtkViewport;

#define VTK_DISPLAY 0
#define VTK_NORMALIZED_DISPLAY 1
#define VTK_VIEWPORT 2
#define VTK_NORMALIZED_VIEWPORT 3
#define VTK_VIEW 4
#define VTK_POSE 5
#define VTK_WORLD 6
#define VTK_USERDEFINED 7

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkCoordinate : public vtkObject
{
public:
  vtkTypeMacro(vtkCoordinate, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates an instance of this class with the following defaults:
   * value of  (0,0,0) in world coordinates.
   */
  static vtkCoordinate* New();

  ///@{
  /**
   * Set/get the coordinate system which this coordinate
   * is defined in. The options are Display, Normalized Display,
   * Viewport, Normalized Viewport, View, and World.
   */
  vtkSetMacro(CoordinateSystem, int);
  vtkGetMacro(CoordinateSystem, int);
  void SetCoordinateSystemToDisplay() { this->SetCoordinateSystem(VTK_DISPLAY); }
  void SetCoordinateSystemToNormalizedDisplay()
  {
    this->SetCoordinateSystem(VTK_NORMALIZED_DISPLAY);
  }
  void SetCoordinateSystemToViewport() { this->SetCoordinateSystem(VTK_VIEWPORT); }
  void SetCoordinateSystemToNormalizedViewport()
  {
    this->SetCoordinateSystem(VTK_NORMALIZED_VIEWPORT);
  }
  void SetCoordinateSystemToView() { this->SetCoordinateSystem(VTK_VIEW); }
  void SetCoordinateSystemToPose() { this->SetCoordinateSystem(VTK_POSE); }
  void SetCoordinateSystemToWorld() { this->SetCoordinateSystem(VTK_WORLD); }
  ///@}

  const char* GetCoordinateSystemAsString();

  ///@{
  /**
   * Set/get the value of this coordinate. This can be thought of as
   * the position of this coordinate in its coordinate system.
   */
  vtkSetVector3Macro(Value, double);
  vtkGetVector3Macro(Value, double);
  void SetValue(double a, double b) { this->SetValue(a, b, 0.0); }
  ///@}

  ///@{
  /**
   * If this coordinate is relative to another coordinate,
   * then specify that coordinate as the ReferenceCoordinate.
   * If this is NULL the coordinate is assumed to be absolute.
   */
  virtual void SetReferenceCoordinate(vtkCoordinate*);
  vtkGetObjectMacro(ReferenceCoordinate, vtkCoordinate);
  ///@}

  ///@{
  /**
   * If you want this coordinate to be relative to a specific
   * vtkViewport (vtkRenderer) then you can specify that here.
   * NOTE: this is a raw pointer, not a weak pointer nor a reference counted
   * object, to avoid reference cycle loop between rendering classes and filter
   * classes.
   */
  void SetViewport(vtkViewport* viewport);
  vtkGetObjectMacro(Viewport, vtkViewport);
  ///@}

  ///@{
  /**
   * Return the computed value in a specified coordinate system.
   */
  double* GetComputedWorldValue(vtkViewport*) VTK_SIZEHINT(3);
  int* GetComputedViewportValue(vtkViewport*) VTK_SIZEHINT(2);
  int* GetComputedDisplayValue(vtkViewport*) VTK_SIZEHINT(2);
  int* GetComputedLocalDisplayValue(vtkViewport*) VTK_SIZEHINT(2);
  ///@}

  double* GetComputedDoubleViewportValue(vtkViewport*) VTK_SIZEHINT(2);
  double* GetComputedDoubleDisplayValue(vtkViewport*) VTK_SIZEHINT(2);

  /**
   * GetComputedValue() will return either World, Viewport or
   * Display based on what has been set as the coordinate system.
   * This is good for objects like vtkLineSource, where the
   * user might want to use them as World or Viewport coordinates.
   */
  double* GetComputedValue(vtkViewport*) VTK_SIZEHINT(3);

  /**
   * GetComputedUserDefinedValue() is to be used only when
   * the coordinate system is VTK_USERDEFINED. The user
   * must subclass vtkCoordinate and override this function,
   * when set as the TransformCoordinate in 2D-Mappers, the user
   * can customize display of 2D polygons
   */
  virtual double* GetComputedUserDefinedValue(vtkViewport*) VTK_SIZEHINT(3) { return this->Value; }

protected:
  vtkCoordinate();
  ~vtkCoordinate() override;

  double Value[3];
  int CoordinateSystem;
  vtkCoordinate* ReferenceCoordinate;
  vtkViewport* Viewport;
  double ComputedWorldValue[3];
  int ComputedDisplayValue[2];
  int ComputedViewportValue[2];
  int Computing;

  double ComputedDoubleDisplayValue[2];
  double ComputedDoubleViewportValue[2];
  double ComputedUserDefinedValue[3];

private:
  vtkCoordinate(const vtkCoordinate&) = delete;
  void operator=(const vtkCoordinate&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
