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

#define ASSERT( cond, message )     \
 if (!(cond)) {                     \
    perror (message);               \
    putchar ('\n');                 \
    abort();                        \
}

const int BUF_SIZE = 1024;
const unsigned char BIT_MASK[8] = {1, 2, 4, 8, 16, 32, 64, 128};

int  child (int ppid);
void c_handler_wtd (int sig); //child_handler_what_to_do
void c_handler_get_bit (int sig);
void c_handler_par_term (int sig);
void p_handler (int sig);

int  Success = 0, Nbit = 0;
unsigned char Byte = 0;

int main (int argc, char** argv)
{
//=============== CHECKING_MAIN_ARGS ========================================
    ASSERT (argc == 2,
              "ERROR: the program needs one argument");

//=============== PREPARATIONS ==============================================
    struct sigaction act;
    struct sigaction *act_ptr = &act;
    memset (act_ptr, 0, sizeof(act));

    sigset_t set;
    sigset_t *set_ptr = &set;
    sigemptyset (set_ptr);
    sigaddset (set_ptr, SIGUSR1);
    sigprocmask (SIG_BLOCK, set_ptr, NULL); // не хочу получать SIGUSR1 во время программы в случайные моменты

    act.sa_handler = p_handler;
    act.sa_mask = set;
    sigaction (SIGUSR1, act_ptr, NULL);
    sigaction (SIGCHLD, act_ptr, NULL);

//=============== FORK ======================================================
    int ppid = getpid();
    int cpid = fork();
    ASSERT (cpid != -1,
            "ERROR: fork failed");
    if (cpid != 0);
    else {
        prctl (PR_SET_PDEATHSIG, SIGALRM, 0, 0, 0); // настриваю сигнал, который придет при смерти родителя
        ASSERT (kill (ppid, 0) != -1,
                    "ERROR: parent terminated, but the job even was not started");
        act.sa_handler = c_handler_par_term; // обработчик смерти родителя
        sigaction (SIGALRM, &act, NULL);
        sigaddset (set_ptr, SIGUSR2);
        act.sa_mask = set;
        sigprocmask (SIG_BLOCK, &set, NULL); // не хочу получать SIGUSR1 и SIGUSR2 во время программы в случайные моменты
        child (ppid);
        return 0;
    }

    unsigned char* Buf = (unsigned char*) calloc (BUF_SIZE, 1);
    ASSERT (Buf,
                "ERROR: calloc failed");

    int fd = open (argv[1], O_RDONLY);
    ASSERT (fd != -1,
                "ERROR: open (input) failed");

    sigemptyset (set_ptr);// опустошаем маску для sigsuspend
    sigsuspend (set_ptr); // ожидаю сигнала от ребенка, что он готов работать
    int nBytes = 0, i = 0;
    while (1) {
        nBytes = read (fd, Buf, BUF_SIZE);
        if (!nBytes) break;
        for (i = 0; i < nBytes; i++) {
            char byte = Buf[i];
            kill (cpid, SIGUSR1); // говорю ребенку, что буду посылать биты очередного байта
            sigsuspend (set_ptr); // жду ответа от ребенка
            for (Nbit = 0; Nbit < 8; Nbit++) {
                if ((byte & BIT_MASK [Nbit]) == 0) kill (cpid, SIGUSR1);
                else kill (cpid, SIGUSR2);
                sigsuspend (set_ptr); // жду сигнала от ребенка, что он обработал очередной бит
            }
        }
    }
    kill (cpid, SIGUSR2); // говорю ребенку, что закончил пересылку
    sigsuspend (set_ptr); // жду ответа, что ребенок меня понял и выставил Success = 1
    free (Buf);
    close (fd);
    return 0;
}

int child (int ppid)
{
    struct sigaction act;
    struct sigaction *act_ptr = &act;
    memset (act_ptr, 0, sizeof(act));

    sigset_t set;
    sigset_t *set_ptr = &set;
    sigemptyset (set_ptr); // для sigsuspend

    int nBytes = 0;
    unsigned char* Buf = (unsigned char*) calloc (BUF_SIZE, 1);
    ASSERT (Buf,
                "ERROR: calloc failed");

    kill (ppid, SIGUSR1); // я готов принимать файл
    while (1) {
        act.sa_handler = c_handler_wtd; // меняю хэндлер, с помощью которого пойму, что делать дальше
        sigaction (SIGUSR1, &act, NULL);
        sigaction (SIGUSR2, &act, NULL);
        sigsuspend (set_ptr); //узнаю, что делать дальше
        if (Success) break;

        kill (ppid, SIGUSR1);
        act.sa_handler = c_handler_get_bit; // ставлю обработчик для приема битов очередного байта
        sigaction (SIGUSR1, &act, NULL);
        sigaction (SIGUSR2, &act, NULL);
        Byte = 0;
        for (Nbit = 0; Nbit < 7; Nbit++) {
            sigsuspend (set_ptr);
            kill (ppid, SIGUSR1); // говорю родителю, что обработал бит и могу обрабатывать следующий
        }
        Nbit = 7;
        sigsuspend (set_ptr);
        Buf[nBytes] = Byte;
        nBytes++;
        if (nBytes == BUF_SIZE) {
            write (STDOUT_FILENO, Buf, nBytes);
            memset (Buf, 0, BUF_SIZE);
            nBytes = 0;
        }
        kill (ppid, SIGUSR1); // говорю родителю, что обработал последний бит и могу обрабатывать следующий
    }
    write (STDOUT_FILENO, Buf, nBytes);

//=============== FINISHING =================================================
    kill (ppid, SIGUSR1);
    free (Buf);
    return 0;
}


//=============== CHILD_HANDLERS ===========================================
void c_handler_wtd (int sig) //child_handler_what_to_do
{
    if (sig == SIGUSR2) Success = 1;
    else ASSERT (sig == SIGUSR1,
                    "ERROR: c_handler_wtd was called by wrong signal");
}

void c_handler_get_bit (int sig)
{
    if (sig == SIGUSR2) Byte = Byte | BIT_MASK[Nbit];
    else ASSERT (sig == SIGUSR1,
                    "ERROR: c_handler_get_bit was called by wrong signal");
}

void c_handler_par_term (int sig)
{
    ASSERT (sig == SIGALRM,
                "ERROR: c_handler_par_term was called by wrong signal");
    ASSERT (Success == 1,
                "ERROR: parent terminated, but the job was not finished");
}
//=============== PARENT_HANDLERS ============================================
void p_handler (int sig)
{
    if (sig == SIGCHLD) {
        printf ("ERROR: child terminated, but the job was not finished\n");
        kill (getpid(), SIGKILL);
    }

    ASSERT (sig == SIGUSR1,
                "ERROR: p_handler was called by wrong signal");
}
