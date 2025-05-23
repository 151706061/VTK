// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCamera
 * @brief   a virtual camera for 3D rendering
 *
 * vtkCamera is a virtual camera for 3D rendering. It provides methods
 * to position and orient the view point and focal point. Convenience
 * methods for moving about the focal point also are provided. More
 * complex methods allow the manipulation of the computer graphics
 * model including view up vector, clipping planes, and
 * camera perspective.
 * @sa
 * vtkPerspectiveTransform
 */

#ifndef vtkCamera_h
#define vtkCamera_h

#include "vtkObject.h"
#include "vtkRect.h"                // for ivar
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkHomogeneousTransform;
class vtkInformation;
class vtkMatrix4x4;
class vtkPerspectiveTransform;
class vtkRenderer;
class vtkTransform;
class vtkCallbackCommand;
class vtkCameraCallbackCommand;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkCamera : public vtkObject
{
public:
  vtkTypeMacro(vtkCamera, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct camera instance with its focal point at the origin,
   * and position=(0,0,1). The view up is along the y-axis,
   * view angle is 30 degrees, and the clipping range is (.1,1000).
   */
  static vtkCamera* New();

  ///@{
  /**
   * Set/Get the position of the camera in world coordinates.
   * The default position is (0,0,1).
   */
  void SetPosition(double x, double y, double z);
  void SetPosition(const double a[3]) { this->SetPosition(a[0], a[1], a[2]); }
  vtkGetVector3Macro(Position, double);
  ///@}

  ///@{
  /**
   * Set/Get the focal of the camera in world coordinates.
   * The default focal point is the origin.
   */
  void SetFocalPoint(double x, double y, double z);
  void SetFocalPoint(const double a[3]) { this->SetFocalPoint(a[0], a[1], a[2]); }
  vtkGetVector3Macro(FocalPoint, double);
  ///@}

  ///@{
  /**
   * Set/Get the view up direction for the camera.  The default
   * is (0,1,0).
   */
  void SetViewUp(double vx, double vy, double vz);
  void SetViewUp(const double a[3]) { this->SetViewUp(a[0], a[1], a[2]); }
  vtkGetVector3Macro(ViewUp, double);
  ///@}

  /**
   * Recompute the ViewUp vector to force it to be perpendicular to
   * camera->focalpoint vector.  Unless you are going to use
   * Yaw or Azimuth on the camera, there is no need to do this.
   */
  void OrthogonalizeViewUp();

  /**
   * Move the focal point so that it is the specified distance from
   * the camera position.  This distance must be positive.
   */
  void SetDistance(double);

  ///@{
  /**
   * Return the distance from the camera position to the focal point.
   * This distance is positive.
   */
  vtkGetMacro(Distance, double);
  ///@}

  ///@{
  /**
   * Get the vector in the direction from the camera position to the
   * focal point.  This is usually the opposite of the ViewPlaneNormal,
   * the vector perpendicular to the screen, unless the view is oblique.
   */
  vtkGetVector3Macro(DirectionOfProjection, double);
  ///@}

  /**
   * Divide the camera's distance from the focal point by the given
   * dolly value.  Use a value greater than one to dolly-in toward
   * the focal point, and use a value less than one to dolly-out away
   * from the focal point.
   */
  void Dolly(double value);

  ///@{
  /**
   * Set the roll angle of the camera about the direction of projection.
   */
  void SetRoll(double angle);
  double GetRoll();
  ///@}

  /**
   * Rotate the camera about the direction of projection.  This will
   * spin the camera about its axis.
   */
  void Roll(double angle);

  /**
   * Rotate the camera about the view up vector centered at the focal point.
   * Note that the view up vector is whatever was set via SetViewUp, and is
   * not necessarily perpendicular to the direction of projection.  The
   * result is a horizontal rotation of the camera.
   */
  void Azimuth(double angle);

  /**
   * Rotate the focal point about the view up vector, using the camera's
   * position as the center of rotation. Note that the view up vector is
   * whatever was set via SetViewUp, and is not necessarily perpendicular
   * to the direction of projection.  The result is a horizontal rotation
   * of the scene.
   */
  void Yaw(double angle);

  /**
   * Rotate the camera about the cross product of the negative of the
   * direction of projection and the view up vector, using the focal point
   * as the center of rotation.  The result is a vertical rotation of the
   * scene.
   */
  void Elevation(double angle);

  /**
   * Rotate the focal point about the cross product of the view up vector
   * and the direction of projection, using the camera's position as the
   * center of rotation.  The result is a vertical rotation of the camera.
   */
  void Pitch(double angle);

  ///@{
  /**
   * Set/Get the value of the ParallelProjection instance variable. This
   * determines if the camera should do a perspective or parallel projection.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetParallelProjection(vtkTypeBool flag);
  vtkGetMacro(ParallelProjection, vtkTypeBool);
  vtkBooleanMacro(ParallelProjection, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the value of the UseHorizontalViewAngle instance variable. If
   * set, the camera's view angle represents a horizontal view angle, rather
   * than the default vertical view angle. This is useful if the application
   * uses a display device which whose specs indicate a particular horizontal
   * view angle, or if the application varies the window height but wants to
   * keep the perspective transform unchanges.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetUseHorizontalViewAngle(vtkTypeBool flag);
  vtkGetMacro(UseHorizontalViewAngle, vtkTypeBool);
  vtkBooleanMacro(UseHorizontalViewAngle, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the camera view angle, which is the angular height of the
   * camera view measured in degrees.  The default angle is 30 degrees.
   * This method has no effect in parallel projection mode.
   * The formula for setting the angle up for perfect perspective viewing
   * is: angle = 2*atan((h/2)/d) where h is the height of the RenderWindow
   * (measured by holding a ruler up to your screen) and d is the
   * distance from your eyes to the screen.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetViewAngle(double angle);
  vtkGetMacro(ViewAngle, double);
  ///@}

  ///@{
  /**
   * Set/Get the scaling used for a parallel projection, i.e. the half of the height
   * of the viewport in world-coordinate distances. The default is 1.
   * Note that the "scale" parameter works as an "inverse scale" ---
   * larger numbers produce smaller images.
   * This method has no effect in perspective projection mode.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetParallelScale(double scale);
  vtkGetMacro(ParallelScale, double);
  ///@}

  /**
   * In perspective mode, decrease the view angle by the specified factor.
   * In parallel mode, decrease the parallel scale by the specified factor.
   * A value greater than 1 is a zoom-in, a value less than 1 is a zoom-out.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void Zoom(double factor);

  ///@{
  /**
   * Set/Get the location of the near and far clipping planes along the
   * direction of projection.  Both of these values must be positive.
   * How the clipping planes are set can have a large impact on how
   * well z-buffering works.  In particular the front clipping
   * plane can make a very big difference. Setting it to 0.01 when it
   * really could be 1.0 can have a big impact on your z-buffer resolution
   * farther away.  The default clipping range is (0.1,1000).
   * Clipping distance is measured in world coordinate unless a scale factor
   * exists in camera's ModelTransformMatrix.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetClippingRange(double dNear, double dFar);
  void SetClippingRange(const double a[2]) { this->SetClippingRange(a[0], a[1]); }
  vtkGetVector2Macro(ClippingRange, double);
  ///@}

  ///@{
  /**
   * Set the distance between clipping planes.  This method adjusts the
   * far clipping plane to be set a distance 'thickness' beyond the
   * near clipping plane.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetThickness(double);
  vtkGetMacro(Thickness, double);
  ///@}

  ///@{
  /**
   * Set/Get the center of the window in viewport coordinates.
   * The viewport coordinate range is ([-1,+1],[-1,+1]).  This method
   * is for if you have one window which consists of several viewports,
   * or if you have several screens which you want to act together as
   * one large screen.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetWindowCenter(double x, double y);
  vtkGetVector2Macro(WindowCenter, double);
  ///@}

  /**
   * Get/Set the oblique viewing angles.  The first angle, alpha, is the
   * angle (measured from the horizontal) that rays along the direction
   * of projection will follow once projected onto the 2D screen.
   * The second angle, beta, is the angle between the view plane and
   * the direction of projection.  This creates a shear transform
   * x' = x + dz*cos(alpha)/tan(beta), y' = dz*sin(alpha)/tan(beta)
   * where dz is the distance of the point from the focal plane.
   * The angles are (45,90) by default.  Oblique projections
   * commonly use (30,63.435).
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetObliqueAngles(double alpha, double beta);

  /**
   * Apply a transform to the camera.  The camera position, focal-point,
   * and view-up are re-calculated using the transform's matrix to
   * multiply the old points by the new transform.
   */
  void ApplyTransform(vtkTransform* t);

  ///@{
  /**
   * Get the ViewPlaneNormal.  This vector will point opposite to
   * the direction of projection, unless you have created a sheared output
   * view using SetViewShear/SetObliqueAngles.
   */
  vtkGetVector3Macro(ViewPlaneNormal, double);
  ///@}

  ///@{
  /**
   * Set/get the shear transform of the viewing frustum.  Parameters are
   * dx/dz, dy/dz, and center.  center is a factor that describes where
   * to shear around. The distance dshear from the camera where
   * no shear occurs is given by (dshear = center * FocalDistance).
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  void SetViewShear(double dxdz, double dydz, double center);
  void SetViewShear(double d[3]);
  vtkGetVector3Macro(ViewShear, double);
  ///@}

  ///@{
  /**
   * Set/Get the separation between eyes (in degrees). This is used
   * when generating stereo images.
   */
  vtkSetMacro(EyeAngle, double);
  vtkGetMacro(EyeAngle, double);
  ///@}

  ///@{
  /**
   * Set the size of the cameras lens in world coordinates. This is only
   * used when the renderer is doing focal depth rendering. When that is
   * being done the size of the focal disk will effect how significant the
   * depth effects will be.
   */
  vtkSetMacro(FocalDisk, double);
  vtkGetMacro(FocalDisk, double);
  ///@}

  ///@{
  /**
   * Sets the distance at which rendering is in focus. This is currently
   * only used by the ray tracing renderers. 0 (default) disables
   * ray traced depth of field.
   * Not to be confused with FocalPoint that is the camera target and
   * is centered on screen. Using a separate focal distance property
   * enables out-of-focus areas anywhere on screen.
   */
  vtkSetMacro(FocalDistance, double);
  vtkGetMacro(FocalDistance, double);
  ///@}

  ///@{
  /**
   * Set/Get use offaxis frustum.
   * OffAxis frustum is used for off-axis frustum calculations specifically
   * for head-tracking with stereo rendering.
   * For reference see "Generalized Perspective Projection" by Robert Kooima,
   * 2008.
   * @note This setting is ignored when UseExplicitProjectionTransformMatrix
   * is true.
   */
  vtkSetMacro(UseOffAxisProjection, vtkTypeBool);
  vtkGetMacro(UseOffAxisProjection, vtkTypeBool);
  vtkBooleanMacro(UseOffAxisProjection, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get adjustment to clipping thickness, computed by camera based on the
   * physical size of the screen and the direction to the tracked head/eye.
   */
  double GetOffAxisClippingAdjustment();
  ///@}

  ///@{
  /**
   * Set/Get top left corner point of the screen.
   * This will be used only for offaxis frustum calculation.
   * Default is (-1.0, -1.0, -1.0).
   */
  vtkSetVector3Macro(ScreenBottomLeft, double);
  vtkGetVector3Macro(ScreenBottomLeft, double);
  ///@}

  ///@{
  /**
   * Set/Get bottom left corner point of the screen.
   * This will be used only for offaxis frustum calculation.
   * Default is (1.0, -1.0, -1.0).
   */
  vtkSetVector3Macro(ScreenBottomRight, double);
  vtkGetVector3Macro(ScreenBottomRight, double);
  ///@}

  ///@{
  /**
   * Set/Get top right corner point of the screen.
   * This will be used only for offaxis frustum calculation.
   * Default is (1.0, 1.0, -1.0).
   */
  vtkSetVector3Macro(ScreenTopRight, double);
  vtkGetVector3Macro(ScreenTopRight, double);
  ///@}

  ///@{
  /**
   * Set/Get distance between the eyes.
   * This will be used only for offaxis frustum calculation.
   * Default is 0.06.
   */
  vtkSetMacro(EyeSeparation, double);
  vtkGetMacro(EyeSeparation, double);
  ///@}

  ///@{
  /**
   * Set/Get the eye position (center point between two eyes).
   * This is a convenience function that sets the translation
   * component of EyeTransformMatrix.
   * This will be used only for offaxis frustum calculation.
   */
  void SetEyePosition(double eyePosition[3]);
  void GetEyePosition(double eyePosition[3]);
  ///@}

  ///@{
  /**
   * Using the LeftEye property to determine whether left or right
   * eye is being requested, this method computes and returns the
   * position of the requested eye, taking head orientation
   * and eye separation into account.
   * The eyePosition parameter is output only, all elements are
   * overwritten.
   */
  void GetStereoEyePosition(double eyePosition[3]);
  ///@}

  /**
   * Get normal vector from eye to screen rotated by EyeTransformMatrix.
   * This will be used only for offaxis frustum calculation.
   */
  void GetEyePlaneNormal(double normal[3]);

  ///@{
  /**
   * Set/Get eye transformation matrix.
   * This is the transformation matrix for the point between eyes.
   * This will be used only for offaxis frustum calculation.
   * Default is identity.
   */
  void SetEyeTransformMatrix(vtkMatrix4x4* matrix);
  void SetEyeTransformMatrix(const double elements[16]);
  vtkGetObjectMacro(EyeTransformMatrix, vtkMatrix4x4);
  ///@}

  ///@{
  /**
   * Set/Get model transformation matrix.
   * This matrix could be used for model related transformations
   * such as scale, shear, rotations and translations.
   */
  void SetModelTransformMatrix(vtkMatrix4x4* matrix);
  void SetModelTransformMatrix(const double elements[16]);
  vtkGetObjectMacro(ModelTransformMatrix, vtkMatrix4x4);
  ///@}

  /**
   * Return the model view matrix of model view transform.
   */
  virtual vtkMatrix4x4* GetModelViewTransformMatrix();

  /**
   * Return the model view transform.
   */
  virtual vtkTransform* GetModelViewTransformObject();

  /**
   * For backward compatibility. Use GetModelViewTransformMatrix() now.
   * Return the matrix of the view transform.
   * The ViewTransform depends on only three ivars:  the Position, the
   * FocalPoint, and the ViewUp vector.  All the other methods are there
   * simply for the sake of the users' convenience.
   */
  virtual vtkMatrix4x4* GetViewTransformMatrix();

  /**
   * For backward compatibility. Use GetModelViewTransformObject() now.
   * Return the view transform.
   * If the camera's ModelTransformMatrix is identity then
   * the ViewTransform depends on only three ivars:
   * the Position, the FocalPoint, and the ViewUp vector.
   * All the other methods are there simply for the sake of the users'
   * convenience.
   */
  virtual vtkTransform* GetViewTransformObject();

  /**
   * Set/get an explicit 4x4 projection matrix to use, rather than computing
   * one from other state variables. Only used when
   * UseExplicitProjectionTransformMatrix is true.
   * @{
   */
  virtual void SetExplicitProjectionTransformMatrix(vtkMatrix4x4*);
  vtkGetObjectMacro(ExplicitProjectionTransformMatrix, vtkMatrix4x4);
  /**@}*/

  /**
   * If true, the ExplicitProjectionTransformMatrix is used for the projection
   * transformation, rather than computing a transform from internal state.
   * @{
   */
  vtkSetMacro(UseExplicitProjectionTransformMatrix, bool);
  vtkGetMacro(UseExplicitProjectionTransformMatrix, bool);
  vtkBooleanMacro(UseExplicitProjectionTransformMatrix, bool);
  /**@}*/

  /**
   * Set/get an explicit aspect to use, rather than computing it from the renderer.
   * Only used when UseExplicitAspect is true.
   * @{
   */
  vtkSetMacro(ExplicitAspectRatio, double);
  vtkGetMacro(ExplicitAspectRatio, double);
  /**@}*/

  /**
   * If true, the ExplicitAspect is used for the projection
   * transformation, rather than computing it from the renderer.
   * Default is false.
   * @{
   */
  vtkSetMacro(UseExplicitAspectRatio, bool);
  vtkGetMacro(UseExplicitAspectRatio, bool);
  vtkBooleanMacro(UseExplicitAspectRatio, bool);
  /**@}*/

  /**
   * Return the projection transform matrix, which converts from camera
   * coordinates to viewport coordinates.  The 'aspect' is the
   * width/height for the viewport, and the nearz and farz are the
   * Z-buffer values that map to the near and far clipping planes.
   * The viewport coordinates of a point located inside the frustum are in the
   * range ([-1,+1],[-1,+1],[nearz,farz]).
   * aspect is ignored if UseExplicitAspectRatio is true.
   * @sa ExplicitProjectionTransformMatrix
   */
  virtual vtkMatrix4x4* GetProjectionTransformMatrix(double aspect, double nearz, double farz);

  /**
   * Return the projection transform matrix, which converts from camera
   * coordinates to viewport coordinates. The 'aspect' is the
   * width/height for the viewport, and the nearz and farz are the
   * Z-buffer values that map to the near and far clipping planes.
   * The viewport coordinates of a point located inside the frustum are in the
   * range ([-1,+1],[-1,+1],[nearz,farz]).
   * aspect is ignored if UseExplicitAspectRatio is true.
   * @sa ExplicitProjectionTransformMatrix
   */
  virtual vtkPerspectiveTransform* GetProjectionTransformObject(
    double aspect, double nearz, double farz);

  /**
   * Return the concatenation of the ViewTransform and the
   * ProjectionTransform. This transform will convert world
   * coordinates to viewport coordinates. The 'aspect' is the
   * width/height for the viewport, and the nearz and farz are the
   * Z-buffer values that map to the near and far clipping planes.
   * The viewport coordinates of a point located inside the frustum are in the
   * range ([-1,+1],[-1,+1],[nearz,farz]).
   * aspect is ignored if UseExplicitAspectRatio is true.
   * @sa ExplicitProjectionTransformMatrix
   */
  virtual vtkMatrix4x4* GetCompositeProjectionTransformMatrix(
    double aspect, double nearz, double farz);

  /**
   * Return the projection transform matrix, which converts from camera
   * coordinates to viewport coordinates. This method computes
   * the aspect, nearz and farz, then calls the more specific
   * signature of GetCompositeProjectionTransformMatrix
   * @sa ExplicitProjectionTransformMatrix
   */
  virtual vtkMatrix4x4* GetProjectionTransformMatrix(vtkRenderer* ren);

  ///@{
  /**
   * In addition to the instance variables such as position and orientation,
   * you can add an additional transformation for your own use.  This
   * transformation is concatenated to the camera's ViewTransform
   */
  void SetUserViewTransform(vtkHomogeneousTransform* transform);
  vtkGetObjectMacro(UserViewTransform, vtkHomogeneousTransform);
  ///@}

  ///@{
  /**
   * In addition to the instance variables such as position and orientation,
   * you can add an additional transformation for your own use. This
   * transformation is concatenated to the camera's ProjectionTransform
   */
  void SetUserTransform(vtkHomogeneousTransform* transform);
  vtkGetObjectMacro(UserTransform, vtkHomogeneousTransform);
  ///@}

  /**
   * This method causes the camera to set up whatever is required for
   * viewing the scene. This is actually handled by an subclass of
   * vtkCamera, which is created through New()
   */
  virtual void Render(vtkRenderer*) {}

  /**
   * Return the MTime that concerns recomputing the view rays of the camera.
   */
  vtkMTimeType GetViewingRaysMTime();

  /**
   * Mark that something has changed which requires the view rays
   * to be recomputed.
   */
  void ViewingRaysModified();

  /**
   * Get the plane equations that bound the view frustum.
   * The plane normals point inward. The planes array contains six
   * plane equations of the form (Ax+By+Cz+D=0), the first four
   * values are (A,B,C,D) which repeats for each of the planes.
   * The planes are given in the following order: -x,+x,-y,+y,-z,+z.
   * Warning: it means left,right,bottom,top,far,near (NOT near,far)
   * The aspect of the viewport is needed to correctly compute the planes.
   * aspect is ignored if UseExplicitAspectRatio is true.
   */
  virtual void GetFrustumPlanes(double aspect, double planes[24]);

  ///@{
  /**
   * The following methods are used to support view dependent methods
   * for normalizing data (typically point coordinates). When dealing with
   * data that can exceed floating point resolution sometimes is it best
   * to normalize that data based on the current camera view such that
   * what is seen will be in the sweet spot for floating point resolution.
   * Input datasets may be double precision but the rendering engine
   * is currently single precision which also can lead to these issues.
   * See vtkOpenGLVertexBufferObject for related information.
   */
  virtual void UpdateIdealShiftScale(double aspect);
  vtkGetVector3Macro(FocalPointShift, double);
  vtkGetMacro(FocalPointScale, double);
  vtkGetVector3Macro(NearPlaneShift, double);
  vtkGetMacro(NearPlaneScale, double);
  vtkSetMacro(ShiftScaleThreshold, double);
  vtkGetMacro(ShiftScaleThreshold, double);
  ///@}

  ///@{
  /**
   * Get the orientation of the camera.
   */
  double* GetOrientation() VTK_SIZEHINT(3);
  double* GetOrientationWXYZ() VTK_SIZEHINT(4);
  ///@}

  /**
   * This method is called automatically whenever necessary, it
   * should never be used outside of vtkCamera.cxx.
   */
  void ComputeViewPlaneNormal();

  /**
   * Returns a transformation matrix for a coordinate frame attached to
   * the camera, where the camera is located at (0, 0, 1) looking at the
   * focal point at (0, 0, 0), with up being (0, 1, 0).
   */
  vtkMatrix4x4* GetCameraLightTransformMatrix();

  /**
   * Update the viewport
   */
  virtual void UpdateViewport(vtkRenderer* vtkNotUsed(ren)) {}

  ///@{
  /**
   * Get the stereo setting
   */
  vtkGetMacro(Stereo, int);
  ///@}

  ///@{
  /**
   * Set the Left Eye setting
   */
  vtkSetMacro(LeftEye, int);
  vtkGetMacro(LeftEye, int);
  ///@}

  /**
   * Copy the properties of `source' into `this'.
   * Copy pointers of matrices.
   * \pre source_exists!=0
   * \pre not_this: source!=this
   */
  void ShallowCopy(vtkCamera* source);

  /**
   * Copy the properties of `source' into `this'.
   * Copy the contents of the matrices.
   * \pre source_exists!=0
   * \pre not_this: source!=this
   */
  void DeepCopy(vtkCamera* source);

  ///@{
  /**
   * Set/Get the value of the FreezeDolly instance variable. This
   * determines if the camera should move the focal point with the camera position.
   * HACK!!!
   */
  vtkSetMacro(FreezeFocalPoint, bool);
  vtkGetMacro(FreezeFocalPoint, bool);
  ///@}

  ///@{
  /**
   * Enable/Disable the scissor
   */
  vtkSetMacro(UseScissor, bool);
  vtkGetMacro(UseScissor, bool);
  ///@}

  ///@{
  /**
   * Set/Get the vtkRect value of the scissor
   */
  void SetScissorRect(vtkRecti scissorRect);
  void GetScissorRect(vtkRecti& scissorRect);
  ///@}

  ///@{
  /**
   * Set/Get the information object associated with this camera.
   */
  vtkGetObjectMacro(Information, vtkInformation);
  virtual void SetInformation(vtkInformation*);
  ///@}

protected:
  vtkCamera();
  ~vtkCamera() override;

  ///@{
  /**
   * These methods should only be used within vtkCamera.cxx.
   */
  void ComputeDistance();
  virtual void ComputeViewTransform();
  ///@}

  /**
   * These methods should only be used within vtkCamera.cxx.
   */
  virtual void ComputeProjectionTransform(double aspect, double nearz, double farz);

  /**
   * These methods should only be used within vtkCamera.cxx.
   */
  void ComputeCompositeProjectionTransform(double aspect, double nearz, double farz);

  void ComputeCameraLightTransform();

  /**
   * Given screen screen top, bottom left and top right
   * calculate screen orientation.
   */
  void ComputeScreenOrientationMatrix();

  /**
   * Compute and use frustum using offaxis method.
   */
  void ComputeOffAxisProjectionFrustum();

  /**
   * Compute model view matrix for the camera.
   */
  void ComputeModelViewMatrix();

  /**
   * Copy the ivars. Do nothing for the matrices.
   * Called by ShallowCopy() and DeepCopy()
   * \pre source_exists!=0
   * \pre not_this: source!=this
   */
  void PartialCopy(vtkCamera* source);

  double WindowCenter[2];
  double ObliqueAngles[2];
  double FocalPoint[3];
  double Position[3];
  double ViewUp[3];
  double ViewAngle;
  double ClippingRange[2];
  double EyeAngle;
  vtkTypeBool ParallelProjection;
  double ParallelScale;
  int Stereo;
  int LeftEye;
  double Thickness;
  double Distance;
  double DirectionOfProjection[3];
  double ViewPlaneNormal[3];
  double ViewShear[3];
  vtkTypeBool UseHorizontalViewAngle;

  vtkTypeBool UseOffAxisProjection;

  double ScreenBottomLeft[3];
  double ScreenBottomRight[3];
  double ScreenTopRight[3];
  double ScreenCenter[3];

  double OffAxisClippingAdjustment;
  double EyeSeparation;

  vtkMatrix4x4* EyeTransformMatrix;
  vtkMatrix4x4* ProjectionPlaneOrientationMatrix;

  vtkMatrix4x4* ModelTransformMatrix;

  vtkHomogeneousTransform* UserTransform;
  vtkHomogeneousTransform* UserViewTransform;

  vtkMatrix4x4* ExplicitProjectionTransformMatrix;
  bool UseExplicitProjectionTransformMatrix;

  double ExplicitAspectRatio;
  bool UseExplicitAspectRatio;

  vtkTransform* ViewTransform;
  vtkPerspectiveTransform* ProjectionTransform;
  vtkPerspectiveTransform* Transform;
  vtkTransform* CameraLightTransform;

  vtkTransform* ModelViewTransform;

  double FocalDisk;
  double FocalDistance;

  double FocalPointShift[3];
  double FocalPointScale;
  double NearPlaneShift[3];
  double NearPlaneScale;
  double ShiftScaleThreshold;

  vtkCameraCallbackCommand* UserViewTransformCallbackCommand;
  friend class vtkCameraCallbackCommand;

  // ViewingRaysMtime keeps track of camera modifications which will
  // change the calculation of viewing rays for the camera before it is
  // transformed to the camera's location and orientation.
  vtkTimeStamp ViewingRaysMTime;
  bool FreezeFocalPoint;
  bool UseScissor;

  vtkRecti ScissorRect;

  // Arbitrary extra information associated with this camera.
  vtkInformation* Information;

private:
  vtkCamera(const vtkCamera&) = delete;
  void operator=(const vtkCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
