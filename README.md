#  This is a tool to detect non-determinism in ADF applications. 

### Copyright notice: ###

  (c) 2015, 2016 - Hassan Salehe Matar & MSRC at Koc University.
    Copying or using this code by any means whatsoever 
    without consent of the owner is strictly prohibited.


### Contacts: 
     hmatar-at-ku-dot-edu-dot-tr


### Prerequisites: ###
One of the following compilers is required.
  g++ 4.7 or later.
  clang++ 3.3. or later

### How to compile the tool: ###
  Under the main directory, on the console/terminal run the command "make".

### How to run: ###

Given the log files traceLog.txt and HBlog.txt, the log files you collected after instrumenting and running your ADF application, execute the command "./ADFinspec traceLog.txt HBlog.txt"