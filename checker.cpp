/////////////////////////////////////////////////////////////////
//  ADFinspec: a lightweight non-determinism checking 
//          tool for ADF applications
//
//    (c) 2015 - Hassan Salehe Matar & MSRC at Koc University
//      Copying or using this code by any means whatsoever 
//      without consent of the owner is strictly prohibited.
//   
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// this file implements the checking tool functionalities.

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

void Checker::saveWrite(INTEGER taskId, ADDRESS addr, VALUE value) {
  // CASES
  // 1. first write -> just save
  // 2. nth write of the same task -> just save
  // 3. a previous write in a happens-before -> just save
  // 4. previous write is parallel:
  //    4.1 but same value -> append new write
  //    4.2 otherwise report a race and look for a happens-before 
  //        write in the parallel writes,update and take it forward

  if(writes.find(addr) == writes.end()) { // 1. first write
     writes[addr] = vector<Write>();
     writes[addr].push_back(Write(taskId, value));
  }
  else {
    auto wTable = writes[addr];
    auto lastWrt = wTable.back();
    if(lastWrt.wrtTaskId == taskId) { // 2. same writer
      lastWrt.value = value;
      return;
    }
    else { // check race
      auto found = p_bags[taskId]->HB.find(lastWrt.wrtTaskId);
      auto end = p_bags[taskId]->HB.end();
      if(found != end) {// 3. there's happens-before
        lastWrt.wrtTaskId = taskId;
        lastWrt.value = value;
        return;
      }
      else { // 4. parallel, possible race!

         wTable.push_back(Write(taskId, value));
         if(lastWrt.value == value) // 4.1 same value written
           return; // no determinism error, just return
         else { // 4.2 this is definitely determinism error

            // code for recording errors
            auto key = make_pair(lastWrt.wrtTaskId, taskId);
            if(conflictTable.find(key) != conflictTable.end())
              conflictTable[key].addresses.insert(addr);
            else {
              conflictTable[key] = Report();
              conflictTable[key].task1Name = p_bags[lastWrt.wrtTaskId]->name;
              conflictTable[key].task2Name = p_bags[taskId]->name;
              conflictTable[key].addresses.insert(addr);  
            }
            return;
         }
      }
    }
  }
}

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
    ssin >> taskName; // get task name
    ssin >> operation; // get operation

    // if new task creation
    if(operation.find("ST") != string::npos) {

      if(ssin.good()) { // get parent id
        string t;
        ssin >> t;
        // remove spaces
        t.erase(remove_if(t.begin(), t.end(), [](char x){return isspace(x);}), t.end());

        if(t.length() <= 0) { // initial tasks
          if(p_bags.find(taskID) == p_bags.end()) {
            auto newTask = new Sbag();
            if(graph.find(taskID) != graph.end()) {
              newTask->outStr = graph[taskID].outEdges.size();
            }
            newTask->name = taskName;
            p_bags[taskID] = newTask;
          }
        }
        else { // other tasks, have parents
          INTEGER parentId = stoi(t);

          // check if no bag for this task
          if(p_bags.find(taskID) == p_bags.end()) {           
            // clone the parent bag or take it all.
            PSbag taskBag = NULL;

            // 1.find the parent bag which can be inherited       
            auto inEdg = graph[taskID].inEdges.begin(); 
            for(; inEdg != graph[taskID].inEdges.end(); inEdg++) {
              // take with outstr 1 and longest
              auto curBag = p_bags[*inEdg];
              if(curBag->outStr == 1) {
                p_bags.erase(*inEdg);  
                graph[taskID].inEdges.erase(*inEdg);
                taskBag = curBag;
                curBag->HB.insert(*inEdg);
                break;  // could optimize by looking all bags
              }
            }
            if(!taskBag)
              taskBag = new Sbag(); // no bag inherited

            // for inheriting bags
            taskBag->outStr = graph[taskID].outEdges.size();

            // 2. merge the HBs of the parent nodes
            inEdg = graph[taskID].inEdges.begin(); 
            for(; inEdg != graph[taskID].inEdges.end(); inEdg++) { 
              auto aBag = p_bags[*inEdg];

              taskBag->HB.insert(aBag->HB.begin(), aBag->HB.end()); // merging...
              taskBag->HB.insert(*inEdg); // parents happen-before me

              aBag->outStr--; // for inheriting bags
              if(!aBag->outStr) 
                p_bags.erase(*inEdg);
            }
            taskBag->name = taskName;
            p_bags[taskID] = taskBag; // 3. add the bag to p_bags
          }
        }
        //cout << tid << endl; 
      }
    }
    // if write
    else if(operation.find("WR") != string::npos) {

      string addr; // address
      ssin >> addr;

      ADDRESS address = (ADDRESS)stoul(addr, 0, 16);
      string value;  // value
      ssin >> value;
      VALUE val = stol(value);
      saveWrite(taskID, address, val);
    }
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
  cout << " The following " << conflictTable.size() <<" task pairs have conflicts: " << endl;

  for(auto it = conflictTable.begin(); it != conflictTable.end(); ++it) {
    cout << "    "<< it->first.first << " ("<< it->second.task1Name<<")  <--> ";
    cout << it->first.second << " (" << it->second.task2Name << ")";
    cout << " on "<< it->second.addresses.size() << " memory addresses" << endl;
  }

  cout << "                                                            " << endl;
  cout << "============================================================" << endl;
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
  for(auto it = p_bags.begin(); it != p_bags.end(); it++)
  {
      cout << it->first << " ("<< it->second->outStr<< "): {";
      for(auto x = it->second->HB.begin(); x != it->second->HB.end(); x++ )
        cout << *x << " ";
      cout << "}" << endl;
  }
}


// implementation of the checker destructor
// frees the memory dynamically generated for S-bags
Checker::~Checker() {
 for(auto it = p_bags.begin(); it != p_bags.end(); it++)
   delete it->second;
}

