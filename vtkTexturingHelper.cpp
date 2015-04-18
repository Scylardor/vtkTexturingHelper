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

#include <stdexcept>    // std::out_of_range
#include <sstream>      // std::stringstream

#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkImageReader2Factory.h"
#include "vtkOBJReader.h"
#include "vtkProperty.h"
#include "vtkOpenGLHardwareSupport.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkTexturingHelper.h"

vtkTexturingHelper::vtkTexturingHelper() {
    m_polyData = vtkSmartPointer<vtkPolyData>::New();
    m_actor = vtkSmartPointer<vtkActor>::New();
    m_geoExtensionsMap[".obj"] = &vtkTexturingHelper::GeoReadOBJ;
    m_TCoordsCount = 0;
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
}

vtkTexturingHelper::~vtkTexturingHelper() {
}

void vtkTexturingHelper::Clear() {
    ClearTCoordsList();
    ClearTexturesList();
    m_actor = vtkSmartPointer<vtkActor>::New();
    m_polyData = vtkSmartPointer<vtkPolyData>::New();
}

int vtkTexturingHelper::GetNumberOfSupportedTextureUnits() const {
    const size_t nbTextures = m_textures.size();
    // We need a temporary render window to know whether the hardware supports multitexturing.
    vtkRenderWindow * tmp = vtkRenderWindow::New();
    vtkOpenGLHardwareSupport * hardware = vtkOpenGLRenderWindow::SafeDownCast(tmp)->GetHardwareSupport();
    const int tu = hardware->GetSupportsMultiTexturing() ? hardware->GetNumberOfFixedTextureUnits() : 0; // number of supported texture units
    tmp->Delete(); // free the temporary render window

    if (tu < nbTextures) {
        std::string warning = "Warning: Hardware does not support the number of textures defined. Using only first ";
        std::stringstream ss;
        ss << tu;
        warning += ss.str();
        warning += " textures instead of ";
        ss.clear();
        ss.str("");
        ss << nbTextures;
        warning += ss.str() + ".";
        std::cerr << warning << std::endl;
    }
    return tu;
}

// applyTextures - will attribute to each texture unit the matching TCoords array and provide the actor the texture data.
// This is the function where we discover hardware multitexturing capability. It falls back to monotexturing if multitexturing is unavailable.
void vtkTexturingHelper::ApplyTextures() {
    const size_t nbTextures = m_textures.size();

    if (nbTextures == 0) {
        throw vtkTexturingHelperException("applyTextures: trying to apply textures with no textures stored");
    }
    const int tu = GetNumberOfSupportedTextureUnits(); // number of supported texture units
    if (tu >= 2 && nbTextures > 1) {
        int textureUnit = vtkProperty::VTK_TEXTURE_UNIT_0;

        // Map each TCoords array to the good texture unit in the polyData.
        for (unsigned int texIdx = 0; texIdx < tu && texIdx < m_TCoordsArrays.size(); texIdx++) {
            m_mapper->MapDataArrayToMultiTextureAttribute(textureUnit,
                                                          m_TCoordsArrays[texIdx]->GetName(),
                                                          vtkDataObject::FIELD_ASSOCIATION_POINTS);
            m_polyData->GetPointData()->AddArray(m_TCoordsArrays[texIdx]);
            textureUnit++;
        }
        // Set the textures to use in the actor.
        textureUnit = vtkProperty::VTK_TEXTURE_UNIT_0;
        for (unsigned int texIdx = 0; texIdx < tu && texIdx < m_TCoordsArrays.size(); texIdx++) {
            m_actor->GetProperty()->SetTexture(textureUnit, m_textures[texIdx]);
            textureUnit++;
        }
    }
    else { // if no multitexturing or using only one texture
        m_actor->SetTexture(m_textures[0]);
        if (tu < 2) { // hardware doesn't support multitexturing
            std::cerr << "vtkTexturingHelper: Warning, multi-texturing isn't supported - falling back to mono-texturing" << std::endl;
        }
    }
}

// insertNewTCoordsArray - insert a new array of texture coordinates
// When using multitexturing, each TCoord array gives coordinates to texturize a part of the mesh
// but these arrays need to contain (-1.0, -1.0) coordinates for vertices that don't concern their mesh part, so do it here
void vtkTexturingHelper::InsertNewTCoordsArray() {
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
void vtkTexturingHelper::RetrieveOBJFileTCoords() {
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
                InsertNewTCoordsArray();
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

// Method to obtain a PolyData out of a Wavefront OBJ file (internal)
// This function can take a long time since it parses OBJ files
void vtkTexturingHelper::GeoReadOBJ() {
    vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();

    reader->SetFileName(m_geoFile.c_str());
    reader->Update();
    m_polyData = reader->GetOutput();
#if VTK_MAJOR_VERSION < 6
    m_mapper->SetInput(reader->GetOutput());
#else
    m_mapper->SetInputData(reader->GetOutput());
#endif
    RetrieveOBJFileTCoords();
}

// readGeometryFile - Read a geometry file according to its filename extension
// This function tries to guess the geometry file type by looking at its extension to call the right parser to extract the geometry data.
void vtkTexturingHelper::ReadGeometryFile(const std::string & _filename) {
    SetGeometryFile(_filename);

    std::string ext = "";
    try {
        ext = m_geoFile.substr(m_geoFile.find_last_of("."));
    } catch (const std::out_of_range & oor) {
        std::string errMsg = "Cannot guess geometry file format using extension of filename: ";
        errMsg += _filename;

        throw vtkTexturingHelperException(errMsg.c_str());
    }
    if (m_geoExtensionsMap.find(ext) != m_geoExtensionsMap.end()) {
        (this->*m_geoExtensionsMap[ext])();
    }
    else {
        std::string errMsg = "Unmanaged geometry file extension: ";
        errMsg += ext;

        throw vtkTexturingHelperException(errMsg.c_str());
    }
}

void vtkTexturingHelper::ConvertImageToTexture(vtkSmartPointer<vtkImageReader2> _imageReader) {
    vtkSmartPointer<vtkTexture> newTexture = vtkSmartPointer<vtkTexture>::New();
    newTexture->SetInputConnection(_imageReader->GetOutputPort());

    // We must use REPLACE for the first texture, and ADD for the others, otherwise the result will be buggy !
    const bool isFirst = m_textures.size() == 0;
    if (isFirst) {
        newTexture->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE);
    }
    else {
        newTexture->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
    }
    m_textures.push_back(newTexture);
}

vtkSmartPointer<vtkImageReader2> vtkTexturingHelper::GetImageReaderForFile(const std::string & _fileName) const {
    vtkImageReader2 * imageReader = vtkImageReader2Factory::CreateImageReader2(_fileName.c_str());
    // This image format isn't supported by VTK
    if (imageReader == NULL) {
        std::string errMsg = "This file is not supported or cannot be opened by VTK vtkImageReader2Factory: ";
        errMsg += _fileName;
        throw vtkTexturingHelperException(errMsg.c_str());
    }
    // wrap it in a smart pointer - no risk of forgetting to call Delete !
    vtkSmartPointer<vtkImageReader2> smartImageReader;
    smartImageReader.TakeReference(imageReader);
    return smartImageReader;
}

// addTextureFile - Primary function to manually specify a texture to use by its name.
// It can be useful when texture files names don't respect the "name_imageNumber.ext" convention.
// However, it is important to specify the textures in the same order than they are described in the geometry file !
// The results will otherwise be erroneous.
void vtkTexturingHelper::ReadTextureFile(const std::string & _fileName) {
    vtkSmartPointer<vtkImageReader2> imageReader = GetImageReaderForFile(_fileName);
    // This image format isn't supported by VTK
    if (imageReader == NULL) {
        std::string errMsg = "File format is not supported by VTK vtkImageReader2Factory: ";
        errMsg += _fileName;
        throw vtkTexturingHelperException(errMsg.c_str());
    }
    // Next, read the file data and create a texture with it.
    imageReader->SetFileName(_fileName.c_str());
    imageReader->Update();
    ConvertImageToTexture(imageReader);
}

// associateTextureFiles - feed the vtkTexturingHelper with texture files with a specific filename pattern.
// Instead of adding textures one by one, you can make the helper add textures with similar names.
// This function currently supports the filename format "example_imgNumber.ext",
// e.g.: using ("sasha", ".jpg", 3) as parameters will make it add "sasha_0.jpg", "sasha_1.jpg", and "sasha_2.jpg" to the list of the texture files to use.
// When doing so, be sure that the texture coordinates data in the geometry file references the textures in that exact same order.
void vtkTexturingHelper::ReadTextureFiles(const std::string & _prefix, const std::string & _extension, int _numberOfFiles) {
    vtkSmartPointer<vtkImageReader2> imageReader = NULL;

    if (_numberOfFiles <= 0) {
        throw vtkTexturingHelperException("Please provide a non-null positive number of texture files to read");
    }
    for (int fileIdx = 0; fileIdx < _numberOfFiles; fileIdx++) {
        std::stringstream ss;
        ss << fileIdx;
        const std::string fileName = _prefix + "_" + ss.str() + _extension;

        ReadTextureFile(fileName);
    }
}

void vtkTexturingHelper::ClearTexturesList() {
    m_textures.clear();
}

vtkSmartPointer<vtkPolyData> vtkTexturingHelper::GetPolyData() const {
    return m_polyData;
}

vtkSmartPointer<vtkActor> vtkTexturingHelper::GetActor() const {
    m_actor->SetMapper(m_mapper);
    return m_actor;
}

void vtkTexturingHelper::SetGeometryFile(const std::string & _file) {
    m_geoFile = _file;
}

void vtkTexturingHelper::ClearTCoordsList() {
    m_TCoordsArrays.clear();
    m_TCoordsCount = 0;
}
