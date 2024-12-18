#include "headers/parser.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "headers/repl.h"

/*******************************************************************************
                           PRIVATE DECLARATION STUFF
*******************************************************************************/
#define TABLE_MAX_PAGES 100
#define COLUMN_USERNAME_SIZE 32

// Some cool offsetof trick to get the sizeof the datamembers of the struct
// without actually creating the instance of it.
#define size_of_attribute(Struct, Attribute) sizeof(((Struct *)0)->Attribute)

#define ENSURE_ROW_SLOT(table, row_num)                       \
    void *row_slot = _row_slot(table, row_num);               \
    if (row_slot == NULL) {                                   \
        fprintf(stderr, "Error: Unable to fetch row_slot\n"); \
        return;                                               \
    }

/*******************************************************************************
                        ENUMS/STRUCTS DECLARATION STUFF
*******************************************************************************/
typedef enum { INSERT, SELECT, DELETE, ROLLBACK } StatementType;

typedef enum {
    STATEMENT_RECOGNIZED,
    STATEMENT_UNRECOGNIZED,
    STATEMENT_INCOMPLETE,
} StatementRecognitionResult;

typedef struct {
    char username[COLUMN_USERNAME_SIZE];
    size_t id;
} Row;

typedef struct {
    void *pages[TABLE_MAX_PAGES];
    size_t num_rows;
} Table;

typedef struct {
    StatementType type;
    Row data;
} Statement;

// TODO:- might have to refactor the function pointer a little to be more
// generic ig?
typedef struct {
    const char *keyword;
    StatementType type;
    void (*function)(const Statement *statement);
} KeywordMapping;

/*******************************************************************************
                           FUNCTION DECLARATION STUFF
*******************************************************************************/
static StatementRecognitionResult _make_statement(
    const inputbuffer_t *input_buffer, Statement *statement);

static void _serializer(const Row *src, void *const dest);
static void _deserializer(const void *src, Row *const dest);
static void _print_row(const Row *row);

static void *_row_slot(Table *table, size_t row_num);
static void __attribute__((destructor)) _cleanup(void);

static void _statement_executer(Statement *statement);
// START: FUNCTION USED AS FUNCTION POINTERS
static void _insert_execute(const Statement *statement);
static void _select_execute(const Statement *statement);
static void _delete_execute(const Statement *statement);
static void _rollback_execute(const Statement *statement);
// END: FUNCTION USED AS FUNCTION POINTERS

/*******************************************************************************
                         STATIC CONST DECLARATION STUFF
*******************************************************************************/
static const KeywordMapping mappings[] = {
    {"insert", INSERT, _insert_execute},
    {"select", SELECT, _select_execute},
    {"delete", DELETE, _delete_execute},
    {"rollback", ROLLBACK, _rollback_execute}};

static const size_t num_mappings = sizeof(mappings) / sizeof(mappings[0]);
static Table table = {0};

// This shit might not look that useful now but will surely be very useful
// in the future as we'll be dynamically be having our rows and stuff.
static const uint8_t ID_SIZE = size_of_attribute(Row, id);
static const uint8_t USERNAME_SIZE = size_of_attribute(Row, username);
static const uint8_t ID_OFFSET = 0;
static const uint8_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
static const uint8_t ROW_SIZE = ID_SIZE + USERNAME_SIZE;
static const uint16_t PAGE_SIZE = 4096;
static const uint8_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
static const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

/*******************************************************************************
                                PUBLIC FUNCTIONS
*******************************************************************************/
void Parser(inputbuffer_t *input_buffer) {
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

/*******************************************************************************
                               PRIVATE FUNCTIONS
*******************************************************************************/
static void __attribute__((destructor)) _cleanup(void) {
    for (uint8_t i = 0; table.pages[i]; ++i) {
        free(table.pages[i]);
        table.pages[i] = NULL;
    }
}

static StatementRecognitionResult _make_statement(
    const inputbuffer_t *input_buffer, Statement *statement) {
    for (uint8_t i = 0; i < num_mappings; ++i) {
        if (strncmp(input_buffer->buffer, mappings[i].keyword,
                    strlen(mappings[i].keyword)) == 0) {
            statement->type = mappings[i].type;
            switch (statement->type) {
                case (INSERT):;
                    size_t id;
                    char username[COLUMN_USERNAME_SIZE];
                    if (sscanf(input_buffer->buffer, "insert %zu %31s", &id,
                               username) < 2) {
                        fprintf(stderr,
                                "Syntax Error: Correct usage: insert <id> "
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

static void _serializer(const Row *src, void *const dest) {
    memcpy(dest + ID_OFFSET, &(src->id), ID_SIZE);
    memcpy(dest + USERNAME_OFFSET, &(src->username), USERNAME_SIZE);
}

static void _deserializer(const void *src, Row *const dest) {
    memcpy(&(dest->id), src + ID_OFFSET, ID_SIZE);
    memcpy(&(dest->username), src + USERNAME_OFFSET, USERNAME_SIZE);
}

static void *_row_slot(Table *table, size_t row_num) {
    size_t page_num = row_num / ROWS_PER_PAGE;
    size_t row_offset = row_num % ROWS_PER_PAGE;
    if (page_num >= TABLE_MAX_PAGES) {
        fprintf(stderr, "Error: Exceeded maximum number of pages\n");
        return NULL;
    }
    void *page = table->pages[page_num];
    if (page == NULL) {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
        if (page == NULL) {
            fprintf(stderr, "Error: Memory allocation failed for page %zu\n",
                    page_num);
            return NULL;
        }
    }
    // printf("page_num -> %zu | row_offset = %zu | row_size -> %hhu\n",
    // page_num,
    //        row_offset, ROW_SIZE);
    return page + (row_offset * ROW_SIZE);
}

static void _statement_executer(Statement *statement) {
    for (uint8_t i = 0; i < num_mappings; ++i) {
        if (mappings[i].type == statement->type) {
            mappings[i].function(statement);
            break;
        }
    }
}

static void _print_row(const Row *row) {
    printf("(%zu, %s)\n", row->id, row->username);
}

static void _insert_execute(const Statement *statement) {
    if (table.num_rows >= TABLE_MAX_ROWS) {
        fprintf(stderr, "Error: Table full.\n");
        return;
    }
    const Row *row = &(statement->data);
    ENSURE_ROW_SLOT(&table, table.num_rows);
    _serializer(row, row_slot);
    // printf("_row_slot(&table, table.num_rows) -> %p AND num_row -> %zu\n",
    //        _row_slot(&table, table.num_rows), table.num_rows);
    table.num_rows += 1;
}

static void _select_execute(const Statement *statement) {
    (void)statement;
    Row row;
    for (size_t i = 0; i < table.num_rows; ++i) {
        ENSURE_ROW_SLOT(&table, i);
        _deserializer(row_slot, &row);
        _print_row(&row);
    }
}

static void _delete_execute(const Statement *statement) {
    (void)statement;
    printf("DELETE goes here\n");
}

static void _rollback_execute(const Statement *statement) {
    (void)statement;
    printf("ROLLBACK goes here\n");
}
