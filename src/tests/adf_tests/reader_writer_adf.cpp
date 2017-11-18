#include <iostream>
#include <cstring>

#include "adf.h"

using namespace std;
int   num_threads = 2;

// balance
double ResearchPosition = 1000;

// tokens
int token1;
int token2;

// tasks

void InitialTask()
{
   void *outtokens[] = {&token1, &token2};
   adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
   {
      int token = 1; // token value
      adf_pass_token(outtokens[0], &token, sizeof(token));    /* pass tokens */
      adf_pass_token(outtokens[1], &token, sizeof(token));    /* pass tokens */

      // stop task
      adf_task_stop();
   });
}


void ReaderTask()
{
   void *intokens[] = {&token1}; // for receiving token
   adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
   {
      int token;

      // copy token value
      memcpy(&token, tokens->value, sizeof(token));

      // read research position
      int rp = ResearchPosition;

      // end task
      adf_task_stop();
   });
}


void WriterTask()
{
   void *intokens[] = {&token2}; // for receiving token
   adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
   {
      int token;

      // read
      int temp = ResearchPosition;

      // receive token
      memcpy(&token, tokens->value, sizeof(token));

      // write to research position
      ResearchPosition = 711;

      adf_task_stop();
   });
}

/**
 * The main function
 */
int main(int argc, char** argv)
{

   adf_init(num_threads); // initialize the ADF scheduler

   InitialTask(); // generate comission task instance
   WriterTask(); // generate deposit task instance
   ReaderTask(); // generate withdraw task instance

   adf_start();  // start sceduling dataflow tasks

   adf_taskwait(); // wait completion of all tasks

   adf_terminate(); // terminate ADF scheduler

   return 0;
}

