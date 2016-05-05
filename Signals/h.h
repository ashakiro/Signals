#define DEBUG_MOD
#ifdef DEBUG_MOD
#define DEBUG
#else
#define DEBUG if(0)
#endif

#define ASSERT( cond, message )     \
 if (!(cond)) {                     \
    perror (message);               \
    putchar ('\n');                 \
    abort();                        \
}


enum SOME_CONSTS
{
    BUF_SIZE = 1,
};

const unsigned char BIT_MASK[8] = {1, 2, 4, 8, 16, 32, 64, 128};

int child (int ppid);

void c_handler_wtd (int sig); //child_handler_what_to_do
void c_handler_get_bit (int sig);
void p_handler (int sig);
