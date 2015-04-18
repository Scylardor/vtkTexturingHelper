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

#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkImageReader2.h"
#include "vtkTexture.h"
#include "vtkFloatArray.h"
#include "vtkSmartPointer.h"


class vtkTexturingHelperException : public std::exception {
public:
	vtkTexturingHelperException(const char * _errMsg) : m_errMsg(_errMsg) {}
	~vtkTexturingHelperException() throw() {}
	const char * what() const throw() { return m_errMsg; }

private:
	const char * m_errMsg;
};

/* vtkTexturingHelper */
// A helper class to let you render multi-textured objects in VTK
// Currently supported geometry file formats: OBJ
// Currently supported texture file formats: every image format known to vtkImageReader2Factory
class vtkTexturingHelper {
public:
	vtkTexturingHelper();
	~vtkTexturingHelper();

	// Accessors

	vtkSmartPointer<vtkActor> GetActor() const;
	vtkSmartPointer<vtkPolyData> GetPolyData() const;


	// Modifiers

	void	Clear();

	// Textures
	void	ReadTextureFile(const std::string & _fileName);
	void	ReadTextureFiles(const std::string & _prefix, const std::string & _extension, int _numberOfFiles);
	void	ApplyTextures();
	void	ClearTexturesList();
	vtkSmartPointer<vtkImageReader2> GetImageReaderForFile(const std::string & _fileName) const;

	// Geometry
	void	ReadGeometryFile(const std::string & _filename);
	void	ClearTCoordsList();

private:
	typedef	void (vtkTexturingHelper::*geoReaderFunction)();

	// Geometry
	void	SetGeometryFile(const std::string & _file);
	void	GeoReadOBJ();
	void	RetrieveOBJFileTCoords();
	void	InsertNewTCoordsArray();

	// Textures
	void	ConvertImageToTexture(vtkSmartPointer<vtkImageReader2>  _imageReader);
	int		GetNumberOfSupportedTextureUnits() const;

	std::vector<vtkSmartPointer<vtkFloatArray> > m_TCoordsArrays;
	std::vector<vtkSmartPointer<vtkTexture> > m_textures;
	std::map<std::string, geoReaderFunction> m_geoExtensionsMap;
	unsigned short m_TCoordsCount;
	std::string m_geoFile;
	vtkSmartPointer<vtkPolyData> m_polyData;
	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkSmartPointer<vtkActor> m_actor;
};

#endif
