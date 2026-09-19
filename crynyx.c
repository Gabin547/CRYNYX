#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include <conio.h>
#include <direct.h>

// commands
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);
int lsh_crypt(char **args);
int lsh_decrypt(char **args);
int lsh_crynyx(char **args);
int lsh_pwd(char **args);

char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "crypt",
    "decrypt",
    "crynyx",
    "pwd"
};

int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit,
    &lsh_crypt,
    &lsh_decrypt,
    &lsh_crynyx,
    &lsh_pwd
};

int lsh_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

// Remplace getpass (inexistant sur Windows)
char *my_getpass(const char *prompt)
{
    static char password[256];
    int i = 0;
    char c;

    printf("%s", prompt);

    while ((c = _getch()) != '\r' && i < 255) {
        if (c == '\b' && i > 0) {
            i--;
            printf("\b \b");
        } else if (c != '\b') {
            password[i++] = c;
            printf("*");
        }
    }
    password[i] = '\0';
    printf("\n");
    return password;
}

int lsh_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "crynyx: expected argument to \"cd\"\n");
    } else {
        if (_chdir(args[1]) != 0) {
            perror("crynyx");
        }
    }
    return 1;
}

int lsh_help(char **args)
{
    int i;
    printf("Here, there is all commands for this terminal\n");
    for (i = 0; i < lsh_num_builtins(); i++) {
        printf("  %s\n", builtin_str[i]);
    }
    printf("For more help, visit: https://gabin547.github.io/CRYNYX/ \n");
    return 1;
}

int lsh_exit(char **args)
{
    return 0;
}

int lsh_launch(char **args)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char cmd[1024] = "";
    int i = 0;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    while (args[i] != NULL) {
        strcat(cmd, args[i]);
        strcat(cmd, " ");
        i++;
    }

    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "crynyx: command not found\n");
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 1;
}

int lsh_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < lsh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return lsh_launch(args);
}

bool file_exists(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    bool is_exist = false;
    if (fp != NULL)
    {
        is_exist = true;
        fclose(fp);
    }
    return is_exist;
}

int lsh_pwd(char **args){
   char buffer[1024];
   if(_getcwd(buffer, sizeof(buffer)) != NULL){
    printf("%s\n", buffer);
   } else {
    perror("crynyx");
   }
   return 1;
}

int lsh_crypt(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "crynyx: expected argument to \"crypt\"\n");
        return 1;
    }

    char *file_name = args[1];
    char *key = NULL;

    if (args[2] != NULL && strcmp(args[2], "-key") == 0) {
        if (args[3] != NULL) {
            key = args[3];
        } else {
            fprintf(stderr, "crynyx: -key requires a password\n");
            return 1;
        }
    }

    if (key == NULL) {
        key = my_getpass("Enter encryption key: ");
        if (key == NULL || strlen(key) == 0) {
            fprintf(stderr, "crynyx: key cannot be empty\n");
            return 1;
        }
    }

    if (file_exists(file_name) == false) {
        printf("File %s not found.\n", file_name);
        return 1;
    }

    FILE *in = fopen(file_name, "rb");
    if (in == NULL) {
        printf("Error: cannot open file %s\n", file_name);
        return 1;
    }

    char output[1024];
    snprintf(output, sizeof(output), "%s.enc", file_name);
    FILE *out = fopen(output, "wb");
    if (out == NULL) {
        printf("Error: cannot create file %s\n", output);
        fclose(in);
        return 1;
    }

    int c;
    int pos = 0;
    int key_len = strlen(key);

    while ((c = fgetc(in)) != EOF) {
        c = c ^ key[pos % key_len];
        fputc(c, out);
        pos++;
    }

    fclose(in);
    fclose(out);

    printf("%s -> %s (successfully encrypted!)\n", file_name, output);
    return 1;
}

int lsh_decrypt(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "crynyx: expected argument to \"decrypt\"\n");
        return 1;
    }

    char *file_name = args[1];
    char *key = NULL;

    if (args[2] != NULL && strcmp(args[2], "-key") == 0) {
        if (args[3] != NULL) {
            key = args[3];
        } else {
            fprintf(stderr, "crynyx: -key requires a password\n");
            return 1;
        }
    }

    if (key == NULL) {
        key = my_getpass("Enter decryption key: ");
        if (key == NULL || strlen(key) == 0) {
            fprintf(stderr, "crynyx: key cannot be empty\n");
            return 1;
        }
    }

    if (file_exists(file_name) == false) {
        printf("File %s not found.\n", file_name);
        return 1;
    }

    FILE *in = fopen(file_name, "rb");
    if (in == NULL) {
        printf("Error: cannot open file %s\n", file_name);
        return 1;
    }

    char output[1024];
    if (strlen(file_name) > 4 && strcmp(file_name + strlen(file_name) - 4, ".enc") == 0) {
        strncpy(output, file_name, strlen(file_name) - 4);
        output[strlen(file_name) - 4] = '\0';
    } else {
        snprintf(output, sizeof(output), "%s.dec", file_name);
    }

    FILE *out = fopen(output, "wb");
    if (out == NULL) {
        printf("Error: cannot create file %s\n", output);
        fclose(in);
        return 1;
    }

    int c;
    int pos = 0;
    int key_len = strlen(key);

    while ((c = fgetc(in)) != EOF) {
        c = c ^ key[pos % key_len];
        fputc(c, out);
        pos++;
    }

    fclose(in);
    fclose(out);

    printf("%s -> %s (successfully decrypted!)\n", file_name, output);
    return 1;
}

int lsh_crynyx(char **args)
{
    if (args[1] != NULL && strcmp(args[1], "--version") == 0) {
        printf("CRYNYX version 1.0.0\n");
    } else {
        printf("Usage: crynyx --version\n");
    }
    return 1;
}

// Historique simple pour Windows
#define HISTORY_SIZE 100
char *history[HISTORY_SIZE];
int history_count = 0;

void add_to_history(const char *line)
{
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(line);
    }
}

char *lsh_read_line(void)
{
#define LSH_RL_BUFSIZE 1024
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer) {
        fprintf(stderr, "crynyx: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1) {
        c = getchar();

        if (c == EOF) {
            exit(EXIT_SUCCESS);
        } else if (c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                fprintf(stderr, "crynyx: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "crynyx: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, LSH_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += LSH_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "crynyx: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, LSH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void lsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("CRYNYX > ");
        fflush(stdout);

        line = lsh_read_line();

        if (line == NULL || strlen(line) == 0) {
            free(line);
            continue;
        }

        add_to_history(line);

        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);

    } while (status);
}

int main(int argc, char **argv)
{
    lsh_loop();
    return EXIT_SUCCESS;
}