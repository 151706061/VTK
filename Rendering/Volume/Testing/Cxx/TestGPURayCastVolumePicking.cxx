// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers volume picking with vtkGPURayCastVolumePicking using
// vtkHardwareSelector.
// This test renders volume data along with polydata objects and selects
// the volume.
// Use 'p' for point picking and 'r' for area selection.

#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkRenderedAreaPicker.h"
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageData.h>
#include <vtkImageReader.h>
#include <vtkImageShiftScale.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>
#include <vtkTimerLog.h>
#include <vtkVolumeProperty.h>
#include <vtkXMLImageDataReader.h>

#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkHardwareSelector.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"

namespace
{
class VolumePickingCommand : public vtkCommand
{
public:
  static VolumePickingCommand* New() { return new VolumePickingCommand; }

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId),
    void* vtkNotUsed(callData)) override
  {
    assert(this->Renderer != nullptr);

    vtkNew<vtkHardwareSelector> selector;
    selector->SetRenderer(this->Renderer);
    selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);

    unsigned int const x1 = static_cast<unsigned int>(this->Renderer->GetPickX1());
    unsigned int const y1 = static_cast<unsigned int>(this->Renderer->GetPickY1());
    unsigned int const x2 = static_cast<unsigned int>(this->Renderer->GetPickX2());
    unsigned int const y2 = static_cast<unsigned int>(this->Renderer->GetPickY2());
    selector->SetArea(x1, y1, x2, y2);
    // std::cout << "->>> SetArea (x1, y1, x2, y2): (" << x1 << ", " << y1 << ", "
    //  << x2 << ", " << y2 << ")" << '\n';

    vtkSelection* result = selector->Select();
    // result->Print(std::cout);

    unsigned int const numProps = result->GetNumberOfNodes();

    for (unsigned int n = 0; n < numProps; n++)
    {
      vtkSelectionNode* node = result->GetNode(n);
      vtkInformation* properties = node->GetProperties();
      vtkInformationIntegerKey* infoIntKey = vtkSelectionNode::PROP_ID();

      vtkAbstractArray* abs = node->GetSelectionList();
      vtkIdType size = abs->GetSize();
      std::cout << "PropId: " << infoIntKey->Get(properties) << "/ Num. Attr.:  " << size << '\n';

      if (numProps > 1)
        continue;

      // Get the vtkAlgorithm instance of the prop to connect it to
      // the outline filter.
      vtkInformationObjectBaseKey* key = vtkSelectionNode::PROP();
      vtkObjectBase* keyObj = key->Get(properties);
      if (!keyObj)
        continue;

      vtkAbstractMapper3D* mapper = nullptr;
      vtkActor* actor = vtkActor::SafeDownCast(keyObj);
      vtkVolume* vol = vtkVolume::SafeDownCast(keyObj);
      if (actor)
        mapper = vtkAbstractMapper3D::SafeDownCast(actor->GetMapper());
      else if (vol)
        mapper = vtkAbstractMapper3D::SafeDownCast(vol->GetMapper());
      else
        continue;

      if (!mapper)
        continue;

      vtkAlgorithm* algo = mapper->GetInputAlgorithm();
      if (!algo)
        continue;

      this->OutlineFilter->SetInputConnection(algo->GetOutputPort());
    }

    result->Delete();
  }
  //////////////////////////////////////////////////////////////////////////////

  vtkSmartPointer<vtkRenderer> Renderer;
  vtkSmartPointer<vtkOutlineFilter> OutlineFilter;
};
}

// =============================================================================
int TestGPURayCastVolumePicking(int argc, char* argv[])
{
  // volume source and mapper
  vtkNew<vtkXMLImageDataReader> reader;
  const char* volumeFile = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vase_1comp.vti");
  reader->SetFileName(volumeFile);

  delete[] volumeFile;

  vtkNew<vtkImageChangeInformation> changeInformation;
  changeInformation->SetInputConnection(reader->GetOutputPort());
  changeInformation->SetOutputSpacing(1, 2, 3);
  changeInformation->SetOutputOrigin(10, 20, 30);
  changeInformation->Update();

  vtkNew<vtkGPUVolumeRayCastMapper> volumeMapper;
  double scalarRange[2];
  volumeMapper->SetInputConnection(changeInformation->GetOutputPort());
  volumeMapper->GetInput()->GetScalarRange(scalarRange);
  volumeMapper->SetBlendModeToComposite();

  vtkNew<vtkPiecewiseFunction> scalarOpacity;
  scalarOpacity->AddPoint(scalarRange[0], 0.0);
  scalarOpacity->AddPoint(scalarRange[1], 1.0);

  vtkNew<vtkVolumeProperty> volumeProperty;
  volumeProperty->ShadeOff();
  volumeProperty->SetInterpolationType(VTK_LINEAR_INTERPOLATION);
  volumeProperty->SetScalarOpacity(scalarOpacity);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    volumeProperty->GetRGBTransferFunction(0);
  colorTransferFunction->RemoveAllPoints();
  colorTransferFunction->AddRGBPoint(scalarRange[0], 0.0, 0.0, 0.0);
  colorTransferFunction->AddRGBPoint(scalarRange[1], 1.0, 1.0, 1.0);

  vtkNew<vtkVolume> volume;
  volume->PickableOn();
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);

  // polygonal sources and mappers
  vtkNew<vtkConeSource> cone;
  cone->SetHeight(100.0);
  cone->SetRadius(50.0);
  cone->SetResolution(200.0);
  cone->SetCenter(80, 100, 100);
  cone->Update();

  vtkNew<vtkPolyDataMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());

  vtkNew<vtkActor> coneActor;
  coneActor->SetMapper(coneMapper);
  coneActor->PickableOn();

  vtkNew<vtkSphereSource> sphere;
  sphere->SetPhiResolution(20.0);
  sphere->SetThetaResolution(20.0);
  sphere->SetCenter(90, 40, 170);
  sphere->SetRadius(40.0);
  sphere->Update();

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->AddInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  sphereActor->PickableOn();

  // Add outline filter
  vtkNew<vtkActor> outlineActor;
  vtkNew<vtkPolyDataMapper> outlineMapper;
  vtkNew<vtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputConnection(cone->GetOutputPort());
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());
  outlineActor->SetMapper(outlineMapper);
  outlineActor->PickableOff();

  // rendering setup
  vtkNew<vtkRenderer> ren;
  ren->SetBackground(0.2, 0.2, 0.5);
  ren->AddActor(coneActor);
  ren->AddActor(sphereActor);
  ren->AddActor(outlineActor);
  ren->AddViewProp(volume);

  vtkNew<vtkRenderWindow> renWin;
  // renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);
  renWin->SetSize(400, 400);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();
  ren->ResetCamera();

  // interaction & picking
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkInteractorStyleRubberBandPick* rbp = vtkInteractorStyleRubberBandPick::New();
  rwi->SetInteractorStyle(rbp);
  vtkRenderedAreaPicker* areaPicker = vtkRenderedAreaPicker::New();
  rwi->SetPicker(areaPicker);

  // Add selection observer
  vtkNew<VolumePickingCommand> vpc;
  vpc->Renderer = ren;
  vpc->OutlineFilter = outlineFilter;
  rwi->AddObserver(vtkCommand::EndPickEvent, vpc);

  // run the actual test
  areaPicker->AreaPick(177, 125, 199, 206, ren);
  vpc->Execute(nullptr, 0, nullptr);
  renWin->Render();

  // initialize render loop
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Initialize();
    iren->Start();
  }

  areaPicker->Delete();
  rbp->Delete();

  return !retVal;
}
