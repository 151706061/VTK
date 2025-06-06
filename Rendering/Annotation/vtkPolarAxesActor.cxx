// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPolarAxesActor.h"

#include "vtkAxisFollower.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCoordinate.h"
#include "vtkEllipseArcSource.h"
#include "vtkFollower.h"
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

#include <numeric>
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolarAxesActor);
vtkCxxSetSmartPointerMacro(vtkPolarAxesActor, Camera, vtkCamera);
vtkCxxSetSmartPointerMacro(vtkPolarAxesActor, PolarAxisLabelTextProperty, vtkTextProperty);
vtkCxxSetSmartPointerMacro(vtkPolarAxesActor, PolarAxisTitleTextProperty, vtkTextProperty);
vtkCxxSetSmartPointerMacro(vtkPolarAxesActor, LastRadialAxisTextProperty, vtkTextProperty);
vtkCxxSetSmartPointerMacro(vtkPolarAxesActor, SecondaryRadialAxesTextProperty, vtkTextProperty);
vtkCxxSetSmartPointerMacro(vtkPolarAxesActor, LastRadialAxisProperty, vtkProperty);
vtkCxxSetSmartPointerMacro(vtkPolarAxesActor, SecondaryRadialAxesProperty, vtkProperty);

namespace
{
constexpr double VTK_MAXIMUM_RATIO = 1000.0;
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ScreenSize: " << this->ScreenSize << "\n";

  os << indent << "Pole: (" << this->Pole[0] << ", " << this->Pole[1] << ", " << this->Pole[2]
     << ")\n";

  os << indent << "Number of radial axes: " << this->NumberOfRadialAxes << endl;
  os << indent << "Number of polar axes: " << this->NumberOfPolarAxes << endl;
  os << indent << "Angle between two radial axes: " << this->DeltaAngleRadialAxes << endl;
  os << indent << "Range between two polar axes: " << this->DeltaRangePolarAxes << endl;
  os << indent << "Minimum Radius: " << this->MinimumRadius << endl;
  os << indent << "Maximum Radius: " << this->MaximumRadius << endl;
  os << indent << "Log Scale: " << (this->Log ? "On" : "Off") << endl;
  os << indent << "Ratio: " << this->Ratio << endl;
  os << indent << "Polar Arc Resolution per Degree: " << this->PolarArcResolutionPerDegree << endl;
  os << indent << "Minimum Angle: " << this->MinimumAngle << endl;
  os << indent << "Maximum Angle: " << this->MaximumAngle << endl;
  os << indent << "Smallest Visible Polar Angle: " << this->SmallestVisiblePolarAngle << endl;
  os << indent << "Radial Units (degrees): " << (this->RadialUnits ? "On\n" : "Off\n") << endl;
  os << indent << "Range: (" << this->Range[0] << ", " << this->Range[1] << ")\n";

  if (this->Camera)
  {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Camera: (none)\n";
  }

  os << indent << "EnableDistanceLOD: " << (this->EnableDistanceLOD ? "On" : "Off") << endl;
  os << indent << "DistanceLODThreshold: " << this->DistanceLODThreshold << "\n";

  os << indent << "EnableViewAngleLOD: " << (this->EnableViewAngleLOD ? "On" : "Off") << endl;
  os << indent << "ViewAngleLODThreshold: " << this->ViewAngleLODThreshold << "\n";

  os << indent << "Polar Axis Title: " << this->PolarAxisTitle << "\n";
  os << indent << "Polar Label Format: " << this->PolarLabelFormat << "\n";
  os << indent << "Polar title offset: " << this->PolarTitleOffset[0] << ", "
     << this->PolarTitleOffset[1] << "\n";
  os << indent << "Radial title offset: " << this->RadialTitleOffset[0] << ", "
     << this->RadialTitleOffset[1] << "\n";
  os << indent << "Polar label Y-offset: " << this->PolarLabelOffset << "\n";
  os << indent << "Polar exponent Y-offset: " << this->PolarExponentOffset << "\n";
  os << indent << "Radial Angle Format: " << this->RadialAngleFormat << "\n";
  os << indent << "PolarAxisLabelTextProperty: " << this->PolarAxisLabelTextProperty << endl;
  os << indent << "PolarAxisTitleTextProperty: " << this->PolarAxisTitleTextProperty << endl;
  os << indent << "RadialAxisTextProperty: " << this->LastRadialAxisTextProperty << endl;
  os << indent << "SecondaryRadialAxesTextProperty: " << this->SecondaryRadialAxesTextProperty
     << endl;
  os << indent << "Polar Axis Visibility: " << (this->PolarAxisVisibility ? "On\n" : "Off\n");
  os << indent << "Polar Title Visibility: " << (this->PolarTitleVisibility ? "On" : "Off") << endl;
  os << indent << "Polar Label Visibility: " << (this->PolarLabelVisibility ? "On" : "Off") << endl;
  if (this->PolarAxisTitleLocation == VTK_TITLE_BOTTOM)
  {
    os << indent << "Polar Title Location: BOTTOM" << endl;
  }
  else if (this->PolarAxisTitleLocation == VTK_TITLE_EXTERN)
  {
    os << indent << "Polar Title Location: EXTERN" << endl;
  }

  os << indent << "Polar Label exponent location: ";

  if (this->ExponentLocation == VTK_EXPONENT_BOTTOM)
  {
    os << " next to the polar axis title." << endl;
  }
  else if (this->ExponentLocation == VTK_EXPONENT_EXTERN)
  {
    os << " outer side." << endl;
  }
  else
  {
    os << " bound to labels." << endl;
  }

  os << indent << "Radial Axes Visibility: " << (this->RadialAxesVisibility ? "On\n" : "Off\n");
  os << indent << "Radial Title Visibility: " << (this->RadialTitleVisibility ? "On" : "Off")
     << endl;
  if (this->RadialAxisTitleLocation == VTK_TITLE_BOTTOM)
  {
    os << indent << "Radial Title Location: BOTTOM" << endl;
  }
  else if (this->RadialAxisTitleLocation == VTK_TITLE_EXTERN)
  {
    os << indent << "Radial Title Location: EXTERN" << endl;
  }

  os << indent << "Polar Arcs Visibility: " << (this->PolarArcsVisibility ? "On" : "Off") << endl;
  os << indent << "Draw Radial Gridlines: " << (this->DrawRadialGridlines ? "On" : "Off") << endl;
  os << indent << "Draw Polar Arcs Gridlines: " << (this->DrawPolarArcsGridlines ? "On" : "Off")
     << endl;
  os << indent
     << "Draw Radial Axes From Polar Axis: " << (this->RadialAxesOriginToPolarAxis ? "On" : "Off")
     << endl;

  //--------------------- TICKS ------------------
  os << indent << "TickLocation: " << this->TickLocation << endl;

  os << indent << "Ticks overall enabled: " << (this->PolarTickVisibility ? "On" : "Off") << endl;
  os << indent << "Ratio maximum radius / major tick size: " << this->TickRatioRadiusSize << endl;
  os << indent
     << "Draw Arc Ticks From Polar Axis: " << (this->ArcTicksOriginToPolarAxis ? "On" : "Off")
     << endl;

  //--- major ticks ---
  // polar axis and last radial axis
  os << indent << "Axes Major Tick Visibility: " << (this->AxisTickVisibility ? "On" : "Off")
     << endl;
  if (this->AxisTickVisibility && this->PolarTickVisibility)
  {
    os << indent
       << "Axes Major Ticks Matches Polar Axes: " << (this->AxisTickMatchesPolarAxes ? "On" : "Off")
       << endl;
    os << indent << "Axes Major Tick Step: " << this->DeltaRangeMajor << endl;
    os << indent << "PolarAxis Major Tick Size: " << this->PolarAxisMajorTickSize << endl;
    os << indent << "PolarAxis Major Tick Thickness: " << this->PolarAxisMajorTickThickness << endl;
    if (this->RadialAxesVisibility)
    {
      os << indent << "Last Radial Axis Major Ticks Size: " << this->LastRadialAxisMajorTickSize
         << endl;
      os << indent
         << "Last Radial Axis Major Ticks Thickness: " << this->LastRadialAxisMajorTickThickness
         << endl;
    }
  }

  // last arc
  os << indent << "Arc Major Ticks Visibility: " << (this->ArcTickVisibility ? "On" : "Off")
     << endl;
  if (this->ArcTickVisibility && this->PolarTickVisibility)
  {
    os << indent
       << "Arc Major Ticks Matches Radial Axes: " << (this->ArcTickMatchesRadialAxes ? "On" : "Off")
       << endl;
    os << indent << "Arc Major Angle Step: " << this->DeltaAngleMajor << endl;
    os << indent << "Arc Major Ticks Size: " << this->ArcMajorTickSize << endl;
    os << indent << "Arc Major Ticks Thickness: " << this->ArcMajorTickThickness << endl;
  }

  //--- minor ticks ---
  //  polar axis and last radial axis
  os << indent << "Axis Minor Ticks Visibility: " << (this->AxisMinorTickVisibility ? "On" : "Off")
     << endl;
  if (this->AxisMinorTickVisibility && this->PolarTickVisibility)
  {
    os << indent << "Axes Minor Tick Step: " << this->DeltaRangeMinor << endl;
    os << indent
       << "Ratio Between PolarAxis Major and Minor Tick : " << this->PolarAxisTickRatioSize << endl;
    os << indent << "Ratio Between PolarAxis Major and Minor Tick Thickness : "
       << this->PolarAxisTickRatioThickness << endl;
    if (this->RadialAxesVisibility)
    {
      os << indent
         << "Ratio Between LastAxis Major and Minor Tick : " << this->LastAxisTickRatioSize << endl;
      os << indent << "Ratio Between LastAxis Major and Minor Tick Thickness: "
         << this->LastAxisTickRatioThickness << endl;
    }
  }
  os << indent << "Arc Minor Ticks Visibility: " << (this->ArcMinorTickVisibility ? "On" : "Off")
     << endl;
  if (this->ArcMinorTickVisibility && this->PolarTickVisibility)
  {
    os << indent << "Arc Minor Angle Step: " << this->DeltaAngleMinor << endl;
    os << indent << "Ratio Between Last Arc Major and Minor Tick : " << this->ArcTickRatioSize
       << endl;
    os << indent
       << "Ratio Between Last Arc Major and Minor Tick Thickness: " << this->ArcTickRatioThickness
       << endl;
  }
}

//------------------------------------------------------------------------------
vtkPolarAxesActor::vtkPolarAxesActor()
{
  vtkMath::UninitializeBounds(this->Bounds);

  // Text properties of polar axis title and labels, with default color white
  // Properties of the radial axes, with default color black
  this->PolarAxisProperty = vtkSmartPointer<vtkProperty>::New();
  this->PolarAxisProperty->SetColor(0., 0., 0.);
  this->PolarAxisTitleTextProperty = vtkSmartPointer<vtkTextProperty>::New();
  this->PolarAxisTitleTextProperty->SetOpacity(1.0);
  this->PolarAxisTitleTextProperty->SetColor(1., 1., 1.);
  this->PolarAxisTitleTextProperty->SetFontFamilyToArial();
  this->PolarAxisLabelTextProperty = vtkSmartPointer<vtkTextProperty>::New();
  this->PolarAxisLabelTextProperty->SetColor(1., 1., 1.);
  this->PolarAxisLabelTextProperty->SetFontFamilyToArial();

  // Create and set polar axis of type X
  this->PolarAxis->SetAxisTypeToX();

  // Properties of the last radial axe, with default color black
  this->LastRadialAxisProperty = vtkSmartPointer<vtkProperty>::New();
  this->LastRadialAxisProperty->SetAmbient(1.0);
  this->LastRadialAxisProperty->SetDiffuse(0.0);
  this->LastRadialAxisProperty->SetColor(0., 0., 0.);

  this->LastRadialAxisTextProperty = vtkSmartPointer<vtkTextProperty>::New();
  this->LastRadialAxisTextProperty->SetOpacity(1.0);
  this->LastRadialAxisTextProperty->SetColor(1., 1., 1.);
  this->LastRadialAxisTextProperty->SetFontFamilyToArial();

  // Properties of the secondaries radial axes, with default color black
  this->SecondaryRadialAxesProperty = vtkSmartPointer<vtkProperty>::New();
  this->SecondaryRadialAxesProperty->SetAmbient(1.0);
  this->SecondaryRadialAxesProperty->SetDiffuse(0.0);
  this->SecondaryRadialAxesProperty->SetColor(0., 0., 0.);

  this->SecondaryRadialAxesTextProperty = vtkSmartPointer<vtkTextProperty>::New();
  this->SecondaryRadialAxesTextProperty->SetOpacity(1.0);
  this->SecondaryRadialAxesTextProperty->SetColor(1., 1., 1.);
  this->SecondaryRadialAxesTextProperty->SetFontFamilyToArial();

  // Create and set principal polar arcs and ancillary objects, with default color white
  this->PolarArcsMapper->SetInputData(this->PolarArcs);
  this->PolarArcsActor->SetMapper(this->PolarArcsMapper);
  this->PolarArcsActor->GetProperty()->SetColor(1., 1., 1.);

  // Create and set secondary polar arcs and ancillary objects, with default color white
  this->SecondaryPolarArcsMapper->SetInputData(this->SecondaryPolarArcs);
  this->SecondaryPolarArcsActor->SetMapper(this->SecondaryPolarArcsMapper);
  this->SecondaryPolarArcsActor->GetProperty()->SetColor(1., 1., 1.);

  // Create the vtk Object for arc ticks
  this->ArcTickPolyDataMapper->SetInputData(this->ArcTickPolyData);
  this->ArcMinorTickPolyDataMapper->SetInputData(this->ArcMinorTickPolyData);

  this->ArcTickActor->SetMapper(this->ArcTickPolyDataMapper);
  this->ArcMinorTickActor->SetMapper(this->ArcMinorTickPolyDataMapper);

  this->PolarLabelFormat = new char[8];
  snprintf(this->PolarLabelFormat, 8, "%s", "%-#6.3g");

  this->RadialAngleFormat = new char[8];
  snprintf(this->RadialAngleFormat, 8, "%s", "%-#3.1f");
}

//------------------------------------------------------------------------------
vtkPolarAxesActor::~vtkPolarAxesActor()
{
  this->SetCamera(nullptr);

  delete[] this->PolarLabelFormat;
  this->PolarLabelFormat = nullptr;

  delete[] this->RadialAngleFormat;
  this->RadialAngleFormat = nullptr;
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::GetRendered3DProps(vtkPropCollection* collection, bool translucent)
{
  if (this->PolarAxisVisibility)
  {
    collection->AddItem(this->PolarAxis);
  }

  if (this->RadialAxesVisibility)
  {
    for (int i = 0; i < this->NumberOfRadialAxes; ++i)
    {
      bool isInnerAxis = (i != this->NumberOfRadialAxes - 1) ||
        (vtkMathUtilities::FuzzyCompare(this->MaximumAngle, this->MinimumAngle));
      bool isAxisVisible = !isInnerAxis || this->DrawRadialGridlines;
      if (this->RadialAxesVisibility && isAxisVisible)
      {
        collection->AddItem(this->RadialAxes[i]);
      }
    }
  }

  if (this->PolarArcsVisibility && !translucent)
  {
    collection->AddItem(this->PolarArcsActor);
    collection->AddItem(this->SecondaryPolarArcsActor);
    if (this->PolarTickVisibility)
    {
      if (this->ArcTickVisibility)
      {
        collection->AddItem(this->ArcTickActor);
      }
      if (this->ArcMinorTickVisibility)
      {
        collection->AddItem(this->ArcMinorTickActor);
      }
    }
  }
}

//------------------------------------------------------------------------------
vtkTypeBool vtkPolarAxesActor::HasTranslucentPolygonalGeometry()
{
  vtkNew<vtkPropCollection> renderedProps;
  this->GetRendered3DProps(renderedProps, true);
  renderedProps->InitTraversal();
  for (int idx = 0; idx < renderedProps->GetNumberOfItems(); idx++)
  {
    vtkProp* prop = renderedProps->GetNextProp();
    if (prop->HasTranslucentPolygonalGeometry())
    {
      return 1;
    }
  }

  return Superclass::HasTranslucentPolygonalGeometry();
}

//------------------------------------------------------------------------------
int vtkPolarAxesActor::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  int numberOfRenderedProps = 0;

  vtkNew<vtkPropCollection> renderedProps;
  this->GetRendered3DProps(renderedProps, true);
  renderedProps->InitTraversal();
  for (int idx = 0; idx < renderedProps->GetNumberOfItems(); idx++)
  {
    vtkProp* prop = renderedProps->GetNextProp();
    prop->SetPropertyKeys(this->GetPropertyKeys());
    numberOfRenderedProps += prop->RenderTranslucentPolygonalGeometry(viewport);
  }

  return numberOfRenderedProps;
}

//------------------------------------------------------------------------------
int vtkPolarAxesActor::RenderOpaqueGeometry(vtkViewport* viewport)
{
  // Initialization
  int numberOfRenderedProps = 0;

  this->BuildAxes(viewport);

  vtkNew<vtkPropCollection> renderedProps;
  this->GetRendered3DProps(renderedProps, false);
  renderedProps->InitTraversal();
  for (int idx = 0; idx < renderedProps->GetNumberOfItems(); idx++)
  {
    vtkProp* prop = renderedProps->GetNextProp();
    prop->SetPropertyKeys(this->GetPropertyKeys());
    numberOfRenderedProps += prop->RenderOpaqueGeometry(viewport);
  }

  return numberOfRenderedProps;
}

//------------------------------------------------------------------------------
int vtkPolarAxesActor::RenderOverlay(vtkViewport* viewport)
{
  int numberOfRenderedProps = 0;

  if (this->PolarAxisVisibility && this->PolarAxis->GetUse2DMode())
  {
    this->PolarAxis->SetPropertyKeys(this->GetPropertyKeys());
    numberOfRenderedProps += this->PolarAxis->RenderOverlay(viewport);
  }

  if (this->RadialAxesVisibility)
  {
    for (int i = 0; i < this->NumberOfRadialAxes; ++i)
    {
      if (this->RadialAxes[i]->GetUse2DMode())
      {
        this->RadialAxes[i]->SetPropertyKeys(this->GetPropertyKeys());
        numberOfRenderedProps += this->RadialAxes[i]->RenderOverlay(viewport);
      }
    }
  }
  return numberOfRenderedProps;
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::ReleaseGraphicsResources(vtkWindow* win)
{
  this->PolarAxis->ReleaseGraphicsResources(win);
  for (int i = 0; i < this->NumberOfRadialAxes; ++i)
  {
    this->RadialAxes[i]->ReleaseGraphicsResources(win);
  }
  this->SecondaryPolarArcsActor->ReleaseGraphicsResources(win);
  this->PolarArcsActor->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::CalculateBounds()
{
  // Fetch angles, at this point it is already known that angular sector <= 360.
  double minAngle = this->MinimumAngle;
  double maxAngle = this->MaximumAngle;

  // Ensure that angles are not both < -180 nor both > 180 degrees
  if (maxAngle < -180.)
  {
    // Increment angles modulo 360 degrees
    minAngle += 360.;
    maxAngle += 360.;
  }
  else if (minAngle > 180.)
  {
    // Decrement angles modulo 360 degrees
    minAngle -= 360.;
    maxAngle -= 360.;
  }

  // Prepare trigonometric quantities
  double thetaMin = vtkMath::RadiansFromDegrees(minAngle);
  double cosThetaMin = cos(thetaMin);
  double sinThetaMin = sin(thetaMin);
  double thetaMax = vtkMath::RadiansFromDegrees(maxAngle);
  double cosThetaMax = cos(thetaMax);
  double sinThetaMax = sin(thetaMax);

  // Calculate extremal cosines across angular sector
  double minCos;
  double maxCos;
  if (minAngle * maxAngle < 0.)
  {
    // Angular sector contains null angle
    maxCos = 1.;
    if (minAngle < 180. && maxAngle > 180.)
    {
      // Angular sector also contains flat angle
      minCos = -1.;
    }
    else
    {
      // Angular sector does not contain flat angle
      minCos = cosThetaMin < cosThetaMax ? cosThetaMin : cosThetaMax;
    }
  }
  else if (minAngle < 180. && maxAngle > 180.)
  {
    // Angular sector does not contain flat angle (and not null angle)
    minCos = -1.;
    maxCos = cosThetaMax > cosThetaMin ? cosThetaMax : cosThetaMin;
  }
  else
  {
    // Angular sector does not contain flat nor null angle
    minCos = cosThetaMin < cosThetaMax ? cosThetaMin : cosThetaMax;
    maxCos = cosThetaMax > cosThetaMin ? cosThetaMax : cosThetaMin;
  }

  // Calculate extremal sines across angular sector
  double minSin;
  double maxSin;
  if (minAngle < -90. && maxAngle > -90.)
  {
    // Angular sector contains negative right angle
    minSin = -1.;
    if (minAngle < 90. && maxAngle > 90.)
    {
      // Angular sector also contains positive right angle
      maxSin = 1.;
    }
    else
    {
      // Angular sector contain does not contain positive right angle
      maxSin = sinThetaMax > sinThetaMin ? sinThetaMax : sinThetaMin;
    }
  }
  else if (minAngle < 90. && maxAngle > 90.)
  {
    // Angular sector contains positive right angle (and not negative one)
    minSin = sinThetaMin < sinThetaMax ? sinThetaMin : sinThetaMax;
    maxSin = 1.;
  }
  else
  {
    // Angular sector contain does not contain either right angle
    minSin = sinThetaMin < sinThetaMax ? sinThetaMin : sinThetaMax;
    maxSin = sinThetaMax > sinThetaMin ? sinThetaMax : sinThetaMin;
  }

  // Now calculate bounds
  // xmin
  this->Bounds[0] = this->Pole[0] + this->MaximumRadius * minCos;
  // xmax
  this->Bounds[1] = this->Pole[0] + this->MaximumRadius * maxCos;
  // ymin
  this->Bounds[2] = this->Pole[1] + this->MaximumRadius * minSin;
  // ymax
  this->Bounds[3] = this->Pole[1] + this->MaximumRadius * maxSin;
  // zmin
  this->Bounds[4] = this->Pole[2];
  // zmax
  this->Bounds[5] = this->Pole[2];

  // Update modification time of bounds
  this->BoundsMTime.Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::GetBounds(double bounds[6])
{
  for (int i = 0; i < 6; i++)
  {
    bounds[i] = this->Bounds[i];
  }
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::GetBounds(
  double& xmin, double& xmax, double& ymin, double& ymax, double& zmin, double& zmax)
{
  xmin = this->Bounds[0];
  xmax = this->Bounds[1];
  ymin = this->Bounds[2];
  ymax = this->Bounds[3];
  zmin = this->Bounds[4];
  zmax = this->Bounds[5];
}

//------------------------------------------------------------------------------
double* vtkPolarAxesActor::GetBounds()
{
  return this->Bounds;
}

bool vtkPolarAxesActor::CheckMembersConsistency()
{
  if (this->MaximumAngle > 360.0 || this->MinimumAngle > 360.0)
  {
    // Incorrect MaximumRadius input
    vtkWarningMacro(<< "Cannot draw polar axis, Angle > 360.0: "
                    << "MinimumAngle : " << this->MinimumAngle
                    << " _ MaximumAngle: " << this->MaximumAngle);
    return false;
  }

  // Min/Max Radius
  if (vtkMathUtilities::FuzzyCompare(this->MaximumRadius, this->MinimumRadius))
  {
    // MaximumRadius and this->MinimumRadius are too close
    vtkWarningMacro(<< "Maximum and Minimum Radius cannot be distinct: "
                    << " MinimumRadius: " << this->MinimumRadius
                    << " _ MaximumRadius: " << this->MaximumRadius);
    return false;
  }

  if (this->MaximumRadius <= 0.0 || this->MinimumRadius < 0.0)
  {
    // Incorrect MaximumRadius input
    vtkWarningMacro(<< "Cannot draw polar axis, Negative Radius value set: "
                    << "MinimumRadius : " << this->MinimumRadius
                    << " _ MaximumRadius: " << this->MaximumRadius);
    return false;
  }

  if (this->MaximumRadius < this->MinimumRadius)
  {
    // MaximumRadius should not be lower than MinimumRadius
    vtkWarningMacro(<< "Maximum Radius cannot be lower than Minimum one: "
                    << "MinimumRadius : " << this->MinimumRadius
                    << " _ MaximumRadius: " << this->MaximumRadius);
    return false;
  }

  // Min/Max Range
  if (vtkMathUtilities::FuzzyCompare(this->Range[0], this->Range[1]))
  {
    // MaximumRadius and this->MinimumRadius are too close
    vtkWarningMacro(<< "Maximum and Minimum Range cannot be distinct: "
                    << " Range[0]: " << this->Range[0] << " _ Range[1]: " << this->Range[1]);
    return false;
  }

  if (this->Range[1] < this->Range[0])
  {
    // Range bounds should respect ascending order
    vtkWarningMacro(<< "Maximum range bound cannot be lower than Minimum one: "
                    << "Range[0] : " << this->Range[0] << " _ Range[1]: " << this->Range[1]);
    return false;
  }

  // Log Mode
  if (this->Log != 0 && this->Range[0] <= 0.0)
  {
    vtkWarningMacro(<< "Scale Set to Linear. Range value undefined for log scale enabled. "
                    << "Current Range: (" << this->Range[0] << ", " << this->Range[1] << ")"
                    << "Range must be > 0.0 for log scale to be enabled"
                    << ".");

    this->Log = false;
  }

  // Range Step
  if (this->RequestedNumberOfPolarAxes == 0 && this->RequestedDeltaRangePolarAxes == 0.0)
  {
    vtkWarningMacro(<< "Either NumberOfPolarAxes or DeltaRangePolarAxes must be set. "
                    << "Both values equal 0: can't perform automatic computation.");
    return false;
  }

  if (!this->AxisTickMatchesPolarAxes &&
    (this->DeltaRangeMajor <= 0.0 || this->DeltaRangeMajor > fabs(this->Range[1] - this->Range[0])))
  {
    vtkWarningMacro(<< "Axis Major Step invalid or range length invalid: "
                    << "DeltaRangeMajor: " << this->DeltaRangeMajor
                    << "_ Range length: " << fabs(this->Range[1] - this->Range[0]));
    return false;
  }
  if (!this->AxisTickMatchesPolarAxes &&
    (this->DeltaRangeMinor <= 0.0 || this->DeltaRangeMinor > fabs(this->Range[1] - this->Range[0])))
  {
    vtkWarningMacro(<< "Axis Minor Step or range length invalid: "
                    << "DeltaRangeMinor: " << this->DeltaRangeMinor
                    << "_ Range length: " << fabs(this->Range[1] - this->Range[0]));
    return false;
  }

  // Requested angle/number of radial axes
  if (this->RequestedNumberOfRadialAxes == 0 && this->RequestedDeltaAngleRadialAxes == 0.0)
  {
    vtkWarningMacro(<< "Either NumberOfRadialAxes or DeltaAngleRadialAxes must be set. "
                    << "Both values equal 0: can't perform automatic computation.");
    return false;
  }

  // Angle Step
  if (!this->ArcTickMatchesRadialAxes &&
    (this->DeltaAngleMajor <= 0.0 || this->DeltaAngleMajor >= 360.0 ||
      this->DeltaAngleMinor <= 0.0 || this->DeltaAngleMinor >= 360.0))
  {
    vtkWarningMacro(<< "Arc Delta Angle: "
                    << "DeltaAngleMajor: " << this->DeltaAngleMajor << " _ DeltaAngleMinor: "
                    << this->DeltaAngleMinor << "_ DeltaAngles should be in ]0.0, 360.0[ range. ");
    return false;
  }

  // Tick ratios range check
  if (this->PolarAxisTickRatioThickness < (1.0 / VTK_MAXIMUM_RATIO) ||
    this->PolarAxisTickRatioThickness > VTK_MAXIMUM_RATIO ||
    this->LastAxisTickRatioThickness < (1.0 / VTK_MAXIMUM_RATIO) ||
    this->LastAxisTickRatioThickness > VTK_MAXIMUM_RATIO ||
    this->ArcTickRatioThickness < (1.0 / VTK_MAXIMUM_RATIO) ||
    this->ArcTickRatioThickness > VTK_MAXIMUM_RATIO ||
    this->PolarAxisTickRatioSize < (1.0 / VTK_MAXIMUM_RATIO) ||
    this->PolarAxisTickRatioSize > VTK_MAXIMUM_RATIO ||
    this->LastAxisTickRatioSize < (1.0 / VTK_MAXIMUM_RATIO) ||
    this->LastAxisTickRatioSize > VTK_MAXIMUM_RATIO ||
    this->ArcTickRatioSize < (1.0 / VTK_MAXIMUM_RATIO) ||
    this->ArcTickRatioSize > VTK_MAXIMUM_RATIO ||
    this->TickRatioRadiusSize < (1.0 / VTK_MAXIMUM_RATIO) ||
    this->TickRatioRadiusSize > VTK_MAXIMUM_RATIO)
  {
    // clang-format off
    vtkWarningMacro(
      << "A size/thickness ratio between major and minor ticks is way too large/thin: "
      << "PolarAxisTickRatioThickness: " << this->PolarAxisTickRatioThickness << "\n"
      << "LastAxisTickRatioThickness: " << this->LastAxisTickRatioThickness << "\n"
      << "ArcTickRatioThickness: " << this->ArcTickRatioThickness << "\n"
      << "PolarAxisTickRatioSize: " << this->PolarAxisTickRatioSize << "\n"
      << "LastAxisTickRatioSize: " << this->LastAxisTickRatioSize << "\n"
      << "ArcTickRatioSize: " << this->ArcTickRatioSize << "\n"
      << "TickRatioRadiusSize: " << this->TickRatioRadiusSize);
    // clang-format on
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::BuildAxes(vtkViewport* viewport)
{
  if (!this->Camera)
  {
    vtkWarningMacro("vtkPolarAxesActor requires a Camera to be built.");
    return;
  }

  if (this->GetMTime() < this->BuildTime.GetMTime())
  {
    this->AutoScale(viewport);
    return;
  }

  // ---------- Angles check -----------
  // set angle range [0.0; 360.0]
  this->MaximumAngle = std::fmod(this->MaximumAngle, 360);
  this->MinimumAngle = std::fmod(this->MinimumAngle, 360);

  if (this->MaximumAngle < 0.0)
  {
    this->MaximumAngle += 360.0;
  }

  // set angle range [0.0; 360.0]
  if (this->MinimumAngle < 0.0)
  {
    this->MinimumAngle += 360.0;
  }

  // this->MaximumAngle < this->MinimumAngle is possible, no swap

  if (!this->CheckMembersConsistency())
  {
    return;
  }

  // Determine the bounds
  this->CalculateBounds();

  // Set polar axis endpoints
  vtkAxisActor* axis = this->PolarAxis;

  // compute ellipse angle
  double miniAngleEllipse = vtkPolarAxesActor::ComputeEllipseAngle(this->MinimumAngle, this->Ratio);

  // Set the start point and end point (world coord system) of the Polar Axis.
  double startPt[3], endPt[3];
  startPt[0] = this->Pole[0] + this->MinimumRadius * cos(miniAngleEllipse);
  startPt[1] = this->Pole[1] + this->MinimumRadius * this->Ratio * sin(miniAngleEllipse);
  startPt[2] = this->Pole[2];

  endPt[0] = this->Pole[0] + this->MaximumRadius * cos(miniAngleEllipse);
  endPt[1] = this->Pole[1] + this->MaximumRadius * this->Ratio * sin(miniAngleEllipse);
  endPt[2] = this->Pole[2];

  axis->GetPoint1Coordinate()->SetValue(startPt);
  axis->GetPoint2Coordinate()->SetValue(endPt);

  // axis Type. We assume the polar graph is built in the local plane x-y
  if ((this->MinimumAngle > 45.0 && this->MinimumAngle < 135.0) ||
    (this->MinimumAngle > 225.0 && this->MinimumAngle < 315.0))
  {
    axis->SetAxisTypeToY();
  }
  else
  {
    axis->SetAxisTypeToX();
  }

  // Set axess attributes (range, tick location)
  this->SetCommonAxisAttributes(axis);
  this->SetPolarAxisAttributes(axis);

  // ------- Ticks thickness -------

  // Polar Axis
  this->PolarAxis->GetAxisMajorTicksProperty()->SetLineWidth(this->PolarAxisMajorTickThickness);
  double minorThickness = this->PolarAxisTickRatioThickness * this->PolarAxisMajorTickThickness;
  if (minorThickness < 1.0)
  {
    minorThickness = 1.0;
  }
  this->PolarAxis->GetAxisMinorTicksProperty()->SetLineWidth(minorThickness);

  // Last arc
  this->ArcTickActor->GetProperty()->SetLineWidth(this->ArcMajorTickThickness);
  minorThickness = std::max(this->ArcMajorTickThickness * this->ArcTickRatioThickness, 1.);

  this->ArcMinorTickActor->GetProperty()->SetLineWidth(minorThickness);

  // last polar axis line width is set in BuildRadialAxes() function

  // Build polar axis ticks
  if (this->Log)
  {
    this->BuildLabelsLog();
    this->BuildPolarArcsLog();
  }
  else
  {
    // Build polar axis labels
    this->BuildPolarAxisLabelsArcs();
  }

  // Set title relative location from the axis
  if (this->PolarAxisTitleLocation == VTK_TITLE_BOTTOM)
  {
    this->PolarAxis->SetTitleAlignLocation(vtkAxisActor::VTK_ALIGN_BOTTOM);
  }
  else
  {
    this->PolarAxis->SetTitleAlignLocation(vtkAxisActor::VTK_ALIGN_POINT2);
  }

  // Build radial axes
  this->BuildRadialAxes(viewport);

  // Build ticks located on the last arc
  if (this->PolarTickVisibility)
  {
    this->BuildArcTicks();
  }

  // color copy
  vtkProperty* prop = this->PolarArcsActor->GetProperty();
  double color[3];
  prop->GetColor(color);
  this->ArcTickActor->GetProperty()->SetColor(color);
  this->ArcMinorTickActor->GetProperty()->SetColor(color);

  // Update axis title follower
  vtkAxisFollower* follower = axis->GetTitleActor();

  follower->SetAxis(axis);
  follower->SetEnableDistanceLOD(this->EnableDistanceLOD);
  follower->SetDistanceLODThreshold(this->DistanceLODThreshold);
  follower->SetEnableViewAngleLOD(this->EnableViewAngleLOD);
  follower->SetViewAngleLODThreshold(this->ViewAngleLODThreshold);

  // Update axis title follower
  vtkAxisFollower* expFollower = this->PolarAxis->GetExponentActor();
  expFollower->SetAxis(this->PolarAxis);
  expFollower->SetEnableDistanceLOD(this->EnableDistanceLOD);
  expFollower->SetDistanceLODThreshold(this->DistanceLODThreshold);
  expFollower->SetEnableViewAngleLOD(this->EnableViewAngleLOD);
  expFollower->SetViewAngleLODThreshold(this->ViewAngleLODThreshold);

  // Update axis label followers
  int numberOfLabels = axis->GetNumberOfLabelsBuilt();
  for (int i = 0; i < numberOfLabels; ++i)
  {
    vtkAxisFollower* labelActor = axis->GetLabelFollower(i);
    labelActor->SetAxis(axis);
    labelActor->SetEnableDistanceLOD(this->EnableDistanceLOD);
    labelActor->SetDistanceLODThreshold(this->DistanceLODThreshold);
    labelActor->SetEnableViewAngleLOD(this->EnableViewAngleLOD);
    labelActor->SetViewAngleLODThreshold(this->ViewAngleLODThreshold);
  }

  // Build polar axis
  this->PolarAxis->BuildAxis(viewport, true);

  // Scale appropriately
  this->AutoScale(viewport);

  this->BuildTime.Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetCommonAxisAttributes(vtkAxisActor* axis)
{
  vtkProperty* prop = this->GetProperty();
  prop->SetAmbient(1.0);
  prop->SetDiffuse(0.0);
  axis->SetProperty(prop);

  axis->SetScreenSize(this->ScreenSize);

  // Common space and range attributes
  axis->SetCamera(this->Camera);
  axis->SetBounds(this->Bounds);

  // User defined range
  axis->SetRange(this->Range[0], this->Range[1]);

  // Axis scale type
  axis->SetLog(this->Log);

  // Major and minor ticks draw begins at Range[0]
  axis->SetMajorRangeStart(axis->GetRange()[0]);
  axis->SetMinorRangeStart(axis->GetRange()[0]);

  // Set polar axis ticks
  axis->SetTickVisibility(this->AxisTickVisibility && this->PolarTickVisibility);

  // Set polar axis minor ticks
  axis->SetMinorTicksVisible(this->AxisMinorTickVisibility && this->PolarTickVisibility);

  axis->SetTickLocation(this->TickLocation);
}

void vtkPolarAxesActor::SetPolarAxisAttributes(vtkAxisActor* axis)
{
  // Set polar axis lines
  axis->SetAxisVisibility(this->PolarAxisVisibility);

  // #### Warning #### : Set this property BEFORE apply the ticks thickness of the vtkAxisActor
  // instances
  axis->SetAxisLinesProperty(this->PolarAxisProperty);

  // Set polar axis title
  axis->SetTitleVisibility(this->PolarTitleVisibility);
  axis->SetTitle(this->PolarAxisTitle);
  axis->SetTitleTextProperty(this->PolarAxisTitleTextProperty);
  axis->SetTitleOffset(this->PolarTitleOffset);

  // Set Labels exponent value
  axis->SetExponentOffset(this->PolarExponentOffset);
  if (this->ExponentLocation == VTK_EXPONENT_BOTTOM)
  {
    axis->SetExponentLocation(vtkAxisActor::VTK_ALIGN_BOTTOM);
    axis->SetExponentVisibility(true);
  }
  else if (this->ExponentLocation == VTK_EXPONENT_EXTERN)
  {
    axis->SetExponentLocation(vtkAxisActor::VTK_ALIGN_POINT2);
    axis->SetExponentVisibility(true);
  }
  else
  {
    axis->SetExponentVisibility(false);
  }

  // Set polar axis labels
  axis->SetLabelVisibility(this->PolarLabelVisibility);
  axis->SetLabelTextProperty(this->PolarAxisLabelTextProperty);
  axis->SetLabelOffset(this->PolarLabelOffset);

  double tickSize = this->PolarAxisMajorTickSize == 0.0
    ? this->TickRatioRadiusSize * this->MaximumRadius
    : this->PolarAxisMajorTickSize;

  axis->SetMajorTickSize(tickSize);
  axis->SetMinorTickSize(this->PolarAxisTickRatioSize * tickSize);
}

//------------------------------------------------------------------------------
double vtkPolarAxesActor::FFix(double value)
{
  int ivalue = static_cast<int>(value);
  return ivalue;
}

//------------------------------------------------------------------------------
double vtkPolarAxesActor::FSign(double value, double sign)
{
  value = fabs(value);
  if (sign < 0.)
  {
    value *= -1.;
  }
  return value;
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::CreateRadialAxes(int axisCount)
{
  // If number of radial axes does not change, do nothing
  if (this->NumberOfRadialAxes == axisCount)
  {
    return;
  }

  this->RadialAxes.clear();

  this->NumberOfRadialAxes = axisCount;

  // Create requested number of secondary radial axes
  this->RadialAxes.resize(this->NumberOfRadialAxes);
  for (int i = 0; i < this->NumberOfRadialAxes; ++i)
  {
    // Create axis of type X
    this->RadialAxes[i] = vtkSmartPointer<vtkAxisActor>::New();
    vtkAxisActor* axis = this->RadialAxes[i].Get();
    axis->SetAxisTypeToX();
    axis->SetLabelVisibility(false);
    axis->SetUse2DMode(this->PolarAxis->GetUse2DMode());
    axis->SetUseTextActor3D(this->PolarAxis->GetUseTextActor3D());
    axis->LastMajorTickPointCorrectionOn();
  }
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::BuildRadialAxes(vtkViewport* viewport)
{
  bool originToPolarAxis = this->RadialAxesOriginToPolarAxis != 0.0;

  // set MaximumAngle and MinimumAngle range: [0.0; 360.0]
  double angleSection = (this->MaximumAngle > this->MinimumAngle)
    ? this->MaximumAngle - this->MinimumAngle
    : 360.0 - fabs(this->MaximumAngle - this->MinimumAngle);

  if (vtkMathUtilities::FuzzyCompare(this->MaximumAngle, this->MinimumAngle) ||
    angleSection == 360.0)
  {
    angleSection = 360.0;
  }

  // Update delta angle of radial axes
  if (this->RequestedDeltaAngleRadialAxes > 0.0)
  {
    if (this->DeltaAngleRadialAxes != this->RequestedDeltaAngleRadialAxes)
    {
      this->DeltaAngleRadialAxes = this->RequestedDeltaAngleRadialAxes;
    }
  }
  else if (this->RequestedNumberOfRadialAxes > 1)
  {
    this->ComputeDeltaAngleRadialAxes(this->RequestedNumberOfRadialAxes);
  }

  bool positiveSection = false;
  double dAlpha = this->DeltaAngleRadialAxes;
  double alphaDeg, currentAlpha;

  // current ellipse angle
  double actualAngle;
  int i = 0;

  double minorThickness;

  double alphaStart = (originToPolarAxis)
    ? this->MinimumAngle + dAlpha
    : std::floor(this->MinimumAngle / dAlpha) * dAlpha + dAlpha;

  int nAxes;

  // Delta angle to big, only last radial axis
  if (this->DeltaAngleRadialAxes >= angleSection)
  {
    nAxes = 1;
    alphaStart = angleSection + this->MinimumAngle;
  }
  else if (this->RequestedNumberOfRadialAxes == 0)
  {
    nAxes = std::ceil(angleSection / dAlpha);
  }
  else
  {
    nAxes = std::min(
      this->RequestedNumberOfRadialAxes - 1, static_cast<int>(std::ceil(angleSection / dAlpha)));
  }

  // init radial axis. Does nothing if number of radial axes doesn't change
  this->CreateRadialAxes(nAxes);

  char titleValue[64];
  for (alphaDeg = alphaStart; i < this->NumberOfRadialAxes; alphaDeg += dAlpha, ++i)
  {
    const bool isLastAxis = i == this->NumberOfRadialAxes - 1;
    currentAlpha = alphaDeg;

    if (isLastAxis)
    {
      currentAlpha = angleSection + this->MinimumAngle;
    }

    // Calculate startpoint coordinates
    double thetaEllipse = vtkPolarAxesActor::ComputeEllipseAngle(currentAlpha, this->Ratio);
    double xStart = this->Pole[0] + this->MinimumRadius * cos(thetaEllipse);
    double yStart = this->Pole[1] + this->MinimumRadius * this->Ratio * sin(thetaEllipse);

    // Calculate endpoint coordinates
    double xEnd = this->Pole[0] + this->MaximumRadius * cos(thetaEllipse);
    double yEnd = this->Pole[1] + this->MaximumRadius * this->Ratio * sin(thetaEllipse);

    // radius angle (different from angle used to compute ellipse point)
    actualAngle = vtkMath::DegreesFromRadians(atan2(yEnd - this->Pole[1], xEnd - this->Pole[0]));

    // to keep angle positive for the last ones
    if (actualAngle > 0.0 || this->MinimumAngle < 180.0)
    {
      positiveSection = true;
    }

    if (actualAngle < 0.0 && positiveSection)
    {
      actualAngle += 360.0;
    }

    // Set radial axis endpoints
    vtkAxisActor* axis = this->RadialAxes[i];

    // The last arc has its own property
    if (isLastAxis)
    {
      axis->SetAxisLinesProperty(this->LastRadialAxisProperty);
      axis->SetTitleTextProperty(this->LastRadialAxisTextProperty);
    }
    else
    {
      axis->SetAxisLinesProperty(this->SecondaryRadialAxesProperty);
      axis->SetTitleTextProperty(this->SecondaryRadialAxesTextProperty);
    }

    axis->GetPoint1Coordinate()->SetValue(xStart, yStart, this->Pole[2]);
    axis->GetPoint2Coordinate()->SetValue(xEnd, yEnd, this->Pole[2]);

    // set the range steps
    axis->SetDeltaRangeMajor(this->PolarAxis->GetDeltaRangeMajor());
    axis->SetDeltaRangeMinor(this->PolarAxis->GetDeltaRangeMinor());

    // Set common axis attributes
    this->SetCommonAxisAttributes(axis);

    // Set radial axis lines
    axis->SetAxisVisibility(this->RadialAxesVisibility);

    // Set radial axis title offset
    axis->SetTitleOffset(this->RadialTitleOffset);

    // Set title relative location from the axis
    if (this->RadialAxisTitleLocation == VTK_TITLE_BOTTOM)
    {
      axis->SetTitleAlignLocation(vtkAxisActor::VTK_ALIGN_BOTTOM);
    }
    else
    {
      axis->SetTitleAlignLocation(vtkAxisActor::VTK_ALIGN_POINT2);
    }

    // Set radial axis title with polar angle as title for non-polar axes
    if (this->PolarAxisVisibility && fabs(alphaDeg) < 2.)
    {
      // Prevent conflict between radial and polar axes titles
      axis->SetTitleVisibility(false);

      if (fabs(alphaDeg) < this->SmallestVisiblePolarAngle)
      {
        // Do not show radial axes too close to polar axis
        axis->SetAxisVisibility(false);
      }
    }
    else
    {
      // Use polar angle as a title for the radial axis
      axis->SetTitleVisibility(this->RadialTitleVisibility);
      std::ostringstream title;
      title.setf(std::ios::fixed, std::ios::floatfield);
      snprintf(titleValue, sizeof(titleValue), this->RadialAngleFormat, actualAngle);
      title << titleValue << (this->RadialUnits ? " deg" : "");
      axis->SetTitle(title.str());

      // Update axis title followers
      axis->GetTitleActor()->SetAxis(axis);
      axis->GetTitleActor()->SetEnableDistanceLOD(this->EnableDistanceLOD);
      axis->GetTitleActor()->SetDistanceLODThreshold(this->DistanceLODThreshold);
      axis->GetTitleActor()->SetEnableViewAngleLOD(this->EnableViewAngleLOD);
      axis->GetTitleActor()->SetViewAngleLODThreshold(this->ViewAngleLODThreshold);
    }

    // Ticks for the last radial axis
    if (angleSection != 360.0 && i == this->NumberOfRadialAxes - 1)
    {
      // axis Type. We assume the polar graph is built in the local plane x-y
      if ((actualAngle > 45.0 && actualAngle < 135.0) ||
        (actualAngle > 225.0 && actualAngle < 315.0))
      {
        axis->SetAxisTypeToY();
      }
      else
        axis->SetAxisTypeToX();

      // Set polar axis ticks
      double tickSize = this->LastRadialAxisMajorTickSize == 0.0
        ? this->TickRatioRadiusSize * this->MaximumRadius
        : this->LastRadialAxisMajorTickSize;

      axis->SetTickVisibility(this->AxisTickVisibility && this->PolarTickVisibility);
      axis->SetMajorTickSize(tickSize);

      // Set polar axis minor ticks
      axis->SetMinorTicksVisible(this->AxisMinorTickVisibility && this->PolarTickVisibility);
      axis->SetMinorTickSize(this->LastAxisTickRatioSize * tickSize);

      // Set the tick orientation
      axis->SetTickLocation(this->TickLocation);

      axis->GetAxisMajorTicksProperty()->SetLineWidth(this->LastRadialAxisMajorTickThickness);
      minorThickness = this->LastRadialAxisMajorTickThickness * LastAxisTickRatioThickness;
      if (minorThickness < 1.0)
      {
        minorThickness = 1.0;
      }
      axis->GetAxisMinorTicksProperty()->SetLineWidth(minorThickness);
    }
    else
    {
      axis->SetLabelVisibility(false);
      axis->SetTickVisibility(false);
    }

    if (viewport)
    {
      // Build to make sure properties are immediately set
      axis->BuildAxis(viewport, true);
    }
  }
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::BuildArcTicks()
{
  bool originToPolarAxis = this->ArcTicksOriginToPolarAxis != 0.0;

  // set MaximumAngle and MinimumAngle range: [0.0; 360.0]
  double angleSection = (this->MaximumAngle > this->MinimumAngle)
    ? this->MaximumAngle - this->MinimumAngle
    : 360.0 - fabs(this->MaximumAngle - this->MinimumAngle);

  if (vtkMathUtilities::FuzzyCompare(this->MaximumAngle, this->MinimumAngle) ||
    angleSection == 360.0)
  {
    angleSection = 360.0;
  }

  // Clear Tick Points
  this->ArcMajorTickPts->Reset();
  this->ArcMinorTickPts->Reset();

  // Arc tick actual size
  double tickSize = this->ArcMajorTickSize == 0.0 ? this->TickRatioRadiusSize * this->MaximumRadius
                                                  : this->ArcMajorTickSize;

  double dAlpha =
    this->ArcTickMatchesRadialAxes ? this->DeltaAngleRadialAxes : this->DeltaAngleMajor;
  double alphaStart;
  alphaStart = (originToPolarAxis) ? this->MinimumAngle + dAlpha
                                   : std::floor(this->MinimumAngle / dAlpha) * dAlpha + dAlpha;
  for (double alphaDeg = alphaStart; alphaDeg < (angleSection + this->MinimumAngle);
       alphaDeg += dAlpha)
  {
    double thetaEllipse = ComputeEllipseAngle(alphaDeg, this->Ratio);
    this->StoreTicksPtsFromParamEllipse(
      this->MaximumRadius, thetaEllipse, tickSize, this->ArcMajorTickPts);
  }

  // Copy/paste should be replaced with a python-like generator to provide parameters to
  // StoreTicksPtsFromParamEllipse()
  // without running twice through the ellipse

  dAlpha =
    this->ArcTickMatchesRadialAxes ? this->DeltaAngleRadialAxes / 2.0 : this->DeltaAngleMinor;
  alphaStart = (originToPolarAxis) ? this->MinimumAngle + dAlpha
                                   : std::floor(this->MinimumAngle / dAlpha) * dAlpha + dAlpha;
  for (double alphaDeg = alphaStart; alphaDeg < (angleSection + this->MinimumAngle);
       alphaDeg += dAlpha)
  {
    double thetaEllipse = ComputeEllipseAngle(alphaDeg, this->Ratio);
    this->StoreTicksPtsFromParamEllipse(
      this->MaximumRadius, thetaEllipse, this->ArcTickRatioSize * tickSize, this->ArcMinorTickPts);
  }

  // set vtk object to draw the ticks
  vtkNew<vtkPoints> majorPts;
  vtkNew<vtkPoints> minorPts;
  vtkNew<vtkCellArray> majorLines;
  vtkNew<vtkCellArray> minorLines;
  vtkIdType ptIds[2];
  int numTickPts, numLines, i;
  this->ArcTickPolyData->SetPoints(majorPts);
  this->ArcTickPolyData->SetLines(majorLines);
  this->ArcMinorTickPolyData->SetPoints(minorPts);
  this->ArcMinorTickPolyData->SetLines(minorLines);

  if (this->ArcTickVisibility)
  {
    numTickPts = this->ArcMajorTickPts->GetNumberOfPoints();
    for (i = 0; i < numTickPts; i++)
    {
      majorPts->InsertNextPoint(this->ArcMajorTickPts->GetPoint(i));
    }
  }
  if (this->ArcMinorTickVisibility)
  {
    // In 2D mode, the minorTickPts for yz portion or xz portion have been removed.
    numTickPts = this->ArcMinorTickPts->GetNumberOfPoints();
    for (i = 0; i < numTickPts; i++)
    {
      minorPts->InsertNextPoint(this->ArcMinorTickPts->GetPoint(i));
    }
  }

  // create lines
  if (this->ArcTickVisibility)
  {
    numLines = majorPts->GetNumberOfPoints() / 2;
    for (i = 0; i < numLines; i++)
    {
      ptIds[0] = 2 * i;
      ptIds[1] = 2 * i + 1;
      majorLines->InsertNextCell(2, ptIds);
    }
  }
  if (this->ArcMinorTickVisibility)
  {
    numLines = minorPts->GetNumberOfPoints() / 2;
    for (i = 0; i < numLines; i++)
    {
      ptIds[0] = 2 * i;
      ptIds[1] = 2 * i + 1;
      minorLines->InsertNextCell(2, ptIds);
    }
  }
}

void vtkPolarAxesActor::StoreTicksPtsFromParamEllipse(
  double a, double angleEllipseRad, double tickSize, vtkPoints* tickPts)
{
  // plane point: point located in the plane of the ellipse
  // normal Dir Point: point located according to the direction of the z vector

  // inside direction: direction from the arc to its center for plane points, and positive z
  // direction
  // outside direction: direction from the arc to the outer radial direction for plane points, and
  // negative z direction

  int i;
  double planeInPt[3], planeOutPt[3], normalDirPt[3], invNormalDirPt[3];

  if (!tickPts)
  {
    return;
  }

  double b = a * this->Ratio;
  double xArc = this->Pole[0] + a * cos(angleEllipseRad);
  double yArc = this->Pole[1] + b * sin(angleEllipseRad);
  double ellipsePt[3] = { xArc, yArc, this->Pole[2] };

  double deltaVector[3] = { a * cos(angleEllipseRad), b * sin(angleEllipseRad), 0.0 };
  vtkMath::Normalize(deltaVector);

  double orthoVector[3] = { 0.0, 0.0, 1.0 };

  // init
  for (i = 0; i < 3; i++)
  {
    planeInPt[i] = planeOutPt[i] = normalDirPt[i] = invNormalDirPt[i] = ellipsePt[i];
  }

  if (this->TickLocation == vtkAxisActor::VTK_TICKS_INSIDE ||
    this->TickLocation == vtkAxisActor::VTK_TICKS_BOTH)
  {
    for (i = 0; i < 3; i++)
    {
      planeInPt[i] = ellipsePt[i] - tickSize * deltaVector[i];
    }

    for (i = 0; i < 3; i++)
    {
      normalDirPt[i] = ellipsePt[i] + tickSize * orthoVector[i];
    }
  }

  if (this->TickLocation == vtkAxisActor::VTK_TICKS_OUTSIDE ||
    this->TickLocation == vtkAxisActor::VTK_TICKS_BOTH)
  {
    for (i = 0; i < 3; i++)
    {
      planeOutPt[i] = ellipsePt[i] + tickSize * deltaVector[i];
    }

    for (i = 0; i < 3; i++)
    {
      invNormalDirPt[i] = ellipsePt[i] - tickSize * orthoVector[i];
    }
  }

  vtkIdType nPoints = tickPts->GetNumberOfPoints();
  tickPts->Resize(nPoints + 4);
  tickPts->SetNumberOfPoints(nPoints + 4);
  tickPts->SetPoint(nPoints, planeInPt);
  tickPts->SetPoint(nPoints + 1, planeOutPt);
  tickPts->SetPoint(nPoints + 2, normalDirPt);
  tickPts->SetPoint(nPoints + 3, invNormalDirPt);
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::BuildPolarAxisLabelsArcs()
{
  double angleSection = (this->MaximumAngle > this->MinimumAngle)
    ? this->MaximumAngle - this->MinimumAngle
    : 360.0 - fabs(this->MaximumAngle - this->MinimumAngle);

  // if Min and max angle are the same, interpret it as 360 segment opening
  if (vtkMathUtilities::FuzzyCompare(this->MaximumAngle, this->MinimumAngle))
  {
    angleSection = 360.0;
  }

  // Prepare trigonometric quantities
  vtkIdType arcResolution =
    static_cast<vtkIdType>(angleSection * this->PolarArcResolutionPerDegree * this->Ratio);

  // Principal Arc points
  vtkNew<vtkPoints> polarArcsPoints;
  this->PolarArcs->SetPoints(polarArcsPoints);

  // Principal Arc lines
  vtkNew<vtkCellArray> polarArcsLines;
  this->PolarArcs->SetLines(polarArcsLines);

  // Secondary Arc points
  vtkNew<vtkPoints> secondaryPolarArcsPoints;
  this->SecondaryPolarArcs->SetPoints(secondaryPolarArcsPoints);

  // Secondary Arc lines
  vtkNew<vtkCellArray> secondaryPolarArcsLines;
  this->SecondaryPolarArcs->SetLines(secondaryPolarArcsLines);

  vtkAxisActor* axis = this->PolarAxis;

  // Base ellipse arc value, refers to world coordinate system
  double axisLength = this->MaximumRadius - this->MinimumRadius;
  double rangeLength = axis->GetRange()[1] - axis->GetRange()[0];
  double rangeScale = axisLength / rangeLength;

  // Update delta range of polar axes
  if (this->RequestedDeltaRangePolarAxes > 0.0)
  {
    if (this->DeltaRangePolarAxes != this->RequestedDeltaRangePolarAxes)
    {
      this->DeltaRangePolarAxes = this->RequestedDeltaRangePolarAxes;
    }
  }
  else if (this->RequestedNumberOfPolarAxes > 1)
  {
    this->ComputeDeltaRangePolarAxes(this->RequestedNumberOfPolarAxes);
  }

  int nAxes;
  // If range too big, only first and last arcs
  if (this->DeltaRangePolarAxes >= rangeLength)
  {
    nAxes = 2;
  }
  else if (this->RequestedNumberOfPolarAxes == 0)
  {
    nAxes = std::ceil(rangeLength / this->DeltaRangePolarAxes) + 1;
  }
  else
  {
    nAxes = std::min(this->RequestedNumberOfPolarAxes,
      static_cast<int>(std::ceil(rangeLength / this->DeltaRangePolarAxes)) + 1);
  }

  if (this->NumberOfPolarAxes != nAxes)
  {
    this->NumberOfPolarAxes = nAxes;
  }

  // Label values refers to range values
  double valueRange = axis->GetRange()[0];
  double deltaRange = this->DeltaRangePolarAxes;
  double deltaArc;

  vtkIdType pointIdOffset = 0;
  bool isOuterArc, isArcVisible, isLastArc;

  for (int i = 0; i < this->NumberOfPolarAxes; ++i)
  {
    deltaArc = (valueRange - axis->GetRange()[0]) * rangeScale;

    isLastArc = i == this->NumberOfPolarAxes - 1;
    isOuterArc = i == 0 || isLastArc;
    isArcVisible = isOuterArc || this->DrawPolarArcsGridlines;

    // Build polar arcs for non-zero values
    if (deltaArc + this->MinimumRadius > 0. && isArcVisible)
    {
      // Create elliptical polar arc with corresponding to this tick mark
      vtkNew<vtkEllipseArcSource> arc;
      arc->SetCenter(this->Pole);
      arc->SetRatio(this->Ratio);
      arc->SetNormal(0., 0., 1.);
      arc->SetMajorRadiusVector(deltaArc + this->MinimumRadius, 0.0, 0.0);
      arc->SetStartAngle(this->MinimumAngle);
      arc->SetSegmentAngle(angleSection);
      arc->SetResolution(arcResolution);
      arc->Update();

      if (isLastArc)
      {
        // Add polar arc
        vtkPoints* arcPoints = nullptr;
        vtkIdType nPoints = 0;
        if (arc->GetOutput()->GetNumberOfPoints() > 0)
        {
          arcPoints = arc->GetOutput()->GetPoints();
          nPoints = arcResolution + 1;
          std::vector<vtkIdType> arcPointIds(nPoints);
          std::iota(arcPointIds.begin(), arcPointIds.end(), 0);
          for (vtkIdType j = 0; j < nPoints; ++j)
          {
            polarArcsPoints->InsertNextPoint(arcPoints->GetPoint(j));
          }
          polarArcsLines->InsertNextCell(nPoints, arcPointIds.data());
        }
      }
      else
      {
        // Append new secondary polar arc to existing ones
        vtkPoints* arcPoints = nullptr;
        vtkIdType nPoints = 0;
        if (arc->GetOutput()->GetNumberOfPoints() > 0)
        {
          arcPoints = arc->GetOutput()->GetPoints();
          nPoints = arcResolution + 1;
          std::vector<vtkIdType> arcPointIds(nPoints);
          std::iota(arcPointIds.begin(), arcPointIds.end(), pointIdOffset);

          for (vtkIdType j = 0; j < nPoints; ++j)
          {
            secondaryPolarArcsPoints->InsertNextPoint(arcPoints->GetPoint(j));
          }
          secondaryPolarArcsLines->InsertNextCell(nPoints, arcPointIds.data());
        }

        // Update polyline cell offset
        pointIdOffset += nPoints;
      }
    }

    // Move to next value
    valueRange = std::min(valueRange + deltaRange, axis->GetRange()[1]);
  }

  // Update polar axis there because DeltaRange might be needed
  // And we use the range for labels
  axis->SetDeltaRangeMajor(
    this->AxisTickMatchesPolarAxes ? this->DeltaRangePolarAxes : this->DeltaRangeMajor);
  axis->SetDeltaRangeMinor(
    this->AxisTickMatchesPolarAxes ? this->DeltaRangePolarAxes / 2.0 : this->DeltaRangeMinor);

  // Prepare storage for polar axis labels
  std::list<double> labelValList;
  int nTicks = this->AxisTickMatchesPolarAxes
    ? this->NumberOfPolarAxes
    : std::ceil(rangeLength / axis->GetDeltaRangeMajor()) + 1;
  valueRange = axis->GetRange()[0];
  for (int i = 0; i < nTicks; ++i)
  {
    // Store value
    labelValList.push_back(valueRange);

    // Move to next value
    valueRange = std::min(valueRange + axis->GetDeltaRangeMajor(), axis->GetRange()[1]);
  }

  // set up vtk collection to store labels
  vtkNew<vtkStringArray> labels;

  if (this->ExponentLocation != VTK_EXPONENT_LABELS)
  {
    // it modifies the values of labelValList
    std::string commonLbl = FindExponentAndAdjustValues(labelValList);
    axis->SetExponent(commonLbl);

    this->GetSignificantPartFromValues(labels, labelValList);
  }
  else
  {
    axis->SetExponent("");
    // construct label string array
    labels->SetNumberOfValues(static_cast<vtkIdType>(labelValList.size()));

    std::list<double>::iterator itList;
    vtkIdType i = 0;
    for (itList = labelValList.begin(); itList != labelValList.end(); ++i, ++itList)
    {
      char label[64];
      snprintf(label, sizeof(label), this->PolarLabelFormat, *itList);
      labels->SetValue(i, label);
    }
  }

  // Store labels
  axis->SetLabels(labels);
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::BuildPolarArcsLog()
{
  double angleSection = (this->MaximumAngle > this->MinimumAngle)
    ? this->MaximumAngle - this->MinimumAngle
    : 360.0 - fabs(this->MaximumAngle - this->MinimumAngle);

  // if Min and max angle are the same, interpret it as 360 segment opening
  if (vtkMathUtilities::FuzzyCompare(this->MaximumAngle, this->MinimumAngle))
  {
    angleSection = 360.0;
  }

  vtkIdType arcResolution =
    static_cast<vtkIdType>(angleSection * this->PolarArcResolutionPerDegree * this->Ratio);

  // Principal Arc points
  vtkNew<vtkPoints> polarArcsPoints;
  this->PolarArcs->SetPoints(polarArcsPoints);

  // Principal Arc lines
  vtkNew<vtkCellArray> polarArcsLines;
  this->PolarArcs->SetLines(polarArcsLines);

  // Secondary Arc points
  vtkNew<vtkPoints> secondaryPolarArcsPoints;
  this->SecondaryPolarArcs->SetPoints(secondaryPolarArcsPoints);

  // Secondary Arc lines
  vtkNew<vtkCellArray> secondaryPolarArcsLines;
  this->SecondaryPolarArcs->SetLines(secondaryPolarArcsLines);

  //--- prepare significant values ----
  double miniAngleEllipseRad = ComputeEllipseAngle(this->MinimumAngle, this->Ratio);

  // Distance from Pole to Range[0]
  vtkAxisActor* axis = this->PolarAxis;

  double deltaVector[3], polarAxisUnitVect[3];
  vtkMath::Subtract(axis->GetPoint2(), axis->GetPoint1(), deltaVector);
  vtkMath::Subtract(axis->GetPoint2(), axis->GetPoint1(), polarAxisUnitVect);
  vtkMath::Normalize(polarAxisUnitVect);

  // polar axis actor length
  double axisLength = vtkMath::Norm(deltaVector);

  // conversion factor
  double rangeScaleLog = axisLength / log10(axis->GetRange()[1] / axis->GetRange()[0]);

  // reuse deltaVector
  vtkMath::Subtract(axis->GetPoint1(), this->Pole, deltaVector);
  double distanceAxisPoint1FromPole = vtkMath::Norm(deltaVector);

  double base = 10.0;
  double log10Range0 = log10(axis->GetRange()[0]);
  double log10Range1 = log10(axis->GetRange()[1]);
  double lowBound = std::pow(base, static_cast<int>(std::floor(log10Range0)));
  double upBound = std::pow(base, static_cast<int>(ceil(log10Range1)));

  int i;
  double tickVal, tickRangeVal, indexTickRangeValue;

  vtkIdType pointIdOffset = 0;
  bool isInnerArc, isArcVisible, isLastArc;
  double a, b;
  double epsilon = 1e-8;

  for (indexTickRangeValue = lowBound; indexTickRangeValue <= upBound; indexTickRangeValue *= base)
  {
    // to keep major values as power of 10
    tickRangeVal = indexTickRangeValue;

    isInnerArc = tickRangeVal > lowBound && tickRangeVal < upBound;
    isArcVisible = !isInnerArc || this->DrawPolarArcsGridlines;
    isLastArc = tickRangeVal == upBound;

    if (!isArcVisible)
    {
      continue;
    }

    if (tickRangeVal < axis->GetRange()[0])
    {
      tickRangeVal = axis->GetRange()[0];
    }

    if (tickRangeVal > axis->GetRange()[1])
    {
      tickRangeVal = axis->GetRange()[1];
    }

    // conversion range value to world value
    tickVal = (log10(tickRangeVal) - log10Range0) * rangeScaleLog;

    // Vector from Pole to major tick
    for (i = 0; i < 3; i++)
    {
      deltaVector[i] = polarAxisUnitVect[i] * (tickVal + distanceAxisPoint1FromPole);
    }

    if (vtkMath::Norm(deltaVector) == 0.0)
    {
      continue;
    }

    // epsilon is a very low value. vtkMathUtilities::FuzzyCompare is not fuzzy enough ...
    if (fabs(fabs(miniAngleEllipseRad) - vtkMath::Pi() / 2.0) < epsilon)
    {
      b = deltaVector[1] / sin(miniAngleEllipseRad);
      a = b / this->Ratio;
    }
    else
    {
      a = deltaVector[0] / cos(miniAngleEllipseRad);
    }

    // Create elliptical polar arc with corresponding to this tick mark
    vtkNew<vtkEllipseArcSource> arc;
    arc->SetCenter(this->Pole);
    arc->SetRatio(this->Ratio);
    arc->SetNormal(0., 0., 1.);
    arc->SetMajorRadiusVector(a, 0.0, 0.0);
    arc->SetStartAngle(this->MinimumAngle);
    arc->SetSegmentAngle(angleSection);
    arc->SetResolution(arcResolution);
    arc->Update();

    if (isLastArc)
    {
      // Add principal polar arc
      vtkPoints* arcPoints = nullptr;
      vtkIdType nPoints;
      if (arc->GetOutput()->GetNumberOfPoints() > 0)
      {
        arcPoints = arc->GetOutput()->GetPoints();
        nPoints = arcResolution + 1;
        std::vector<vtkIdType> arcPointIds(nPoints);
        std::iota(arcPointIds.begin(), arcPointIds.end(), 0);
        for (vtkIdType j = 0; j < nPoints; ++j)
        {
          polarArcsPoints->InsertNextPoint(arcPoints->GetPoint(j));
        }
        polarArcsLines->InsertNextCell(nPoints, arcPointIds.data());
      }
    }
    else
    {
      // Append new polar arc to existing ones
      vtkPoints* arcPoints = nullptr;
      vtkIdType nPoints = 0;
      if (arc->GetOutput()->GetNumberOfPoints() > 0)
      {
        arcPoints = arc->GetOutput()->GetPoints();
        nPoints = arcResolution + 1;
        std::vector<vtkIdType> arcPointIds(nPoints);
        std::iota(arcPointIds.begin(), arcPointIds.end(), pointIdOffset);
        for (vtkIdType j = 0; j < nPoints; ++j)
        {
          secondaryPolarArcsPoints->InsertNextPoint(arcPoints->GetPoint(j));
          arcPointIds[j] = pointIdOffset + j;
        }
        secondaryPolarArcsLines->InsertNextCell(nPoints, arcPointIds.data());
      }

      // Update polyline cell offset
      pointIdOffset += nPoints;
    }
  }
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::BuildLabelsLog()
{
  // Prepare storage for polar axis labels
  std::list<double> labelValList;

  vtkAxisActor* axis = this->PolarAxis;
  double base = 10.0;

  if (axis->GetRange()[0] <= 0.0)
  {
    return;
  }

  // define major ticks label values
  double indexTickRangeValue;
  double tickRangeVal;
  double log10Range0 = log10(axis->GetRange()[0]);
  double log10Range1 = log10(axis->GetRange()[1]);
  double lowBound = std::pow(base, static_cast<int>(std::floor(log10Range0)));
  double upBound = std::pow(base, static_cast<int>(ceil(log10Range1)));

  for (indexTickRangeValue = lowBound; indexTickRangeValue <= upBound; indexTickRangeValue *= base)
  {
    tickRangeVal = indexTickRangeValue;

    if (indexTickRangeValue < axis->GetRange()[0])
    {
      tickRangeVal = axis->GetRange()[0];
    }

    else if (indexTickRangeValue > axis->GetRange()[1])
    {
      tickRangeVal = axis->GetRange()[1];
    }

    labelValList.push_back(tickRangeVal);
  }

  // set up vtk collection to store labels
  vtkNew<vtkStringArray> labels;

  if (this->ExponentLocation != VTK_EXPONENT_LABELS)
  {
    // it modifies the values of labelValList
    std::string commonLbl = FindExponentAndAdjustValues(labelValList);
    axis->SetExponent(commonLbl);

    this->GetSignificantPartFromValues(labels, labelValList);
  }
  else
  {
    axis->SetExponent("");
    labels->SetNumberOfValues(static_cast<vtkIdType>(labelValList.size()));

    std::list<double>::iterator itList;
    vtkIdType i = 0;
    for (itList = labelValList.begin(); itList != labelValList.end(); ++i, ++itList)
    {
      char label[64];
      snprintf(label, sizeof(label), this->PolarLabelFormat, *itList);
      labels->SetValue(i, label);
    }
  }

  // Store labels
  axis->SetLabels(labels);
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::BuildPolarAxisLabelsArcsLog()
{
  this->BuildPolarArcsLog();

  this->BuildLabelsLog();

  // Update axis title follower
  vtkAxisFollower* follower = this->PolarAxis->GetTitleActor();
  follower->SetAxis(this->PolarAxis);
  follower->SetEnableDistanceLOD(this->EnableDistanceLOD);
  follower->SetDistanceLODThreshold(this->DistanceLODThreshold);
  follower->SetEnableViewAngleLOD(this->EnableViewAngleLOD);
  follower->SetViewAngleLODThreshold(this->ViewAngleLODThreshold);

  // Update axis title follower
  vtkAxisFollower* expFollower = this->PolarAxis->GetExponentActor();
  expFollower->SetAxis(this->PolarAxis);
  expFollower->SetEnableDistanceLOD(this->EnableDistanceLOD);
  expFollower->SetDistanceLODThreshold(this->DistanceLODThreshold);
  expFollower->SetEnableViewAngleLOD(this->EnableViewAngleLOD);
  expFollower->SetViewAngleLODThreshold(this->ViewAngleLODThreshold);

  // Update axis label followers
  int labelCount = this->PolarAxis->GetNumberOfLabelsBuilt();
  for (int i = 0; i < labelCount; ++i)
  {
    vtkAxisFollower* labelActor = this->PolarAxis->GetLabelFollower(i);
    labelActor->SetAxis(this->PolarAxis);
    labelActor->SetEnableDistanceLOD(this->EnableDistanceLOD);
    labelActor->SetDistanceLODThreshold(this->DistanceLODThreshold);
    labelActor->SetEnableViewAngleLOD(this->EnableViewAngleLOD);
    labelActor->SetViewAngleLODThreshold(this->ViewAngleLODThreshold);
  }
}

//------------------------------------------------------------------------------
std::string vtkPolarAxesActor::FindExponentAndAdjustValues(std::list<double>& valuesList)
{
  std::list<double>::iterator itDouble;

  double exponentMean = 0.0;
  int count = 0;

  // find common exponent
  for (itDouble = valuesList.begin(); itDouble != valuesList.end(); ++itDouble)
  {
    if (*itDouble != 0.0)
    {
      double exponent = std::floor(log10(fabs(*itDouble)));
      exponentMean += exponent;
      count++;
    }
  }

  if (count == 0)
  {
    return "";
  }

  exponentMean /= count;

  // adjust exponent to int value. Round it if fract part != 0.0
  double intPart, fractPart;
  fractPart = modf(exponentMean, &intPart);

  if (exponentMean < 0.0)
  {
    if (fabs(fractPart) >= 0.5)
    {
      intPart -= 1.0;
    }
  }
  else
  {
    if (fabs(fractPart) >= 0.5)
    {
      intPart += 1.0;
    }
  }
  exponentMean = intPart;

  // shift every values
  for (itDouble = valuesList.begin(); itDouble != valuesList.end(); ++itDouble)
  {
    if (*itDouble != 0.0)
    {
      *itDouble /= std::pow(10, exponentMean);
    }
  }

  // Layout of the exponent:
  std::stringstream ss;
  int exponentInt = static_cast<int>(fabs(exponentMean));

  // add sign
  ss << (exponentMean >= 0.0 ? "+" : "-");

  // add 0 for pow < 10
  if (exponentInt < 10.0)
  {
    ss << "0";
  }

  ss << exponentInt;

  return ss.str();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::GetSignificantPartFromValues(
  vtkStringArray* valuesStr, std::list<double>& valuesList)
{
  if (!valuesStr || valuesList.empty())
  {
    return;
  }

  valuesStr->SetNumberOfValues(static_cast<vtkIdType>(valuesList.size()));

  std::list<double>::iterator itList;
  vtkIdType i = 0;
  for (itList = valuesList.begin(); itList != valuesList.end(); ++i, ++itList)
  {
    char label[64];
    if (this->ExponentLocation == VTK_EXPONENT_LABELS)
    {
      snprintf(label, sizeof(label), this->PolarLabelFormat, *itList);
      valuesStr->SetValue(i, label);
    }
    else
    {
      std::stringstream ss;
      if (*itList == 0.0)
      {
        ss << std::fixed << std::setw(1) << std::setprecision(0) << 0.0;
        valuesStr->SetValue(i, ss.str().c_str());
        continue;
      }

      // get pow of ten of the value to set the precision of the label
      int exponent = static_cast<int>(std::floor(log10(fabs(*itList))));
      if (exponent < 0)
      {
        ss << std::fixed << std::setw(1) << setprecision(-exponent) << *itList;
      }
      else
      {
        ss << std::fixed << setprecision(1) << *itList;
      }

      valuesStr->SetValue(i, ss.str().c_str());
    }
  }
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::AutoScale(vtkViewport* viewport)
{
  // Scale polar axis title
  vtkAxisActor* axis = this->PolarAxis.Get();
  double newTitleScale = vtkAxisFollower::AutoScale(
    viewport, this->Camera, this->ScreenSize, axis->GetTitleActor()->GetPosition());
  axis->SetTitleScale(newTitleScale);

  // Scale polar axis labels
  axis->SetLabelScale(newTitleScale);

  // Loop over radial axes
  for (int i = 0; i < this->NumberOfRadialAxes; ++i)
  {
    axis = this->RadialAxes[i];
    // Scale title
    newTitleScale = vtkAxisFollower::AutoScale(
      viewport, this->Camera, this->ScreenSize, axis->GetTitleActor()->GetPosition());
    axis->SetTitleScale(newTitleScale);
  }
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetPole(double p[3])
{
  this->Pole[0] = p[0];
  this->Pole[1] = p[1];
  this->Pole[2] = p[2];

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetPole(double x, double y, double z)
{
  this->Pole[0] = x;
  this->Pole[1] = y;
  this->Pole[2] = z;

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetMinimumRadius(double r)
{
  this->MinimumRadius = r > 0. ? r : 0.;

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetMaximumRadius(double r)
{
  this->MaximumRadius = r > 0. ? r : 0.;

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetMinimumAngle(double a)
{
  if (a > 360.)
  {
    this->MinimumAngle = 360.;
  }
  else if (a < -360.)
  {
    this->MinimumAngle = -360.;
  }
  else
  {
    this->MinimumAngle = a;
  }

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetMaximumAngle(double a)
{
  if (a > 360.)
  {
    this->MaximumAngle = 360.;
  }
  else if (a < -360.)
  {
    this->MaximumAngle = -360.;
  }
  else
  {
    this->MaximumAngle = a;
  }

  // Update bounds
  this->CalculateBounds();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetUseTextActor3D(bool enable)
{
  for (int i = 0; i < this->NumberOfRadialAxes; ++i)
  {
    this->RadialAxes[i]->SetUseTextActor3D(enable);
  }

  this->PolarAxis->SetUseTextActor3D(enable);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetUse2DMode(bool enable)
{
  for (int i = 0; i < this->NumberOfRadialAxes; ++i)
  {
    this->RadialAxes[i]->SetUse2DMode(enable);
  }

  this->PolarAxis->SetUse2DMode(enable);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkPolarAxesActor::GetUse2DMode()
{
  return this->PolarAxis->GetUse2DMode();
}

//------------------------------------------------------------------------------
vtkCamera* vtkPolarAxesActor::GetCamera()
{
  return this->Camera.Get();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkPolarAxesActor::GetPolarAxisTitleTextProperty()
{
  return this->PolarAxisTitleTextProperty.Get();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkPolarAxesActor::GetPolarAxisLabelTextProperty()
{
  return this->PolarAxisLabelTextProperty.Get();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkPolarAxesActor::GetLastRadialAxisTextProperty()
{
  return this->LastRadialAxisTextProperty.Get();
}

//------------------------------------------------------------------------------
vtkTextProperty* vtkPolarAxesActor::GetSecondaryRadialAxesTextProperty()
{
  return this->SecondaryRadialAxesTextProperty.Get();
}

//------------------------------------------------------------------------------
vtkProperty* vtkPolarAxesActor::GetPolarAxisProperty()
{
  return this->PolarAxisProperty.Get();
}

//------------------------------------------------------------------------------
vtkProperty* vtkPolarAxesActor::GetLastRadialAxisProperty()
{
  return this->LastRadialAxisProperty.Get();
}

//------------------------------------------------------------------------------
vtkProperty* vtkPolarAxesActor::GetSecondaryRadialAxesProperty()
{
  return this->SecondaryRadialAxesProperty.Get();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetPolarAxisProperty(vtkProperty* prop)
{
  this->PolarAxisProperty->DeepCopy(prop);
  this->PolarAxisProperty->SetLineWidth(this->PolarAxisMajorTickThickness);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetPolarArcsProperty(vtkProperty* prop)
{
  this->PolarArcsActor->SetProperty(prop);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkProperty* vtkPolarAxesActor::GetPolarArcsProperty()
{
  return this->PolarArcsActor->GetProperty();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::SetSecondaryPolarArcsProperty(vtkProperty* prop)
{
  this->SecondaryPolarArcsActor->SetProperty(prop);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkProperty* vtkPolarAxesActor::GetSecondaryPolarArcsProperty()
{
  return this->SecondaryPolarArcsActor->GetProperty();
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::ComputeDeltaRangePolarAxes(vtkIdType n)
{
  double rangeLength = fabs(this->Range[1] - this->Range[0]);
  double step = rangeLength / (n - 1);

  if (this->DeltaRangePolarAxes != step)
  {
    this->DeltaRangePolarAxes = step;
  }
}

//------------------------------------------------------------------------------
void vtkPolarAxesActor::ComputeDeltaAngleRadialAxes(vtkIdType n)
{
  double angleSection = (this->MaximumAngle > this->MinimumAngle)
    ? this->MaximumAngle - this->MinimumAngle
    : 360.0 - fabs(this->MaximumAngle - this->MinimumAngle);

  // if Min and max angle are the same, interpret it as 360 segment opening
  if (vtkMathUtilities::FuzzyCompare(this->MaximumAngle, this->MinimumAngle))
  {
    angleSection = 360.0;
  }

  double step = this->ComputeIdealStep(n - 1, angleSection);
  if (step == 0.0)
  {
    step = angleSection / (n - 1);
  }

  if (this->DeltaAngleRadialAxes != step)
  {
    this->DeltaAngleRadialAxes = step;
  }
}

//------------------------------------------------------------------------------
double vtkPolarAxesActor::ComputeIdealStep(int subDivsRequired, double rangeLength, int maxSubDivs)
{
  double pow10, pow10Start, pow10End;
  double rawStep, roundStep, roundStepSup;

  if (rangeLength == 0.0 || subDivsRequired >= maxSubDivs)
  {
    return 0.0;
  }

  if (subDivsRequired <= 1)
  {
    return rangeLength;
  }
  if (subDivsRequired <= 4)
  {
    return rangeLength / subDivsRequired;
  }

  // range step, if axis range is strictly subdivided by the number of ticks wished
  rawStep = rangeLength / subDivsRequired;

  // pow of 10 order of magnitude
  pow10Start = std::floor(log10(rawStep));
  pow10End = -10.0;
  if (pow10End >= pow10Start)
  {
    pow10End -= 1.0;
  }

  if (rawStep <= std::pow(10, pow10End))
  {
    return 0.0;
  }

  double dividend = rawStep;

  double pow10Step;
  double idealStep = 0.0;
  double subdivs = 1.0, subdivsSup = 1.0;

  int currentPow10Multiple;

  for (pow10 = pow10Start; pow10 >= pow10End; pow10 -= 1.0)
  {
    // 10.0, 1.0, 0.1, ...
    pow10Step = std::pow(10.0, pow10);

    // example: 4 = 0.4874 / 0.1 for pow10Step = 0.1
    currentPow10Multiple = static_cast<int>(dividend / pow10Step);

    // 0.4 = 4 * 0.1
    roundStep = currentPow10Multiple * pow10Step;

    // 0.5 = 5 * 0.1
    roundStepSup = (currentPow10Multiple + 1) * pow10Step;

    // currentIdealStep is the previous digits of the ideal step we seek
    subdivs = rangeLength / (idealStep + roundStep);
    subdivsSup = rangeLength / (idealStep + roundStepSup);

    if (fabs(subdivs - subDivsRequired) < 1.0 || fabs(subdivsSup - subDivsRequired) < 1.0)
    {
      // if currentStep + the current power of 10, is closer to the require tick count
      if (fabs(subdivs - subDivsRequired) > fabs(subdivsSup - subDivsRequired) &&
        fabs(subdivsSup - subDivsRequired) < 1.0)
      {
        idealStep += roundStepSup;
      }

      // subdivs closer to subdiv than subdivsSup
      else
      {
        idealStep += roundStep;
      }
      break;
    }

    idealStep += roundStep;

    // 0.4874 - 0.4 for roundStep = 0.4
    // remainder becomes dividend
    dividend = dividend - roundStep;
  }

  // if idealStep is too small
  if (static_cast<int>(rangeLength / idealStep) > subDivsRequired)
  {
    idealStep = rawStep;
  }

  return idealStep;
}

double vtkPolarAxesActor::ComputeEllipseAngle(double angleInDegrees, double ratio)
{
  double miniAngleEllipse;
  double minimumAngleRad = vtkMath::RadiansFromDegrees(angleInDegrees);
  minimumAngleRad = std::fmod(minimumAngleRad, 2.0 * vtkMath::Pi());

  // result range: -pi / 2, pi / 2
  miniAngleEllipse = atan(tan(minimumAngleRad) / ratio);

  // ellipse range: 0, 2 * pi
  if (minimumAngleRad > vtkMath::Pi() / 2 && minimumAngleRad <= vtkMath::Pi())
  {
    miniAngleEllipse += vtkMath::Pi();
  }
  else if (minimumAngleRad > vtkMath::Pi() && minimumAngleRad <= 1.5 * vtkMath::Pi())
  {
    miniAngleEllipse -= vtkMath::Pi();
  }
  return miniAngleEllipse;
}
VTK_ABI_NAMESPACE_END
