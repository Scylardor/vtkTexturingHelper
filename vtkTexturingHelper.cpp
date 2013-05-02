# include "vtkPointData.h"
# include "vtkPoints.h"
# include "vtkPolyDataMapper.h"
# include "vtkRenderWindow.h"
# include "vtkJPEGReader.h"
# include "vtkOBJReader.h"
# include "vtkProperty.h"
# include "vtkOpenGLHardwareSupport.h"
# include "vtkOpenGLRenderWindow.h"

#include "vtkTexturingHelper.h"

vtkTexturingHelper::vtkTexturingHelper()
{
	this->thePolyData_ = NULL;
	this->theActor_ = vtkActor::New();
	this->geoExtensionsMap_[".obj"] = &vtkTexturingHelper::readOBJFile;
	this->imgExtensionsMap_[".jpg"] = &vtkTexturingHelper::readjPEGTexture;
	this->TCoordsCount_ = 0;
}

vtkTexturingHelper::~vtkTexturingHelper()
{
}

void	vtkTexturingHelper::readjPEGTexture(int index)
{
	vtkJPEGReader*	jPEGReader = vtkJPEGReader::New();
	vtkTexture*		newTexture = vtkTexture::New();

	jPEGReader->SetFileName(this->texturesFiles_[index].c_str());
	jPEGReader->Update();
	newTexture->SetInputConnection(jPEGReader->GetOutputPort());
	this->textures_.push_back(newTexture);
}

void	vtkTexturingHelper::configureTexture(int texIndex)
{
	if (texIndex == 0) {
		std::cout << "replace tex" << std::endl;
		this->textures_[texIndex]->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_REPLACE);
	} else {
		std::cout << "add tex" << std::endl;
		this->textures_[texIndex]->SetBlendingMode(vtkTexture::VTK_TEXTURE_BLENDING_MODE_ADD);
	}
	/*this->textures_[texIndex]->RepeatOff();
	this->textures_[texIndex]->EdgeClampOn();
	this->textures_[texIndex]->InterpolateOn(); */
}

void	vtkTexturingHelper::convertImagesToTextures()
{
	int	texIndex;

	for (texIndex = 0; texIndex != this->texturesFiles_.size(); texIndex++)
	{
		std::string	ext = this->texturesFiles_[texIndex].substr(this->texturesFiles_[texIndex].find_last_of("."));

		std::cout << "Extension of " << this->texturesFiles_[texIndex] << ": " << ext << std::endl;
		if (this->imgExtensionsMap_.find(ext) != this->imgExtensionsMap_.end()) {
			(this->*imgExtensionsMap_[ext])(texIndex);
			this->configureTexture(texIndex);
		} else {
			std::cerr << "Unknown extension: " << ext << std::endl;
		}
	}
}

void	vtkTexturingHelper::applyTextures()
{
	vtkPolyDataMapper * mapper = vtkPolyDataMapper::New();
	
	this->convertImagesToTextures();
	std::cout << "yello" << std::endl;
	mapper->SetInput(this->thePolyData_);

	bool	supported;
	int	tu = 0;
	int	textureUnit = vtkProperty::VTK_TEXTURE_UNIT_0;
	vtkRenderWindow	*tmp = vtkRenderWindow::New();
	vtkOpenGLHardwareSupport * hardware = 
    vtkOpenGLRenderWindow::SafeDownCast(tmp)->GetHardwareSupport();

  supported = hardware->GetSupportsMultiTexturing();
  if (supported)
    {
		tu = hardware->GetNumberOfFixedTextureUnits();
		std::cout << "Multi-texturing is hardware supported: supports " << tu << " multi textures" << std::endl;
    }
  if (supported && tu >= 2)
    {
		 std::cout << "Mapping multi-textures" << std::endl;
		
		for (unsigned int texNumber = 0; texNumber <= tu && texNumber < this->TCoordsArrays_.size(); texNumber++) {
			 std::cout << "Mapping multi-texture " << this->TCoordsArrays_[texNumber]->GetName() << std::endl;
			mapper->MapDataArrayToMultiTextureAttribute(
				textureUnit, this->TCoordsArrays_[texNumber]->GetName(),
			vtkDataObject::FIELD_ASSOCIATION_POINTS);
			this->thePolyData_->GetPointData()->AddArray(this->TCoordsArrays_[texNumber]);
			textureUnit++;
		}
		textureUnit = vtkProperty::VTK_TEXTURE_UNIT_0;
		for (unsigned int texNumber = 0; texNumber < this->TCoordsArrays_.size(); texNumber++) {
			std::cout << "Setting Textures" << std::endl;
			this->theActor_->GetProperty()->SetTexture(textureUnit, this->textures_[texNumber]);
			textureUnit++;
		}
    }
  else
  {
    // no multitexturing just show one texture.
	this->theActor_->SetTexture(this->textures_[0]);
  }
  this->theActor_->SetMapper(mapper);
  std::cout << "Done Mapping multi-textures" << std::endl;
  std::cout << "PolyData arrays = " << this->thePolyData_->GetPointData()->GetNumberOfArrays() << std::endl;
  tmp->Delete();
}

void	vtkTexturingHelper::insertNewTCoordsArray()
{
	vtkFloatArray	*newTCoords = vtkFloatArray::New();
	std::stringstream	ss;
	std::string	arrayName;

	ss << this->TCoordsCount_;
	arrayName = "TCoords" + ss.str();
	newTCoords->SetName(arrayName.c_str());
	newTCoords->SetNumberOfComponents(2);
	this->TCoordsCount_++;
	for (unsigned int pointNbr = 0; pointNbr < this->thePolyData_->GetPointData()->GetNumberOfTuples(); pointNbr++) {	
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
void		vtkTexturingHelper::retrieveOBJFileTCoords()
{
	std::string		prevWord = "";
	std::ifstream	objFile;
	std::string		line;
	unsigned int	texPointsCount = 0;

	objFile.open(this->geoFile_);
	std::cout << "get lucky opening" << std::endl;
	if (!objFile.is_open())
	{
		std::cerr << "Unable to open OBJ file " << this->geoFile_ << std::endl;
	}
	while ( objFile.good() )
	{
		std::stringstream	ss;
		std::string			word;
		float				x, y;

		getline(objFile, line);
		ss << line;
		ss >> word >> x >> y; // assumes the line is well made (in case of a "vt line" it should be like "vt 42.84 42.84")
		if (word == "vt") {
			if (prevWord != "vt") {
				std::cout << "on " << word << " " << x << " " << y << " with before " << prevWord << std::endl;
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
	std::cout << "Object number of points: " << this->thePolyData_->GetPointData()->GetNumberOfTuples() << std::endl;
	int arrayNbr;
	// In a Obj file, texture coordinates are listed by texture, so we want to put it in the TCoords array of the right texture
	// but not in the array of other textures. But every TCoords array musts also have the same number of points than the mesh!
	// So to do this we append the coordinates to the concerned TCoords array (the last one created)
	// and put (-1.0, -1.0) in the others instead.
	for (arrayNbr = 0; arrayNbr != this->TCoordsArrays_.size(); arrayNbr++) {
		std::cout << this->TCoordsArrays_[arrayNbr]->GetName() << " number of tuples: " << this->TCoordsArrays_[arrayNbr]->GetNumberOfTuples() << std::endl;
	}
}

// Method to obtain a PolyData out of a Wavefront OBJ file
void		vtkTexturingHelper::readOBJFile()
{
	vtkOBJReader* reader = vtkOBJReader::New();

	cout << "Reading obj" << endl;
	reader->SetFileName(this->geoFile_.c_str());
	reader->Update();
	this->thePolyData_ = reader->GetOutput();
	std::cout << "Object number of points: " << this->thePolyData_->GetPointData()->GetNumberOfTuples() << std::endl;
	//reader->Delete();
	std::cout << "Object number of points: " << this->thePolyData_->GetPointData()->GetNumberOfTuples() << std::endl;
	cout << "Done reading" << endl;
	this->retrieveOBJFileTCoords();
}

void		vtkTexturingHelper::addTextureFile(std::string imageFile)
{
	this->texturesFiles_.push_back(imageFile);
}

//This method avoids the user to call the addTextureFile function a lot of times, when you have a lot of texture files that follow the
// same naming convention,
//	since it permits to simply tell to " get *number* images which name begins by *rootName* ", which will be actually followed by
//	an underscore and the number of the image (starting from 0), and ended by the file extension.
//	e.g. passing it ("sasha", ".jpg", 3) as parameters will make it add "sasha_0.jpg", "sasha_1.jpg", and "sasha_2.jpg" to the
//	list of the texture files to use.
// This is a filename convention since you usually have a geometry file (like "sasha.obj"), and its texture files with the same name,
// an underscore and a number appended to it.
void		vtkTexturingHelper::associateTextureFiles(std::string rootName, std::string extension, int numberOfFiles)
{
	std::string textureFile;

	for (int i = 0; i < numberOfFiles; i++)
	{
		std::stringstream	ss;

		ss << i;
		textureFile = rootName + "_" + ss.str() + extension;
		std::cout << "Adding " << textureFile << std::endl;
		this->addTextureFile(textureFile);
	}
}

void		vtkTexturingHelper::readGeometryFile()
{
	std::cout << this->geoFile_ << "?" << std::endl;
	int idx = this->geoFile_.rfind('.');
	std::cout << "?" << std::endl;
	if (idx != std::string::npos) {
		std::cerr << "Extension of " << geoFile_ << " is wat"  << std::endl;
	}
	else {
		std::cout << "euh" << std::endl;
	}
	std::string	ext = this->geoFile_.substr(geoFile_.find_last_of("."));
	if (this->geoExtensionsMap_.find(ext) != this->geoExtensionsMap_.end()) {
		(this->*geoExtensionsMap_[ext])();
	}
}

void	vtkTexturingHelper::clearTexturesList()
{
	this->texturesFiles_.clear();
}

vtkPolyData*	vtkTexturingHelper::getPolyData() const
{
	return this->thePolyData_;
}

vtkActor*	vtkTexturingHelper::getActor() const
{
	return this->theActor_;
}

void	vtkTexturingHelper::setPolyData(vtkPolyData *polyData)
{
	this->thePolyData_ = polyData;
}

void	vtkTexturingHelper::setGeometryFile(std::string file)
{
	this->geoFile_ = file;
}