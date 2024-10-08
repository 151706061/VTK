// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAttributeDataToTableFilter.h"
#include "vtkBitArray.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkDataRepresentation.h"
#include "vtkDelimitedTextReader.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraph.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkIntArray.h"
#include "vtkLabeledDataMapper.h"
#include "vtkMergeTables.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkStringArray.h"
#include "vtkStringToCategory.h"
#include "vtkTable.h"
#include "vtkTableToGraph.h"
#include "vtkTestUtilities.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkUndirectedGraph.h"
#include "vtkVariant.h"

// Uncomment the following line to show Qt tables of
// the vertex and edge tables.
// #define SHOW_QT_DATA_TABLES 1

#ifdef SHOW_QT_DATA_TABLES
#include "vtkQtTableView.h"
#include <QApplication>
#endif

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void TestTableToGraphRender(vtkRenderer* ren, vtkGraphAlgorithm* alg, int test, int cols,
  const char* labelArray, bool circular)
{
  double distance = circular ? 2.5 : 100.0;
  double xoffset = (test % cols) * distance;
  double yoffset = -(test / cols) * distance;

  VTK_CREATE(vtkStringToCategory, cat);
  cat->SetInputConnection(alg->GetOutputPort());
  cat->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "domain");

  cat->Update();
  vtkUndirectedGraph* output = vtkUndirectedGraph::SafeDownCast(cat->GetOutput());
  VTK_CREATE(vtkUndirectedGraph, graph);
  graph->DeepCopy(output);

  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputData(graph);
  if (circular)
  {
    VTK_CREATE(vtkCircularLayoutStrategy, strategy);
    layout->SetLayoutStrategy(strategy);
  }
  else
  {
    VTK_CREATE(vtkSimple2DLayoutStrategy, strategy);
    strategy->SetMaxNumberOfIterations(10);
    layout->SetLayoutStrategy(strategy);
  }

  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(layout->GetOutputPort());

  VTK_CREATE(vtkGlyphSource2D, glyph);
  glyph->SetGlyphTypeToVertex();
  VTK_CREATE(vtkGlyph3D, vertexGlyph);
  vertexGlyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  vertexGlyph->SetInputConnection(1, glyph->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, vertexMapper);
  vertexMapper->SetInputConnection(vertexGlyph->GetOutputPort());
  vertexMapper->SetScalarModeToUsePointFieldData();
  vertexMapper->SelectColorArray("category");
  double rng[2] = { 0, 0 };
  graph->GetVertexData()->GetArray("category")->GetRange(rng);
  cerr << rng[0] << "," << rng[1] << endl;
  vertexMapper->SetScalarRange(rng[0], rng[1]);
  VTK_CREATE(vtkActor, vertexActor);
  vertexActor->SetMapper(vertexMapper);
  vertexActor->GetProperty()->SetPointSize(7.0);
  vertexActor->GetProperty()->SetColor(0.7, 0.7, 0.7);
  vertexActor->SetPosition(xoffset, yoffset, 0.001);

  VTK_CREATE(vtkPolyDataMapper, edgeMapper);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  edgeMapper->ScalarVisibilityOff();
  VTK_CREATE(vtkActor, edgeActor);
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(0.6, 0.6, 0.6);
  edgeActor->SetPosition(xoffset, yoffset, 0);

  if (labelArray)
  {
    VTK_CREATE(vtkLabeledDataMapper, labelMapper);
    labelMapper->SetInputConnection(graphToPoly->GetOutputPort());
    labelMapper->SetLabelModeToLabelFieldData();
    labelMapper->SetFieldDataName(labelArray);
    labelMapper->GetLabelTextProperty()->SetColor(0, 0, 0);
    labelMapper->GetLabelTextProperty()->SetShadow(0);
    VTK_CREATE(vtkTransform, translate);
    translate->Translate(xoffset, yoffset, 0);
    labelMapper->SetTransform(translate);
    VTK_CREATE(vtkActor2D, labelActor);
    labelActor->SetMapper(labelMapper);
    ren->AddActor(labelActor);
  }

  ren->AddActor(vertexActor);
  ren->AddActor(edgeActor);
}

int TestTableToGraph(int argc, char* argv[])
{
#ifdef SHOW_QT_DATA_TABLES
  QApplication app(argc, argv);
#endif

  const char* label = nullptr;
  bool circular = true;
  for (int a = 1; a < argc; a++)
  {
    if (!strcmp(argv[a], "-L"))
    {
      label = "label";
    }
    if (!strcmp(argv[a], "-F"))
    {
      circular = false;
    }
  }

  // Read edge table from a file.
  char* file =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/Infovis/authors-tabletographtest.csv");

  VTK_CREATE(vtkDelimitedTextReader, reader);
  reader->SetFileName(file);
  delete[] file;
  reader->SetHaveHeaders(true);

  // Create a simple person table.
  VTK_CREATE(vtkTable, personTable);
  VTK_CREATE(vtkStringArray, nameArr);
  nameArr->SetName("name");
  VTK_CREATE(vtkStringArray, petArr);
  petArr->SetName("pet");
  nameArr->InsertNextValue("Biff");
  petArr->InsertNextValue("cat");
  nameArr->InsertNextValue("Bob");
  petArr->InsertNextValue("bird");
  nameArr->InsertNextValue("Baz");
  petArr->InsertNextValue("dog");
  nameArr->InsertNextValue("Bippity");
  petArr->InsertNextValue("lizard");
  nameArr->InsertNextValue("Boppity");
  petArr->InsertNextValue("chinchilla");
  nameArr->InsertNextValue("Boo");
  petArr->InsertNextValue("rabbit");
  personTable->AddColumn(nameArr);
  personTable->AddColumn(petArr);

  // Insert rows for organizations
  VTK_CREATE(vtkTable, orgTable);
  VTK_CREATE(vtkStringArray, orgNameArr);
  orgNameArr->SetName("name");
  VTK_CREATE(vtkIntArray, sizeArr);
  sizeArr->SetName("size");
  orgNameArr->InsertNextValue("NASA");
  sizeArr->InsertNextValue(10000);
  orgNameArr->InsertNextValue("Bob's Supermarket");
  sizeArr->InsertNextValue(100);
  orgNameArr->InsertNextValue("Oil Changes 'R' Us");
  sizeArr->InsertNextValue(20);
  orgTable->AddColumn(orgNameArr);
  orgTable->AddColumn(sizeArr);

  // Merge the two tables
  VTK_CREATE(vtkMergeTables, merge);
  merge->SetInputData(0, personTable);
  merge->SetFirstTablePrefix("person.");
  merge->SetInputData(1, orgTable);
  merge->SetSecondTablePrefix("organization.");
  merge->MergeColumnsByNameOff();
  merge->PrefixAllButMergedOn();

  // Create the renderer.
  VTK_CREATE(vtkRenderer, ren);

  // Create table to graph filter with edge and vertex table inputs
  VTK_CREATE(vtkTableToGraph, tableToGraph);
  tableToGraph->SetInputConnection(0, reader->GetOutputPort());

  int cols = 3;
  int test = 0;

  // Path
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Boss", "person");
  tableToGraph->AddLinkVertex("Affiliation", "organization");
  tableToGraph->AddLinkVertex("Alma Mater", "school");
  tableToGraph->AddLinkVertex("Categories", "interest");
  tableToGraph->AddLinkEdge("Author", "Boss");
  tableToGraph->AddLinkEdge("Boss", "Affiliation");
  tableToGraph->AddLinkEdge("Affiliation", "Alma Mater");
  tableToGraph->AddLinkEdge("Alma Mater", "Categories");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Star
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Boss", "person");
  tableToGraph->AddLinkVertex("Affiliation", "organization");
  tableToGraph->AddLinkVertex("Alma Mater", "school");
  tableToGraph->AddLinkVertex("Categories", "interest");
  tableToGraph->AddLinkEdge("Author", "Boss");
  tableToGraph->AddLinkEdge("Author", "Affiliation");
  tableToGraph->AddLinkEdge("Author", "Alma Mater");
  tableToGraph->AddLinkEdge("Author", "Categories");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Affiliation
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Affiliation", "organization");
  tableToGraph->AddLinkEdge("Author", "Affiliation");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Group by affiliation (hide affiliation)
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person", 0);
  tableToGraph->AddLinkVertex("Affiliation", "organization", 1);
  tableToGraph->AddLinkEdge("Author", "Affiliation");
  tableToGraph->AddLinkEdge("Affiliation", "Author");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Boss
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Boss", "person");
  tableToGraph->AddLinkEdge("Author", "Boss");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Boss in different domain
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person");
  tableToGraph->AddLinkVertex("Boss", "boss");
  tableToGraph->AddLinkEdge("Author", "Boss");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Use simple linking of column path
  tableToGraph->ClearLinkVertices();
  VTK_CREATE(vtkStringArray, pathColumn);
  VTK_CREATE(vtkStringArray, pathDomain);
  VTK_CREATE(vtkBitArray, pathHidden);
  pathColumn->InsertNextValue("Author");
  pathHidden->InsertNextValue(0);
  pathColumn->InsertNextValue("Boss");
  pathHidden->InsertNextValue(0);
  pathColumn->InsertNextValue("Affiliation");
  pathHidden->InsertNextValue(0);
  pathColumn->InsertNextValue("Alma Mater");
  pathHidden->InsertNextValue(0);
  pathColumn->InsertNextValue("Categories");
  pathHidden->InsertNextValue(0);
  // Set domains to equal column names, except put Author and Boss
  // in the same domain.
  pathDomain->DeepCopy(pathColumn);
  pathDomain->SetValue(0, "person");
  pathDomain->SetValue(1, "person");
  tableToGraph->LinkColumnPath(pathColumn, pathDomain, pathHidden);
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  // Use a vertex table.
  tableToGraph->SetInputConnection(1, merge->GetOutputPort());
  tableToGraph->ClearLinkVertices();
  tableToGraph->AddLinkVertex("Author", "person.name", 0);
  tableToGraph->AddLinkVertex("Affiliation", "organization.name", 0);
  tableToGraph->AddLinkEdge("Author", "Affiliation");
  TestTableToGraphRender(ren, tableToGraph, test++, cols, label, circular);

  VTK_CREATE(vtkRenderWindow, win);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(win);
  win->AddRenderer(ren);
  ren->SetBackground(1, 1, 1);

  //  VTK_CREATE(vtkGraphLayoutView, view);
  //  view->SetupRenderWindow(win);
  //  view->SetRepresentationFromInputConnection(tableToGraph->GetOutputPort());
  //  view->SetVertexLabelArrayName("label");
  //  view->VertexLabelVisibilityOn();
  //  view->SetLayoutStrategyToCircular();
  //  view->Update();
  //  view->GetRenderer()->ResetCamera();
  //  view->Update();

#ifdef SHOW_QT_DATA_TABLES
  VTK_CREATE(vtkQtTableView, mergeView);
  mergeView->SetRepresentationFromInputConnection(merge->GetOutputPort());
  mergeView->GetWidget()->show();

  VTK_CREATE(vtkAttributeDataToTableFilter, vertToTable);
  vertToTable->SetInputConnection(tableToGraph->GetOutputPort());
  vertToTable->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
  VTK_CREATE(vtkQtTableView, vertView);
  vertView->SetRepresentationFromInputConnection(vertToTable->GetOutputPort());
  vertView->GetWidget()->show();
  vertView->Update();

  VTK_CREATE(vtkAttributeDataToTableFilter, edgeToTable);
  edgeToTable->SetInputConnection(tableToGraph->GetOutputPort());
  edgeToTable->SetFieldAssocitation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  VTK_CREATE(vtkQtTableView, edgeView);
  edgeView->SetRepresentationFromInputConnection(edgeToTable->GetOutputPort());
  edgeView->GetWidget()->show();
#endif

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
#ifdef SHOW_QT_DATA_TABLES
    QApplication::exec();
#else
    iren->Initialize();
    iren->Start();
#endif

    retVal = vtkRegressionTester::PASSED;
  }

  return !retVal;
}
