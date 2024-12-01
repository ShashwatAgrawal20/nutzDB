#include "headers/parser.h"

#include <stdio.h>
#include <string.h>

#include "headers/repl.h"

/***************************************************************
                   PRIVATE DECLARATION STUFF
****************************************************************/

#define TABLE_MAX_ROWS 10
#define FILENAME "database.db"

typedef enum { INSERT, SELECT, DELETE, ROLLBACK } StatementType;

typedef enum {
    STATEMENT_RECOGNIZED,
    STATEMENT_UNRECOGNIZED,
    STATEMENT_INCOMPLETE,
} StatementRecognitionResult;

typedef struct {
    char username[32];
    size_t id;
} Row;

typedef struct {
    Row rows[TABLE_MAX_ROWS];
    size_t num_rows;
} Table;

typedef struct {
    StatementType type;
    Row data;
} Statement;

typedef void (*ExecutorFunction)(Statement* statement);

typedef struct {
    const char* keyword;
    StatementType type;
    ExecutorFunction function;
} KeywordMapping;

static StatementRecognitionResult _make_statement(InputBuffer* input_buffer,
                                                  Statement* statement);
static void _statement_executer(Statement* statement);
static void _debug_table(Table* table);

static void _insert_execute(Statement* statement);
static void _select_execute(Statement* statement);
static void _delete_execute(Statement* statement);
static void _rollback_execute(Statement* statement);

static const KeywordMapping mappings[] = {
    {"insert", INSERT, _insert_execute},
    {"select", SELECT, _select_execute},
    {"delete", DELETE, _delete_execute},
    {"rollback", ROLLBACK, _rollback_execute}};

static const size_t num_mappings = sizeof(mappings) / sizeof(mappings[0]);

static Table db = {.num_rows = 0};

/***************************************************************
                        PUBLIC FUNCTIONS
****************************************************************/
void Parser(InputBuffer* input_buffer) {
    Statement statement;
    switch (_make_statement(input_buffer, &statement)) {
        case (STATEMENT_RECOGNIZED):
            _statement_executer(&statement);
            break;
        case (STATEMENT_INCOMPLETE):
            break;
        case (STATEMENT_UNRECOGNIZED):
            printf("Unrecognized '%s'.\n", input_buffer->buffer);
            break;
    }
}

void save_table_to_disk() {
    FILE* file = fopen(FILENAME, "wb");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open file.");
        return;
    }

    // TODO: bruhh there's no need for explaination here :skull:
    fwrite(&db.num_rows, sizeof(size_t), 1, file);
    fwrite(db.rows, sizeof(Row), db.num_rows, file);

    fclose(file);
    printf("Successfully saved data to %s\n", FILENAME);
}

/***************************************************************
                        PRIVATE FUNCTIONS
****************************************************************/
static StatementRecognitionResult _make_statement(InputBuffer* input_buffer,
                                                  Statement* statement) {
    for (size_t i = 0; i < num_mappings; ++i) {
        if (strncmp(input_buffer->buffer, mappings[i].keyword,
                    strlen(mappings[i].keyword)) == 0) {
            statement->type = mappings[i].type;
            switch (statement->type) {
                case (INSERT):;
                    size_t id;
                    char username[32];
                    if (sscanf(input_buffer->buffer, "insert %zu %31s", &id,
                               username) < 2) {
                        printf(
                            "Syntax error. Correct usage: insert <id> "
                            "<username>\n");
                        return STATEMENT_INCOMPLETE;
                    }
                    // printf("id = %zu and username = %s\n", id, username);
                    statement->data.id = id;
                    strncpy(statement->data.username, username,
                            sizeof(statement->data.username));
                    break;
                case (SELECT):
                    break;
                case (DELETE):
                    break;
                case (ROLLBACK):
                    break;
            }
            return STATEMENT_RECOGNIZED;
        }
    }
    return STATEMENT_UNRECOGNIZED;
}

static void _statement_executer(Statement* statement) {
    for (size_t i = 0; i < num_mappings; ++i) {
        if (mappings[i].type == statement->type) {
            mappings[i].function(statement);
            break;
        }
    }
}

static void _debug_table(Table* table) {
    for (size_t i = 0; i < table->num_rows; ++i) {
        printf("id -> %zu & username -> %s\n", table->rows[i].id,
               table->rows[i].username);
    }
}

static void _insert_execute(Statement* statement) {
    if (db.num_rows >= TABLE_MAX_ROWS) {
        printf("Error: Table full.\n");
        return;
    }
    Row* row = &db.rows[db.num_rows++];
    row->id = statement->data.id;
    strncpy(row->username, statement->data.username, sizeof(row->username));
    _debug_table(&db);
}

static void _select_execute(Statement* statement) {
    (void)statement;
    printf("SELECT goes here\n");
}

static void _delete_execute(Statement* statement) {
    (void)statement;
    printf("DELETE goes here\n");
}

static void _rollback_execute(Statement* statement) {
    (void)statement;
    printf("ROLLBACK goes here\n");
}
