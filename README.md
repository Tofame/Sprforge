What is Tofame's 'SprForge' Asset Editor?
----

Just as name suggests, SprForge is an Asset Editor allowing for creation/load of graphical assets and compiling them into a single binary file(s), such as `.spr` and `.dat`. 
<hr>

What is ``.spr`` - it is a file containing graphics, compressed to remain low sized.

What is ``.dat`` - it is a file containing game data for items, outfits, effects and missiles. For instance, it decides the items properties - isGround, isStackable etc.
<hr>

Currently both of those formats are based on how [Object Builder](https://github.com/punkice3407/ObjectBuilder) and other similar tools' load/compile implementations, in order to be compatible with game engines/clients. However, my end goal plan, is to create my own formats, that will be more efficient.


Some features:
* Very versatile config file, allowing for customization of app functionality such as Sprites List etc.
* Support animation frames, item width and height bigger than 1x1
* A lot of import/export options, such as:
  * Graphical: `.png`, `.bmp`, `.jpg`
  * ItemType Data: `.itf` (my own binary format), `.toml`

* Load and compiling `.spr` (compiling untested, always do backup!)
* Load `.dat` (compiling unfinished)

Image
----

<img width="620" height="480" alt="image" src="https://github.com/user-attachments/assets/21e918d1-29e6-48af-b6a1-c35802dffcad" />


Compiling in Windows
----

You need [visual studio](https://visualstudio.microsoft.com/)

Get [premake](https://premake.github.io/download/), it will be e.g. ``premake5.exe``.

Open cmd where the project folder is, type: ``premake5 vs2022``, files such as ``.sln``, ``.vcxproj`` should be generated.

Open ``.sln`` and build the project.

Compiling on other OS
----

We have workflows, for macOS and linux but I don't know if they work.

Another person, who is knowledgable enough about either of those, would have to maintain that.

Donate
----
Currently no donation option available, though I appreciate contributions in the form of reporting issues or making PRs.
