#include <iostream>
#include <cstring>

#include "adf.h"

using namespace std;
int   num_threads = 2;

// balance
float balance;

// tokens
int token1;
int token2;
int token3;

// tasks

/**
 * This initial task initializes the account balance
 */
void OpenAccountTask()
{
   // for sending tokens to commission, deposit and withdraw tasks.
   void *outtokens[] = {&token1, &token2, &token3};
   adf_create_task(1, 0, NULL, [=](token_t *tokens) -> void
   {
      balance  = 1000; // set balance
      int token = 1; // token value
      adf_pass_token(outtokens[0], &token, sizeof(token));    /* pass tokens */
      adf_pass_token(outtokens[1], &token, sizeof(token));    /* pass tokens */
      adf_pass_token(outtokens[2], &token, sizeof(token));    /* pass tokens */

      // stop task
      adf_task_stop();
   });
}


/**
 * Pays 20% commission the account balance
 */
void CommissionTask()
{

   void *intokens[] = {&token1}; // for receiving token
   adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
   {
      int token;
      float rate = 0.2; // commission rate

      // copy token value
      memcpy(&token, tokens->value, sizeof(token));

      // add commission
      balance -= (balance * rate);

      // end task
      adf_task_stop();
   });
}


/**
 * This task deposits 200 to the account balance
 *
 */
void DepositTask()
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
 * Withdraws 500 from the account balance.
 */
void WithdrawTask()
{
   void *intokens[] = {&token3}; // for receiving token
   adf_create_task(1, 1, intokens, [=] (token_t *tokens) -> void
   {
      int token;
      int amount = 500; // amount to withdraw

      // receive token
      memcpy(&token, tokens->value, sizeof(token));

      balance -= amount; // deduce amount withdrawn
      adf_task_stop(); // terminate task task
   });
}

/**
 * The main function
 */
int main(int argc, char** argv)
{

   adf_init(num_threads); // initialize the ADF scheduler

   CommissionTask(); // generate comission task instance
   DepositTask(); // generate deposit task instance
   WithdrawTask(); // generate withdraw task instance
   OpenAccountTask(); // generate the initial task instance

   adf_start();  // start sceduling dataflow tasks

   adf_taskwait(); // wait completion of all tasks

   adf_terminate(); // terminate ADF scheduler

   return 0;
}

