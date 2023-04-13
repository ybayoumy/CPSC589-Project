Installment and Execution:
git clone the repository github link to install the project. Once the project is installed you then right-click the project folder and open it in Visual Studio. Finally, in Visual Studio
there should be a run debug green play symbol on the top center of the screen with a drop down menu next to it. Click the drop down menu and select "589-project.exe". Then click on the
the run program button.

Program Usage and Features:
-Once you open the program you will in free view mode and unable to draw.
-On the top right there is a button that says draw mode which will let you draw curves.
-You are given 3 non-free views of XY,XZ,ZY

-Once you have drawn 2 curves you will be presented with the option to apply a Rotational blending surface.
-Now that you have an object you can also change it's color  with the RGB options infront of you.
-You may modify the curves you drew to define the object.
-You can modify the profile of the object, draw more curves to edit the shape into a more complex one.
-finally you may also apply a cross section to the object.

-Once you have created/drawn your desired object. Return to free view mode and give your object a name then export it as an obj.


Where to find created OBJ files:
[project path]/out/build/x64-Debug



# CPSC589-Project

This skeleton builds upon the 589/689 assignment skeleton as well as the 453 Assignment 4 skeleton.

However, there are some important differences highlighted in a below section.

You are encouraged to modify this skeleton, and any of the files within, in any way you want or need in order to complete your course project.

If you have questions about this skeleton, its structure, or any of the classes, please feel free to email your TA.

## Important Differences from Previous Skeletons

### GLDebug
In the 589/689 assignment skeleton, there was the following line of code to "ignore non-significant error/warning codes":
```c++
if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
```
For your projects, you might be modifying the skeleton much more than for the assignments, and there is a chance that these codes could serve some purpose.

Thus, this line has been commented out, but you can add it back in if you are being flooded with too many OpenGL messages in the console.

### .obj File Loading
A .obj file loader (GeomLoaderForOBJ) using the tinyobjloader library (added to "thirdparty") has been implemented.

You may view the GeomLoaderForOBJ.h and GeomLoaderForOBJ.cpp comments for more details.

### VertexBuffer
The way that "AttribArrays" are handled has been changed to accomodate objects that may or may not have certain attributes.

For example, some .obj files may not have UV coordinates.

## Adding Your Own Textures & Models

To add textures to the project, place them in the "textures" folder and make sure CMake is called again so that it can copy them over.
In Visual Studio, this is as simple as opening up and saving CMakeLists.txt (even if you didn't change CMakeLists.txt) or clicking "Project > Configure Cache".
The textures will be copied to the output directory in a directory also called textures.
To path the textures in the program, do: "./textures/<File>" or "textures/<File>".

The same is true for 3D .obj files placed in the "models" directory.

A way to select models on your computer via a file dialog has not been added.
Part of this is because different systems handle file dialogs differently, so adding this functionality could break the program on some computers.
If you want to write it yourself, or pick a library to handle this, you are free to do so!

## Current Controls

The provided code gives you a spherical camera and renders a selected .obj file with generic Phong shading.

To control the spherical camera:

 * Scroll wheel zooms in and out on the cube
 * Holding the right mouse button and dragging allows you to rotate the camera around the cube

To switch objects, use the ImGui drop-down.

The rest of the ImGui controls should be fairly intuitive.

