#include "Generator.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* MODULE INTERNAL STATE */

const char indentationCharacter = ' ';
const char indentationSize = 4;
static Logger *logger = NULL;

void initializeGeneratorModule() {
    logger = createLogger("Generator");
}

void shutdownGeneratorModule() {
    if (logger != NULL) {
        destroyLogger(logger);
    }
}

boolean validateSemantic(Program *program, SymbolTable *table, GlobalConfig *config, ScheduleEntry **schedule, int *scheduleCount) {
    logDebugging(logger, "Starting semantic validation...");

    // Inicializar configuración global
    config->openFrom = program->configuration.universityOpen.openFrom;
    config->openTo = program->configuration.universityOpen.openTo;
    config->hasClassDuration = program->configuration.hasClassDuration;
    if (config->hasClassDuration) {
        config->minHours = program->configuration.classDuration.minHours;
        config->maxHours = program->configuration.classDuration.maxHours;
    }

    // Recorrer declaraciones para llenar la tabla de símbolos
    Declaration *decl = program->declarations;
    while (decl) {
        if (decl->type == DECLARATION_ENTITY) {
            Entity *entity = decl->entity;
            void *data = calloc(1, entity->type == ENTITY_PROFESSOR ? sizeof(ProfessorData) :
                                entity->type == ENTITY_COURSE ? sizeof(CourseData) :
                                sizeof(ClassroomData));
            if (!data) {
                logError(logger, "Failed to allocate memory for entity data");
                return false;
            }
            Attribute *attr = entity->attributes;
            if (entity->type == ENTITY_PROFESSOR) {
                ProfessorData *prof = (ProfessorData *)data;
                while (attr) {
                    if (strcmp(attr->key, "name") == 0) {
                        prof->name = attr->strValue ? strdup(attr->strValue) : NULL;
                        logDebugging(logger, "Assigned name %s to professor", prof->name);
                    } else if (strcmp(attr->key, "available") == 0) {
                        prof->availability = realloc(prof->availability, (prof->availabilityCount + 1) * sizeof(IntervalDayOfWeek));
                        prof->availability[prof->availabilityCount] = attr->intervalValue;
                        prof->availabilityCount++;
                    } else if (strcmp(attr->key, "canTeach") == 0) {
                        prof->canTeach = realloc(prof->canTeach, (prof->canTeachCount + 1) * sizeof(char *));
                        prof->canTeach[prof->canTeachCount] = attr->strValue ? strdup(attr->strValue) : NULL;
                        logDebugging(logger, "Assigned canTeach %s to professor", prof->canTeach[prof->canTeachCount]);
                        prof->canTeachCount++;
                    }
                    attr = attr->next;
                }
            } else if (entity->type == ENTITY_COURSE) {
                CourseData *course = (CourseData *)data;
                while (attr) {
                    if (strcmp(attr->key, "name") == 0) {
                        course->name = attr->strValue ? strdup(attr->strValue) : NULL;
                        logDebugging(logger, "Assigned name %s to course", course->name);
                    } else if (strcmp(attr->key, "hours") == 0) {
                        course->hours = attr->intValue;
                    } else if (strcmp(attr->key, "requires") == 0) {
                        course->requires = attr->strValue ? strdup(attr->strValue) : NULL;
                        logDebugging(logger, "Assigned requires %s to course", course->requires);
                    }
                    attr = attr->next;
                }
            } else if (entity->type == ENTITY_CLASSROOM) {
                ClassroomData *room = (ClassroomData *)data;
                while (attr) {
                    if (strcmp(attr->key, "name") == 0) {
                        room->name = attr->strValue ? strdup(attr->strValue) : NULL;
                        logDebugging(logger, "Assigned name %s to classroom", room->name);
                    } else if (strcmp(attr->key, "building") == 0) {
                        room->building = attr->strValue ? strdup(attr->strValue) : NULL;
                        logDebugging(logger, "Assigned building %s to classroom", room->building);
                    } else if (strcmp(attr->key, "capacity") == 0) {
                        room->capacity = attr->intValue;
                    } else if (strcmp(attr->key, "has") == 0) {
                        room->has = realloc(room->has, (room->hasCount + 1) * sizeof(char *));
                        room->has[room->hasCount] = attr->strValue ? strdup(attr->strValue) : NULL;
                        logDebugging(logger, "Assigned has %s to classroom", room->has[room->hasCount]);
                        room->hasCount++;
                    }
                    attr = attr->next;
                }
            }
            addSymbol(table, entity->id, entity->type, data);
        }
        decl = decl->next;
    }

    // Validar preferencias duras y construir el horario
    *scheduleCount = 0;
    *schedule = NULL;
    decl = program->declarations;
    while (decl) {
        if (decl->type == DECLARATION_PREFERENCE && decl->preference->type == HARD_PREFERENCE) {
            Preference *pref = decl->preference;
            ScheduleEntry entry = {0};
            entry.professorId = pref->details->professorId ? strdup(pref->details->professorId) : NULL;
            entry.courseId = pref->details->courseId ? strdup(pref->details->courseId) : NULL;
            entry.classroomId = pref->details->classroomId ? strdup(pref->details->classroomId) : NULL;
            if (pref->details->hasTime) {
                entry.startTime = pref->details->startTime;
                entry.endTime = pref->details->endTime;
            }
            if (pref->details->hasDay) {
                entry.day = pref->details->day;
            }
            entry.isHard = true;

            logDebugging(logger, "Processing hard preference for professor %s, course %s, classroom %s", 
                         entry.professorId ? entry.professorId : "NULL", 
                         entry.courseId ? entry.courseId : "NULL",
                         entry.classroomId ? entry.classroomId : "NULL");

            // Validar existencia de entidades
            if (!entry.professorId || !entry.courseId || !findSymbol(table, entry.professorId) || 
                !findSymbol(table, entry.courseId) || 
                (entry.classroomId && !findSymbol(table, entry.classroomId))) {
                logError(logger, "Reference to undeclared entity: %s", 
                         entry.professorId ? entry.professorId : "NULL");
                free(entry.professorId);
                free(entry.courseId);
                free(entry.classroomId);
                return false;
            }

            // Validar que el profesor pueda enseñar el curso
            ProfessorData *prof = (ProfessorData *)findSymbol(table, entry.professorId)->data;
            boolean canTeach = false;
            for (int i = 0; i < prof->canTeachCount; i++) {
                if (prof->canTeach[i] && strcmp(prof->canTeach[i], entry.courseId) == 0) {
                    canTeach = true;
                    break;
                }
            }
            if (!canTeach) {
                logError(logger, "Professor %s cannot teach %s", entry.professorId, entry.courseId);
                free(entry.professorId);
                free(entry.courseId);
                free(entry.classroomId);
                return false;
            }

            // Validar horario dentro de la apertura de la universidad
            if (pref->details->hasTime) {
                if (entry.startTime.hour < config->openFrom.hour || entry.endTime.hour > config->openTo.hour) {
                    logError(logger, "Schedule outside university open hours");
                    free(entry.professorId);
                    free(entry.courseId);
                    free(entry.classroomId);
                    return false;
                }

                // Validar duración de la clase
                int duration = (entry.endTime.hour * 60 + entry.endTime.minute) -
                              (entry.startTime.hour * 60 + entry.startTime.minute);
                if (config->hasClassDuration && (duration < config->minHours * 60 || duration > config->maxHours * 60)) {
                    logError(logger, "Invalid class duration: %d minutes", duration);
                    free(entry.professorId);
                    free(entry.courseId);
                    free(entry.classroomId);
                    return false;
                }

                // Validar disponibilidad del profesor
                boolean available = false;
                for (int i = 0; i < prof->availabilityCount; i++) {
                    IntervalDayOfWeek avail = prof->availability[i];
                    if ((avail.dayOfWeek == entry.day || avail.dayOfWeek == DAY_EVERYDAY) &&
                        entry.startTime.hour >= avail.start.hour && entry.endTime.hour <= avail.end.hour) {
                        available = true;
                        break;
                    }
                }
                if (!available) {
                    logError(logger, "Professor %s not available at specified time", entry.professorId);
                    free(entry.professorId);
                    free(entry.courseId);
                    free(entry.classroomId);
                    return false;
                }
            }

            // Validar requisitos del aula
            if (entry.classroomId) {
                ClassroomData *room = (ClassroomData *)findSymbol(table, entry.classroomId)->data;
                CourseData *course = (CourseData *)findSymbol(table, entry.courseId)->data;
                if (course->requires) {
                    boolean hasRequirement = false;
                    for (int i = 0; i < room->hasCount; i++) {
                        if (room->has[i] && strcmp(room->has[i], course->requires) == 0) {
                            hasRequirement = true;
                            break;
                        }
                    }
                    if (!hasRequirement) {
                        logError(logger, "Classroom %s does not have required %s for %s", 
                                 entry.classroomId, course->requires, entry.courseId);
                        free(entry.professorId);
                        free(entry.courseId);
                        free(entry.classroomId);
                        return false;
                    }
                }
            }

            // Validar conflicto de aula con otras entradas
            if (entry.classroomId && pref->details->hasTime && pref->details->hasDay) {
                for (int i = 0; i < *scheduleCount; i++) {
                    if ((*schedule)[i].classroomId && strcmp((*schedule)[i].classroomId, entry.classroomId) == 0 &&
                        (*schedule)[i].day == entry.day) {
                        // Comprobar superposición de horarios
                        if (!((*schedule)[i].endTime.hour < entry.startTime.hour ||
                              (*schedule)[i].startTime.hour > entry.endTime.hour)) {
                            logError(logger, "Classroom %s is already assigned at overlapping time on %s", 
                                     entry.classroomId, entry.day == DAY_WEDNESDAY ? "Wednesday" : "Unknown");
                            free(entry.professorId);
                            free(entry.courseId);
                            free(entry.classroomId);
                            return false;
                        }
                    }
                }
            }

            // Agregar a la lista de horarios
            *schedule = realloc(*schedule, (*scheduleCount + 1) * sizeof(ScheduleEntry));
            if (!*schedule) {
                logError(logger, "Failed to allocate memory for schedule");
                free(entry.professorId);
                free(entry.courseId);
                free(entry.classroomId);
                return false;
            }
            (*schedule)[*scheduleCount] = entry;
            logDebugging(logger, "Added schedule entry %d", *scheduleCount);
            (*scheduleCount)++;
        }
        decl = decl->next;
    }

    // Validar preferencias blandas (sin agregar al horario)
    decl = program->declarations;
    while (decl) {
        if (decl->type == DECLARATION_PREFERENCE && decl->preference->type == SOFT_PREFERENCE) {
            Preference *pref = decl->preference;
            logDebugging(logger, "Validating soft preference for professor %s, course %s", 
                         pref->details->professorId ? pref->details->professorId : "NULL", 
                         pref->details->courseId ? pref->details->courseId : "NULL");

            // Validar existencia de entidades
            if (!pref->details->professorId || !pref->details->courseId || 
                !findSymbol(table, pref->details->professorId) || 
                !findSymbol(table, pref->details->courseId)) {
                logError(logger, "Soft preference references undeclared entity: %s", 
                         pref->details->professorId ? pref->details->professorId : "NULL");
                return false;
            }

            // Validar que el profesor pueda enseñar el curso
            ProfessorData *prof = (ProfessorData *)findSymbol(table, pref->details->professorId)->data;
            boolean canTeach = false;
            for (int i = 0; i < prof->canTeachCount; i++) {
                if (prof->canTeach[i] && strcmp(prof->canTeach[i], pref->details->courseId) == 0) {
                    canTeach = true;
                    break;
                }
            }
            if (!canTeach) {
                logError(logger, "Professor %s cannot teach %s in soft preference", 
                         pref->details->professorId, pref->details->courseId);
                return false;
            }

            // Validar día si está especificado
            if (pref->details->hasDay) {
                boolean available = false;
                for (int i = 0; i < prof->availabilityCount; i++) {
                    IntervalDayOfWeek avail = prof->availability[i];
                    if (avail.dayOfWeek == pref->details->day || avail.dayOfWeek == DAY_EVERYDAY) {
                        available = true;
                        break;
                    }
                }
                if (!available) {
                    logError(logger, "Professor %s not available on specified day for soft preference", 
                             pref->details->professorId);
                    return false;
                }
            }
        }
        decl = decl->next;
    }

    logDebugging(logger, "Found %d schedule entries", *scheduleCount);

    // Validar demandas
    decl = program->declarations;
    while (decl) {
        if (decl->type == DECLARATION_DEMAND) {
            Demand *demand = decl->demand;
            Symbol *course = findSymbol(table, demand->courseId);
            if (!course) {
                logError(logger, "Course %s not declared in demand", demand->courseId);
                return false;
            }
            ClassroomData *room = NULL;
            for (int i = 0; i < *scheduleCount; i++) {
                if (strcmp((*schedule)[i].courseId, demand->courseId) == 0 && (*schedule)[i].classroomId) {
                    room = (ClassroomData *)findSymbol(table, (*schedule)[i].classroomId)->data;
                    if (room->capacity < demand->students) {
                        logError(logger, "Classroom %s has insufficient capacity for %d students", 
                                 (*schedule)[i].classroomId, demand->students);
                        return false;
                    }
                }
            }
        }
        decl = decl->next;
    }

    logDebugging(logger, "Semantic validation succeeded!");
    return true;
}

void generateCode(Program *program, SymbolTable *table, ScheduleEntry *schedule, int scheduleCount, const char *outputFile) {
    char absolutePath[1024];
    if (getcwd(absolutePath, sizeof(absolutePath)) == NULL) {
        logError(logger, "Failed to get current working directory: %s", strerror(errno));
        return;
    }
    strcat(absolutePath, "/");
    strcat(absolutePath, outputFile);
    logDebugging(logger, "Attempting to open file: %s", absolutePath);

    FILE *filePointer = fopen(outputFile, "w");
    if (!filePointer) {
        logError(logger, "Could not open file %s: %s", outputFile, strerror(errno));
        return;
    }

    logDebugging(logger, "Generating HTML output with %d schedule entries", scheduleCount);

    fprintf(filePointer, "<!DOCTYPE html>\n<html>\n<head>\n");
    fprintf(filePointer, "<title>University Schedule</title>\n");
    fprintf(filePointer, "<style>\n");
    fprintf(filePointer, "table { border-collapse: collapse; width: 100%%; }\n");
    fprintf(filePointer, "th, td { border: 1px solid black; padding: 8px; text-align: left; }\n");
    fprintf(filePointer, "th { background-color: #f2f2f2; }\n");
    fprintf(filePointer, "</style>\n");
    fprintf(filePointer, "</head>\n<body>\n");
    fprintf(filePointer, "<h2>Class Schedule</h2>\n");
    fprintf(filePointer, "<table>\n");
    fprintf(filePointer, "<tr><th>Course</th><th>Professor</th><th>Classroom</th><th>Building</th><th>Day</th><th>Time</th></tr>\n");

    // Mantener un registro de aulas usadas para evitar conflictos
    char **usedClassrooms = calloc(scheduleCount, sizeof(char *));
    int usedClassroomCount = 0;

    for (int i = 0; i < scheduleCount; i++) {
        ScheduleEntry *entry = &schedule[i];
        CourseData *course = (CourseData *)findSymbol(table, entry->courseId)->data;
        ProfessorData *prof = (ProfessorData *)findSymbol(table, entry->professorId)->data;
        ClassroomData *classroom = entry->classroomId ? (ClassroomData *)findSymbol(table, entry->classroomId)->data : NULL;

        // Asignar aula si no está especificada
        if (!entry->classroomId) {
            for (int j = 0; j < table->count; j++) {
                if (table->symbols[j].type == ENTITY_CLASSROOM) {
                    ClassroomData *candidate = (ClassroomData *)table->symbols[j].data;
                    boolean valid = true;
                    // Verificar que el aula no esté ya asignada en el mismo horario
                    for (int k = 0; k < usedClassroomCount; k++) {
                        if (strcmp(usedClassrooms[k], table->symbols[j].id) == 0 &&
                            schedule[i].day == schedule[k].day &&
                            !(schedule[i].endTime.hour < schedule[k].startTime.hour ||
                              schedule[i].startTime.hour > schedule[k].endTime.hour)) {
                            valid = false;
                            break;
                        }
                    }
                    // Verificar requisitos del curso
                    if (valid && course->requires) {
                        valid = false;
                        for (int k = 0; k < candidate->hasCount; k++) {
                            if (candidate->has[k] && strcmp(candidate->has[k], course->requires) == 0) {
                                valid = true;
                                break;
                            }
                        }
                    }
                    if (valid) {
                        entry->classroomId = strdup(table->symbols[j].id);
                        logDebugging(logger, "Assigned classroom %s to course %s", table->symbols[j].id, course->name);
                        classroom = candidate;
                        usedClassrooms[usedClassroomCount++] = entry->classroomId;
                        break;
                    }
                }
            }
            if (!entry->classroomId) {
                logWarning(logger, "No suitable classroom found for course %s", course->name);
                classroom = NULL;
            }
        }

        const char *dayString;
        switch (entry->day) {
            case DAY_MONDAY: dayString = "Monday"; break;
            case DAY_TUESDAY: dayString = "Tuesday"; break;
            case DAY_WEDNESDAY: dayString = "Wednesday"; break;
            case DAY_THURSDAY: dayString = "Thursday"; break;
            case DAY_FRIDAY: dayString = "Friday"; break;
            default: dayString = "Unknown"; break;
        }

        // Limpiar comillas de las cadenas
        char *cleanCourseName = course->name ? strdup(course->name) : strdup("Unknown");
        char *cleanProfName = prof->name ? strdup(prof->name) : strdup("Unknown");
        char *cleanClassroomName = classroom ? (classroom->name ? strdup(classroom->name) : strdup("Not assigned")) : strdup("Not assigned");
        char *cleanBuilding = classroom ? (classroom->building ? strdup(classroom->building) : strdup("Not assigned")) : strdup("Not assigned");
        if (cleanCourseName[0] == '"' && cleanCourseName[strlen(cleanCourseName) - 1] == '"') {
            cleanCourseName[strlen(cleanCourseName) - 1] = '\0';
            memmove(cleanCourseName, cleanCourseName + 1, strlen(cleanCourseName));
        }
        if (cleanProfName[0] == '"' && cleanProfName[strlen(cleanProfName) - 1] == '"') {
            cleanProfName[strlen(cleanProfName) - 1] = '\0';
            memmove(cleanProfName, cleanProfName + 1, strlen(cleanProfName));
        }
        if (cleanClassroomName[0] == '"' && cleanClassroomName[strlen(cleanClassroomName) - 1] == '"') {
            cleanClassroomName[strlen(cleanClassroomName) - 1] = '\0';
            memmove(cleanClassroomName, cleanClassroomName + 1, strlen(cleanClassroomName));
        }
        if (cleanBuilding[0] == '"' && cleanBuilding[strlen(cleanBuilding) - 1] == '"') {
            cleanBuilding[strlen(cleanBuilding) - 1] = '\0';
            memmove(cleanBuilding, cleanBuilding + 1, strlen(cleanBuilding));
        }

        fprintf(filePointer, "<tr>\n");
        fprintf(filePointer, "<td>%s</td>\n", cleanCourseName);
        fprintf(filePointer, "<td>%s</td>\n", cleanProfName);
        fprintf(filePointer, "<td>%s</td>\n", cleanClassroomName);
        fprintf(filePointer, "<td>%s</td>\n", cleanBuilding);
        fprintf(filePointer, "<td>%s</td>\n", dayString);
        fprintf(filePointer, "<td>%02d:%02d - %02d:%02d</td>\n", entry->startTime.hour, entry->startTime.minute,
                entry->endTime.hour, entry->endTime.minute);
        fprintf(filePointer, "</tr>\n");

        free(cleanCourseName);
        free(cleanProfName);
        free(cleanClassroomName);
        free(cleanBuilding);
    }

    // Liberar aulas usadas
    for (int i = 0; i < usedClassroomCount; i++) {
        free(usedClassrooms[i]);
    }
    free(usedClassrooms);

    fprintf(filePointer, "</table>\n");
    fprintf(filePointer, "</body>\n</html>\n");
    fclose(filePointer);

    logDebugging(logger, "HTML generation completed!");
}

void generate(CompilerState *compilerState) {
    logDebugging(logger, "Generating final output...");

    SymbolTable table;
    GlobalConfig config = {0};
    ScheduleEntry *schedule = NULL;
    int scheduleCount = 0;

    initSymbolTable(&table);
    if (!validateSemantic(compilerState->abstractSyntaxtTree, &table, &config, &schedule, &scheduleCount)) {
        logError(logger, "Semantic validation failed!");
        freeSymbolTable(&table);
        if (schedule) {
            for (int i = 0; i < scheduleCount; i++) {
                free(schedule[i].professorId);
                free(schedule[i].courseId);
                free(schedule[i].classroomId);
            }
            free(schedule);
        }
        compilerState->succeed = false;
        return;
    }

    generateCode(compilerState->abstractSyntaxtTree, &table, schedule, scheduleCount, "output.html");
    freeSymbolTable(&table);
    // Comentar liberación de schedule para evitar double free
    /*
    if (schedule) {
        for (int i = 0; i < scheduleCount; i++) {
            free(schedule[i].professorId);
            free(schedule[i].courseId);
            free(schedule[i].classroomId);
        }
        free(schedule);
    }
    */

    logDebugging(logger, "Generation completed!");
    compilerState->succeed = true;
}
