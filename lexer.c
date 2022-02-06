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

enum { SIZE_TOKEN_NAME = 32 };

typedef struct Token_Desc Token_Desc;

struct Token_Desc {
    int type; // one of TOKEN_NORMAL, TOKEN_NO_DATA, TOKEN_THROW
    char name[SIZE_TOKEN_NAME]; // name of the token
    Regex * regex; // regex describing the token (can be NULL when automata is known)
    Automata automata; // automata describing the token (may be NULL then computed from regex)
};

enum {
    TOKEN_NORMAL, // token containing the characters it matched (an integer value for example)
    TOKEN_NO_DATA, // token not containing the characters it matched (a semicolon for example)
    TOKEN_THROWN // token thrown up from the output of lexical analysis (spaces for example)
};

typedef struct Token Token;

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
    int nb_characs_used, num_node;

    // search first matching token (there is a break)
    for (int i = 0; i < lexicon_desc.nb_token_descs; i ++) {
        token_desc = lexicon_desc.token_descs[i];

        explore_farthest_success_node(
            token_desc.automata, 0, characs, nb_characs,
            &num_node, &nb_characs_used, NULL, NULL, NULL);

        if (num_node != BAD_NUM && token_desc.automata.nodes[num_node].success) {
            switch (token_desc.type) {
                case TOKEN_NORMAL:
                    result = FOUND_TOKEN;
                    for (int j = 0; j < SIZE_TOKEN_NAME; j ++) {
                        token.name[j] = token_desc.name[j];
                    }
                    token.nb_characs = nb_characs_used;
                    token.characs = malloc(nb_characs_used * sizeof(int));
                    for (int j = 0; j < nb_characs_used; j ++) { token.characs[j] = characs[j]; }
                    *res_token = token;
                break;
                case TOKEN_NO_DATA:
                    result = FOUND_TOKEN;
                    for (int j = 0; j < SIZE_TOKEN_NAME; j ++) {
                        token.name[j] = token_desc.name[j];
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
            break; // exit the for loop
        }
    }

    return result;
}

int main (int argc, char ** argv) {

    char filepath[] = "./source.txt";

    int nb_characs;
    int * characs = NULL;
    int success_read = read_complete_file (filepath, &nb_characs, &characs);

    if (success_read) {
        printf("nb of characs in file is %i.\n", nb_characs);
        for (int i = 0; i < nb_characs; i ++) printf("%i ", characs[i]);
        if (nb_characs > 0) printf("\n");
    }
    else printf("problem reading the file.\n");

    Regex regex1;
    regex1.type = REGEX_REPEAT;
    regex1.min = 2;
    regex1.max = 2;
    regex1.one = &ascii_digit;

    Regex regex2;
    regex2.type = REGEX_SEQUENCE;
    regex2.one = &regex1;
    regex2.two = &ascii_letter_all;

    Automata automata0 = regex_to_automata(regex2);
    Token_Desc token_desc0 = { TOKEN_NORMAL, "2digits1letter", &regex2, automata0 };

    Automata automata1 = regex_to_automata(ascii_digit);
    Token_Desc token_desc1 = { TOKEN_NORMAL, "digit", &ascii_digit, automata1 };

    Lexicon_Desc lexicon_desc = new_lexicon_desc();
    add_token_desc(&lexicon_desc, token_desc0);
    add_token_desc(&lexicon_desc, token_desc1);

    Token token;
    int nb_characs_used, result_find;
    result_find = find_token (lexicon_desc, nb_characs, characs, &token, &nb_characs_used);

    switch (result_find) {
        case FOUND_TOKEN:
            if (token.nb_characs == 0) printf("found token %s\n", token.name);
            else {
                printf("found token %s(", token.name);
                for (int i = 0; i < token.nb_characs; i ++) printf("%i ", token.characs[i]);
                printf(")\n");
                free(token.characs);
            }
        break;
        case FOUND_TOKEN_THROWN:
            printf("found token %s thrown\n", token.name);
        break;
        case NOT_FOUND_TOKEN:
            printf("not found token\n");
        break;
    }

    free(characs);
    free_automata(automata0);
    free_automata(automata1);
    free_lexicon_desc(lexicon_desc);

    return success_read ? EXIT_SUCCESS : EXIT_FAILURE;

}
