#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SYSFS_FILENAME "/sys/kernel/ksysfs_logger/filename"
#define SYSFS_PERIOD_MS "/sys/kernel/ksysfs_logger/period_ms"

#define BASE_DIR "/var/tmp/test_module"

static int ensure_dir_exists(const char *path) {
    struct stat st;

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode))
            return 0;
        fprintf(stderr, "%s exists but is not a directory\n", path);
        return -1;
    }

    if (errno != ENOENT) {
        fprintf(stderr, "stat(%s) failed: %s\n", path, strerror(errno));
        return -1;
    }

    if (mkdir(path, 0777) != 0) {
        fprintf(stderr, "mkdir(%s) failed: %s\n", path, strerror(errno));
        return -1;
    }

    return 0;
}

static int write_file(const char *path, const char *data) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "open(%s) failed: %s\n", path, strerror(errno));
        return -1;
    }

    size_t len = strlen(data);
    ssize_t w = write(fd, data, len);
    close(fd);

    if (w < 0) {
        fprintf(stderr, "write(%s) failed: %s\n", path, strerror(errno));
        return -1;
    }
    if ((size_t)w != len) {
        fprintf(stderr, "write(%s): partial write (%zd/%zu)\n", path, w, len);
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <filename> <period_seconds>\n", argv[0]);
        return 2;
    }

    const char *filename = argv[1];

    char *end = NULL;
    errno = 0;
    long seconds = strtol(argv[2], &end, 10);
    if (errno != 0 || end == argv[2] || *end != '\0' || seconds <= 0) {
        fprintf(stderr, "Invalid period_seconds: %s\n", argv[2]);
        return 2;
    }

    long ms = seconds * 1000;

    // ensure directory test_module exists
    if (ensure_dir_exists(BASE_DIR) != 0) {
        return 1;
    }

    char buf_name[512];
    char buf_ms[64];

    snprintf(buf_name, sizeof(buf_name), "%s\n", filename);
    snprintf(buf_ms, sizeof(buf_ms), "%ld\n", ms);

    if (write_file(SYSFS_FILENAME, buf_name) != 0)
        return 1;
    if (write_file(SYSFS_PERIOD_MS, buf_ms) != 0)
        return 1;

    printf("OK\n");
    return 0;
}