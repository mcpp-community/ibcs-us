/*
 * A program of the interface, with no C library: it issues System V i386 system
 * calls through the call gate (sc.S) and states what it saw. One line a check;
 * the status is the number of checks that did not hold.
 *
 * Each check but the last is one of the changes of patches/: with ibcs-us 4.3 as
 * released, every one of them fails. The last holds there too: a program of the
 * host that exec starts is still the host's.
 */
int sc(int nr, int a, int b, int c, int *second, int *error);

enum { SYS_exit = 1, SYS_fork = 2, SYS_write = 4, SYS_open = 5, SYS_close = 6, SYS_wait = 7,
       SYS_unlink = 10, SYS_chmod = 15,
       SYS_stat = 18, SYS_lseek = 19, SYS_fstat = 28, SYS_fcntl = 62, SYS_rmdir = 79, SYS_mkdir = 80,
       SYS_getdents = 81, SYS_exece = 59 };
enum { O_WRONLY = 1, O_RDWR = 2, O_CREAT = 0400, F_FREESP = 11 };

struct stat32 { short dev; unsigned short ino, mode; short nlink; unsigned short uid, gid; short rdev, pad;
                int size, atime, mtime, ctime; };
struct dirent32 { int ino; int off; unsigned short reclen; char name[1]; };
struct flock32 { short type, whence; int start, len, sysid, pid, pad[4]; };

static int second_, error_, failed_;
static char zeroed_[3 * 4096 + 17];             /* the zero-filled part of the data segment */

static unsigned len(const char *s) { unsigned n = 0; while (s[n]) ++n; return n; }
static void put(const char *s) { sc(SYS_write, 1, (int)s, (int)len(s), &second_, &error_); }
static void check(const char *what, int held) {
    put(held ? "held      " : "NOT HELD  "); put(what); put("\n");
    if (!held) ++failed_;
}
static int call(int nr, int a, int b, int c) { return sc(nr, a, b, c, &second_, &error_); }
static int same(const char *a, const char *b) { while (*a && *a == *b) ++a, ++b; return *a == *b; }

/* sp: the stack the program was started with --- argc, the arguments, a null, the environment. */
void cmain(int *sp) {
    const int argc = sp[0];
    const char *const *argv = (const char *const *)(sp + 1);
    const char *const *envp = argv + argc + 1;
    static const char *const names[] = { "d/alpha", "d/beta", "d/gamma", "d/delta", "d/epsilon" };
    struct stat32 st;
    int i, fd, seen = 0, calls = 0, good = 1;

    /* The program an exec of check 8 starts: its first argument as the caller gave it, the
     * second, and a call through the gate, which faults if the program is not in the emulation. */
    if (argc == 3 && same(argv[1], "exec-child")) {
        sc(SYS_write, 1, (int)"", 0, &second_, &error_);
        call(SYS_exit, same(argv[0], "probe-as-child") && same(argv[2], "x") ? 42 : 41, 0, 0);
    }

    /* 4: the zero-filled part is there, and zero, in an executable with nothing after its data. */
    for (i = 0; i < (int)sizeof zeroed_; i += 4096) { if (zeroed_[i]) good = 0; zeroed_[i] = 1; }
    check("the zero-filled part of a segment is memory, and zero", good);

    call(SYS_mkdir, (int)"d", 0755, 0);
    for (i = 0; i < 5; ++i) call(SYS_close, call(SYS_open, (int)names[i], O_WRONLY | O_CREAT, 0644), 0, 0);

    /* 1, 2, 3: a directory is listed whole, once, with its names where the layout puts them,
     * through a buffer that holds two entries at a time. */
    fd = call(SYS_open, (int)"d", 0, 0);
    good = fd >= 0;
    for (;;) {
        char buf[40];
        int at = 0, got = call(SYS_getdents, fd, (int)buf, (int)sizeof buf);
        if (got <= 0 || ++calls > 64) break;
        while (at < got) {
            struct dirent32 *e = (struct dirent32 *)(buf + at);
            const char *n = e->name;
            int known = (n[0] == '.' && (!n[1] || (n[1] == '.' && !n[2])));
            for (i = 0; i < 5 && !known; ++i) {
                const char *want = names[i] + 2; unsigned k = 0;
                while (want[k] && want[k] == n[k]) ++k;
                known = !want[k] && !n[k];
            }
            if (!known || e->reclen == 0) good = 0;
            ++seen;
            if (e->reclen == 0) break;
            at += e->reclen;
        }
    }
    check("a directory of five files is listed as seven names, each once", good && seen == 7);
    call(SYS_close, fd, 0, 0);

    /* 3: a query of the position leaves it. */
    fd = call(SYS_open, (int)"d/alpha", O_RDWR, 0);
    call(SYS_write, fd, (int)"0123456789", 10);
    check("the position after ten bytes is ten", call(SYS_lseek, fd, 0, 1) == 10);

    /* 6: the space from an offset to the end is freed. */
    { struct flock32 fl = { 0, 0, 4, 0, 0, 0, { 0, 0, 0, 0 } };
      int r = call(SYS_fcntl, fd, F_FREESP, (int)&fl);
      st.size = -1; call(SYS_fstat, fd, (int)&st, 0);
      check("a file of ten bytes freed from the fourth is four bytes", r == 0 && st.size == 4); }
    call(SYS_close, fd, 0, 0);

    /* 5: a file's mode is changed. */
    { int r = call(SYS_chmod, (int)"d/alpha", 0600, 0);
      st.mode = 0; call(SYS_stat, (int)"d/alpha", (int)&st, 0);
      check("a file's mode is set", r == 0 && (st.mode & 0777) == 0600); }

    for (i = 0; i < 5; ++i) call(SYS_unlink, (int)names[i], 0, 0);
    check("the directory is removed once it is empty", call(SYS_rmdir, (int)"d", 0, 0) == 0);

    /* 8: a program started by exec is run in the emulation, with the arguments it was given. */
    { static const char *const av[] = { "probe-as-child", "exec-child", "x", 0 };
      int status = -1, pid = call(SYS_fork, 0, 0, 0);
      if (pid == 0) {
          call(SYS_exece, (int)argv[0], (int)av, (int)envp);
          call(SYS_exit, 40, 0, 0);
      }
      i = call(SYS_wait, (int)&status, 0, 0);
      check("a program exec starts runs in the emulation, with the arguments it was given",
            pid > 0 && i == pid && status == 42 << 8); }

    /* 9: a file the emulation does not run (the host's shell) is still started by Linux. */
    { static const char *const av[] = { "sh", "-c", "exit 5", 0 };
      int status = -1, pid = call(SYS_fork, 0, 0, 0);
      if (pid == 0) {
          call(SYS_exece, (int)"/bin/sh", (int)av, (int)envp);
          call(SYS_exit, 40, 0, 0);
      }
      i = call(SYS_wait, (int)&status, 0, 0);
      check("a program of the host that exec starts is the host's, as before",
            pid > 0 && i == pid && status == 5 << 8); }
    call(SYS_exit, failed_, 0, 0);
    for (;;) {}
}
