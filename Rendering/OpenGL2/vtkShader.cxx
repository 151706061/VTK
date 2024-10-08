// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkShader.h"
#include "vtkObjectFactory.h"

#include "vtk_glad.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkShader);

vtkShader::vtkShader()
{
  this->Dirty = true;
  this->Handle = 0;
  this->ShaderType = vtkShader::Unknown;
}

vtkShader::~vtkShader() = default;

void vtkShader::SetType(Type type)
{
  this->ShaderType = type;
  this->Dirty = true;
}

void vtkShader::SetSource(const std::string& source)
{
  this->Source = source;
  this->Dirty = true;
}

bool vtkShader::Compile()
{
  if (this->Source.empty() || this->ShaderType == Unknown || !this->Dirty)
  {
    return false;
  }

  // Ensure we delete the previous shader if necessary.
  if (this->Handle != 0)
  {
    glDeleteShader(static_cast<GLuint>(this->Handle));
    this->Handle = 0;
  }

  GLenum type = GL_VERTEX_SHADER;
  switch (this->ShaderType)
  {
    case vtkShader::Geometry:
#ifdef GL_GEOMETRY_SHADER
      type = GL_GEOMETRY_SHADER;
      break;
#else
      this->Error = "Geometry shaders are not supported in this build of VTK";
      return false;
#endif
    case vtkShader::Compute:
#ifdef GL_COMPUTE_SHADER
      type = GL_COMPUTE_SHADER;
      break;
#else
      this->Error = "Compute shaders are not supported in this build of VTK";
      return false;
#endif
    case vtkShader::TessEvaluation:
#ifdef GL_TESS_EVALUATION_SHADER
      type = GL_TESS_EVALUATION_SHADER;
      break;
#else
      this->Error = "Tessellation evaluation shaders are not supported in this build of VTK";
      return false;
#endif
    case vtkShader::TessControl:
#ifdef GL_TESS_CONTROL_SHADER
      type = GL_TESS_CONTROL_SHADER;
      break;
#else
      this->Error = "Tessellation control shaders are not supported in this build of VTK";
      return false;
#endif
    case vtkShader::Fragment:
      type = GL_FRAGMENT_SHADER;
      break;
    case vtkShader::Vertex:
    case vtkShader::Unknown:
    default:
      type = GL_VERTEX_SHADER;
      break;
  }

  GLuint handle = glCreateShader(type);

  // Handle shader creation failures.
  if (handle == 0)
  {
    this->Error = "Could not create shader object.";
    return false;
  }

  const GLchar* source = static_cast<const GLchar*>(this->Source.c_str());
  glShaderSource(handle, 1, &source, nullptr);
  glCompileShader(handle);
  GLint isCompiled;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &isCompiled);

  // Handle shader compilation failures.
  if (!isCompiled)
  {
    GLint length(0);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);
    if (length > 1)
    {
      char* logMessage = new char[length];
      glGetShaderInfoLog(handle, length, nullptr, logMessage);
      this->Error = logMessage;
      delete[] logMessage;
    }
    glDeleteShader(handle);
    return false;
  }

  // The shader compiled, store its handle and return success.
  this->Handle = static_cast<int>(handle);
  this->Dirty = false;

  return true;
}

void vtkShader::Cleanup()
{
  if (this->ShaderType == Unknown || this->Handle == 0)
  {
    return;
  }

  glDeleteShader(static_cast<GLuint>(this->Handle));
  this->Handle = 0;
  this->Dirty = true;
}

//------------------------------------------------------------------------------
bool vtkShader::IsComputeShaderSupported()
{
#if defined(GL_ES_VERSION_3_0) || defined(GL_ES_VERSION_2_0)
  return false;
#else
  return GLAD_GL_ARB_compute_shader != 0;
#endif
}

//------------------------------------------------------------------------------
bool vtkShader::IsTessellationShaderSupported()
{
#if defined(GL_ES_VERSION_3_0) || defined(GL_ES_VERSION_2_0)
  return false;
#else
  return GLAD_GL_ARB_tessellation_shader != 0;
#endif
}

//------------------------------------------------------------------------------
void vtkShader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
