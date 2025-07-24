What is Tofame's 'Sprforge' Asset Editor?
----

Just as name suggests, Sprforge is an Asset Editor allowing for creation/load of graphical assets and compiling them into a single binary file(s), such as `.spr` and `.dat`. 
As long as your client reads them, you can edit or add new items, {outfits, effects or missiles are WIP} and see them in your game.

Some features:

* Very versatile config file, allowing for customization of app functionality such as Sprites List etc.
* Support animation frames, item width and height bigger than 1x1
* A lot of import/export options, such as:
  * Graphical: `.png`, `.bmp`, `.jpg`
  * ItemType Data: `.itf` (my own binary format), `.toml`

* Load OTs `.spr`, compile `.spr`
* Load OTS `.dat`, compiling OTDat is WIP

Image
----

<img width="1004" height="669" alt="obraz" src="https://github.com/user-attachments/assets/8acb6ed4-6221-4bca-b691-ee625dc56764" />

Compiling in Windows
----

It is compillable in CLion.
* You have to set working directory in run setup to be the folder Sprforge/ as this is where data/ and config.toml is.

Uses CMake's fetch content to get most of the required libs.
The libs you need to get/update on your own:
1. Nativefiledialog-extended
* Download a release of [native-filedialog-extended](https://github.com/btzy/nativefiledialog-extended/releases) - I tested with .zip of v1.2.1 release.
* Add it (just drag n drop) to the dependencies/nativefiledialog-extended

Donate
----
Currently no donation option available, though I appreciate contributions in the form of reporting issues or making PRs.
