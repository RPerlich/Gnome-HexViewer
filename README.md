![image](https://github.com/RPerlich/Gnome-HexViewer/screenshot/1.PNG)

What is it?
-------------------------------
HexViewer is an open source GUI file viewer / editor in hexadecimal format 
that runs on Gnome Linux systems.

The program is currently in a very early stage of development.

Features
-------------------------------
HexViewer can open files and show / edit the content in a hexadecimal format.

  * Display of bytes per line fixed or variable
  * Selecting data via mouse and keyboard
  * Printing the current file in hexadecimal form
  * Editing data (limited to overwrite data at the moment)
  * Preferences dialog to control some properties

Building / Running
-------------------------------
HexViewer is using the normal "make" method with AutoTools.
To build HexViewer run the following commands:
  * ./configure
  * make

There can be an issue when running this application out of the build directory
without installing it. To handle this situation, you have to set the 
GSETTINGS_SCHEMA_DIR environment variable to tell GLib where to find the 
compiled schema. Example for "launch.json" in VSC:

"environment": [
  {
    "name": "GSETTINGS_SCHEMA_DIR",
    "value": "data" // set this the the folder where the profile is stored
  }
],

Contributing
-------------------------------
Contributions to the editorapplication welcome.
If you've fixed a bug or implemented a cool new feature that 
you would like to share, please feel free to open a pull request here.

Licensing
-------------------------------
The license used is the GNU General Public License, version 3 or later
