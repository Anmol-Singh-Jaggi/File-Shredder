#File-Shredder
=============

A C++ program to shred files and folders securely.

**Requirements -:**
 - A compiler supporting C++ 11
 - The [Boost][1] [filesystem][2] library.
 
***Features*** -

- Renames files to random strings before shredding them.
- Asks for confirmation before shredding a file.
- Creates a log entry for every file being shredded.

**Files once shred cannot be recovered !! Use with extreme caution !!**

**TODO -:**

- Ask for the number of iteration to be performed through the command line argument list.
- Provide an option of *silent* execution, which, when activated, does not ask for user confirmation before shredding a file.

[1]:http://www.boost.org
[2]:http://www.boost.org/doc/libs/1_57_0/libs/filesystem/doc/index.htm