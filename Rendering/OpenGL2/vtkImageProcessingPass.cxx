// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageProcessingPass.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLFramebufferObject.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkTextureObject.h"
#include "vtk_glad.h"
#include <cassert>

// to be able to dump intermediate passes into png files for debugging.
// only for vtkImageProcessingPass developers.
// #define VTK_IMAGE_PROCESSING_PASS_DEBUG

#ifdef VTK_IMAGE_PROCESSING_PASS_DEBUG
#include "vtkImageImport.h"
#include "vtkPNGWriter.h"
#include "vtkPixelBufferObject.h"
#endif

#include "vtkCamera.h"
#include "vtkMath.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkImageProcessingPass, DelegatePass, vtkRenderPass);

//------------------------------------------------------------------------------
vtkImageProcessingPass::vtkImageProcessingPass()
{
  this->DelegatePass = nullptr;
}

//------------------------------------------------------------------------------
vtkImageProcessingPass::~vtkImageProcessingPass()
{
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkImageProcessingPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "DelegatePass:";
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
}
//------------------------------------------------------------------------------
// Description:
// Render delegate with a image of different dimensions than the
// original one.
// \pre s_exists: s!=0
// \pre fbo_exists: fbo!=0
// \pre fbo_has_context: fbo->GetContext()!=0
// \pre target_exists: target!=0
// \pre target_has_context: target->GetContext()!=0
void vtkImageProcessingPass::RenderDelegate(const vtkRenderState* s, int width, int height,
  int newWidth, int newHeight, vtkOpenGLFramebufferObject* fbo, vtkTextureObject* target)
{
  assert("pre: s_exists" && s != nullptr);
  assert("pre: fbo_exists" && fbo != nullptr);
  assert("pre: fbo_has_context" && fbo->GetContext() != nullptr);
  assert("pre: target_exists" && target != nullptr);
  assert("pre: target_has_context" && target->GetContext() != nullptr);

#ifdef VTK_IMAGE_PROCESSING_PASS_DEBUG
  cout << "width=" << width << endl;
  cout << "height=" << height << endl;
  cout << "newWidth=" << newWidth << endl;
  cout << "newHeight=" << newHeight << endl;
#endif

  vtkRenderer* r = s->GetRenderer();
  vtkRenderState s2(r);
  s2.SetPropArrayAndCount(s->GetPropArray(), s->GetPropArrayCount());

  // Adapt camera to new window size
  vtkCamera* savedCamera = r->GetActiveCamera();
  savedCamera->Register(this);
  vtkCamera* newCamera = vtkCamera::New();
  newCamera->DeepCopy(savedCamera);

  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderWindow*>(r->GetVTKWindow())->GetState();

#ifdef VTK_IMAGE_PROCESSING_PASS_DEBUG
  cout << "old camera params=";
  savedCamera->Print(cout);
  cout << "new camera params=";
  newCamera->Print(cout);
#endif
  r->SetActiveCamera(newCamera);

  if (newCamera->GetParallelProjection())
  {
    newCamera->SetParallelScale(
      newCamera->GetParallelScale() * newHeight / static_cast<double>(height));
  }
  else
  {
    double largeDim;
    double smallDim;
    if (newCamera->GetUseHorizontalViewAngle())
    {
      largeDim = newWidth;
      smallDim = width;
    }
    else
    {
      largeDim = newHeight;
      smallDim = height;
    }
    double angle = vtkMath::RadiansFromDegrees(newCamera->GetViewAngle());

#ifdef VTK_IMAGE_PROCESSING_PASS_DEBUG
    cout << "old angle =" << angle << " rad=" << vtkMath::DegreesFromRadians(angle) << " deg"
         << endl;
#endif

    angle = 2.0 * atan(tan(angle / 2.0) * largeDim / smallDim);

#ifdef VTK_IMAGE_PROCESSING_PASS_DEBUG
    cout << "new angle =" << angle << " rad=" << vtkMath::DegreesFromRadians(angle) << " deg"
         << endl;
#endif

    newCamera->SetViewAngle(vtkMath::DegreesFromRadians(angle));
  }

  s2.SetFrameBuffer(fbo);

  if (target->GetWidth() != static_cast<unsigned int>(newWidth) ||
    target->GetHeight() != static_cast<unsigned int>(newHeight))
  {
    target->Create2D(newWidth, newHeight, 4, VTK_UNSIGNED_CHAR, false);
  }

  fbo->Bind();
  fbo->AddColorAttachment(0, target);

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  fbo->ActivateBuffer(0);

  fbo->AddDepthAttachment();
  fbo->StartNonOrtho(newWidth, newHeight);
  if (r->Transparent())
  {
    // Clear is not called on transparent renderers. But since this is a offscreen render target we
    // want it cleared
    ostate->vtkglClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ostate->vtkglClear(GL_COLOR_BUFFER_BIT);
  }
  ostate->vtkglViewport(0, 0, newWidth, newHeight);
  ostate->vtkglScissor(0, 0, newWidth, newHeight);

  // 2. Delegate render in FBO
  ostate->vtkglEnable(GL_DEPTH_TEST);
  this->DelegatePass->Render(&s2);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

#ifdef VTK_IMAGE_PROCESSING_PASS_DEBUG
  vtkPixelBufferObject* pbo = target->Download();

  unsigned int dims[2];
  vtkIdType continuousInc[3];

  dims[0] = static_cast<unsigned int>(newWidth);
  dims[1] = static_cast<unsigned int>(newHeight);
  continuousInc[0] = 0;
  continuousInc[1] = 0;
  continuousInc[2] = 0;

  int byteSize = newWidth * newHeight * 4 * sizeof(float);
  float* buffer = new float[newWidth * newHeight * 4];
  pbo->Download2D(VTK_FLOAT, buffer, dims, 4, continuousInc);

  vtkImageImport* importer = vtkImageImport::New();
  importer->CopyImportVoidPointer(buffer, static_cast<int>(byteSize));
  importer->SetDataScalarTypeToFloat();
  importer->SetNumberOfScalarComponents(4);
  importer->SetWholeExtent(0, newWidth - 1, 0, newHeight - 1, 0, 0);
  importer->SetDataExtentToWholeExtent();

  importer->Update();

  vtkPNGWriter* writer = vtkPNGWriter::New();
  writer->SetFileName("ip.png");
  writer->SetInputConnection(importer->GetOutputPort());
  importer->Delete();
  cout << "Writing " << writer->GetFileName() << endl;
  writer->Write();
  cout << "Wrote " << writer->GetFileName() << endl;
  writer->Delete();

  pbo->Delete();
  delete[] buffer;
#endif

  newCamera->Delete();
  r->SetActiveCamera(savedCamera);
  savedCamera->UnRegister(this);
}

//------------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkImageProcessingPass::ReleaseGraphicsResources(vtkWindow* w)
{
  assert("pre: w_exists" && w != nullptr);
  if (this->DelegatePass != nullptr)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}
VTK_ABI_NAMESPACE_END
