#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

static void f_on_exit(void){ printf("atexit: PID=%d завершает работу\n", getpid()); }

static void h_sigint(int s){ printf("SIGINT у PID=%d: %s\n", getpid(), strsignal(s)); }

static void h_sigterm(int sig, siginfo_t *info, void *u){
    (void)u;
    printf("SIGTERM у PID=%d: %s (от PID=%d)\n", getpid(), strsignal(sig), info ? info->si_pid : -1);
}

int main(void){
    setbuf(stdout, NULL);
    if (atexit(f_on_exit) != 0){ perror("atexit"); return 1; }

    if (signal(SIGINT, h_sigint) == SIG_ERR){ perror("signal"); return 1; }

    struct sigaction sa = {0};
    sa.sa_sigaction = h_sigterm;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTERM, &sa, NULL) == -1){ perror("sigaction"); return 1; }

    pid_t p = fork();
    if (p < 0){ perror("fork"); return 1; }

    if (p == 0){
        printf("CHILD PID=%d PPID=%d\n", getpid(), getppid());
        sleep(5);
        printf("CHILD PID=%d: завершение. \n", getpid());
        exit(42);
    } else {
        printf("PARENT PID=%d PPID=%d CHILD=%d\n", getpid(), getppid(), p);
        int st;
        pid_t w = waitpid(p, &st, 0);
        if (w == -1){ perror("waitpid"); return 1; }
        if (WIFEXITED(st)){
            printf("PARENT: дочерний %d завершился, код=%d\n", w, WEXITSTATUS(st));
        } else if (WIFSIGNALED(st)){
            printf("PARENT: дочерний %d убит сигналом %d (%s)\n",
                   w, WTERMSIG(st), strsignal(WTERMSIG(st)));
        } else {
            printf("PARENT: дочерний %d завершён нестандартно\n", w);
        }
    }
    return 0;
}
