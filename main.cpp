#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <vector>
#include <set>

#include "checker.h"  // header


int main(int argc, char * argv[])
{

  if(argc != 3) {
    cout << endl;
    cout << "ERROR!" << endl;
    cout << "Usage: ./ADDFinspec TraceLog.txt HBlog.txt" << endl;
    cout << endl;
    exit(-1); 
  }

  Checker aChecker; // checker instance
  string logLine;
  ifstream HBlog(argv[2]); //  hb file

  while( getline(HBlog, logLine) )
  {
    aChecker.addTaskNode(logLine);
  }
  HBlog.close();

  ifstream log(argv[1]); //  log file
  while( getline(log, logLine) ) {
    aChecker.processLogLines(logLine); 
  }
  // testing writes
  aChecker.testing();
  return 0;
}
