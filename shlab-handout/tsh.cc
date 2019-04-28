// 
// tsh - A tiny shell program with job control
// 
// <Put your name and login ID here>
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string>

#include "globals.h"
#include "jobs.h"
#include "helper-routines.h"

using namespace std;

//
// Needed global variable definitions
//

static char prompt[] = "tsh> ";
int verbose = 0;

//
// You need to implement the functions eval, builtin_cmd, do_bgfg,
// waitfg, sigchld_handler, sigstp_handler, sigint_handler
//
// The code below provides the "prototypes" for those functions
// so that earlier code can refer to them. You need to fill in the
// function bodies below.
// 

//%%%%% Helper Functions %%%%%//
void errorMessage(const char *msg); //unix error from text
pid_t Fork(void); // Wrapper function for fork() from text

void Sigprocmask(int i, const sigset_t * mask, sigset_t * oldset);
void Sigaddset(sigset_t *set, int signum);
void Sigemptyset(sigset_t * oldset);
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%//

void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

//
// main - The shell's main routine 
//
int main(int argc, char **argv) 
{
  int emit_prompt = 1; // emit prompt (default)

  //
  // Redirect stderr to stdout (so that driver will get all output
  // on the pipe connected to stdout)
  //
  dup2(1, 2);

  /* Parse the command line :  Checks the ./tsh for option characters. In this case, -h -v and -p */ 
  char c;
  while ((c = getopt(argc, argv, "hvp")) != EOF) {
    switch (c) {
    case 'h':             // print help message
      usage();
      break;
    case 'v':             // emit additional diagnostic info
      verbose = 1;
      break;
    case 'p':             // don't print a prompt
      emit_prompt = 0;  // handy for automatic testing
      break;
    default:
      usage();
    }
  }

  //
  // Install the signal handlers
  //

  //
  // These are the ones you will need to implement
  //
  Signal(SIGINT,  sigint_handler);   // ctrl-c
  Signal(SIGTSTP, sigtstp_handler);  // ctrl-z
  Signal(SIGCHLD, sigchld_handler);  // Terminated or stopped child

  //
  // This one provides a clean way to kill the shell
  //
  Signal(SIGQUIT, sigquit_handler); 

  //
  // Initialize the job list
  //
  initjobs(jobs);

  //
  // Execute the shell's read/eval loop
  //
  for(;;) {
    //
    // Read command line
    //
    if (emit_prompt) {
      printf("%s", prompt);
      fflush(stdout);
    }

    char cmdline[MAXLINE];

    if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) {
      app_error("fgets error");
    }
    //
    // End of file? (did user type ctrl-d?)
    //
    if (feof(stdin)) {
      fflush(stdout);
      exit(0);
    }

    //
    // Evaluate command line
    //
    eval(cmdline);
    fflush(stdout);
    fflush(stdout);
  } 

  exit(0); //control never reaches here
}

void errorMessage(const char *msg) //unix style error message
{
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

pid_t Fork(void)  //Wrapper for fork funtion
{
  pid_t pid;

  if ((pid = fork()) < 0)
    errorMessage("Fork Error");
  return pid;
}
/////////////////////////////////////////////////////////////////////////////
//
// eval - Evaluate the command line that the user has just typed in
// 
// If the user has requested a built-in command (quit, jobs, bg or fg)
// then execute it immediately. Otherwise, fork a child process and
// run the job in the context of the child. If the job is running in
// the foreground, wait for it to terminate and then return.  Note:
// each child process must have a unique process group ID so that our
// background children don't receive SIGINT (SIGTSTP) from the kernel
// when we type ctrl-c (ctrl-z) at the keyboard.
//

void eval(char *cmdline) 
{
  /* Parse command line */
  //
  // The 'argv' vector is filled in by the parseline
  // routine below. It provides the arguments needed
  // for the execve() routine, which you'll need to
  // use below to launch a process.
  //
  char *argv[MAXARGS];

  pid_t pid;
  sigset_t mask;

  //
  // The 'bg' variable is TRUE if the job should run
  // in background mode or FALSE if it should run in FG
  //
  int bg = parseline(cmdline, argv); 
  if (argv[0] == NULL)  
    return;   /* ignore empty lines */

  // strcpy(buf, cmdline);
  if(!builtin_cmd(argv)){ // determines if cmdline was a built in function

  Sigemptyset(&mask); // empties the mask set
  Sigaddset(&mask, SIGCHLD); // adds the SIGCHLD signal to the mask list
  Sigprocmask(SIG_BLOCK, &mask, NULL); //Blocks the SIGCHLD signals inorder to statisfy race conditions.

    if((pid = Fork()) == 0){ // if Forked is the child process
      setpgid(0, 0);
      Sigprocmask(SIG_UNBLOCK, &mask, NULL); //unblocks the SIGCHLD signal before exicution
      if((execv(argv[0], argv)) < 0) // runs the desired prcocess in the child
        printf("%s: Command not found.\n", argv[0]);
        exit(0);
    }

    if (!bg){ //commands that will only apply to the foreground processes.
      // int status;
      addjob(jobs, pid, FG, cmdline); // adds jobs to though parent process
      Sigprocmask(SIG_UNBLOCK, &mask, NULL); // unblocks SIGCHLD in the parent process

      //what if the parent process just sleeps until the forgound finishs rather an actually waiting
      //thus similating a waitpid call?? hmmmmm

      // if (waitpid(pid, &status, WNOHANG|WUNTRACED) < 0) 
      //   errorMessage("waitfg: waitpid error");
      waitfg(pid);
    }
    else{ // commands for those background processes.
        addjob(jobs, pid, BG, cmdline); // adds jobs to though parent process
        Sigprocmask(SIG_UNBLOCK, &mask, NULL); // unblocks SIGCHLD in the parent process
        printf("[%d] (%d) %s", getjobpid(jobs, pid)->jid, pid, getjobpid(jobs, pid)->cmdline);
    }
  }
  return;
}

// be sure to unfold here dumbass
  /////////////////////////////////////////////////////////////////////////////
  //
  // builtin_cmd - If the user has typed a built-in command then execute
  // it immediately. The command name would be in argv[0] and
  // is a C string. We've cast this to a C++ string type to simplify
  // string comparisons; however, the do_bgfg routine will need 
  // to use the argv array as well to look for a job number.
  //
  int builtin_cmd(char **argv) 
  {
    string cmd(argv[0]);

    if (string(argv[0]) == "quit")
      exit(0); 
    if (string(argv[0]) == "jobs"){
      listjobs(jobs);
      return 1;
    }
    if (string(argv[0]) == "fg")
      printf("Command not yet implemented");

    if (string(argv[0]) == "bg")
      printf("Command not yet implemented");
    
    return 0;     /* not a builtin command */
  }

  /////////////////////////////////////////////////////////////////////////////
  //
  // do_bgfg - Execute the builtin bg and fg commands
  //
  void do_bgfg(char **argv) 
  {
    struct job_t *jobp=NULL;
      
    /* Ignore command if no argument */
    if (argv[1] == NULL) {
      printf("%s command requires PID or %%jobid argument\n", argv[0]);
      return;
    }
      
    /* Parse the required PID or %JID arg */
    if (isdigit(argv[1][0])) {
      pid_t pid = atoi(argv[1]);
      if (!(jobp = getjobpid(jobs, pid))) {
        printf("(%d): No such process\n", pid);
        return;
      }
    }
    else if (argv[1][0] == '%') {
      int jid = atoi(&argv[1][1]);
      if (!(jobp = getjobjid(jobs, jid))) {
        printf("%s: No such job\n", argv[1]);
        return;
      }
    }	    
    else {
      printf("%s: argument must be a PID or %%jobid\n", argv[0]);
      return;
    }

    //
    // You need to complete rest. At this point,
    // the variable 'jobp' is the job pointer
    // for the job ID specified as an argument.
    //
    // Your actions will depend on the specified command
    // so we've converted argv[0] to a string (cmd) for
    // your benefit.
    //
    string cmd(argv[0]);

    return;
  }

  /////////////////////////////////////////////////////////////////////////////
  //
  // waitfg - Block until process pid is no longer the foreground process
  //
  void waitfg(pid_t pid)
  {
    while(true){
        if (fgpid(jobs) != 0) //fancy fucntion found in the jobs.cc file. give 0 if there are no foreground processes.
          sleep(1); //simply puts the parent process to sleep while the foreground job works.
        else
          break; //wakes parent up after the child process has been reaped.
      }
    return;
  }

  /////////////////////////////////////////////////////////////////////////////
  //
  // Signal handlers
  //


  /////////////////////////////////////////////////////////////////////////////
  //
  // sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
  //     a child job terminates (becomes a zombie), or stops because it
  //     received a SIGSTOP or SIGTSTP signal. The handler reaps all
  //     available zombie children, but doesn't wait for any other
  //     currently running children to terminate.  
  //
void sigchld_handler(int sig) 
{
  int status;
  pid_t pid;

  while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0){
    if (WIFEXITED(status)){
      deletejob(jobs, pid);
    }
    else if (WIFSIGNALED(status)){
      deletejob(jobs, pid);
      printf ("Job [%d] (%d) terminated by signal 2", getjobpid(jobs, pid)->jid, getjobpid(jobs, pid)->pid);
    }
  }
  if (errno != ECHILD)
    errorMessage("waitpid erro   r\n");
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigint_handler - The kernel sends a SIGINT to the shell whenver the
//    user types ctrl-c at the keyboard.  Catch it and send it along
//    to the foreground job.  
//
void sigint_handler(int sig) 
{

  pid_t pid = fgpid(jobs);

  if (pid != 0){
    if (kill(-pid, sig) < 0)
      errorMessage("kill error");
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////
//
// sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
//     the user types ctrl-z at the keyboard. Catch it and suspend the
//     foreground job by sending it a SIGTSTP.  
//
void sigtstp_handler(int sig) 
{
  return;
}

/*********************
 * End signal handlers
 *********************/


void Sigprocmask(int i, const sigset_t * mask, sigset_t * oldset)
{
  if(sigprocmask(i, mask, oldset) < 0)
    errorMessage("sigprocmask error");
}

void Sigemptyset(sigset_t * oldset)
{
  if(sigemptyset(oldset) < 0)
    errorMessage("sigemptyset error");
}

void Sigaddset(sigset_t *set, int signum)
{
  if(sigaddset(set, signum) < 0)
    errorMessage("sigaddset error");
}
