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

// Include for the instrumentation passes.

#ifndef _PASSES_INCLUDES_EXCLUDES_HPP_
#define _PASSES_INCLUDES_EXCLUDES_HPP_

#include "Libs.hpp" // the LLVM includes put there
#include "ADFSchedulerSignatures.hpp"

namespace INS {

   // function signature for terminating the scheduler
   llvm::StringRef schedTerminFunc;

   // function signature for initializing the scheduler
   llvm::StringRef schedInitiFunc;

   // function singature for passing tokens to the succeeding tasks
   llvm::StringRef tokenPassingFunc;

   // function signature for receiving tokens in a task
   llvm::StringRef tokenReceivingFunc;

   // function signature of a task body
   llvm::StringRef taskSignature;

   // This method parses the signature file and initializes
   // the signatures accordingly.
   void InitializeSignatures() {
     dfinspec::ADFSchedulerSignatures signatureFile;

     // check if file does not exist
     if (!signatureFile.good()) {
       printf( "                                           \n");
       printf( "===========================================\n");
       printf( " ERROR!                                    \n");
       printf( "Error opening signature file in Excludes.h \n");
       printf( "                                           \n");
       printf( "===========================================\n");
       exit ( EXIT_FAILURE );
     }

     if (signatureFile.is_open()) {

       for (std::string line; signatureFile.getline(line); ) {
         llvm::StringRef key(line);

         if (key.startswith("TASK:")) {
           // task body name
           std::string ts = std::get<1>(key.split(':')).str();
           char *holder   = new char[ts.length()];
           ts.copy(holder, ts.length());
           taskSignature  = llvm::StringRef(holder, ts.length());
         } else if (key.startswith("PASS:")) { // for passing token

           std::string ts   = std::get<1>(key.split(':')).str();
           char *holder     = new char[ts.length()];
           ts.copy(holder, ts.length());
           tokenPassingFunc = llvm::StringRef(holder, ts.length());
         } else if (key.startswith("RECV:")) {
           // for receiving token
           std::string ts     = std::get<1>(key.split(':')).str();
           char *holder       = new char[ts.length()];
           ts.copy(holder, ts.length());
           tokenReceivingFunc = llvm::StringRef(holder, ts.length());
         } else if (key.startswith("CREA:")) {
           // for creating task
           //adf_create_task
           continue;
         } else if (key.startswith("INIT:")) {
           // for scheduler initialization
           std::string ts = std::get<1>(key.split(':')).str();
           char * holder  = new char[ts.length()];
           ts.copy(holder, ts.length());
           schedInitiFunc = llvm::StringRef(holder, ts.length());
         } else if (key.startswith("TERM")) {
           // for scheduler rermination
           std::string ts  = std::get<1>(key.split(':')).str();
           char * holder   = new char[ts.length()];
           ts.copy(holder, ts.length());
           schedTerminFunc = llvm::StringRef(holder, ts.length());
         }
         line.clear();
       } // end loop
       signatureFile.close(); // close file
     }
   }

   bool DontInstrument(llvm::StringRef name) {
     int status = -1;
     std::string tmpName(name.str());
     char* d = abi::__cxa_demangle(tmpName.c_str(),
                                   nullptr,
                                   nullptr,
                                   &status);
      if (! status) {
       llvm::StringRef ab(d);
       std::string dname(d);
       //errs() << ab << "\n";
       if (dname.find("genmat") != std::string::npos) {
         return true;
       }
     }
      return false;
     //return true;
   }

   llvm::StringRef demangleName(llvm::StringRef name)
   {
      int status = -1;
      std::string tmpName(name.str());
      char* d = abi::__cxa_demangle(tmpName.c_str(),
                                    nullptr,
                                    nullptr,
                                    &status);
      if (! status) {
        llvm::StringRef ab(d);
        return ab;
      }
      return name;
   }

   std::string Demangle(llvm::StringRef name)
   {
      int status = -1;
      char* d = abi::__cxa_demangle(name.str().c_str(),
                                    nullptr,
                                    nullptr,
                                    &status);
      if (! status) {
        std::string dname(d);
        return dname;
      }
      return name;
   }

   bool isTaskBodyFunction(llvm::StringRef name) {

     int status = -1;
     char* d =abi::__cxa_demangle(name.str().c_str(),
                                  nullptr,
                                  nullptr,
                                  &status);
     if (! status) {
       std::string dname(d);
       if (dname.find(taskSignature) != std::string::npos) {
         return true;
       }
     }
     return false;
   }

   bool isLLVMCall(llvm::StringRef name) {
      if (name.find("llvm") != llvm::StringRef::npos) {
        return true;
      }
      return false;
   }

   bool isPassTokenFunc(llvm::StringRef name) {
     return name.find(tokenPassingFunc) !=llvm::StringRef::npos;
   }

   bool isTaskCreationFunc(llvm::StringRef name) {
     return name.find("adf_create_task") != llvm::StringRef::npos;
   }

   bool isRuntimeInitializer(llvm::StringRef name){
     return name.find(schedInitiFunc)!= llvm::StringRef::npos;
   }

  bool isRuntimeTerminator(llvm::StringRef name) {
     return name.find(schedTerminFunc) != llvm::StringRef::npos;
  }

  void ClearSignatures() {
    delete [] schedTerminFunc.data();
    delete [] schedInitiFunc.data();
    delete [] tokenPassingFunc.data();
    delete [] tokenReceivingFunc.data();
    delete [] taskSignature.data();
  }
}

#endif
