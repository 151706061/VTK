vtk_module_test_data(
  Data/csvWithBOM.csv
  Data/Infovis/DimacsGraphs/,REGEX:.*
  Data/Infovis/XML/example_phylo.xml
  Data/Infovis/clustered-graph.tlp
  Data/Infovis/eg1.ris
  Data/Infovis/eg2.isi
  Data/Infovis/multi_tree.tre
  Data/Infovis/otu_table.biom
  Data/Infovis/rep_set.tre
  Data/Infovis/small.tlp
  Data/authors.csv
  Data/delimited.txt
  Data/delimited2.txt
  Data/delimited2UTF16.txt
  Data/delimited2UTF16BE.txt
  Data/delimited2UTF16LE.txt
  Data/delimitedUTF16.txt
  Data/delimitedUTF16BE.txt
  Data/delimitedUTF16LE.txt
  Data/fixedwidth.txt
  Data/half_sphere_commented.csv
  Data/vehicle_data.csv)

if (NOT vtk_testing_cxx_disabled)
  add_subdirectory(Cxx)
endif ()

if (VTK_WRAP_PYTHON)
  add_subdirectory(Python)
endif ()
