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

// this file implements the checking tool functionalities.

#include "checker.h"  // header

//#define VERBOSE

void Checker::checkDetOnPreviousTasks(const Action& writeAction ) {
  auto wTable = writes.find(writeAction.addr);
  auto end = serial_bags[writeAction.tid]->HB.end();

  // 4.2.1 check conflicts with other parallel tasks
  for(auto parWrite = wTable->second.begin(); parWrite != wTable->second.end(); parWrite++) {
    if(parWrite->tid != writeAction.tid && parWrite->value != writeAction.value) {
      // not write of same task and same value
      auto HBfound = serial_bags[writeAction.tid]->HB.find(parWrite->tid);
      if(HBfound == end && parWrite->value != writeAction.value) {// 3. there's no happens-before
        // code for recording errors
        saveNondeterminismReport(writeAction, *parWrite);
      }
    }
  }
}

void Checker::saveWrite( const Action& writeAction ) {
  VALUE lineNo = writeAction.lineNo;
  string funcName = writeAction.funcName;
  // CASES
  // 1. first write -> just save
  // 2. nth write of the same task -> just save
  // 3. a previous write in a happens-before -> just save
  // 4. previous write is parallel:
  //    4.1 but same value -> append new write
  //    4.2 otherwise report a race and look for a happens-before
  //        write in the parallel writes,update and take it forward
  //        4.2.1 check conflicts with other parallel tasks

  if(writes.find(writeAction.addr) == writes.end()) { // 1. first write
     writes[writeAction.addr] = vector<Action>();
     writes[writeAction.addr].push_back( writeAction );
  }
  else {
    auto wTable = writes.find(writeAction.addr);
    auto lastWrt = wTable->second.back();
    if(lastWrt.tid == writeAction.tid) { // 2. same writer
      writes[writeAction.addr].pop_back();
      writes[writeAction.addr].push_back( writeAction );
      return;
    }
    else { // check race
      auto HBfound = serial_bags[writeAction.tid]->HB.find(lastWrt.tid);
      auto end = serial_bags[writeAction.tid]->HB.end();
      if(HBfound != end) {// 3. there's happens-before
	writes[writeAction.addr].pop_back();
	writes[writeAction.addr].push_back( writeAction );

	// check on the happens-before relations with the previous concurrent tasks
	checkDetOnPreviousTasks( writeAction );
        return;
      }
      else { // 4. parallel, possible race!

         wTable->second.push_back( writeAction );
         if(lastWrt.value == writeAction.value) // 4.1 same value written
           return; // no determinism error, just return
         else { // 4.2 this is definitely determinism error

           // code for recording errors
           saveNondeterminismReport(writeAction, lastWrt);

           // 4.2.1 check conflicts with other parallel tasks
           checkDetOnPreviousTasks( writeAction );
           return;
         }
      }
    }
  }
}


/**
 * Records the nondeterminism warning to the conflicts table.
 */
VOID Checker::saveNondeterminismReport(const Action& curWrite, const Action& prevWrite) {
  Conflict report(curWrite, prevWrite);
  // code for recording errors
  auto key = make_pair(prevWrite.tid, curWrite.tid);
  if(conflictTable.find(key) != conflictTable.end()) // exists
    conflictTable[key].addresses.insert( report );
  else { // add new
    conflictTable[key] = Report();
    conflictTable[key].task1Name = graph[prevWrite.tid].name;
    conflictTable[key].task2Name = graph[curWrite.tid].name;
    conflictTable[key].addresses.insert( report );
  }
}


// Adds a new task node in the simple happens-before graph
// @params: logLine, a log entry the contains the task ids
void Checker:: addTaskNode(string & logLine) {
    stringstream ssin(logLine);
    int sibId;
    int parId;

    ssin >> sibId;
    ssin >> parId;
    //cout << line << "(" << sibId << " " << parId << ")" << endl;

    if(graph.find(parId) == graph.end())
      graph[parId] = Task();

    if(graph.find(sibId) == graph.end())
      graph[sibId] = Task();

    graph[parId].outEdges.insert(sibId);
    graph[sibId].inEdges.insert(parId);
}


void Checker::processLogLines(string & line){

    stringstream ssin(line); // split string

    int taskID;
    string taskName;
    string operation;

    ssin >> taskID; // get task id
    ssin >> operation; // get operation

    // if write
    if(operation.find("WR") != string::npos) {

      Action action;
      string tempBuff;

      ssin >> tempBuff; // address
      action.addr = (ADDRESS)stoul(tempBuff, 0, 16);

      ssin >> tempBuff; // value
      action.value = stol(tempBuff);

      ssin >> tempBuff; // line number
      action.lineNo = stol(tempBuff);

      getline(ssin, action.funcName); // get function name
      action.tid = taskID;
      saveWrite( action );
    }
    else if (operation.find("RD") != string::npos) {
      // it is read just return for now.
      return;
    }
    // if new task creation, parents terminated
    else if(operation.find("ST") != string::npos) {

      ssin >> taskName; // get task name

      // we already know its parents
      // use this information to inherit or greate new serial bag
      auto parentTasks = graph[taskID].inEdges.begin();

      if(parentTasks == graph[taskID].inEdges.end()) { // if no HB tasks in graph

	// check if has no serial bag
	if(serial_bags.find(taskID) == serial_bags.end()) {
	  auto newTaskbag = new SerialBag();
	  if(graph.find(taskID) != graph.end()) {

	    // specify number of tasks dependent of this task
	    newTaskbag->outBufferCount = graph[taskID].outEdges.size();
	  }
	  else //put it in the simple HB graph
	    graph[taskID] = Task();

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
	for(; inEdge != graph[taskID].inEdges.end(); inEdge++) {
	  // take with outstr 1 and longest
	  auto curBag = serial_bags[*inEdge];
	  if(curBag->outBufferCount == 1) {
	    serial_bags.erase(*inEdge);
	    graph[taskID].inEdges.erase(*inEdge);
	    taskBag = curBag;
	    curBag->HB.insert(*inEdge);
	    break;  // could optimize by looking all bags
	  }
	}
	if(!taskBag)
	  taskBag = new SerialBag(); // no bag inherited

	// the number of inheriting bags
	taskBag->outBufferCount = graph[taskID].outEdges.size();

	// 2. merge the HBs of the parent nodes
	inEdge = graph[taskID].inEdges.begin();
	for(; inEdge != graph[taskID].inEdges.end(); inEdge++) {
	  auto aBag = serial_bags[*inEdge];

	  taskBag->HB.insert(aBag->HB.begin(), aBag->HB.end()); // merging...
	  taskBag->HB.insert(*inEdge); // parents happen-before me

	  aBag->outBufferCount--; // for inheriting bags
	  if(!aBag->outBufferCount)
	    serial_bags.erase(*inEdge);
	}
	graph[taskID].name = taskName; // set the name of the task
	serial_bags[taskID] = taskBag; // 3. add the bag to serial_bags
      }
    }
}

CONFLICT_PAIRS & Checker::getConflictingPairs() {

  // generate simplified version of conflicts from scratch
  conflictTasksAndLines.clear();

  // a pair of conflicting task body with a set of line numbers
  for(auto it = conflictTable.begin(); it != conflictTable.end(); ++it) {
    pair<string, string> namesPair = make_pair(it->second.task1Name, it->second.task2Name);
    if(conflictTasksAndLines.find(namesPair) == conflictTasksAndLines.end())
      conflictTasksAndLines[namesPair] = set<pair<int,int>>();

    // get line numbers
    for(auto conf = it->second.addresses.begin(); conf != it->second.addresses.end(); conf++) {
      pair<int,int> lines = make_pair(conf->lineNo1, conf->lineNo2);
      conflictTasksAndLines[namesPair].insert(lines);
    }
  }
  return conflictTasksAndLines;
}


VOID Checker::reportConflicts() {
  cout << "============================================================" << endl;
  cout << "                                                            " << endl;
  cout << "                    Summary                                 " << endl;
  cout << "                                                            " << endl;
  cout << " Total number of tasks: " <<  graph.size() << "             " << endl;
  cout << "                                                            " << endl;
  cout << "                                                            " << endl;
  cout << "                                                            " << endl;
  cout << "        Non-determinism checking report                     " << endl;
  cout << "                                                            " << endl;

#ifdef VERBOSE // print full summary
  cout << " The following " << conflictTable.size() <<" task pairs have conflicts: " << endl;

  for(auto it = conflictTable.begin(); it != conflictTable.end(); ++it) {
    cout << "    "<< it->first.first << " ("<< it->second.task1Name<<")  <--> ";
    cout << it->first.second << " (" << it->second.task2Name << ")";
    cout << " on "<< it->second.addresses.size() << " memory addresses" << endl;

    if(it->second.addresses.size() > 10)
      cout << "    showing at most 10 addresses                        " << endl;
    int addressCount = 0;

    for(auto conf = it->second.addresses.begin(); conf != it->second.addresses.end(); conf++) {
      cout << "      " <<  conf->addr << " lines: " << conf->funcName1 << ": " << conf->lineNo1;
      cout << ", "<< conf->funcName2 << ": " << conf->lineNo2 << endl;
      addressCount++;
      if(addressCount == 10) // we want to print at most 10 addresses if they are too many.
        break;
    }
  }

#else

  // a pair of conflicting task body with a set of line numbers
  for(auto it = conflictTasksAndLines.begin(); it!= conflictTasksAndLines.end(); it++)
  {
    cout << it->first.first << " <--> " << it->first.second << ": line numbers  {";
    for(auto ot = it->second.begin(); ot != it->second.end(); ot++)
      cout << ot->first <<" - "<< ot->second << ", ";
    cout << "}" << endl;
  }
#endif
  cout << "                                                            " << endl;
  cout << "============================================================" << endl;
}


VOID Checker::printHBGraph() {
  FILEPTR flowGraph;
  flowGraph.open("flowGraph.sif",  ofstream::out | ofstream::trunc);

  if( ! flowGraph.is_open() ) {
    cout << "Failed to write to file the graph structure" << endl;
    exit(-1);
  }

  for(auto it = graph.begin(); it != graph.end(); it++)
    for(auto out = it->second.outEdges.begin(); out != it->second.outEdges.end(); out++) {
       flowGraph << it->first << "_" << it->second.name << " pp ";
       flowGraph << *out << "_" << graph[*out].name << endl;
    }
  if(flowGraph.is_open())
    flowGraph.close();
}

VOID Checker::printHBGraphJS() {
  FILEPTR graphJS;
  graphJS.open("flowGraph.js",  ofstream::out | ofstream::trunc);

  if( ! graphJS.is_open() ) {
    cout << "Failed to write to file the graph structure" << endl;
    exit(-1);
  }

  // generate list of nodes
  graphJS << "nodes: [ \n";
  for(auto it = graph.begin(); it != graph.end(); it++) {
    if(it == graph.begin())
      graphJS << "      { data: { id: '" << it->second.name << it->first << "', name: '" << it->second.name << it->first << "' }}";
    else
      graphJS << ",\n      { data: { id: '" << it->second.name << it->first << "', name: '" << it->second.name << it->first << "' }}";
  }
  graphJS << "\n     ],\n";

  // generate list of edges
  graphJS << "edges: [ \n";
  int start = 1;
  for(auto it = graph.begin(); it != graph.end(); it++)
    for(auto out = it->second.outEdges.begin(); out != it->second.outEdges.end(); out++) {
      if(start) {
        graphJS << "      { data: { source: '" << it->second.name << it->first << "', target: '" << graph[*out].name << *out << "' }}";
        start = 0;
      }
      else
        graphJS << ",\n      { data: { source: '" << it->second.name << it->first << "', target: '" << graph[*out].name << *out << "' }}";
    }
  graphJS << "\n     ]\n";

   //    graphJS << it->first << "_" << it->second.name << " pp ";
   //    graphJS << *out << "_" << graph[*out].name << endl;
  if(graphJS.is_open())
    graphJS.close();
}


VOID Checker::testing() {
  for(auto it = writes.begin(); it != writes.end(); it++)
  {
     cout << it->first << ": Bucket {" << it->second.size();
     cout <<"} "<< endl;
  }
  cout << "Total Addresses: " << writes.size() << endl;

  // testing
  cout << "====================" << endl;
  for(auto it = serial_bags.begin(); it != serial_bags.end(); it++)
  {
      cout << it->first << " ("<< it->second->outBufferCount<< "): {";
      for(auto x = it->second->HB.begin(); x != it->second->HB.end(); x++ )
        cout << *x << " ";
      cout << "}" << endl;
  }
}


// implementation of the checker destructor
// frees the memory dynamically generated for S-bags
Checker::~Checker() {
 for(auto it = serial_bags.begin(); it != serial_bags.end(); it++)
   delete it->second;
}

