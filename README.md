vtkTexturingHelper
==================

A class made to help texturing objects with many images, with the 3D rendering toolkit VTK, in C++.

I wrote a blog post on this subject, if you're interested in what's in here, you may want to read it: http://scylardor.fr/2013/05/06/making-multi-texturing-work-with-vtk/

Quickstart
----------

```
#include "vtkActor.h"
#include "vtkTexturingHelper.h"

int main() {
  vtkTexturingHelper helper;

  try {
    // Read the geometry file and extract texture coordinates data from OBJ file
    helper.ReadGeometryFile("mannequin/mannequine_clothed.obj");
    // Read 2 textures files called "mannequin/mannequine_clothed_0.jpg" and "mannequin/mannequine_clothed_1.jpg"
    helper.ReadTextureFiles("mannequin/mannequine_clothed", ".jpg", 2);
    // Bind the textures and texture units data onto the geometry
    helper.ApplyTextures();
    } catch (const vtkTexturingHelperException & helperExc) {
        std::cout << "Whoops ! This isn't supported ! " << helperExc.what() << std::endl;
	return 1;
    }
    // Retrieve your multitextured actor
    vtkSmartPointer<vtkActor> actor = helper.GetActor();

    // Render your actor here...
}
```

[The complete working code example is available on this Gist.](https://gist.github.com/Scylardor/9244b13abbd46fee382c)


Why
---

VTK doesn't handle well multitexturing out of the box, i.e. texturing a 3D model out of multiple images, although it is a pretty common case.

VTK just reads the OBJ file and stacks all the vertices texture coordinates in one big texture coordinates array, and it just works well when using a single texture file.

The problem with this approach when using more than one texture file is that afterwards, it's impossible to deduce which texture is to be used for which texture coordinates.

For that reason, although VTK already provides some multitexturing related functions, it isn't able (yet) to handle multitexturing on its own. We need to give it a hand !

That's why I quickly wrote a small class that aims to ease this process. It currently only works for Wavefront OBJ model files, with all textures types known to the vtkImageReader2Factory class.


How
---

The idea I've found to work is to read again the OBJ file behind VTK (we use vtkOBJReader only to import the geometry) to break the specified texture coordinates into smaller arrays associated to each texture.

It is based on the assumption that the OBJ file will define the vertex texture coordinates of a texture, then the faces to apply them on, and that for each texture image. Examples such as those of the Artec3D scanner gallery are made this way.

It is then possible to deduce that when a vertex texture coordinate line, begun by "vt", comes after a line which isn't begun by this prefix, it's a new list of vertex texture coordinates, thus, those coordinates apply to another texture file.

That's why the class relies heavily on the fact that the user shall pass the textures filenames in the right order to it, i.e. the order in which they are used in the OBJ file. It must be respected, otherwise the results will be erroneous.

The class then keeps an internal TCoords array for each of the textures, and maps as many texture units as necessary to these separate TCoords arrays.


Contribute
----------

This class is still a work in progress, but since I don't currently work with VTK, I stopped working on it. Feel free to fork the project or to make pull requests on it.

Although the class is working by now (the example Gist is functional), I already see many possible improvements:

* a more intelligent reading of the OBJ file: understanding of the usemtl instruction, reading of the .mtl file and, thus, detection of the texture file used, which would permit to not specify the images files to use (the program would directly go and read it itself). The better way would be to totally replace VTK’s vtkOBJReader class, because when dealing with large meshes, the double-pass approach (vtkOBJReader to import the geometry, then vtkTexturingHelper to read the texture coordinates) takes a non-negligible amount of time.

* add others possible reading sources, for the geometry (VRML, PLY, STL…)

* make the class’ use of file extensions case-insensitive, or use an enum in place of it: easier for everyone

* for the moment, the class internally contains only a PolyData and an Actor. It works pretty well for an OBJ file importation, since the OBJ file is meant to import a single 3D object. But since other formats allow to import many objects in one file (VRML does), it would be better if the class would handle a list of objects.

* a thorough error handling.


License
-------

vtkTexturingHelper is under the MIT License.

You can do anything you want with this code, as long as you provide attribution back to me and don't hold me liable.
