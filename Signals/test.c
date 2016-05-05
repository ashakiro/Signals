#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/prctl.h>
#include "task_4.h"

int  Success = 0, Nbit = 0;
unsigned char Byte = 0;
unsigned char *Buf;

int main (int argc, char** argv)
{
//=============== CHECKING_MAIN_ARGS ========================================
    //ASSERT (argc == 2,
    //          "ERROR: the program needs one argument");

//=============== PREPARATIONS ==============================================
    struct sigaction act, oldact;
    struct sigaction *act_ptr    = &act;
    struct sigaction *oldact_ptr = &oldact;

    memset (act_ptr,    0, sizeof(act));
    memset (oldact_ptr, 0, sizeof(oldact));

    sigset_t set, oldset;
    sigset_t *set_ptr    = &set;
    sigset_t *oldset_ptr = &oldset;

    sigemptyset (set_ptr);
    sigaddset (set_ptr, SIGUSR1);
    sigaddset (set_ptr, SIGUSR2);
    sigprocmask (SIG_BLOCK, set_ptr, oldset_ptr);

    act.sa_handler = p_handler_wtd;
    act.sa_mask = set;
    sigaction (SIGUSR1, &act, &oldact);
    sigaction (SIGUSR2, &act, &oldact);

    act.sa_handler = p_handler_chld;
    sigaction (SIGCHLD, &act, &oldact);

//=============== FORK ======================================================
    int ppid = getpid();
    int cpid = fork();
    ASSERT (cpid != -1,
            "ERROR: fork failed");
    if (cpid != 0);
    else {
        prctl (PR_SET_PDEATHSIG, SIGUSR2, 0, 0, 0);
        act.sa_handler = c_handler_mfp;
        sigaction (SIGUSR1, &act, &oldact);
        sigaction (SIGUSR2, &act, &oldact);
        sigdelset (&set, SIGUSR1);
        sigprocmask (SIG_UNBLOCK, &set, &oldset);
        child (argv[1], ppid);
        return 0;
    }
    sleep(2);
    return 0;

//=============== RECIEVING A FILE FROM THE CHILD ===========================
    int nBytes = 0;
    Buf = (unsigned char*) calloc (BUF_SIZE, 1);
    ASSERT_snd (Buf,
                "ERROR: calloc failed");
    sigdelset (set_ptr, SIGUSR1);
    sigdelset (set_ptr, SIGUSR2);

    while (1) {
        act.sa_handler = p_handler_wtd;
        sigaction (SIGUSR1, &act, &oldact);
        sigaction (SIGUSR2, &act, &oldact);
        sigsuspend (set_ptr);
        if (Success) break;

        kill (cpid, SIGUSR1);
        act.sa_handler = p_handler_get_bit;
        sigaction (SIGUSR1, &act, &oldact);
        sigaction (SIGUSR2, &act, &oldact);
        for (Nbit = 0; Nbit < 7; Nbit++) {
            sigsuspend (set_ptr);
            printf ("\n*****%hhu*****\n", Byte);
            kill (cpid, SIGUSR1);
        }
        Nbit = 7;
        sigsuspend (set_ptr);
        printf ("\n=====%hhu=====\n", Byte);
        Buf[nBytes] = Byte;
        Byte = 0;

        nBytes++;
        if (nBytes == BUF_SIZE) {
            write (STDOUT_FILENO, Buf, nBytes);
            memset (Buf, 0, BUF_SIZE);
            nBytes = 0;
        }
        kill (cpid, SIGUSR1);
    }
    printf ("\n\n%d\n\n", nBytes);
    write (STDOUT_FILENO, Buf, nBytes);

//=============== FINISHING =================================================
    kill (cpid, SIGUSR1);
    sigsuspend (set_ptr);
    free (Buf);
    return 0;
}

//=============== CHILD =====================================================
int child (char* input, int ppid)
{
    sigset_t set, oldset;
    sigset_t *set_ptr    = &set;
    sigset_t *oldset_ptr = &oldset;
    sigemptyset (set_ptr);
    sigemptyset (oldset_ptr);

    Buf = (unsigned char*) calloc (BUF_SIZE, 1);
    ASSERT_snd (Buf,
                "ERROR: calloc failed");

    int fd = open (input, O_RDONLY);
    ASSERT_snd (fd > 2,
                "ERROR: open (input) failed");

    int nBytes = 0, i = 0;
    while (1) {
        nBytes = read (fd, Buf, BUF_SIZE);
        if (!nBytes) break;
        for (i = 0; i < nBytes; i++) {
            char byte = Buf[i];
            kill (ppid, SIGUSR1);
            sigsuspend (set_ptr);
            for (Nbit = 0; Nbit < 8; Nbit++) {
                if ((byte & BIT_MASK [7 - Nbit]) == 0) kill (ppid, SIGUSR1);
                else kill (ppid, SIGUSR2);
                sigsuspend (set_ptr);
            }
        }
    }

    kill (ppid, SIGUSR2);
    sigsuspend (set_ptr);
    free (Buf);
    close (fd);
    return 0;
}

//=============== PARENT_HANDLERS ===========================================
void p_handler_wtd (int sig) //parent_handler_what_to_do
{
    if (sig == SIGUSR1) printf ("(parent) p_handler_wtd was called by SIGUSR1\n");
    else if (sig == SIGUSR2) printf ("(parent) p_handler_wtd was called by SIGUSR2\n");
         else printf ("(parent) ERROR: p_handler_wtd was called by WRONG SIGNAL\n");

    if (sig == SIGUSR2) Success = 1;
    else ASSERT (sig == SIGUSR1,
                 "ERROR: p_handler_wtd was called by wrong signal");
}

void p_handler_get_bit (int sig)
{
    if (sig == SIGUSR1) printf ("(parent) p_handler_get_bit was called by SIGUSR1\n");
    else if (sig == SIGUSR2) printf ("(parent) p_handler_get_bit was called by SIGUSR2\n");
         else printf ("(parent) ERROR: p_handler_get_bit was called by WRONG SIGNAL\n");

    if (sig == SIGUSR2) Byte = Byte | BIT_MASK[7 - Nbit];
    else ASSERT_rcv (sig == SIGUSR1,
                          "ERROR: p_handler_get_bit was called by wrong signal");
}

void p_handler_chld (int sig)
{
    if (sig == SIGCHLD) printf ("(parent) p_handler_chld was called by SIGCHLD\n");
    else printf ("(parent) ERROR: p_handler_chld was called by WRONG SIGNAL\n");

    ASSERT_rcv (Success == 1,
                "ERROR: child terminated, but the job was not finished");
}

//=============== CHILD_HANDLERS ============================================
void c_handler_mfp (int sig) //child_handler_message_from_parent
{
    if (sig == SIGUSR1) printf ("(child) c_handler_mfp was called by SIGUSR1\n");
    else if (sig == SIGUSR2) printf ("(child) c_handler_mfp was called by SIGUSR2\n");
         else printf ("(child) ERROR: c_handler_mfp was called by WRONG SIGNAL\n");

    if (sig == SIGUSR2) {
        printf ("(child) ERROR: parent terminated, but the job was not finished\n");
        fflush (stdout);
        kill (getpid(), SIGKILL);
    }

    ASSERT_snd (sig == SIGUSR1,
                "ERROR: c_handler_mfp was called by wrong signal");
}

//=============== OTHER_FUNCTIONS ===========================================
void dump_mask (sigset_t* mask_ptr)
{
    printf ("\n********** MASK [%p] **********\n", mask_ptr);
    if (sigismember (mask_ptr, SIGUSR1)) printf ("SIGUSR1\n");
    if (sigismember (mask_ptr, SIGUSR2)) printf ("SIGUSR2\n");
    if (sigismember (mask_ptr, SIGCHLD)) printf ("SIGCHLD\n");
    printf ("*******************************************\n\n");
}
