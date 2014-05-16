vtkTexturingHelper
==================

A class made to help texturing objects with many images, with the 3D rendering toolkit VTK, in C++.

I made an article on this subject, if you're interested in what's in here, you'll probably want to read it: http://scylardor.fr/2013/05/06/making-multi-texturing-work-with-vtk/


Why
---

VTK doesn't handle well multitexturing out of the box, i.e. texturing a 3D model out of multiple images, although it is a pretty common case.

VTK just reads the OBJ file and stacks all the vertices texture coordinates in one big texture coordinates array, and it just works well when using a single texture file.

The problem with this approach when using more than one texture file is that afterwards, it's impossible to deduce which texture is to be used for which texture coordinates.

For that reason, although VTK already provides some multitexturing related functions, it isn't able (yet) to handle multitexturing on its own. We need to give it a hand !

That's why I quickly wrote a small class that aims to ease this process. It currently only works for Wavefront OBJ model files using JPEG textures.


How
---

The class internally uses vtkOBJReader and vtkJPEGReader to read the geometry and texture files.

The idea I've found to work is to read again the OBJ file behind VTK (we use vtkOBJReader only to import the geometry) to break the specified texture coordinates into smaller arrays associated to each texture.

It is based on the assumption that the OBJ file will define the vertex texture coordinates of a texture, then the faces to apply them on, and that for each texture image. Examples such as those of the Artec3D scanner gallery are made this way.

It is then possible to deduce that when a vertex texture coordinate line, begun by "vt", comes after a line which isn't begun by this prefix, it's a new list of vertex texture coordinates, thus, those coordinates apply to another texture file.

That's why the class relies heavily on the fact that the user shall pass the textures filenames in the right order to it, i.e. the order in which they are used in the OBJ file. It must be respected, otherwise the results will be erroneous.

The class then keeps an internal TCoords array for each of the textures, and maps as many texture units as necessary to these separate TCoords arrays.


Quickstart
----------

The general idea behind the class is simple:

* read a geometry file
* extract the texture coordinates
* read the texture images
* map them according to texture coordinates on the 3D object
* retrieve the VTK actor ready to be displayed.

This is done by the following class functions:

* setGeometryFile : specify a geometry file.

* readGeometryFile : reads the file previously specified with setGeometryFile. It guesses the good VTK reader to call according to the extension of the filename. E.g.: given a filename ending by « .obj », the class will use a vtkOBJReader. You can also directly pass the filename to readGeometryFile without calling setGeometryFile first.

* associateTextureFiles : this function permits to tell the class it will have to use a list of images which name follows a precise syntax. I wrote it with in mind the concrete example of my own experience: on all the Artec3D website examples, the image files are all named like the OBJ file (for example: « sasha »), followed by an underscore, the number of the image (beginning at 0) and the extension. This function permits to precise the « root name » of the images list (in this example: « sasha »), to specify their extension (« .jpg », here) and their number (here, 3). It gives a call like « helper.associateTextureFiles(« sasha », « .jpg », 3); », and it permits to tell the class that it will have to use « 3 JPEG files which names are « sasha_0.jpg », « sasha_1.jpg » and « sasha_2.jpg ». The class will take care of correctly rebuild the filename, and import the image data with the VTK class that fits (here a vtkJPEGReader).

* applyTextures: the function that will do the mapping work of the textures onto the polyData obtained by reading the geometry file. Its principal flaw is that it’s entirely based on the order in which have been given the images filenames to it. If the OBJ file is correctly formatted, i.e. it first uses the first image, e.g. sasha_0.jpg, then the second, sasha_1.jpg, etc… it won’t do no harm, but images imported without any order could prevent it from working well. This should sooner or later be the object of an improvement

* getActor: actually the class has an internal PolyData to retrieve the imported geometrical object, e.g. with the vtkOBJReader, and a vtkActor on which we’re going to map the textures and associate the matching images’ data. This getter permits to easily retrieve the already textured actor, which you only have to add to a vtkRenderer to make it appear on the screen. All the complicated stuff 
about multitexturing has been cared of by the class.

You will find a working code example on [this Gist](https://gist.github.com/Scylardor/9244b13abbd46fee382c).


Contribute
----------

This class is still a work in progress, but since I don't currently work with VTK, I stopped working on it. Feel free to fork the project or to make pull requests on it.

Although the class is working by now (the example Gist is functional), I already see many possible improvements:

* a more intelligent reading of the OBJ file: understanding of the usemtl instruction, reading of the .mtl file and, thus, detection of the texture file used, which would permit to not specify the images files to use (the program would directly go and read it itself). The better way would be to totally replace VTK’s vtkOBJReader class, because when dealing with large meshes, the double-pass approach (vtkOBJReader to import the geometry, then vtkTexturingHelper to read the texture coordinates) takes a non-negligible amount of time.

* add others possible reading sources, for the geometry (VRML, PLY, STL…) and for the image formats (PNG…)

* make the class’ use of file extensions case-insensitive, or use an enum in place of it: easier for everyone

* for the moment, the class internally contains only a PolyData and an Actor. It works pretty well for an OBJ file importation, since the OBJ file is meant to import a single 3D object. But since other formats allow to import many objects in one file (VRML does), it would be better if the class would handle a list of objects.

* a thorough error handling. Maybe exception throwing ?


License
-------

vtkTexturingHelper is under the MIT License.

You can do anything you want with this code, as long as you provide attribution back to me and don't hold me liable.
