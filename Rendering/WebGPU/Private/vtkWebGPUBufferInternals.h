// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkWebGPUBufferInternals_h
#define vtkWebGPUBufferInternals_h

#include "vtkRenderingWebGPUModule.h"
#include "vtk_wgpu.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGWEBGPU_NO_EXPORT vtkWebGPUBufferInternals
{
public:
  static wgpu::Buffer Upload(const wgpu::Device& device, unsigned long offset, void* data,
    unsigned long sizeBytes, wgpu::BufferUsage usage, const char* label = nullptr);

  static wgpu::Buffer CreateABuffer(const wgpu::Device& device, unsigned long sizeBytes,
    wgpu::BufferUsage usage, bool mappedAtCreation = false, const char* label = nullptr);
};
VTK_ABI_NAMESPACE_END

#endif
// VTK-HeaderTest-Exclude: vtkWebGPUBufferInternals.h
