// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLFileOutputWindow.h"
#include "vtkObjectFactory.h"
#include "vtksys/Encoding.hxx"
#include "vtksys/FStream.hxx"

#include <mutex>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLFileOutputWindow);

static std::mutex vtkXMLFileOutputWindowMutex;

void vtkXMLFileOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkXMLFileOutputWindow::Initialize()
{
  if (!this->OStream)
  {
    if (!this->FileName)
    {
      const char fileName[] = "vtkMessageLog.xml";
      this->FileName = new char[strlen(fileName) + 1];
      strcpy(this->FileName, fileName);
    }

    this->OStream = new vtksys::ofstream(this->FileName, this->Append ? ios::app : ios::out);
    if (!this->Append)
    {
      this->DisplayTag("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>");
    }
  }
}

void vtkXMLFileOutputWindow::DisplayTag(const char* text)
{
  if (!text)
  {
    return;
  }

  if (!this->OStream)
  {
    this->Initialize();
  }
  *this->OStream << text << endl;

  if (this->Flush)
  {
    this->OStream->flush();
  }
}

// Description:
// Process text to replace XML special characters with escape sequences
void vtkXMLFileOutputWindow::DisplayXML(const char* tag, const char* text)
{
  char* xmlText;

  if (!text)
  {
    return;
  }

  // allocate enough room for the worst case
  xmlText = new char[strlen(text) * 6 + 1];

  const char* s = text;
  char* x = xmlText;
  *x = '\0';

  // replace all special characters
  while (*s)
  {
    switch (*s)
    {
      case '&':
        strcat(x, "&amp;");
        x += 5;
        break;
      case '"':
        strcat(x, "&quot;");
        x += 6;
        break;
      case '\'':
        strcat(x, "&apos;");
        x += 6;
        break;
      case '<':
        strcat(x, "&lt;");
        x += 4;
        break;
      case '>':
        strcat(x, "&gt;");
        x += 4;
        break;
      default:
        *x = *s;
        x++;
        *x = '\0'; // explicitly terminate the new string
    }
    s++;
  }

  if (!this->OStream)
  {
    this->Initialize();
  }
  *this->OStream << "<" << tag << ">" << xmlText << "</" << tag << ">" << endl;

  if (this->Flush)
  {
    this->OStream->flush();
  }
  delete[] xmlText;
}

void vtkXMLFileOutputWindow::DisplayText(const char* text)
{
  std::lock_guard<std::mutex> lock(vtkXMLFileOutputWindowMutex);
  this->DisplayXML("Text", text);
}

void vtkXMLFileOutputWindow::DisplayErrorText(const char* text)
{
  std::lock_guard<std::mutex> lock(vtkXMLFileOutputWindowMutex);
  this->DisplayXML("Error", text);
}

void vtkXMLFileOutputWindow::DisplayWarningText(const char* text)
{
  std::lock_guard<std::mutex> lock(vtkXMLFileOutputWindowMutex);
  this->DisplayXML("Warning", text);
}

void vtkXMLFileOutputWindow::DisplayGenericWarningText(const char* text)
{
  std::lock_guard<std::mutex> lock(vtkXMLFileOutputWindowMutex);
  this->DisplayXML("GenericWarning", text);
}

void vtkXMLFileOutputWindow::DisplayDebugText(const char* text)
{
  std::lock_guard<std::mutex> lock(vtkXMLFileOutputWindowMutex);
  this->DisplayXML("Debug", text);
}
VTK_ABI_NAMESPACE_END
