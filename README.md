#  This is a tool to detect non-determinism in shared memory dataflow applications.
This tool currently detects non-determinism in Atomic Dataflow (ADF) applications.
you can get more information in a relevant paper
[here](http://www.sciencedirect.com/science/article/pii/S016781911730193X).

### Copyright notice: ###

  Copyright (c) 2015 - 2018 Hassan Salehe Matar.
    Copying or using this code by any means whatsoever
    without consent of the owner is strictly prohibited.

### Contacts:
     hmatar-at-ku-dot-edu-dot-tr


### Prerequisites: ###
One of the following compilers is required.
  g++ 4.7 or later.
  clang++ 3.3. or later

### How to compile the tool: ###
Under the main directory, on the console/terminal run the command `$ ./install.sh`.

### How to run: ###
```bash
$ dfinspec <source_code_file> <gcc/clang_compiler_parameters>"
```
