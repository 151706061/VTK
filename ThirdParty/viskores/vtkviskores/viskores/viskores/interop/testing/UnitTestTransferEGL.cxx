//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <GL/glew.h>
#include <viskores/interop/testing/TestingOpenGLInterop.h>
#include <viskores/rendering/CanvasEGL.h>

//This sets up testing with the default device adapter and array container
#include <viskores/cont/serial/DeviceAdapterSerial.h>

int UnitTestTransferEGL(int argc, char* argv[])
{
  viskores::cont::Initialize(argc, argv);

  //get egl canvas to construct a context for us
  viskores::rendering::CanvasEGL canvas(1024, 1024);

  //get glew to bind all the opengl functions
  glewInit();

  return viskores::interop::testing::TestingOpenGLInterop<
    viskores::cont::DeviceAdapterTagSerial>::Run();
}
