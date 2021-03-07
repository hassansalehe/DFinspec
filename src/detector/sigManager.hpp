/////////////////////////////////////////////////////////////////
//  DFinspec: a lightweight non-determinism checking
//          tool for ADF applications
//
//    (c) 2015 - 2018 Hassan Salehe Matar
//      Copying or using this code by any means whatsoever
//      without consent of the owner is strictly prohibited.
//
//   Contact: hmatar-at-ku-dot-edu-dot-tr
//
/////////////////////////////////////////////////////////////////

// Defines class for managing function names

#ifndef _DETECTOR_SIGMANAGER_HPP_
#define _DETECTOR_SIGMANAGER_HPP_

#include <unordered_map>
#include <cassert>

class SigManager {

private:
  std::unordered_map<INTEGER, std::string> functions;

public:
  /**
   * Stores function "name" with id "id"
   */
  void addFuncName(std::string name, INTEGER id) {

    assert(functions.find(id) == functions.end());
    functions[id] = name;
  }

  /**
   * Returns the function signature given the function id
   */
  std::string getFuncName(INTEGER id) {

    auto fIdptr = functions.find( id );
    if ( fIdptr == functions.end() ) {
      std::cout << "This function Id never exists: "
                << id << std::endl;
    }
    assert(fIdptr != functions.end());
    return fIdptr->second;
  }

  /**
   * Returns the function name identifier
   */
  INTEGER getFuncId(std::string name) {
    for (const auto& func : functions) {
       if (func.second == name) {
         return func.first;
       }
     }
    return 0; // FIXME
  }

}; // end class

#endif // SigManager_HPP_
