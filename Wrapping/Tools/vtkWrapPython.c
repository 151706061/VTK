// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkWrapPythonClass.h"
#include "vtkWrapPythonConstant.h"
#include "vtkWrapPythonEnum.h"
#include "vtkWrapPythonMethodDef.h"
#include "vtkWrapPythonNamespace.h"

#include "vtkParseExtras.h"
#include "vtkParseMain.h"
#include "vtkParseSystem.h"
#include "vtkWrap.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* the main entry method, called by vtkParse.y */
void vtkParseOutput(FILE* fp, FileInfo* data);

/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* get the header file for the named VTK class */
static const char* vtkWrapPython_ClassHeader(const HierarchyInfo* hinfo, const char* classname);

/* get the module for the named VTK class */
static const char* vtkWrapPython_ClassModule(const HierarchyInfo* hinfo, const char* classname);

/* print out headers for any special types used by methods */
static void vtkWrapPython_GenerateSpecialHeaders(
  FILE* fp, FileInfo* file_info, const HierarchyInfo* hinfo);

/* -------------------------------------------------------------------- */
/* Get the header file for the specified class */
static const char* vtkWrapPython_ClassHeader(const HierarchyInfo* hinfo, const char* classname)
{
  HierarchyEntry* entry;

  /* if "hinfo" is present, use it to find the file */
  if (hinfo)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      return entry->HeaderFile;
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Get the module for the specified class */
static const char* vtkWrapPython_ClassModule(const HierarchyInfo* hinfo, const char* classname)
{
  HierarchyEntry* entry;

  /* if "hinfo" is present, use it to find the file */
  if (hinfo)
  {
    entry = vtkParseHierarchy_FindEntry(hinfo, classname);
    if (entry)
    {
      return entry->Module;
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* generate includes for any special types that are used */
static void vtkWrapPython_GenerateSpecialHeaders(
  FILE* fp, FileInfo* file_info, const HierarchyInfo* hinfo)
{
  const char** types;
  int numTypes = 0;
  FunctionInfo* currentFunction;
  int i, j, k, n, m, ii, nn;
  const char* classname;
  const char* ownincfile = "";
  ClassInfo* data;
  const ValueInfo* val;
  const char** includedHeaders = NULL;
  size_t nIncludedHeaders = 0;

  types = (const char**)malloc(1000 * sizeof(const char*));

  /* always include vtkVariant, it is often used as a template arg
     for templated array types, and the file_info doesn't tell us
     what types each templated class is instantiated for (that info
     might be in the .cxx files, which we cannot access here) */
  types[numTypes++] = "vtkVariant";

  nn = file_info->Contents->NumberOfClasses;
  for (ii = 0; ii < nn; ii++)
  {
    data = file_info->Contents->Classes[ii];
    n = data->NumberOfFunctions;
    for (i = 0; i < n; i++)
    {
      currentFunction = data->Functions[i];
      if (currentFunction->Access == VTK_ACCESS_PUBLIC && !currentFunction->IsExcluded &&
        strcmp(currentFunction->Class, data->Name) == 0)
      {
        m = vtkWrap_CountWrappedParameters(currentFunction);

        for (j = -1; j < m; j++)
        {
          if (j >= 0)
          {
            val = currentFunction->Parameters[j];
          }
          else
          {
            val = currentFunction->ReturnValue;
          }
          if (vtkWrap_IsVoid(val))
          {
            continue;
          }

          classname = 0;
          /* the IsScalar check is used because the wrappers don't need the
             header for objects passed via a pointer (but they need the header
             for objects passed by reference) */
          if (vtkWrap_IsString(val) && vtkWrap_IsScalar(val))
          {
            classname = val->Class;
          }
          else if (vtkWrap_IsObject(val) && vtkWrap_IsScalar(val) && !vtkWrap_IsRef(val))
          {
            classname = val->Class;
          }
          /* we already include our own header */
          if (classname && strcmp(classname, data->Name) != 0)
          {
            for (k = 0; k < numTypes; k++)
            {
              /* make a unique list of all classes found */
              if (strcmp(classname, types[k]) == 0)
              {
                break;
              }
            }

            if (k == numTypes)
            {
              if (numTypes > 0 && (numTypes % 1000) == 0)
              {
                types =
                  (const char**)realloc((char**)types, (numTypes + 1000) * sizeof(const char*));
              }
              types[numTypes++] = classname;
            }
          }
        }
      }
    }
  }

  /* get our own include file (returns NULL if hinfo is NULL) */
  data = file_info->MainClass;
  if (!data && file_info->Contents->NumberOfClasses > 0)
  {
    data = file_info->Contents->Classes[0];
  }

  if (data)
  {
    ownincfile = vtkWrapPython_ClassHeader(hinfo, data->Name);
  }

  includedHeaders = (const char**)malloc(numTypes * sizeof(const char*));

  /* for each unique type found in the file */
  for (i = 0; i < numTypes; i++)
  {
    const char* incfile;
    incfile = vtkWrapPython_ClassHeader(hinfo, types[i]);

    if (incfile)
    {
      /* make sure it hasn't been included before. */
      size_t nHeader;
      int uniqueInclude = 1;
      for (nHeader = 0; nHeader < nIncludedHeaders; ++nHeader)
      {
        if (!strcmp(incfile, includedHeaders[nHeader]))
        {
          uniqueInclude = 0;
        }
      }

      /* ignore duplicate includes. */
      if (!uniqueInclude)
      {
        continue;
      }

      includedHeaders[nIncludedHeaders] = incfile;
      ++nIncludedHeaders;

      /* make sure it doesn't share our header file */
      if (ownincfile == 0 || strcmp(incfile, ownincfile) != 0)
      {
        fprintf(fp, "#include \"%s\"\n", incfile);
      }
    }
  }

  free((char**)includedHeaders);
  includedHeaders = NULL;

  /* special case for the way vtkGenericDataArray template is used */
  if (data && strcmp(data->Name, "vtkGenericDataArray") == 0)
  {
    fprintf(fp,
      "#include \"vtkSOADataArrayTemplate.h\"\n"
      "#include \"vtkAOSDataArrayTemplate.h\"\n"
      "#ifdef VTK_USE_SCALED_SOA_ARRAYS\n"
      "#include \"vtkScaledSOADataArrayTemplate.h\"\n"
      "#endif\n");
  }
  /* special case for the way vtkGenericDataArray template is used */
  if (data && strcmp(data->Name, "vtkAlgorithm") == 0)
  {
    fprintf(fp, "#include \"vtkAlgorithmOutput.h\"\n");
    fprintf(fp, "#include \"vtkTrivialProducer.h\"\n");
    fprintf(fp, "#include \"vtkDataObject.h\"\n");
  }

  free((char**)types);
}

/* -------------------------------------------------------------------- */
/* This is the main entry point for the python wrappers.  When called,
 * it will print the vtkXXPython.c file contents to "fp".  */

int VTK_PARSE_MAIN(int argc, char* argv[])
{
  ClassInfo** wrappedClasses;
  unsigned char* wrapAsVTKObject;
  ClassInfo* data = NULL;
  NamespaceInfo* contents;
  const OptionInfo* options;
  HierarchyInfo* hinfo = NULL;
  FileInfo* file_info;
  FILE* fp;
  const char* module = "vtkCommonCore";
  const char* name;
  char* name_from_file = NULL;
  int numberOfWrappedClasses = 0;
  int numberOfWrappedNamespaces = 0;
  int wrapped_anything = 0;
  int i, j;
  size_t k, m;
  int is_vtkobject;

  /* pre-define a macro to identify the language */
  vtkParse_DefineMacro("__VTK_WRAP_PYTHON__", 0);

  /* get command-line args and parse the header file */
  file_info = vtkParse_Main(argc, argv);

  /* get the command-line options */
  options = vtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hinfo =
      vtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the output file */
  fp = vtkParse_FileOpen(options->OutputFileName, "w");
  if (!fp)
  {
    int e = errno;
    char* etext = strerror(e);
    etext = (etext ? etext : "Unknown error");
    fprintf(stderr, "Error %d opening output file %s: %s\n", e, options->OutputFileName, etext);
    return vtkParse_FinalizeMain(1);
  }

  /* get the filename without the extension */
  name = file_info->FileName;
  m = strlen(name);
  for (k = m; k > 0; k--)
  {
    if (name[k] == '.')
    {
      break;
    }
  }
  if (k > 0)
  {
    m = k;
  }
  for (k = m; k > 0; k--)
  {
    if (!((name[k - 1] >= 'a' && name[k - 1] <= 'z') ||
          (name[k - 1] >= 'A' && name[k - 1] <= 'Z') ||
          (name[k - 1] >= '0' && name[k - 1] <= '9') || name[k - 1] == '_'))
    {
      break;
    }
  }
  name_from_file = (char*)malloc(m - k + 1);
  strncpy(name_from_file, &name[k], m - k);
  name_from_file[m - k] = '\0';
  name = name_from_file;

  /* get the global namespace */
  contents = file_info->Contents;

  /* use the hierarchy file to find super classes and expand typedefs */
  if (hinfo)
  {
    for (i = 0; i < contents->NumberOfClasses; i++)
    {
      vtkWrap_MergeSuperClasses(contents->Classes[i], file_info, hinfo);
    }
    for (i = 0; i < contents->NumberOfClasses; i++)
    {
      vtkWrap_ExpandTypedefs(contents->Classes[i], file_info, hinfo);
    }
  }

  /* the VTK_WRAPPING_CXX tells header files where they're included from */
  fprintf(fp,
    "// python wrapper for %s\n//\n"
    "#define VTK_WRAPPING_CXX\n",
    name);

  /* unless this is vtkObjectBase.h, define VTK_STREAMS_FWD_ONLY */
  if (strcmp("vtkObjectBase", name) != 0)
  {
    /* Block inclusion of full streams.  */
    fprintf(fp, "#define VTK_STREAMS_FWD_ONLY\n");
  }

  /* lots of important utility functions are defined in vtkPythonArgs.h */
  fprintf(fp,
    "#include \"vtkPythonArgs.h\"\n"
    "#include \"vtkPythonOverload.h\"\n"
    "#include <cstddef>\n"
    "#include <sstream>\n");

  /* vtkPythonCommand is needed to wrap vtkObject.h */
  if (strcmp("vtkObject", name) == 0)
  {
    fprintf(fp, "#include \"vtkPythonCommand.h\"\n");
  }

  /* generate includes for any special types that are used */
  vtkWrapPython_GenerateSpecialHeaders(fp, file_info, hinfo);

  /* the header file for the wrapped class */
  fprintf(fp, "#include \"%s.h\"\n\n", name);

  /* capture the PYTHON_PACKAGE name, if defined */
  fprintf(fp,
    "#if defined(PYTHON_PACKAGE)\n"
    "#define PYTHON_PACKAGE_SCOPE PYTHON_PACKAGE \".\"\n"
    "#else\n"
    "#define PYTHON_PACKAGE_SCOPE\n"
    "#endif\n\n");

  /* do the export of the main entry point */
  fprintf(
    fp, "extern \"C\" { %s void PyVTKAddFile_%s(PyObject *dict); }\n", "VTK_ABI_HIDDEN", name);

  /* get the module that is being wrapped */
  data = file_info->MainClass;
  if (!data && file_info->Contents->NumberOfClasses > 0)
  {
    data = file_info->Contents->Classes[0];
  }
  if (data && hinfo)
  {
    module = vtkWrapPython_ClassModule(hinfo, data->Name);
  }

  /* Identify all enum types that are used by methods */
  vtkWrapPython_MarkAllEnums(file_info->Contents, hinfo);

  /* Wrap any enum types defined in the global namespace */
  for (i = 0; i < contents->NumberOfEnums; i++)
  {
    if (!contents->Enums[i]->IsExcluded)
    {
      vtkWrapPython_GenerateEnumType(fp, module, NULL, contents->Enums[i]);
    }
  }

  /* Wrap any namespaces */
  for (i = 0; i < contents->NumberOfNamespaces; i++)
  {
    if (contents->Namespaces[i]->NumberOfConstants > 0)
    {
      vtkWrapPython_WrapNamespace(fp, module, contents->Namespaces[i]);
      numberOfWrappedNamespaces++;
    }
  }

  /* Check for all special classes before any classes are wrapped */
  wrapAsVTKObject = (unsigned char*)malloc(sizeof(unsigned char) * contents->NumberOfClasses);
  for (i = 0; i < contents->NumberOfClasses; i++)
  {
    data = contents->Classes[i];

    /* guess whether type is a vtkobject */
    is_vtkobject = (data == file_info->MainClass ? 1 : 0);
    if (hinfo)
    {
      is_vtkobject = vtkWrap_IsTypeOf(hinfo, data->Name, "vtkObjectBase");
    }

    if (!is_vtkobject)
    {
      /* mark class as abstract only if it has pure virtual methods */
      /* (does not check for inherited pure virtual methods) */
      data->IsAbstract = 0;
      for (j = 0; j < data->NumberOfFunctions; j++)
      {
        const FunctionInfo* func = data->Functions[j];
        if (func && func->IsPureVirtual)
        {
          data->IsAbstract = 1;
          break;
        }
      }
    }

    wrapAsVTKObject[i] = (is_vtkobject ? 1 : 0);
  }

  /* Wrap all of the classes in the file */
  wrappedClasses = (ClassInfo**)malloc(sizeof(ClassInfo*) * contents->NumberOfClasses);
  for (i = 0; i < contents->NumberOfClasses; i++)
  {
    data = contents->Classes[i];
    if (data->IsExcluded)
    {
      continue;
    }

    is_vtkobject = wrapAsVTKObject[i];

    /* if "hinfo" is present, wrap everything, else just the main class */
    if (hinfo || data == file_info->MainClass)
    {
      if (vtkWrapPython_WrapOneClass(fp, module, data->Name, data, file_info, hinfo, is_vtkobject))
      {
        /* re-index wrapAsVTKObject for wrapped classes */
        wrapAsVTKObject[numberOfWrappedClasses] = (is_vtkobject ? 1 : 0);
        wrappedClasses[numberOfWrappedClasses++] = data;
      }
    }
  }

  /* The function for adding everything to the module dict */
  wrapped_anything = (numberOfWrappedClasses || numberOfWrappedNamespaces ||
    contents->NumberOfConstants || contents->NumberOfEnums);
  fprintf(fp,
    "void PyVTKAddFile_%s(\n"
    "  PyObject *%s)\n"
    "{\n"
    "%s",
    name, (wrapped_anything ? "dict" : " /*dict*/"), (wrapped_anything ? "  PyObject *o;\n" : ""));

  /* Add all of the namespaces */
  for (j = 0; j < contents->NumberOfNamespaces; j++)
  {
    if (contents->Namespaces[j]->NumberOfConstants > 0)
    {
      fprintf(fp,
        "  o = PyVTKNamespace_%s();\n"
        "  if (o && PyDict_SetItemString(dict, \"%s\", o) != 0)\n"
        "  {\n"
        "    Py_DECREF(o);\n"
        "  }\n"
        "\n",
        contents->Namespaces[j]->Name, contents->Namespaces[j]->Name);
    }
  }

  /* Add all of the classes that have been wrapped */
  for (i = 0; i < numberOfWrappedClasses; i++)
  {
    data = wrappedClasses[i];
    is_vtkobject = wrapAsVTKObject[i];

    if (data->Template)
    {
      /* Template generator */
      fprintf(fp,
        "  o = Py%s_TemplateNew();\n"
        "\n",
        data->Name);

      /* Add template specializations to dict */
      fprintf(fp,
        "  if (o)\n"
        "  {\n"
        "    PyObject *l = PyObject_CallMethod(o, \"values\", nullptr);\n"
        "    Py_ssize_t n = PyList_Size(l);\n"
        "    for (Py_ssize_t i = 0; i < n; i++)\n"
        "    {\n"
        "      PyObject *ot = PyList_GetItem(l, i);\n"
        "      const char *nt = nullptr;\n"
        "      if (PyType_Check(ot))\n"
        "      {\n"
        "        nt = vtkPythonUtil::GetTypeName((PyTypeObject *)ot);\n"
        "      }\n"
        "      if (nt)\n"
        "      {\n"
        "        nt = vtkPythonUtil::StripModule(nt);\n"
        "        PyDict_SetItemString(dict, nt, ot);\n"
        "      }\n"
        "    }\n"
        "    Py_DECREF(l);\n"
        "  }\n"
        "\n");
    }
    else if (is_vtkobject)
    {
      /* Class is derived from vtkObjectBase */
      fprintf(fp,
        "  o = Py%s_ClassNew();\n"
        "\n",
        data->Name);
    }
    else
    {
      /* Classes that are not derived from vtkObjectBase */
      fprintf(fp,
        "  o = Py%s_TypeNew();\n"
        "\n",
        data->Name);
    }

    fprintf(fp,
      "  if (o && PyDict_SetItemString(dict, \"%s\", o) != 0)\n"
      "  {\n"
      "    Py_DECREF(o);\n"
      "  }\n"
      "\n",
      data->Name);
  }

  /* add any enum types defined in the file */
  vtkWrapPython_AddPublicEnumTypes(fp, "  ", "dict", "o", contents);

  /* add any constants defined in the file */
  vtkWrapPython_AddPublicConstants(fp, "  ", "dict", "o", contents);

  /* close the AddFile function */
  fprintf(fp, "}\n\n");

  fclose(fp);

  free(name_from_file);
  free(wrapAsVTKObject);
  free(wrappedClasses);

  if (hinfo)
  {
    vtkParseHierarchy_Free(hinfo);
  }

  vtkParse_Free(file_info);

  if (!wrapped_anything)
  {
    vtkWrap_WarnEmpty(options);
  }

  return vtkParse_FinalizeMain(0);
}
