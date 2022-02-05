typedef enum Regex_Type Regex_Type;
typedef struct Regex Regex;

enum Regex_Type {
    REGEX_CHARACTER, // exactly one character
    REGEX_EPSILON, // no character at all
    REGEX_SEQUENCE, // a first subregex, then a second subregex concatened
    REGEX_BRANCH, // either the first subregex, or the second subregex
    REGEX_REPEAT, // the subregex repeated from (0 or n) times to (0 or n or infinite) times
    REGEX_INTERVAL // exactly one character within an interval on the total order of characters
};

struct Regex {
    Regex_Type type; // determine the fields used by the Regex

    int character; // used by REGEX_CHARACTER
    Regex * one; // used by REGEX_SEQUENCE, REGEX_BRANCH, REGEX_REPEAT
    Regex * two; // used by REGEX_SEQUENCE, REGEX_BRANCH
    int min; // used by REGEX_REPEAT, REGEX_INTERVAL
    int max; // used by REGEX_REPEAT, REGEX_INTERVAL
};

enum { MAX_INF = -1 }; // constant meaning positive infinite Regex.max

Automata regex_to_automata (Regex regex);
char * regex_to_string (Regex regex);

// some US-ASCII regexes
extern Regex ascii_digit;
extern Regex ascii_letter_small;
extern Regex ascii_letter_cap;
extern Regex ascii_letter_all;
