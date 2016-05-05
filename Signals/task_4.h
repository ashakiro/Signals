#define DEBUG_MOD
#ifdef DEBUG_MOD
#define DEBUG
#else
#define DEBUG if(0)
#endif

#define ASSERT( cond, message )     \
 if (!(cond)) {                     \
    printf ("(sender) ");           \
    fflush (stdout);                \
    perror (message);               \
    putchar ('\n');                 \
    abort();                        \
}

#define ASSERT_snd( cond, message ) \
 if (!(cond)) {                     \
    printf ("(child) ");            \
    fflush (stdout);                \
    perror (message);               \
    putchar ('\n');                 \
    abort();                        \
 }

#define ASSERT_rcv( cond, message ) \
 if (!(cond)) {                     \
    printf ("(parent) ");           \
    fflush (stdout);                \
    perror (message);               \
    putchar ('\n');                 \
    abort();                        \
 }


enum SOME_CONSTS
{
    BUF_SIZE = 1024,
};

const unsigned char BIT_MASK[8] = {1, 2, 4, 8, 16, 32, 64, 128};

int child (char* input, int ppid);

void p_handler_wtd (int sig); //parent_handler_what_to_do
void p_handler_chld (int sig);
void p_handler_get_bit (int sig);
void c_handler_mfp (int sig); //child_handler_message_from_parent

void dump_mask (sigset_t* mask_ptr);
