set(Module_SRCS
  vtkBMPReader.cxx
  vtkBMPWriter.cxx
  vtkDEMReader.cxx
  vtkDICOMImageReader.cxx
  vtkGESignaReader.cxx
  vtkImageExport.cxx
  vtkImageImport.cxx
  vtkImageImportExecutive.cxx
  vtkImageReader2Collection.cxx
  vtkImageReader2.cxx
  vtkImageReader2Factory.cxx
  vtkImageReader.cxx
  vtkImageWriter.cxx
  vtkJPEGReader.cxx
  vtkJPEGWriter.cxx
  vtkMedicalImageProperties.cxx
  vtkMedicalImageReader2.cxx
  vtkMetaImageReader.cxx
  vtkMetaImageWriter.cxx
  vtkPNGReader.cxx
  vtkPNGWriter.cxx
  vtkPNMReader.cxx
  vtkPNMWriter.cxx
  vtkPostScriptWriter.cxx
  vtkSLCReader.cxx
  vtkTIFFReader.cxx
  vtkTIFFWriter.cxx
  vtkVolume16Reader.cxx
  vtkVolumeReader.cxx
  vtkNrrdReader.cxx
  )

set_source_files_properties(
  vtkVolumeReader
  ABSTRACT
  )

vtk_module_library(vtkIOImage ${Module_SRCS})
