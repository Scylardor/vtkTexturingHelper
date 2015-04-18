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

vtkTexturingHelper::vtkTexturingHelper() {
    m_polyData = NULL;
    m_actor = vtkActor::New();
    m_geoExtensionsMap[".obj"] = &vtkTexturingHelper::geoReadOBJ;
    m_imgExtensionsMap[".jpg"] = &vtkTexturingHelper::readjPEGTexture;
    m_TCoordsCount = 0;
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
}


vtkTexturingHelper::~vtkTexturingHelper() {
}


void vtkTexturingHelper::readjPEGTexture(int _index) {
    vtkSmartPointer<vtkJPEGReader> jPEGReader = vtkSmartPointer<vtkJPEGReader>::New();
    vtkSmartPointer<vtkTexture> newTexture = vtkSmartPointer<vtkTexture>::New();

    jPEGReader->SetFileName(m_texturesFiles[_index].c_str());
    jPEGReader->Update();
    newTexture->SetInputConnection(jPEGReader->GetOutputPort());
    m_textures.push_back(newTexture);
}


void vtkTexturingHelper::configureTexture(int _texIndex) {
    if (_texIndex == 0) {
        m_textures[_texIndex]->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE);
    }
    else {
        m_textures[_texIndex]->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
    }
}


void vtkTexturingHelper::convertImagesToTextures() {
    for (int texIndex = 0; texIndex != m_texturesFiles.size(); texIndex++) {
        const std::string ext = m_texturesFiles[texIndex].substr(m_texturesFiles[texIndex].find_last_of("."));

        if (m_imgExtensionsMap.find(ext) != m_imgExtensionsMap.end()) {
            (this->*m_imgExtensionsMap[ext])(texIndex);
            configureTexture(texIndex);
        }
        else {
            std::string error = "Unmanaged image extension: ";
            error += ext;
            throw vtkTexturingHelperException(error.c_str());
        }
    }
}


void vtkTexturingHelper::applyTextures() {
    this->convertImagesToTextures();

    int tu; // number of supported texture units
    // We need a temporary render window to know whether the hardware supports multitexturing.
    vtkRenderWindow* tmp = vtkRenderWindow::New();
    vtkOpenGLHardwareSupport* hardware = vtkOpenGLRenderWindow::SafeDownCast(tmp)->GetHardwareSupport();

    tu = 0;
    if (hardware->GetSupportsMultiTexturing()) {
        tu = hardware->GetNumberOfFixedTextureUnits();
    }
    if (tu >= 2 && m_textures.size() > 1) {
        int textureUnit = vtkProperty::VTK_TEXTURE_UNIT_0;

        // Map each TCoords array to the good texture unit in the polyData.
        for (unsigned int texNumber = 0; texNumber <= tu && texNumber < m_TCoordsArrays.size(); texNumber++) {
            m_mapper->MapDataArrayToMultiTextureAttribute(textureUnit,
                                                          m_TCoordsArrays[texNumber]->GetName(),
                                                          vtkDataObject::FIELD_ASSOCIATION_POINTS);
            m_polyData->GetPointData()->AddArray(m_TCoordsArrays[texNumber]);
            textureUnit++;
        }
        // Set the textures to use in the actor.
        textureUnit = vtkProperty::VTK_TEXTURE_UNIT_0;
        for (unsigned int texNumber = 0; texNumber < m_TCoordsArrays.size(); texNumber++) {
            m_actor->GetProperty()->SetTexture(textureUnit, m_textures[texNumber]);
            textureUnit++;
        }
    }
    else { // if no multitexturing or using only one texture
        m_actor->SetTexture(m_textures[0]);
        if (tu < 2) { // hardware doesn't support multitexturing
            std::cerr << "vtkTexturingHelper: Warning, multi-texturing isn't supported - falling back to mono-texturing" << std::endl;
        }
    }
    tmp->Delete(); // free the temporary render window
}

// insertNewTCoordsArray - insert a new array of texture coordinates
// When using multitexturing, each TCoord array gives coordinates to texturize a part of the mesh
// but these arrays need to contain (-1.0, -1.0) coordinates for vertices that don't concern their mesh part, so do it here
void vtkTexturingHelper::insertNewTCoordsArray() {
    std::stringstream ss;
    std::string arrayName;
    ss << m_TCoordsCount;
    arrayName = "TCoords" + ss.str();

    vtkSmartPointer<vtkFloatArray> newTCoords = vtkSmartPointer<vtkFloatArray>::New();
    newTCoords->SetName(arrayName.c_str());
    newTCoords->SetNumberOfComponents(2);
    m_TCoordsCount++;
    const unsigned int tuplesNumber = static_cast<unsigned int>(m_polyData->GetPointData()->GetNumberOfTuples());
    for (unsigned int pointNbr = 0; pointNbr < tuplesNumber; pointNbr++) {
        newTCoords->InsertNextTuple2(-1.0, -1.0);
    }
    m_TCoordsArrays.push_back(newTCoords);
}

// retrieveOBJFileTCoords: read an OBJ file to extract vertex texture data
// VTK doesn't make any difference between the texture coordinates of each image: when loading an OBJ file, it stores all TCoords in one big array.
// That's why multi-texturing is broken by design in VTK. To fix it, this function reads the OBJ file, to know how many TCoords points are allocated to each image, and then
// create as many smaller arrays than necessary.
// In order to make this function work, textures images should be passed to the vtkTexturingHelper in the same order they are referenced in the OBJ file.
// This may change in the future if this class implements the .mtl file parsing (which would even avoid us to have to specify the
// image files' names manually).
void vtkTexturingHelper::retrieveOBJFileTCoords() {
    std::ifstream objFile;

    objFile.open(m_geoFile.c_str());
    if (!objFile.is_open()) {
        std::string errMsg = "Unable to open OBJ file: ";
        errMsg += m_geoFile;
        throw vtkTexturingHelperException(errMsg.c_str());
    }

    std::string prevLineWord = "";
    unsigned int texPointsCount = 0;
    while (objFile.good()) {
        std::stringstream ss;
        std::string line;
        std::string word;
        float x, y;

        getline(objFile, line);
        ss << line;
        // assumes a "word float float" line style, e.g. for vertex texture definition in OBJ: "vt 42.84 42.84"
        ss >> word >> x >> y;
        if (word == "vt") { // only interested in vertex texture coordinates data
            if (prevLineWord != "vt") {
                // if we find a vertex texture line and it's the first of a row, we reached the end of an image's coordinates
                // so create a new TCoords array with what we gathered so far
                insertNewTCoordsArray();
            }
            int arrayNbr;
            // In an OBJ file, texture coordinates are listed by texture, so we want to put it in the TCoords array of the right texture
            // but not in the array of other textures. But every TCoords array musts also have the same number of points than the mesh!
            // So to do this we append the coordinates to the concerned TCoords array (the last one created)
            // and put (-1.0, -1.0) in the others instead.
            for (arrayNbr = 0; arrayNbr != m_TCoordsArrays.size() - 1; arrayNbr++) {
                m_TCoordsArrays[arrayNbr]->SetTuple2(texPointsCount, -1.0, -1.0);
            }
            m_TCoordsArrays[arrayNbr]->SetTuple2(texPointsCount, x, y);
            texPointsCount++;
        }
        prevLineWord = word;
    }
    objFile.close();
}

// Method to obtain a PolyData out of a Wavefront OBJ file
void vtkTexturingHelper::readOBJFile(const std::string & _filename) {
    this->setGeometryFile(_filename);
    this->geoReadOBJ();

    vtkOBJReader* reader = vtkOBJReader::New();

    reader->SetFileName(m_geoFile.c_str());
    reader->Update();
    m_polyData = reader->GetOutput();
#if VTK_MAJOR_VERSION < 6
    m_mapper->SetInput(reader->GetOutput());
#else
    m_mapper->SetInputData(reader->GetOutput());
#endif
    this->retrieveOBJFileTCoords();
}

// Method to obtain a PolyData out of a Wavefront OBJ file (internal)
void vtkTexturingHelper::geoReadOBJ() {
    vtkOBJReader* reader = vtkOBJReader::New();

    reader->SetFileName(m_geoFile.c_str());
    reader->Update();
    m_polyData = reader->GetOutput();
#if VTK_MAJOR_VERSION < 6
    m_mapper->SetInput(reader->GetOutput());
#else
    m_mapper->SetInputData(reader->GetOutput());
#endif
    this->retrieveOBJFileTCoords();
}

// Allows to manually specify the texture file names.
// It can be useful when texture files names don't respect the "name_imageNumber.ext" convention.
// However, it is important to specify the textures in the same order than they are described in the geometry file !
// The results will otherwise be erroneous.
void vtkTexturingHelper::addTextureFile(const std::string & _imageFile) {
    m_texturesFiles.push_back(_imageFile);
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
void vtkTexturingHelper::associateTextureFiles(const std::string & _rootName, const std::string & _extension, int _numberOfFiles) {
    std::string textureFile;

    for (int i = 0; i < _numberOfFiles; i++) {
        std::stringstream ss;

        ss << i;
        textureFile = _rootName + "_" + ss.str() + _extension;
        this->addTextureFile(textureFile);
    }
}


void vtkTexturingHelper::readGeometryFile(const std::string & _filename) {
    std::string empty("");

    if (_filename != empty) {
        this->setGeometryFile(_filename);
    }
    std::string ext = m_geoFile.substr(m_geoFile.find_last_of("."));

    if (m_geoExtensionsMap.find(ext) != m_geoExtensionsMap.end()) {
        (this->*m_geoExtensionsMap[ext])();
    }
    else {
        std::cerr << "vtkTexturingHelper: " << ext << ": Unknown geometry file extension" << std::endl;
    }
}


void vtkTexturingHelper::clearTexturesList() {
    m_texturesFiles.clear();
}


vtkPolyData* vtkTexturingHelper::getPolyData() const {
    return m_polyData;
}


vtkActor * vtkTexturingHelper::getActor() const {
    m_actor->SetMapper(m_mapper);
    return m_actor;
}


void vtkTexturingHelper::setPolyData(vtkPolyData * _polyData) {
    m_polyData = _polyData;
}


void vtkTexturingHelper::setGeometryFile(const std::string & _file) {
    m_geoFile = _file;
}
