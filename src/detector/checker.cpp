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

// this file implements the checking tool functionalities.

#include "checker.hpp"
#include "sigManager.hpp" // for managing function names
#include "MemoryActions.hpp"

#define VERBOSE
#define CONC_THREASHOLD 5

void Checker::saveTaskActions( const MemoryActions &taskActions ) {

  // CASES
  // 1. first action -> just save
  // 2. nth write of the same task -> just save
  // 3. a previous write in a happens-before -> just save
  // 4. previous write is parallel:
  //    4.1 but same value -> append new write
  //    4.2 otherwise report a race and look for a happens-before
  //        write in the parallel writes,update and take it forward
  //        4.2.1 check conflicts with other parallel tasks

  auto perAddrActions = writes.find( taskActions.addr );
  if (perAddrActions == writes.end()) { // 1. first action
     writes[taskActions.addr] = std::list<MemoryActions>();
  }

  std::list<MemoryActions> &AddrActions = writes[taskActions.addr];
  for (auto& lastWrt : AddrActions) {
    // actions of same task
    if ( taskActions.taskId == lastWrt.taskId) continue;

    auto HBfound =
        serial_bags[ taskActions.taskId ]->HB.find(lastWrt.taskId);
    auto end = serial_bags[taskActions.taskId]->HB.end();

    if (HBfound != end) continue; // 3. there's happens-before

    // 4. parallel, possible race! ((check race))

    // check write-write case (different values written)
    // 4.1 both write to shared memory
    if ( (taskActions.action.isWrite && lastWrt.action.isWrite) &&
       (taskActions.action.value != lastWrt.action.value) ) {
         // write different values
      // code for recording errors
      saveNondeterminismReport( taskActions.action, lastWrt.action );
    }
    // 4.2 read-after-write or write-after-read conflicts
    // (a) taskActions is read-only and lastWrt is a writer
    else if ( (!taskActions.action.isWrite)
            &&  lastWrt.action.isWrite ) {
      // code for recording errors
      saveNondeterminismReport(taskActions.action, lastWrt.action);
    }
    // (b) lastWrt is read-only and taskActions is a writer
    else if ( (!lastWrt.action.isWrite)
            &&  taskActions.action.isWrite ) {
            // the other task is writer.
      // code for recording errors
      saveNondeterminismReport(taskActions.action, lastWrt.action);
    }
  }

  if (AddrActions.size() >= CONC_THREASHOLD) {
    AddrActions.pop_front(); // remove oldest element
  }
  writes[taskActions.addr].push_back( taskActions ); // save
}


/**
 * Records the nondeterminism warning to the conflicts table.
 * This is per pair of concurrent tasks.
 */
VOID Checker::saveNondeterminismReport(const Action &curMemAction,
                                       const Action &prevMemAction) {
  Conflict report(curMemAction, prevMemAction);
  // code for recording errors
  const char *task1Name = graph[curMemAction.taskId].name.c_str();
  const char *task2Name = graph[prevMemAction.taskId].name.c_str();

  auto taskPair = std::make_pair(task1Name, task2Name);
  if (conflictTable.find(taskPair) != conflictTable.end()) {// exists
    conflictTable[taskPair].buggyAccesses.insert( report );
  } else { // add new
    conflictTable[taskPair] = Report();
    conflictTable[taskPair].task1Name = graph[curMemAction.taskId].name;
    conflictTable[taskPair].task2Name = graph[prevMemAction.taskId].name;
    conflictTable[taskPair].buggyAccesses.insert( report );
  }
}


// Adds a new task node in the simple happens-before graph
// @params: logLine, a log entry the contains the task ids
void Checker::addTaskNode(std::string &logLine) {
    std::stringstream ssin(logLine);
    int sibId;
    int parId;

    ssin >> sibId;
    ssin >> parId;
    //cout << line << "(" << sibId << " " << parId << ")" << std::endl;

    if (graph.find(parId) == graph.end())
      graph[parId] = Task();

    if (graph.find(sibId) == graph.end())
      graph[sibId] = Task();

    graph[parId].outEdges.insert(sibId);
    graph[sibId].inEdges.insert(parId);
}

/** Constructs action object from the log file */
void Checker::constructMemoryAction(
    std::stringstream &ssin,
    std::string &operation,
    Action &action) {

    std::string tempBuff;
    ssin >> tempBuff; // address
    action.addr = (ADDRESS)stoul(tempBuff, 0, 16);

    ssin >> tempBuff; // value
    action.value = stol(tempBuff);

    ssin >> tempBuff; // line number
    action.lineNo = stol(tempBuff);

    ssin >> action.funcId; // get function id

    if (operation == "W") {
      action.isWrite = true;
    } else {
      action.isWrite = false;
    }

#ifdef DEBUG // check if data correctly set.
    std::cout << "Action constructed: ";
    std::ostringstream buff;
    action.printAction(buff);
    std::cout << buff.str() << std::endl;
#endif
}

void Checker::processLogLines(std::string &line) {

  std::stringstream ssin(line); // split string

  int taskID;
  std::string taskName;
  std::string operation;

  ssin >> taskID; // get task id
  ssin >> operation; // get operation

  if (operation.find("W") != std::string::npos || // write action, or
      operation.find("R") != std::string::npos) { // read action
    Action action;
    action.taskId = taskID;
    constructMemoryAction(ssin, operation, action);

    if (action.funcId == 0) {
     std::cout << "Warning function Id 0: " << line << std::endl;
     exit(0);
    }

    MemoryActions memActions( action ); // save first action

    if ( !ssin.eof() ) { // there are still tokens
      std::string separator;
      ssin >> separator;
      ssin >> taskID;
      Action lastWAction;
      lastWAction.taskId = taskID;
      ssin >> operation;
      constructMemoryAction(ssin, operation, lastWAction);
      memActions.storeAction( lastWAction ); // save second action
    }

    saveTaskActions( memActions ); // save the actions
  } else if (operation.find("F") != std::string::npos) {
  // Check if this is just function name

    // task id position is func ID in this case
    int funcID = taskID;

    std::string funcName;
    getline(ssin, funcName); // get function name

    // save the function name
    signatureManager.addFuncName(funcName, funcID);
  } else if (operation.find("B") != std::string::npos) {
  // if new task creation, parents terminated

    ssin >> taskName; // get task name

    // we already know its parents
    // use this information to inherit or greate new serial bag
    auto parentTasks = graph[taskID].inEdges.begin();
    if (parentTasks == graph[taskID].inEdges.end()) { // if no HB tasks in graph

      // check if has no serial bag
      if (serial_bags.find(taskID) == serial_bags.end()) {
        auto newTaskbag = new SerialBag();
        if (graph.find(taskID) != graph.end()) {
          // specify number of tasks dependent of this task
          newTaskbag->outBufferCount = graph[taskID].outEdges.size();
        } else { //put it in the simple HB graph
          graph[taskID] = Task();
        }
        graph[taskID].name = taskName; // save the name of the task
        serial_bags[taskID] = newTaskbag;
      }
    }
    else { // has parent tasks
      // look for the parents serial bags and inherit them
      // or construct your own by cloning the parent's

      // 1.find the parent bag which can be inherited
      SerialBagPtr taskBag = NULL;
      auto inEdge = graph[taskID].inEdges.begin();
      for (; inEdge != graph[taskID].inEdges.end(); inEdge++) {

        // take with outstr 1 and longest
        auto curBag = serial_bags[*inEdge];
        if (curBag->outBufferCount == 1) {
          serial_bags.erase(*inEdge);
          graph[taskID].inEdges.erase(*inEdge);
          taskBag = curBag;
          curBag->HB.insert(*inEdge);
          break;  // could optimize by looking all bags
        }
      }

      if (!taskBag) {
        taskBag = new SerialBag(); // no bag inherited
      }
      // the number of inheriting bags
      taskBag->outBufferCount = graph[taskID].outEdges.size();

      // 2. merge the HBs of the parent nodes
      inEdge = graph[taskID].inEdges.begin();
      for (; inEdge != graph[taskID].inEdges.end(); inEdge++) {

        auto aBag = serial_bags[*inEdge];

        taskBag->HB.insert(aBag->HB.begin(), aBag->HB.end()); // merging...
        taskBag->HB.insert(*inEdge); // parents happen-before me

        aBag->outBufferCount--; // for inheriting bags
        if (!aBag->outBufferCount) {
          serial_bags.erase(*inEdge);
        }
      }

      graph[taskID].name  = taskName; // set the name of the task
      serial_bags[taskID] = taskBag; // 3. add the bag to serial_bags
    }
  }
}

void Checker::checkCommutativeOperations(BugValidator &validator) {

  // generate simplified version of conflicts from scratch
  conflictTasksAndLines.clear();

  // a pair of conflicting task body with a set of line numbers
  for (auto it = conflictTable.begin(); it != conflictTable.end();
      ++it) {
    std::pair<std::string, std::string> namesPair =
        std::make_pair(it->second.task1Name, it->second.task2Name);
    if (conflictTasksAndLines.find(namesPair) ==
        conflictTasksAndLines.end()) {
      conflictTasksAndLines[namesPair] = std::set<std::pair<int,int>>();
    }
    // erase duplicates
    for (auto conf = it->second.buggyAccesses.begin();
         conf != it->second.buggyAccesses.end(); ) {
      std::pair<int,int> lines =
          std::make_pair(conf->action1.lineNo, conf->action2.lineNo);
      auto inserted = conflictTasksAndLines[namesPair].insert(lines);
      if (inserted.second == false) {
        conf = it->second.buggyAccesses.erase( conf );
      } else {
        conf++;
      } // end if
    } // end for
  }

  // a pair of conflicting task body with a set of line numbers
  for (auto it = conflictTable.begin(); it != conflictTable.end(); ) {

    Report & report = it->second;
    validator.validate( report );

    if ( !report.buggyAccesses.size() ) {
       it = conflictTable.erase(it);
    } else {
      ++it;
    } // end if
  } // end for
}


VOID Checker::reportConflicts() {
  std::cout << "============================================================" << std::endl;
  std::cout << "                                              " << std::endl;
  std::cout << "                    Summary                   " << std::endl;
  std::cout << "                                              " << std::endl;
  std::cout << " Total number of tasks: " <<  graph.size()      << std::endl;
  std::cout << "                                              " << std::endl;
  std::cout << "                                              " << std::endl;
  std::cout << "                                              " << std::endl;
  std::cout << "              Non-determinism checking report " << std::endl;
  std::cout << "                                              " << std::endl;

  // print appropriate message in case no errors found
  if (! conflictTable.size() ) {
    std::cout << "                 No nondeterminism found!   " << std::endl;
  } else {
    std::cout << " The following " << conflictTable.size()
              << " task pairs have conflicts: " << std::endl;
  }

#ifdef VERBOSE // print full summary
  for (auto it = conflictTable.begin(); it != conflictTable.end(); ++it) {
    std::cout << "    "<< it->first.first << " ("
              << it->second.task1Name<<")  <--> ";
    std::cout << it->first.second << " ("
              << it->second.task2Name << ")";
    std::cout << " on "<< it->second.buggyAccesses.size()
              << " memory addresses" << std::endl;

    if (it->second.buggyAccesses.size() > 10) {
      std::cout << "    showing at most 10 addresses: " << std::endl;
    }

    int addressCount =  0;
    Report & report  =  it->second;
    for (auto conf = report.buggyAccesses.begin();
         conf != report.buggyAccesses.end(); conf++) {
      std::cout << "      " <<  conf->addr << " lines: "
                << signatureManager.getFuncName( conf->action1.funcId )
                << ": " << conf->action1.lineNo << ", "
                << signatureManager.getFuncName( conf->action2.funcId)
                << ": " << conf->action2.lineNo << std::endl;

      addressCount++;
      if (addressCount == 10) {
        // we want to print at most 10 addresses if they are too many.
        break;
      }
    }
  } // end for

#else // no verbose
  // a pair of conflicting task body with a set of line numbers
  for (auto it = conflictTable.begin();
       it!= conflictTable.end(); it++) {
    Report &report = it->second;
    std::cout << report.task1Name << " <--> "
              << report.task2Name << ": line numbers  {";

    for (auto conflict = report.buggyAccesses.begin();
         conflict != report.buggyAccesses.end(); conflict++) {
      std::cout << conflict->action1.lineNo << " - "
                << conflict->action2.lineNo << ", ";
    }
    std::cout << "}" << std::endl;
  }
#endif // end no verbose
  std::cout << "                                              " << std::endl;
  std::cout << "============================================================" << std::endl;
}


VOID Checker::printHBGraph() {
  FILEPTR flowGraph;
  flowGraph.open("flowGraph.sif",
      std::ofstream::out | std::ofstream::trunc);

  if ( ! flowGraph.is_open() ) {
    std::cout << "Failed to write to file the graph structure\n";
    exit(-1);
  }

  for (auto it = graph.begin(); it != graph.end(); it++) {
    for (auto out = it->second.outEdges.begin();
        out != it->second.outEdges.end(); out++) {
       flowGraph << it->first << "_" << it->second.name << " pp ";
       flowGraph << *out << "_" << graph[*out].name << std::endl;
    }
  }

  if (flowGraph.is_open()) flowGraph.close();
}

VOID Checker::printHBGraphJS() {
  FILEPTR graphJS;
  graphJS.open("flowGraph.js",
      std::ofstream::out | std::ofstream::trunc);

  if ( ! graphJS.is_open() ) {
    std::cout << "Failed to write to file the graph structure\n";
    exit(-1);
  }

  // generate std::list of nodes
  graphJS << "nodes: [ \n";
  for (auto it = graph.begin(); it != graph.end(); it++) {
    if (it == graph.begin()) {
      graphJS << "      { data: { id: '"
              << it->second.name << it->first
              << "', name: '" << it->second.name
              << it->first << "' }}";
    } else {
      graphJS << ",\n      { data: { id: '"
              << it->second.name << it->first
              << "', name: '" << it->second.name
              << it->first << "' }}";
    }
  }
  graphJS << "\n     ],\n";

  // generate std::list of edges
  graphJS << "edges: [ \n";
  int start = 1;
  for (auto it = graph.begin(); it != graph.end(); it++) {
    for (auto out = it->second.outEdges.begin();
        out != it->second.outEdges.end(); out++) {
      if (start) {
        graphJS << "      { data: { source: '" << it->second.name
                << it->first << "', target: '" << graph[*out].name
                << *out << "' }}";
        start = 0;
      } else {
        graphJS << ",\n      { data: { source: '" << it->second.name
                << it->first << "', target: '" << graph[*out].name
                << *out << "' }}";
      }
    }
  }
  graphJS << "\n     ]\n";

   //    graphJS << it->first << "_" << it->second.name << " pp ";
   //    graphJS << *out << "_" << graph[*out].name << std::endl;
  if (graphJS.is_open()) {
    graphJS.close();
  }
}


VOID Checker::testing() {
  for (auto it = writes.begin(); it != writes.end(); it++) {
     std::cout << it->first << ": Bucket {" << it->second.size();
     std::cout <<"} "<< std::endl;
  }
  std::cout << "Total Addresses: " << writes.size() << std::endl;

  // testing
  std::cout << "====================" << std::endl;
  for (auto it = serial_bags.begin(); it != serial_bags.end(); it++) {
      std::cout << it->first << " ("
                << it->second->outBufferCount<< "): {";
      for (auto x = it->second->HB.begin();
          x != it->second->HB.end(); x++ ) {
        std::cout << *x << " ";
      }
      std::cout << "}" << std::endl;
  }
}


// implementation of the checker destructor
// frees the memory dynamically generated for S-bags
Checker::~Checker() {
 for (auto it = serial_bags.begin(); it != serial_bags.end(); it++) {
   delete it->second;
 }
}
