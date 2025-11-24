What is Tofame's 'SprForge' Asset Editor?
----

Just as name suggests, SprForge is an Asset Editor allowing for creation/load of graphical assets and compiling them into a single binary file(s), such as `.spr` and `.dat`. 

What is ``.spr`` - it is a file containing graphics, compressed to remain low sized.
What is ``.dat`` - it is a file containing game data for items, outfits, effects and missiles. For instance, it decides the items properties - isGround, isStackable etc.

Currently both of those formats are based on how [Object Builder](https://github.com/punkice3407/ObjectBuilder) and other similar tools load/compile those files, in order to be compatible with game engines/clients. However, my end goal plan, is to create my own formats, that will be more efficient.


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

It is compillable in CLion.
* You have to set working directory in run setup to be the folder SprForge/ as this is where data/ and config.toml is.

Uses CMake's fetch content to get most of the required libs.
The libs you need to get/update on your own:
1. Nativefiledialog-extended
* Download a release of [native-filedialog-extended](https://github.com/btzy/nativefiledialog-extended/releases) - I tested with .zip of v1.2.1 release.
* Add it (just drag n drop) to the dependencies/nativefiledialog-extended

Donate
----
Currently no donation option available, though I appreciate contributions in the form of reporting issues or making PRs.
