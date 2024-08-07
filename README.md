# elden-ring-damage-optimizer

## updating the regulation data
The included [regulation data file](new_regulation_data.json) contains all the necessary raw data. If, however, this file becomes outdated (e.g. because of an Elden Ring update) or if you are playing on an old version of the game you might want to generate your own regulation data file. To do this you'll need
- [UXM Selective Unpacker](https://github.com/Nordgaren/UXM-Selective-Unpack)
- [WitchyBND](https://github.com/ividyon/WitchyBND)

The first step is to use UXM to unpack the Elden Ring game files as explained on their Github page. AFAIK they are always unpacked directly into the game directory.

Next, open the *damage-optimizer.exe* and in the top left navigate to *file->generate regulation file->yes*. You will be prompted to choose the directory you previously unpacked the game files to (the game directory). Afterwards you will be prompted to choose the WitchyBND.exe file. Once that is done the generation process will begin. A bunch of text will be printed to the console window (which isn't relevant as long as everything works). The process is complete once *successfully generated regulation data file* is printed to the console.

The new *regulation_data.json* file will be created in same directory as the *damage-optimizer.exe* and can now be used via *file->open regulation file*.
