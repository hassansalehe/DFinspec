/////////////////////////////////////////////////////////////////
//  DFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    Copyright (c) 2015 - 2018 Hassan Salehe Matar
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// implements the main function.

#include "checker.hpp"  // header
#include "validator.hpp"

int main(int argc, char * argv[]) {

  if (argc != 4) {
    std::cout << std::endl;
    std::cout << "ERROR!" << std::endl;
    std::cout << "Usage: ./DFchecker TraceLog.txt HBlog.txt IRlog.txt"
              << std::endl;
    std::cout << std::endl;
    exit(-1);
  }

  // take time at begin of analysis
  std::chrono::high_resolution_clock::time_point t1 =
      std::chrono::high_resolution_clock::now();

  Checker aChecker; // checker instance
  std::string logLine;

  std::ifstream HBlog(argv[2]); //  hb file
  // check if HBlog file successfully opened
  if (!HBlog.is_open()) {
    std::cout << "ERROR!" << std::endl;
    std::cout << "HBlog file: " << argv[2]
              << " could not open." << std::endl;
    exit(-1);
  }

  while ( getline(HBlog, logLine) ) {
    aChecker.addTaskNode(logLine);
  }
  HBlog.close();

  std::ifstream log(argv[1]); //  log file
  // check if trace file successfully opened
  if (!log.is_open()) {
    std::cout << "ERROR!" << std::endl;
    std::cout << "Trace file: " << argv[1]
              << " could not open." << std::endl;
    exit(-1);
  }

  while ( getline(log, logLine) ) {
    // processes log file and detects nondeterminism
    aChecker.processLogLines(logLine);
  }
  log.close();

  // validate the detected nondeterminism bugs
  BugValidator validator;
  validator.parseTasksIR( argv[3] ); // read IR file

  // do the validation to eliminate commutative operations
  aChecker.checkCommutativeOperations( validator );

  // take time at end of analyis
  std::chrono::high_resolution_clock::time_point t2 =
      std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>
      ( t2 - t1 ).count();

  // testing writes
  //aChecker.testing();
  aChecker.reportConflicts();
  aChecker.printHBGraph();
  aChecker.printHBGraphJS(); // print in JS format

  std::cout << "Checker execution time: "<< duration/1000.0
            << " milliseconds" << std::endl;
  return 0;
}
