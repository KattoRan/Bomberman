/* client/handlers/session.c */
#include "session.h"

char session_file_path[256];

void determine_session_path(int argc, char *argv[]) {
    // Priority 1: --profile <name>
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--profile") == 0) {
            const char *home = getenv("HOME");
            if (!home) home = ".";
            snprintf(session_file_path, sizeof(session_file_path), "%s/.bomberman_session_%s", home, argv[i+1]);
            return;
        }
    }

    // Priority 2: TTY-based path (Automatic isolation)
    char *tty = ttyname(STDIN_FILENO);
    if (tty) {
        // Sanitize TTY name (replace / with _)
        char safe_tty[64];
        int j = 0;
        for (int i = 0; tty[i] && j < 63; i++) {
            if (tty[i] == '/') safe_tty[j++] = '_';
            else safe_tty[j++] = tty[i];
        }
        safe_tty[j] = '\0';
        
        const char *home = getenv("HOME");
        if (!home) home = ".";
        snprintf(session_file_path, sizeof(session_file_path), "%s/.bomberman_session%s", home, safe_tty);
        return;
    }

    // Priority 3: Fallback default
    const char *home = getenv("HOME");
    if (!home) home = ".";
    snprintf(session_file_path, sizeof(session_file_path), "%s/.bomberman_session_default", home);
}

void save_session_token(const char *token) {
    if (token[0] == '\0') return; // Don't save empty tokens
    FILE *f = fopen(session_file_path, "w");
    if (f) {
        fprintf(f, "%s", token);
        fclose(f);
    }
}

int load_session_token(char *buffer) {
    FILE *f = fopen(session_file_path, "r");
    if (f) {
        if (fgets(buffer, 64, f)) {
            buffer[strcspn(buffer, "\n")] = 0;
            fclose(f);
            return 1;
        }
        fclose(f);
    }
    return 0;
}

void clear_session_token() {
    unlink(session_file_path);
}
