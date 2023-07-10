#include "fastfetch.h"
#include "common/processing.h"
#include "common/io/io.h"
#include "common/time.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>

#ifdef __linux__
    #include <sys/syscall.h>
    #include <poll.h>
#endif

int waitpid_timeout(pid_t pid, int* status)
{
    if (instance.config.processingTimeout <= 0)
        return waitpid(pid, status, 0);

    uint32_t timeout = (uint32_t) instance.config.processingTimeout;

    #if defined(__linux__) && defined(SYS_pidfd_open) // musl don't define SYS_pidfd_open

    FF_AUTO_CLOSE_FD int pidfd = (int) syscall(SYS_pidfd_open, pid, 0);
    if (pidfd >= 0)
    {
        int res = poll(&(struct pollfd) { .events = POLLIN, .fd = pidfd }, 1, (int) timeout);
        if (res > 0)
            return (int) waitpid(pid, status, WNOHANG);
        else if (res == 0)
        {
            kill(pid, SIGTERM);
            return -62; // -ETIME
        }
        return -1;
    }

    #endif

    uint64_t start = ffTimeGetTick();
    while (true)
    {
        int res = (int) waitpid(pid, status, WNOHANG);
        if (res != 0)
            return res;
        if (ffTimeGetTick() - start < timeout)
            ffTimeSleep(timeout / 10);
        else
        {
            kill(pid, SIGTERM);
            return -62; // -ETIME
        }
    }
}

const char* ffProcessAppendOutput(FFstrbuf* buffer, char* const argv[], bool useStdErr)
{
    int pipes[2];

    if(pipe(pipes) == -1)
        return "pipe() failed";

    pid_t childPid = fork();
    if(childPid == -1)
        return "fork() failed";

    //Child
    if(childPid == 0)
    {
        dup2(pipes[1], useStdErr ? STDERR_FILENO : STDOUT_FILENO);
        close(pipes[0]);
        close(pipes[1]);
        close(useStdErr ? STDOUT_FILENO : STDERR_FILENO);
        execvp(argv[0], argv);
        exit(901);
    }

    //Parent
    close(pipes[1]);

    int FF_AUTO_CLOSE_FD childPipeFd = pipes[0];
    int status = -1;
    if(waitpid_timeout(childPid, &status) < 0)
        return "waitpid(childPid, &status) failed";

    if (!WIFEXITED(status))
        return "WIFEXITED(status) == false";

    if(WEXITSTATUS(status) == 901)
        return "WEXITSTATUS(status) == 901 ( execvp failed )";

    if(!ffAppendFDBuffer(childPipeFd, buffer))
        return "ffAppendFDBuffer(childPipeFd, buffer) failed";

    return NULL;
}
