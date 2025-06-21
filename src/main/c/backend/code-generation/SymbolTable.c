#include "SymbolTable.h"
#include "../../shared/Logger.h"
#include "../../shared/String.h"
#include <stdlib.h>
#include <string.h>

static Logger *logger = NULL;

void initSymbolTable(SymbolTable *table) {
    logger = createLogger("SymbolTable");
    table->count = 0;
    table->capacity = 100;
    table->symbols = calloc(table->capacity, sizeof(Symbol));
}

void addSymbol(SymbolTable *table, char *id, EntityType type, void *data) {
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        table->symbols = realloc(table->symbols, table->capacity * sizeof(Symbol));
    }
    Symbol *symbol = &table->symbols[table->count++];
    symbol->id = strdup(id);
    symbol->type = type;
    symbol->data = data;
    logDebugging(logger, "Added symbol: %s (type: %d)", id, type);
}

Symbol *findSymbol(SymbolTable *table, char *id) {
    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].id, id) == 0) {
            return &table->symbols[i];
        }
    }
    logWarning(logger, "Symbol not found: %s", id);
    return NULL;
}

void freeSymbolTable(SymbolTable *table) {
    for (int i = 0; i < table->count; i++) {
        free(table->symbols[i].id);
        if (table->symbols[i].type == ENTITY_PROFESSOR) {
            ProfessorData *data = (ProfessorData *)table->symbols[i].data;
            free(data->name);
            free(data->availability); // No liberar cada elemento, es un array
            for (int j = 0; j < data->canTeachCount; j++) {
                free(data->canTeach[j]);
            }
            free(data->canTeach);
            free(data);
        } else if (table->symbols[i].type == ENTITY_COURSE) {
            CourseData *data = (CourseData *)table->symbols[i].data;
            free(data->name);
            free(data->requires);
            free(data);
        } else if (table->symbols[i].type == ENTITY_CLASSROOM) {
            ClassroomData *data = (ClassroomData *)table->symbols[i].data;
            free(data->name);
            free(data->building);
            for (int j = 0; j < data->hasCount; j++) {
                free(data->has[j]);
            }
            free(data->has);
            free(data);
        }
    }
    free(table->symbols);
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
    destroyLogger(logger);
}