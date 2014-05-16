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

#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkJPEGReader.h"
#include "vtkOBJReader.h"
#include "vtkProperty.h"
#include "vtkOpenGLHardwareSupport.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkTexturingHelper.h"

vtkTexturingHelper::vtkTexturingHelper()
{
  this->thePolyData_ = NULL;
  this->theActor_ = vtkActor::New();
  this->geoExtensionsMap_[".obj"] = &vtkTexturingHelper::geoReadOBJ;
  this->imgExtensionsMap_[".jpg"] = &vtkTexturingHelper::readjPEGTexture;
  this->TCoordsCount_ = 0;
}


vtkTexturingHelper::~vtkTexturingHelper() {}


void vtkTexturingHelper::readjPEGTexture(int index)
{
  vtkSmartPointer<vtkJPEGReader> jPEGReader = vtkSmartPointer<vtkJPEGReader>::New();
  vtkSmartPointer<vtkTexture> newTexture = vtkSmartPointer<vtkTexture>::New();

  jPEGReader->SetFileName(this->texturesFiles_[index].c_str());
  jPEGReader->Update();
  newTexture->SetInputConnection(jPEGReader->GetOutputPort());
  this->textures_.push_back(newTexture);
}


void vtkTexturingHelper::configureTexture(int texIndex)
{
  if (texIndex == 0) {
    this->textures_[texIndex]->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE);
  } else {
    this->textures_[texIndex]->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
  }
}


void vtkTexturingHelper::convertImagesToTextures()
{
  int texIndex;

  for (texIndex = 0; texIndex != this->texturesFiles_.size(); texIndex++)
    {
      std::string	ext = this->texturesFiles_[texIndex].substr(this->texturesFiles_[texIndex].find_last_of("."));

      if (this->imgExtensionsMap_.find(ext) != this->imgExtensionsMap_.end()) {
	(this->*imgExtensionsMap_[ext])(texIndex);
	this->configureTexture(texIndex);
      } else {
	std::cerr << "vtkTexturingHelper: " << ext << ": Unknown image extension" << std::endl;
      }
    }
}


void vtkTexturingHelper::applyTextures()
{
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

  this->convertImagesToTextures();
#if VTK_MAJOR_VERSION < 6
  mapper->SetInput(this->thePolyData_);
#else
  mapper->SetInputData(this->thePolyData_);
#endif
  int tu; // number of texture units
  vtkRenderWindow* tmp = vtkRenderWindow::New();
  vtkOpenGLHardwareSupport* hardware = vtkOpenGLRenderWindow::SafeDownCast(tmp)->GetHardwareSupport();

  tu = 0;
  if (hardware->GetSupportsMultiTexturing())
    {
      tu = hardware->GetNumberOfFixedTextureUnits();
    }
  if (tu >= 2)
    {
      int textureUnit = vtkProperty::VTK_TEXTURE_UNIT_0;

      for (unsigned int texNumber = 0; texNumber <= tu && texNumber < this->TCoordsArrays_.size(); texNumber++) {
	mapper->MapDataArrayToMultiTextureAttribute(
						    textureUnit, this->TCoordsArrays_[texNumber]->GetName(),
						    vtkDataObject::FIELD_ASSOCIATION_POINTS);
	this->thePolyData_->GetPointData()->AddArray(this->TCoordsArrays_[texNumber]);
	textureUnit++;
      }
      textureUnit = vtkProperty::VTK_TEXTURE_UNIT_0;
      for (unsigned int texNumber = 0; texNumber < this->TCoordsArrays_.size(); texNumber++) {
	this->theActor_->GetProperty()->SetTexture(textureUnit, this->textures_[texNumber]);
	textureUnit++;
      }
    }
  else
    {
      // no multitexturing just show one texture.
      this->theActor_->SetTexture(this->textures_[0]);
      std::cerr << "vtkTexturingHelper: Warning, multi-texturing isn't supported - falling back to mono-texturing" << std::endl;
    }
  this->theActor_->SetMapper(mapper);
  tmp->Delete();
}


void vtkTexturingHelper::insertNewTCoordsArray()
{
  vtkSmartPointer<vtkFloatArray> newTCoords = vtkSmartPointer<vtkFloatArray>::New();
  std::stringstream ss;
  std::string arrayName;
  unsigned int tuplesNumber;

  ss << this->TCoordsCount_;
  arrayName = "TCoords" + ss.str();
  newTCoords->SetName(arrayName.c_str());
  newTCoords->SetNumberOfComponents(2);
  this->TCoordsCount_++;
  tuplesNumber = static_cast<unsigned int>(this->thePolyData_->GetPointData()->GetNumberOfTuples());
  for (unsigned int pointNbr = 0; pointNbr < tuplesNumber; pointNbr++) {
    newTCoords->InsertNextTuple2(-1.0, -1.0);
  }
  this->TCoordsArrays_.push_back(newTCoords);
}

// Method called after we retrieved the polyData from .obj file with vtkOBJReader.
// VTK doesn't make the differences between the texture coordinates of each image: it just stores everything in one big array.
// So we have to re-read the file by ourselves, to know how many TCoords points are allocated to each image, and then
// create as many smaller arrays than necessary, filling them with the right values at the right place, putting "-1"'s on all the
// points indexes that aren't supposed to be covered by a texture.
// This process also assumes the files are in the right order (i.e. the order in which this->texturesFiles_ has been filled matches
// the order in which the images' texture coordinates are given in the .obj file).
// This may change in the future if this class implements the .mtl file parsing (which would even avoid us to have to specify the
// image files' names manually).
void vtkTexturingHelper::retrieveOBJFileTCoords()
{
  std::string prevWord = "";
  std::ifstream	objFile;
  std::string line;
  unsigned int texPointsCount = 0;

  objFile.open(this->geoFile_.c_str());
  if (!objFile.is_open())
    {
      std::cerr << "vtkTexturingHelper: Unable to open OBJ file " << this->geoFile_ << std::endl;
    }
  while ( objFile.good() )
    {
      std::stringstream	ss;
      std::string  word;
      float x, y;

      getline(objFile, line);
      ss << line;
      ss >> word >> x >> y; // assumes the line is well made (in case of a "vt line" it should be like "vt 42.84 42.84")
      if (word == "vt") {
	if (prevWord != "vt") {
	  insertNewTCoordsArray(); // if we find a vertex texture line and it's the first of a row: create a new TCoords array
	}
	int arrayNbr;
	// In a Obj file, texture coordinates are listed by texture, so we want to put it in the TCoords array of the right texture
	// but not in the array of other textures. But every TCoords array musts also have the same number of points than the mesh!
	// So to do this we append the coordinates to the concerned TCoords array (the last one created)
	// and put (-1.0, -1.0) in the others instead.
	for (arrayNbr = 0; arrayNbr != this->TCoordsArrays_.size() - 1; arrayNbr++) {
	  this->TCoordsArrays_[arrayNbr]->SetTuple2(texPointsCount, -1.0, -1.0);
	}
	this->TCoordsArrays_[arrayNbr]->SetTuple2(texPointsCount, x, y);
	texPointsCount++;
      }
      prevWord = word;
    }
  objFile.close();
}

// Method to obtain a PolyData out of a Wavefront OBJ file
void vtkTexturingHelper::readOBJFile(const std::string & filename)
{
  this->setGeometryFile(filename);
  this->geoReadOBJ();

  vtkOBJReader* reader = vtkOBJReader::New();

  reader->SetFileName(this->geoFile_.c_str());
  reader->Update();
  this->thePolyData_ = reader->GetOutput();
  this->retrieveOBJFileTCoords();
}

// Method to obtain a PolyData out of a Wavefront OBJ file (internal)
void vtkTexturingHelper::geoReadOBJ()
{
  vtkOBJReader* reader = vtkOBJReader::New();

  reader->SetFileName(this->geoFile_.c_str());
  reader->Update();
  this->thePolyData_ = reader->GetOutput();
  this->retrieveOBJFileTCoords();
}

// Allows to manually specify the texture file names.
// It can be useful when texture files names don't respect the "name_imageNumber.ext" convention.
// However, it is important to specify the textures in the same order than they are described in the geometry file !
// The results will otherwise be erroneous.
void vtkTexturingHelper::addTextureFile(const std::string & imageFile)
{
  this->texturesFiles_.push_back(imageFile);
}

// This method has been created with Artec3D scans examples in mind.
// In most of these examples, a geometry file called "geo.ext" will be accompanied by multiple texture files
// which will, logically, be called "geo_0.jpg", "geo_1.jpg", "geo_2.jpg", etc. in their order of appearance when describing
// the vertices textures coordinates in the geometry file (OBJ, PLY, STL...).
// This function permits to deal with those cases easily.
// e.g.: using ("sasha", ".jpg", 3) as parameters will make it add "sasha_0.jpg", "sasha_1.jpg", and "sasha_2.jpg" to the
// list of the texture files to use.
// This is a filename convention since you usually have a geometry file (like "sasha.obj"), and its texture files with the same name,
// an underscore and a number appended to it.
// This convenience function takes the assumption that the geometry file respects this logical order of appearance in the geometry file,
// like Artec3D examples do.
void vtkTexturingHelper::associateTextureFiles(const std::string & rootName, const std::string & extension, int numberOfFiles)
{
  std::string textureFile;

  for (int i = 0; i < numberOfFiles; i++)
    {
      std::stringstream	ss;

      ss << i;
      textureFile = rootName + "_" + ss.str() + extension;
      this->addTextureFile(textureFile);
    }
}


void vtkTexturingHelper::readGeometryFile(const std::string & filename)
{
  std::string empty("");

  if (filename != empty) {
    this->setGeometryFile(filename);
  }
  std::string ext = this->geoFile_.substr(geoFile_.find_last_of("."));

  if (this->geoExtensionsMap_.find(ext) != this->geoExtensionsMap_.end()) {
    (this->*geoExtensionsMap_[ext])();
  }
  else {
    std::cerr << "vtkTexturingHelper: " << ext << ": Unknown geometry file extension" << std::endl;
  }
}


void vtkTexturingHelper::clearTexturesList()
{
  this->texturesFiles_.clear();
}


vtkPolyData* vtkTexturingHelper::getPolyData() const
{
  return this->thePolyData_;
}


vtkActor* vtkTexturingHelper::getActor() const
{
  return this->theActor_;
}


void vtkTexturingHelper::setPolyData(vtkPolyData *polyData)
{
  this->thePolyData_ = polyData;
}


void vtkTexturingHelper::setGeometryFile(const std::string & file)
{
  this->geoFile_ = file;
}
