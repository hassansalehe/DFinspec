#  This is a tool to detect non-determinism in ADF applications. # 

### Copyright notice: ###

  (c) 2015 - Hassan Salehe Matar & MSRC at Koc University
    Copying or using this code by any means whatsoever 
    without consent of the owner is strictly prohibited.


### Contacts: 
     hmatar-at-ku-dot-edu-dot-tr


### Prerequisites: ###
g++ 4.7 and later.

### How to compile the tool: ###
  Under the main directory, on the console/terminal run the command "make".

### How to run: ###

Given the log files traceLog.txt and HBlog.txt, the log files you collected after instrumenting and running your ADF application, execute the command "./ADFinspec traceLog.txt HBlog.txt"