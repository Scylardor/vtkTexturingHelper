vtkTexturingHelper
==================

SYNOPSIS
--------
A class made to help texturing objects with many images, with the 3D rendering toolkit VTK, in C++.

I made an article on this subject, if you're interested in what's in here, you'll probably want to read it: http://scylardor.fr/2013/05/06/making-multi-texturing-work-with-vtk/


WHY AND HOW IT WOKRS
--------------------
VTK doesn't handle well multitexturing out of the box.

That's why I quickly wrote a small class that aims to ease this process, at least when importing from an OBJ file and using JPEG textures (a case I've already been working on). It works mainly with 4 or 5 important functions:

* associateTextureFiles : this function permits to tell the class it will have to use a list of images which name follows a precise syntax. I wrote it with in mind the concrete example of my own experience: on all the Artec3D website examples, the image files are all named like the OBJ file (for example: « sasha »), followed by an underscore, the number of the image (beginning at 0) and the extension. This function permits to precise the « root name » of the images list (in this example: « sasha »), to specify their extension (« .jpg », here) and their number (here, 3). It gives a call like « helper.associateTextureFiles(« sasha », « .jpg », 3); », and it permits to tell the class that it will have to use « 3 JPEG files which names are « sasha_0.jpg », « sasha_1.jpg » and « sasha_2.jpg ». The class will take care of correctly rebuild the filename, and import the image data with the VTK class that fits (here a vtkJPEGReader).

* setGeometryFile : permits to give a file from which VTK will import the mesh geometry. Not very useful, it will change or maybe merge with readGeometryFile

* readGeometryFile : it’s the function that will call the good VTK reader according to the extension of the filename we passed to it before with the setGeometryFile function. If it’s for example a filename like « *.obj », the class will use a vtkOBJReader.

* applyTextures: the function that will do the mapping work of the textures onto the polyData obtained by reading the geometry file. Its principal flaw is that it’s entirely based on the order in which have been given the images filenames to it. If the OBJ file is correctly formatted, i.e. it first uses the first image, e.g. sasha_0.jpg, then the second, sasha_1.jpg, etc… it won’t do no harm, but images imported without any order could prevent it from working well. This should sooner or later be the object of an improvement

* getActor: actually the class has an internal PolyData to retrieve the imported geometrical object, e.g. with the vtkOBJReader, and a vtkActor on which we’re going to map the textures and associate the matching images’ data. This getter permits to easily retrieve the already textured actor, which you only have to add to a vtkRenderer to make it appear on the screen, all the complicated stuff about multitexturing having been cared of by the class.


HOW TO CONTRIBUTE
-----------------
Although the class is working by now (I used it to generate a working example in the article I talked about before), I already see many possible improvements:

* a more intelligent reading of the OBJ file: understanding of the usemtl instruction, reading of the .mtl file and, thus, detection of the texture file used, which would permit to not specify the images files to use (the program would directly go and read it itself). The better way would be to totally replace VTK’s vtkOBJReader class, because when dealing with large meshes, make the VTK reader read the file, and then make the tkTexturingHelper read it again begins to take a non-negligible amount of time.

* add others possible reading sources, for the geometry (VRML, PLY, STL…) and for the image formats (PNG…)

* make the class’ use of file extensions case-insensitive, or use an enum in place of it: easier for everyone

* for the moment, the class internally contains only a PolyData and an Actor. It works pretty well for an OBJ file importation, since the OBJ file is meant to import a single 3D object (see its name…). But since other formats permit to import many objects in one file (VRML does), we should change from a unique instance of PolyData / Actor to a list which we could access via an index.

* a bit more of error handling, maybe exception throwing ?

I would strongly recommend anyone who would be interested to fork and improve this mini-project **to do it** ! I don’t really work on it anymore, and I have other projects I work on, so this one may not go much further ;)