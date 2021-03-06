#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcgiapp.h>

#include "csif.h"

static void usage(void);
char* concat(const char *s1, const  char *s2);
char *trim_space(char *str);
void execute(char **argv);
char** prepare_so(int argc, char *argv[]);

static void usage(void) {
  fprintf(stderr, "Available options:\n");
  fprintf(stderr, "\t-a\tthe ip address which binding to\n");
  fprintf(stderr, "\t-p\tport number to specified, not for -s\n");
  fprintf(stderr, "\t-s\tunix domain socket path to generate, not for -p\n");
  fprintf(stderr, "\t-q\tnumber of socket backlog\n");
  fprintf(stderr, "\t-w\tnumber of worker process\n");
  fprintf(stderr, "\t-l\tlog file path\n");
  fprintf(stderr, "\t-e\tsignal handling\n");
  fprintf(stderr, "\t-f\tFork Daemon process\n");
  fprintf(stderr, "\t-d\tRun on debug Mode\n");
  fprintf(stderr, "\t-o\tDynamic Link shared object file\n");
  fprintf(stderr, "\t-h\tdisplay usage\n");
  fprintf(stderr, "\t-v\tdisplay version\n");
  exit(1);
}

char* concat(const char *s1, const  char *s2) {
  size_t len1 = strlen(s1);
  size_t len2 = strlen(s2);
  char *result = malloc(len1 + len2 + 1); //+1 for the zero-terminator
  memcpy(result, s1, len1);
  memcpy(result + len1, s2, len2 + 1);
  return result;
}

char *trim_space(char *str) {
  char *end;
  // Trim leading space
  while (isspace(*str)) str++;
  if (*str == 0) // All spaces?
    return str;
  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace(*end)) end--;
  // Write new null terminator
  *(end + 1) = 0;
  return str;
}


void execute(char **argv) {
  pid_t  pid;
  int    status;
  if ((pid = fork()) < 0) {     /* fork a child process           */
    printf("*** ERROR: forking child process failed\n");
    exit(1);
  }
  else if (pid == 0) {          /* for the child process:         */
    if (execvp(*argv, argv) < 0) {     /* execute the command  */
      printf("*** ERROR: failed process\n");
      usage();
      exit(1);
    }
  }
  else {
    while (wait(&status) != pid);       /* wait for completion  */
  }
  // while (*argv) {
  //   printf("%s\n", *argv++);
  // }
  // printf("%s\n", "done");
}

char** prepare_so(int argc, char *argv[]) {
  char **args = malloc((argc + 6) * sizeof(char*));
  char** exec_ = args;
  char *appname;

  *args++ = "/usr/bin/gcc";
  *args++ = "-Wall";
  *args++ = "-shared";

  *args++ = "-o";
  appname = concat("lib", argv[2]); //output
  *args++ = concat(appname, ".so"); //output

  *args++ = "-fPIC";
  argc -= 3;
  argv += 3;
  memcpy(args, argv, argc * sizeof(char*));
  // *args++ = "-rdynamic";
  // *args++ = "-Wl,-rpath";
  // *args++ = "/usr/local/lib";
  // *args++ = "-lfcgi";
  // *args++ = "-ldl";
  // *args++ = "-lcJSON";
  // *args++ = "-lcsif";
  *(args + argc) = NULL;
  return exec_;
}

int
csif_main (int argc, char *argv[], char* all_funs[]) {

  char *sock_port,
       *appname = NULL,
        *logfile = NULL;
  int ch,
      forkable = 0,
      signalable = 0,
      debugmode = 0,
      backlog = 5,
      workers = 15;
  // opterr = 0;
  if (argc < 2) { /* no arguments given */
    printf("%s\n", "No enough option, at least specified the port");
    usage();
    return -1;
  }

  csif_dbg = &no_debug;

  if (strcmp(argv[1], "build") == 0) {
    char** exec_ = prepare_so(argc, argv);
    execute(exec_); //compile
    return 0;
  }

  while ((ch = getopt (argc, argv, "defvhp:s:q:w:l:o:")) != -1) {
    switch (ch) {
    case 'p':
      sock_port = concat(":", optarg);
      break;
    case 's':
      sock_port = optarg; break;
    case 'q':
      backlog = strtol(optarg, NULL, 10); break;
      break;
    case 'w':
      workers = strtol(optarg, NULL, 10); break;
      break;
    case 'l':
      logfile = trim_space(optarg);
      break;
    case 'e':
      signalable = 1; break;
    case 'f':
      forkable = 1; break;
    case 'd':
      debugmode = 1; break;
    case 'o':
      appname = trim_space(optarg); break;
    case 'v':
      printf("-version : %s\n", "0.0.1");
      return -1;
      break;
    case 'h':
    default:
      usage();
      return -1;
    }
  }
  argc -= optind;
  argv += optind;

  if (appname && *appname) {
    int len = strlen(appname);
    if (appname[len - 3] == '.' && appname[len - 2] == 's' && appname[len - 1] == 'o' ) {
      // char *install_lib[4] = {"mv", appname, "/usr/local/csif/", NULL};
      // execute(install_lib);
      init_socket(sock_port, backlog, workers, forkable, signalable, debugmode, logfile, concat("./", appname), all_funs);
    } else {
      printf("%s\n", "only .so file if you need dynamic link your application, else please leave it blank");
    }
  } else {
    init_socket(sock_port, backlog, workers, forkable, signalable, debugmode, logfile, NULL, all_funs);
  }

  return 0;
}
