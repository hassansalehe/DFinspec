/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    (c) 2015, 2016 - Hassan Salehe Matar & MSRC at Koc University
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// implements the main function.

#include "checker.h"  // header
#include "validator.h"

using namespace std::chrono;

int main(int argc, char * argv[])
{

  if(argc != 4) {
    cout << endl;
    cout << "ERROR!" << endl;
    cout << "Usage: ./ADDFinspec TraceLog.txt HBlog.txt IRlog.txt" << endl;
    cout << endl;
    exit(-1);
  }

  // take time at begin of analysis
  high_resolution_clock::time_point t1 = high_resolution_clock::now();

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
  log.close();

  // validate the detected nondeterminism bugs
  BugValidator validator;
  validator.parseTasksIR( argv[3] ); // read IR file

  // do the validation
  validator.validate( aChecker.getConflictingPairs() );

  // take time at end of analyis
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>( t2 - t1 ).count();

  // testing writes
  //aChecker.testing();
  aChecker.reportConflicts();
  aChecker.printHBGraph();
  aChecker.printHBGraphJS(); // print in JS format

  cout << "Checker execution time: "<< duration << " microseconds" << endl;

  return 0;
}
