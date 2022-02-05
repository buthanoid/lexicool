#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"
#include "automata.h"
#include "regex.h"

int read_complete_file (const char filepath[], int * res_nb_chars, int ** res_chars) {

    *res_nb_chars = 0;
    *res_chars = NULL;

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) return FALSE;

    int capacity = 0;
    int nb_chars = 0;
    int * chars = NULL;

    char buffer[8];
    int nb_read;
    int success_adapt = TRUE;

    while ((nb_read = read(fd, buffer, 8)) > 0) {

        success_adapt = adapt_capacity(
            nb_chars + nb_read, &capacity, sizeof(int), (void **) &chars);
        if (success_adapt) {

            // copy buffer in chars
            for (int i = 0; i < nb_read; i ++) {
                chars[nb_chars] = buffer[i];
                nb_chars ++;
            }
        }
        else break;
    }
    close(fd);

    // if there was no error
    if (nb_read != -1 && success_adapt) {
        *res_nb_chars = nb_chars;
        *res_chars = chars;
        return TRUE;
    }
    else {
        free(chars);
        return FALSE;
    }
}

typedef struct Token_Desc Token_Desc;

struct Token_Desc {
    int type; // one of TOKEN_NORMAL, TOKEN_NO_DATA, TOKEN_THROW
    char name[32]; // name of the token
    Regex * regex; // regex describing the token (can be NULL when automata is known)
    Automata automata; // automata describing the token (may be empty then computed from regex)
};

enum {
    TOKEN_NORMAL, // token containing the characters it matched (an integer value for example)
    TOKEN_NO_DATA, // token not containing the characters it matched (a semicolon for example)
    TOKEN_THROWN // token thrown up from the output of lexical analysis (spaces for example)
};

typedef struct Token Token;

enum { SIZE_TOKEN_NAME = 32 };

struct Token {
    char name[SIZE_TOKEN_NAME]; // name of the token
    int nb_characs; // nb of characters of the token (always 0 for a TOKEN_NO_DATA)
    int * characs; // NULL when nb_characs == 0
};

typedef struct Lexicon_Desc Lexicon_Desc;

struct Lexicon_Desc {
    int nb_token_descs; // nb of actual elements in array token_descs
    int nb_token_descs_capacity; // nb of cases in array token_descs
    Token_Desc * token_descs;
};

Lexicon_Desc new_lexicon_desc () {
    Lexicon_Desc empty_lexicon_desc = { 0, 0, NULL };
    return empty_lexicon_desc;
}
void free_lexicon_desc (Lexicon_Desc lexicon_desc) { free(lexicon_desc.token_descs); }

int add_token_desc (Lexicon_Desc * lexicon_desc, Token_Desc token_desc) {

    int success_adapt = adapt_capacity(
		lexicon_desc->nb_token_descs + 1, &lexicon_desc->nb_token_descs_capacity,
		sizeof(Token_Desc), (void **) &lexicon_desc->token_descs);

	if (success_adapt) {
        int num_token_desc = lexicon_desc->nb_token_descs;
        lexicon_desc->token_descs[num_token_desc] = token_desc;
        lexicon_desc->nb_token_descs ++;
        return num_token_desc;
    }
    else return BAD_NUM;
}

enum {
    FOUND_TOKEN = 0, // when a TOKEN_NORMAL or a TOKEN_NO_DATA has been read
    FOUND_TOKEN_THROWN = 1, // when a TOKEN_THROWN has been read
    NOT_FOUND_TOKEN = 2, // when no token desc from lexicon_desc would match
};

int find_token (Lexicon_Desc lexicon_desc, int nb_characs, int * characs,
    Token * res_token, int * res_nb_characs_used
) {
    int result = NOT_FOUND_TOKEN;
    Token token;
    Token_Desc token_desc;
    int nb_characs_used, num_node, nb_explorations_steps, max_nb_points_reached;
    int * counters = NULL;

    // search first matching token (there is a break)
    for (int i = 0; i < lexicon_desc.nb_token_descs; i ++) {
        token_desc = lexicon_desc.token_descs[i];

        counters = malloc(token_desc.automata.nb_counters * sizeof(int));

        explore_farthest_success_node(
            token_desc.automata, 0, characs, nb_characs,
            &num_node, &nb_characs_used, counters,
            &nb_explorations_steps, &max_nb_points_reached);

        free(counters);

        if (num_node != BAD_NUM && token_desc.automata.nodes[num_node].success) {
            switch (token_desc.type) {
                case TOKEN_NORMAL:
                    result = FOUND_TOKEN;
                    for (int j = 0; j < SIZE_TOKEN_NAME; j ++) {
                        token.name[i] = token_desc.name[i];
                    }
                    token.nb_characs = nb_characs_used;
                    token.characs = malloc(nb_characs_used * sizeof(int));
                    for (int j = 0; j < nb_characs_used; j ++) { token.characs[i] = characs[i]; }
                    *res_token = token;
                break;
                case TOKEN_NO_DATA:
                    result = FOUND_TOKEN;
                    for (int j = 0; j < SIZE_TOKEN_NAME; j ++) {
                        token.name[i] = token_desc.name[i];
                    }
                    token.nb_characs = 0;
                    token.characs = NULL;
                    *res_token = token;
                break;
                case TOKEN_THROWN:
                    result = FOUND_TOKEN_THROWN;
                break;
            }
            *res_nb_characs_used = nb_characs_used;
            break;
        }
    }

    return result;
}

int main (int argc, char ** argv) {

    char filepath[] = "./source.txt";

    int nb_chars;
    int * chars;
    int success_read = read_complete_file (filepath, &nb_chars, &chars);

    if (success_read) {
        printf("nb of characters in file is %i.\n", nb_chars);
        free(chars);
        return EXIT_SUCCESS;
    }
    else {
        printf("problem reading the file.\n");
        return EXIT_FAILURE;
    }

}
