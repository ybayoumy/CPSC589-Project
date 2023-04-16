# CPSC589-Project

## How To Run

1. Clone this repository
1. Download Visual Studio (2019 is known to work with this)
1. Open the repository's base directory with Visual Studio
1. The CMakeLists.txt file should run automatically
1. Click the drop down menu and select "589-project.exe" to run the program

## Program Usage/Controls

### Free View

- You are unable to draw in this mode
- Use your scroll wheel to zoom in/out
- Drag Right Click to look around the scene
- Hover and Left click on an object to select it and go into [Object View](#object-view)
- Export a `.obj` file by typing in the desired filename and clicking `Save`
  - the `.obj` file will be created here: [project path]/out/build/x64-Debug
- You can go to [Draw Mode](#draw-mode) from here

### Draw Mode

- You are locked to the XY, XZ, or ZY Planes. You can Toggle through them using the provided buttons
- Hold Left Click to draw a line
- After drawing 2 lines, you can create an object by clicking "Create Rotational Blending Surface"
- Other options include selecting a desired colour to sketch with, editing lines you've drawn, and clearing lines
- After you're done creating objects, you can go back to [Free View](#free-view)

### Object View

- You can delete/recolour the selected object from here
- You can modify the object's original object curves
- You can modify the object's Cross-Section
- You can modifty the object's profile curves
- Afte you're done editing the object, you can go back to [Free View](#free-view)

### Object Cross Section

An object's cross section defines the shape with which to create the rotational blending surface. By default, this cross-section is a circle.

You can either edit the object's existing cross-section or draw a new one. Drawing a new cross section will require one new line to be drawn that defines the shape of half the cross-section. The other half is then mirrored. To draw a new cross-section, you must draw a line starting from one of the points on the lines and ending at one of the points on the other line.

### Object Profile

An object's profile determines how it should be "pinched". This allows for the creation of more complex non-symetric objects

You can either edit the object's existing profile or draw a new one. Drawing a new profile will require two new lines to be drawn that define the shape of the object after "pinching" / "expanding". You can use the Object's original profile lines as reference of what these lines could look like.
