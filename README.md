ExplicitCAD is a minimal graphical user interface for [ImplicitCAD](https://github.com/colah/ImplicitCAD). 

It has a text editor, an output log, and a 3D viewer. 2D output is not currently supported.

Dependencies: 'qt5', 'libqscintilla'.

To compile, run 'qmake', and then 'make'.

To use, put the extopenscad binary in the same directory as the explicitcad binary.

Press F5 to render a preview, press F6 to render a final object. Resolution is currently hardcoded.

The text editor is an instance of [QScintilla](https://qscintilla.com/). The 3D viewer is an instance of [fstl](https://github.com/mkeeter/fstl).

ExplicitCAD is licensed under the [GPLv3](https://www.gnu.org/licenses/gpl.html).

QScintilla is also licensed under the GPLv3.

fstl's license is as follows. I believe it is compatible with GPLv3:


> Copyright (c) 2014-2017 Matthew Keeter
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.



