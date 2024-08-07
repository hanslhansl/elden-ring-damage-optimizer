# elden-ring-damage-optimizer
A tool for finding the highest possible attack rating for every weapon and every character level in Elden Ring.

## how it works
#### setting the character stats
#### applying weapon filters
#### choosing the optimization target
## the algorithm
The brute force algorithm is pretty simple: It tries every stat distribution satisfying the provided minimum stats and character level with every weapon (according to the weapon filter options) and returns the highest result.

There are around 3216 weapons in Elden Ring (including different affinity versions). That in itself wouldn't be a challenge for today's cpus. The algorithm's computing time is mainly driven by the number of different stat distributions. Depending on the minimum stats and character level provided by the user the number of stat distributions can be as high as 57.538.251 (at character level 174 and all minimum stats at 1). $57.538.251 * 3216 = 185.043.015.216$ total variations to iterate over. That's a lot. Even after all the tinkering I did my PC still takes ~10 minutes for this (unrealistic) worst case scenario (with 13 allocated threads).

The solution: Don't apply the brute force algorithm to all weapons at the same time. Use the weapon filters to limit the optimization to e.g. just one weapon type or maybe even to a single weapon. The further you are away from character level 174 (be it above or below) and all minimum stats at 1 the less time it will take too.

Actually, there is another solution: A smarter algorithm. I'm working on it.
## updating the regulation data
The included [regulation data file](new_regulation_data.json) contains all the necessary raw data. If, however, this file becomes outdated (e.g. because of an Elden Ring update) or if you are playing on an old version of the game you might want to generate your own regulation data file. To do this you'll need
- [UXM Selective Unpacker](https://github.com/Nordgaren/UXM-Selective-Unpack)
- [WitchyBND](https://github.com/ividyon/WitchyBND)

The first step is to use UXM to unpack the Elden Ring game files as explained on their Github page. AFAIK they are always unpacked directly into the game directory.

Next, open the *damage-optimizer.exe* and in the top left navigate to *file->generate regulation file->yes*. You will be prompted to choose the directory you previously unpacked the game files to (the game directory). Afterwards you will be prompted to choose the WitchyBND.exe file. Once that is done the generation process will begin. A bunch of text will be printed to the console window (which isn't relevant as long as everything works). The process is complete once *successfully generated regulation data file* is printed to the console.

The new *regulation_data.json* file will be created in same directory as the *damage-optimizer.exe* and can now be used via *file->open regulation file*.
