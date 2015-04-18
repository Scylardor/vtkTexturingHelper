// The MIT License (MIT)

// Copyright (c) 2014 Alexandre Baron (Scylardor)

//   Permission is hereby granted, free of charge, to any person obtaining a copy
//   of this software and associated documentation files (the "Software"), to deal
//   in the Software without restriction, including without limitation the rights
//   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//   copies of the Software, and to permit persons to whom the Software is
//   furnished to do so, subject to the following conditions:

//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef	VTK_TEXTURING_HELPER_H
#define VTK_TEXTURING_HELPER_H

#include <vector>
#include <map>
#include <string>
#include <sstream>

#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkTexture.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"


/* vtkTexturingHelper */
// A helper class to let you render multi-textured objects in VTK
// Currently supported geometry file formats: OBJ
// Currently supported texture file formats: JPEG
class vtkTexturingHelper {
public:
	// Ctors/dtors
	vtkTexturingHelper();
	~vtkTexturingHelper();

	// Textures methods
	void addTextureFile(const std::string & _file);
	void associateTextureFiles(const std::string & _rootName, const std::string & _extension, int _numberOfFiles);
	void clearTexturesList();
	void convertImagesToTextures();
	void applyTextures();
	void configureTexture(int _nbTextures);
	void readjPEGTexture(int _texIndex);

	// Geometry methods
	void readGeometryFile(const std::string & _filename="");
	void readOBJFile(const std::string & _filename);

	// Get/Setters

	// Returns the textured polydata's actor. It is to the caller to delete it
	// (by calling Delete or wrapping it in a smart pointer).
	vtkActor* getActor() const;
	vtkPolyData* getPolyData() const;
	void setPolyData(vtkPolyData * _polyData);
	// Specifies the geometry file to use.
	// Needs to be called before readGeometryFile() (unless you specify the name to readGeometryFile)
	void setGeometryFile(const std::string & _file);

private:
	typedef void (vtkTexturingHelper::*geoReaderFunction)();
	typedef void (vtkTexturingHelper::*imgReaderFunction)(int);

	void geoReadOBJ();
	void retrieveOBJFileTCoords();
	void insertNewTCoordsArray();

	std::vector<std::string> m_texturesFiles;
	std::vector<vtkSmartPointer<vtkFloatArray> > m_TCoordsArrays;
	std::vector<vtkSmartPointer<vtkTexture> > m_textures;
	std::map<std::string, geoReaderFunction> m_geoExtensionsMap;
	std::map<std::string, imgReaderFunction> m_imgExtensionsMap;
	unsigned short m_TCoordsCount;
	std::string m_geoFile;
	vtkPolyData* m_polyData;
	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkActor* m_actor;

};

#endif
