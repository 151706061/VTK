// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtk_glad.h"

#include "vtkHiddenLineRemovalPass.h"

#include "vtkActor.h"
#include "vtkMapper.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderUtilities.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLState.h"
#include "vtkProp.h"
#include "vtkProperty.h"
#include "vtkRenderState.h"
#include "vtkRenderer.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
void annotate(const std::string& str)
{
  vtkOpenGLRenderUtilities::MarkDebugEvent(str);
}
}

vtkStandardNewMacro(vtkHiddenLineRemovalPass);

//------------------------------------------------------------------------------
void vtkHiddenLineRemovalPass::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkHiddenLineRemovalPass::Render(const vtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;

  // Separate the wireframe props from the others:
  std::vector<vtkProp*> wireframeProps;
  std::vector<vtkProp*> otherProps;
  for (int i = 0; i < s->GetPropArrayCount(); ++i)
  {
    bool isWireframe = false;
    vtkProp* prop = s->GetPropArray()[i];
    vtkActor* actor = vtkActor::SafeDownCast(prop);
    if (actor)
    {
      vtkProperty* property = actor->GetProperty();
      if (property->GetRepresentation() == VTK_WIREFRAME)
      {
        isWireframe = true;
      }
    }
    if (isWireframe)
    {
      wireframeProps.push_back(prop);
    }
    else
    {
      otherProps.push_back(prop);
    }
  }

  vtkViewport* vp = s->GetRenderer();
  vtkOpenGLState* ostate = static_cast<vtkOpenGLRenderer*>(vp)->GetState();

  // Render the non-wireframe geometry as normal:
  annotate("Rendering non-wireframe props.");
  this->NumberOfRenderedProps = this->RenderProps(otherProps, vp);
  vtkOpenGLStaticCheckErrorMacro("Error after non-wireframe geometry.");

  // Store the coincident topology parameters -- we want to force polygon
  // offset to keep the drawn lines sharp:
  int ctMode = vtkMapper::GetResolveCoincidentTopology();
  double ctFactor, ctUnits;
  vtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(ctFactor, ctUnits);
  vtkMapper::SetResolveCoincidentTopology(VTK_RESOLVE_POLYGON_OFFSET);
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(2.0, 2.0);

  // Draw the wireframe props as surfaces into the depth buffer only:
  annotate("Rendering wireframe prop surfaces.");
  this->SetRepresentation(wireframeProps, VTK_SURFACE);
  ostate->vtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  this->RenderProps(wireframeProps, vp);
  vtkOpenGLStaticCheckErrorMacro("Error after wireframe surface rendering.");

  // Now draw the wireframes as normal:
  annotate("Rendering wireframes.");
  this->SetRepresentation(wireframeProps, VTK_WIREFRAME);
  ostate->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  this->NumberOfRenderedProps = this->RenderProps(wireframeProps, vp);
  vtkOpenGLStaticCheckErrorMacro("Error after wireframe rendering.");

  // Restore the previous coincident topology parameters:
  vtkMapper::SetResolveCoincidentTopology(ctMode);
  vtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(ctFactor, ctUnits);
}

//------------------------------------------------------------------------------
bool vtkHiddenLineRemovalPass::WireframePropsExist(vtkProp** propArray, int nProps)
{
  for (int i = 0; i < nProps; ++i)
  {
    vtkActor* actor = vtkActor::SafeDownCast(propArray[i]);
    if (actor)
    {
      vtkProperty* property = actor->GetProperty();
      if (property->GetRepresentation() == VTK_WIREFRAME)
      {
        return true;
      }
    }
  }

  return false;
}

//------------------------------------------------------------------------------
vtkHiddenLineRemovalPass::vtkHiddenLineRemovalPass() = default;

//------------------------------------------------------------------------------
vtkHiddenLineRemovalPass::~vtkHiddenLineRemovalPass() = default;

//------------------------------------------------------------------------------
void vtkHiddenLineRemovalPass::SetRepresentation(std::vector<vtkProp*>& props, int repr)
{
  for (std::vector<vtkProp*>::iterator it = props.begin(), itEnd = props.end(); it != itEnd; ++it)
  {
    vtkActor* actor = vtkActor::SafeDownCast(*it);
    if (actor)
    {
      actor->GetProperty()->SetRepresentation(repr);
    }
  }
}

//------------------------------------------------------------------------------
int vtkHiddenLineRemovalPass::RenderProps(std::vector<vtkProp*>& props, vtkViewport* vp)
{
  int propsRendered = 0;
  for (std::vector<vtkProp*>::iterator it = props.begin(), itEnd = props.end(); it != itEnd; ++it)
  {
    propsRendered += (*it)->RenderOpaqueGeometry(vp);
  }
  return propsRendered;
}
VTK_ABI_NAMESPACE_END
