#File-Shredder
An application to shred files and folders securely.

**Requirements -:**
 - A compiler supporting C++11
 - The [Boost.Filesystem][1] library.
 
**Features -:**
 - Renames files to random strings before shredding them.
 - Asks for confirmation before shredding a file.
 - Creates a log entry for every file being shredded.

**Files once shred cannot be recovered !! Use with extreme caution !!**

**Warning** : It will not work for drives/file-systems which perform [wear leveling][2] or other similar techniques.

**TODO -:**
 - Ask for the number of iteration to be performed while shredding.
 - Provide an option of *silent execution*, in which the program does not ask for user confirmation before shredding a file.

[1]:http://www.boost.org/doc/libs/1_57_0/libs/filesystem/doc/index.htm
[2]:http://en.wikipedia.org/wiki/Wear_leveling
