#include <iostream>
#include <cstring>

#include "adf.h"

using namespace std;
int   num_threads = 2;

// balance
float balance = 1000;

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


void Task1()
{
   void *intokens[] = {&token1}; // for receiving token
   adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
   {
      int token;

      // copy token value
      memcpy(&token, tokens->value, sizeof(token));

      // add commission
      balance *= 200;

      // end task
      adf_task_stop();
   });
}


void Task2()
{
   void *intokens[] = {&token2}; // for receiving token
   adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
   {
      int token;
      int amount = 200;

      // receive token
      memcpy(&token, tokens->value, sizeof(token));

      balance += amount;

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
   Task1(); // generate deposit task instance
   Task2(); // generate withdraw task instance

   adf_start();  // start sceduling dataflow tasks

   adf_taskwait(); // wait completion of all tasks

   adf_terminate(); // terminate ADF scheduler

   return 0;
}

