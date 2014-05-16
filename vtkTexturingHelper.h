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
#include "vtkTexture.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"

// A class useful for correctly displaying multi-textured objects (i.e. that are textured by different image files),
//	since VTK doesn't do it by default. The basic use of it should be to just specify a 3D object file (e.g. a .obj file)
//	and some texture files (e.g. .jpg's or .png's), and directly retrieve a correctly-textured actor to add to a renderer.

//	For the moment it only works with vtkOBJReader and vtkjPEGReader, but its design should easily allow using others
//	VTK importers (VRML, PLY, X3D for geometry, PNG for images...) in the future.
class	vtkTexturingHelper
{
public:
	// Ctors/dtors
	vtkTexturingHelper();
	~vtkTexturingHelper();

	// Textures methods
	void addTextureFile(const std::string & file);
	void associateTextureFiles(const std::string & rootName, const std::string & extension, int numberOfFiles);
	void clearTexturesList();
	void convertImagesToTextures();
	void applyTextures();
	void configureTexture(int);
	void readjPEGTexture(int);

	// Geometry methods
	void readGeometryFile(const std::string & filename="");
	void readOBJFile(const std::string & filename);

	// Get/Setters

	// Returns the textured polydata's actor. It is to the caller to delete it
	// (by calling Delete or wrapping it in a smart pointer).
	vtkActor* getActor() const;
	vtkPolyData* getPolyData() const;
	void setPolyData(vtkPolyData *polyData);
	// Specifies the geometry file to use.
	// Needs to be called before readGeometryFile() (unless you specify the name to readGeometryFile)
	void setGeometryFile(const std::string & file);

private:
	typedef void (vtkTexturingHelper::*geoReaderFunction)();
	typedef void (vtkTexturingHelper::*imgReaderFunction)(int);

	void geoReadOBJ();
	void retrieveOBJFileTCoords();
	void insertNewTCoordsArray();

	std::vector<std::string> texturesFiles_;
	std::vector<vtkSmartPointer<vtkFloatArray> > TCoordsArrays_;
	std::vector<vtkSmartPointer<vtkTexture> > textures_;
	std::map<std::string, geoReaderFunction> geoExtensionsMap_;
	std::map<std::string, imgReaderFunction> imgExtensionsMap_;
	unsigned short TCoordsCount_;
	std::string geoFile_;
	vtkPolyData* thePolyData_;
	vtkActor* theActor_;
};

#endif
