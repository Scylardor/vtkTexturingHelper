#ifndef	VTK_TEXTURING_HELPER_H
# define VTK_TEXTURING_HELPER_H

# include <vector>
# include <map>
# include <string>
# include <sstream>
//
# include "vtkActor.h"
# include "vtkPolyData.h"
# include "vtkTexture.h"
# include "vtkFloatArray.h"

// A class useful for correctly displaying multi-textured objects (i.e. that are textured by different image files),
//	since VTK doesn't do it by default. The basic use of it should be to just specify a 3D object file (e.g. a .obj file)
//	and some texture files (e.g. .jpg's or .png's), and directly retrieve a correctly-textured actor to add to a renderer.

//	For the moment it only works with vtkOBJReader and vtkjPEGReader, but its design should easily allow using others
//	VTK importers (VRML, PLY, X3D for geometry, PNG for images...) in the future.
class	vtkTexturingHelper
{
	typedef void	(vtkTexturingHelper::*readerFunction)();
	typedef void	(vtkTexturingHelper::*imgReaderFunction)(int);

	std::vector<std::string>	texturesFiles_;
	std::vector<vtkFloatArray*>				TCoordsArrays_;
	std::vector<vtkTexture*>				textures_;
	std::map<std::string, readerFunction>	geoExtensionsMap_;
	std::map<std::string, imgReaderFunction>	imgExtensionsMap_;
	
	unsigned short							TCoordsCount_;
	std::string					geoFile_;
	vtkPolyData*				thePolyData_;
	vtkActor*					theActor_;

public:
	// Ctors/dtors
	vtkTexturingHelper();
	~vtkTexturingHelper();

	// Usual Methods
	void		addTextureFile(std::string file);
	void		associateTextureFiles(std::string rootName, std::string extension, int numberOfFiles);
	void		clearTexturesList();
	void		readGeometryFile();
	void		insertNewTCoordsArray();
	

	// jPEG Reading methods
	void	readjPEGTexture(int);

	// Textures methods
	void	convertImagesToTextures();
	void	applyTextures();
	void	configureTexture(int);

	// Wavefront .obj methods
	void		readOBJFile();
	void		retrieveOBJFileTCoords();

	// Get/Setters
	vtkActor*		getActor() const;
	vtkPolyData*	getPolyData() const;
	void			setPolyData(vtkPolyData *polyData);
	void			setGeometryFile(std::string file);
};

#endif