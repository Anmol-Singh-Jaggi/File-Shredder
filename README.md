#File-Shredder
=============

A C++ program to shred files and folders securely.

**Requirements -:**
 - A compiler supporting C++11
 - The [Boost][1] [filesystem][2] library.
 
***Features -:***
 - Renames files to random strings before shredding them.
 - Asks for confirmation before shredding a file.
 - Creates a log entry for every file being shredded.

**Files once shred cannot be recovered !! Use with extreme caution !!**

**Warning** : It will not work for drives/file-systems which perform [wear leveling][3] or other similar techniques.

**TODO -:**
 - Ask for the number of iteration to be performed through the command line argument list.
 - Provide an option of *silent* execution, which, when activated, does not ask for user confirmation before shredding a file.
 - Ask the user whether to follow links or not ( as a command-line argument ).
 
[1]:http://www.boost.org
[2]:http://www.boost.org/doc/libs/1_57_0/libs/filesystem/doc/index.htm
[3]:http://en.wikipedia.org/wiki/Wear_leveling
