// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkExporter.h"

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkCxxSetObjectMacro(vtkExporter, RenderWindow, vtkRenderWindow);
vtkCxxSetObjectMacro(vtkExporter, ActiveRenderer, vtkRenderer);

//------------------------------------------------------------------------------
// Construct with no start and end write methods or arguments.
vtkExporter::vtkExporter()
{
  this->RenderWindow = nullptr;
  this->ActiveRenderer = nullptr;
  this->StartWrite = nullptr;
  this->StartWriteArgDelete = nullptr;
  this->StartWriteArg = nullptr;
  this->EndWrite = nullptr;
  this->EndWriteArgDelete = nullptr;
  this->EndWriteArg = nullptr;
}

//------------------------------------------------------------------------------
vtkExporter::~vtkExporter()
{
  this->SetRenderWindow(nullptr);
  this->SetActiveRenderer(nullptr);

  if ((this->StartWriteArg) && (this->StartWriteArgDelete))
  {
    (*this->StartWriteArgDelete)(this->StartWriteArg);
  }
  if ((this->EndWriteArg) && (this->EndWriteArgDelete))
  {
    (*this->EndWriteArgDelete)(this->EndWriteArg);
  }
}

//------------------------------------------------------------------------------
// Write data to output. Method executes subclasses WriteData() method, as
// well as StartWrite() and EndWrite() methods.
void vtkExporter::Write()
{
  // make sure input is available
  if (!this->RenderWindow)
  {
    vtkErrorMacro(<< "No render window provided!");
    return;
  }
  if (this->ActiveRenderer != nullptr && !this->RenderWindow->HasRenderer(this->ActiveRenderer))
  {
    vtkErrorMacro(<< "ActiveRenderer must be a renderer owned by the RenderWindow");
    return;
  }

  if (this->StartWrite)
  {
    (*this->StartWrite)(this->StartWriteArg);
  }
  this->WriteData();
  if (this->EndWrite)
  {
    (*this->EndWrite)(this->EndWriteArg);
  }
}

//------------------------------------------------------------------------------
// Convenient alias for Write() method.
void vtkExporter::Update()
{
  this->Write();
}

//------------------------------------------------------------------------------
// Specify a function to be called before data is written.
// Function will be called with argument provided.
void vtkExporter::SetStartWrite(void (*f)(void*), void* arg)
{
  if (f != this->StartWrite)
  {
    // delete the current arg if there is one and a delete meth
    if ((this->StartWriteArg) && (this->StartWriteArgDelete))
    {
      (*this->StartWriteArgDelete)(this->StartWriteArg);
    }
    this->StartWrite = f;
    this->StartWriteArg = arg;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Set the arg delete method. This is used to free user memory.
void vtkExporter::SetStartWriteArgDelete(void (*f)(void*))
{
  if (f != this->StartWriteArgDelete)
  {
    this->StartWriteArgDelete = f;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Set the arg delete method. This is used to free user memory.
void vtkExporter::SetEndWriteArgDelete(void (*f)(void*))
{
  if (f != this->EndWriteArgDelete)
  {
    this->EndWriteArgDelete = f;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
// Specify a function to be called after data is written.
// Function will be called with argument provided.
void vtkExporter::SetEndWrite(void (*f)(void*), void* arg)
{
  if (f != this->EndWrite)
  {
    // delete the current arg if there is one and a delete meth
    if ((this->EndWriteArg) && (this->EndWriteArgDelete))
    {
      (*this->EndWriteArgDelete)(this->EndWriteArg);
    }
    this->EndWrite = f;
    this->EndWriteArg = arg;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->RenderWindow)
  {
    os << indent << "Render Window: (" << static_cast<void*>(this->RenderWindow) << ")\n";
  }
  else
  {
    os << indent << "Render Window: (none)\n";
  }

  if (this->ActiveRenderer)
  {
    os << indent << "Active Renderer: (" << static_cast<void*>(this->ActiveRenderer) << ")\n";
  }
  else
  {
    os << indent << "Active Renderer: (none)\n";
  }

  if (this->StartWrite)
  {
    os << indent << "Start Write: (" << this->StartWrite << ")\n";
  }
  else
  {
    os << indent << "Start Write: (none)\n";
  }

  if (this->EndWrite)
  {
    os << indent << "End Write: (" << this->EndWrite << ")\n";
  }
  else
  {
    os << indent << "End Write: (none)\n";
  }
}

//------------------------------------------------------------------------------
vtkMTimeType vtkExporter::GetMTime()
{
  vtkMTimeType mTime = this->vtkObject::GetMTime();
  vtkMTimeType time;

  if (this->RenderWindow != nullptr)
  {
    time = this->RenderWindow->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}
VTK_ABI_NAMESPACE_END
