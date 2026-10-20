/*
 * bank_init.c - (Re)create the database with demo data.
 *
 * Usage: ./bank_init [--yes] [-d datadir]
 */
#include <dirent.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "db.h"
#include "seed.h"
#include "util.h"

static int server_running(void)
{
    char buf[32] = {0};
    FILE *f = fopen("logs/audit.log.pid", "r");
    if (!f) return 0;
    int pid = 0;
    if (fgets(buf, sizeof buf, f)) parse_int(buf, &pid);
    fclose(f);
    return pid > 1 && kill(pid, 0) == 0;
}

int main(int argc, char *argv[])
{
    int yes = 0;
    const char *dir = "data";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--yes") || !strcmp(argv[i], "-y")) yes = 1;
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) dir = argv[++i];
        else { printf("Usage: %s [--yes] [-d datadir]\n", argv[0]); return 1; }
    }
    if (server_running()) {
        fprintf(stderr, "The bank server appears to be running. Stop it (Ctrl+C) before resetting the data.\n");
        return 1;
    }
    if (db_init_dir(dir) == -1) return 1;
    if (db_open_all() == -1) return 1;
    if (db_files_exist() && !yes) {
        printf("This will ERASE all existing bank data in %s. Continue? (y/n): ", db_data_dir());
        fflush(stdout);
        char ans[8] = "";
        if (!fgets(ans, sizeof ans, stdin) || (ans[0] != 'y' && ans[0] != 'Y')) {
            printf("Cancelled.\n");
            return 0;
        }
    }

    /* remove any leftover journals */
    char jdir[PATH_MAX];
    db_path("journal", jdir, sizeof jdir);
    DIR *d = opendir(jdir);
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (de->d_name[0] == '.') continue;
            char p[PATH_MAX + 300];
            snprintf(p, sizeof p, "%s/%s", jdir, de->d_name);
            unlink(p);
        }
        closedir(d);
    }
    if (seed_database(1) == -1) return 1;
    db_close_all();
    return 0;
}
