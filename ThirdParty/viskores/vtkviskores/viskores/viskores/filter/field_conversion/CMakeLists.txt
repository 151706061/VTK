##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

##============================================================================
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##============================================================================
set(field_conversion_headers
  CellAverage.h
  PointAverage.h
  )
set(field_conversion_sources
  CellAverage.cxx
  PointAverage.cxx
  )

viskores_library(
  NAME viskores_filter_field_conversion
  HEADERS ${field_conversion_headers}
  DEVICE_SOURCES ${field_conversion_sources}
  USE_VISKORES_JOB_POOL
)

target_link_libraries(viskores_filter PUBLIC INTERFACE viskores_filter_field_conversion)

add_subdirectory(worklet)
