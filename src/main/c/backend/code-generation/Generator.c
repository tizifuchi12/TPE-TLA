#include "Generator.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

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

    // Validar y asignar preferencias duras y blandas con horario específico
    *scheduleCount = 0;
    *schedule = NULL;
    decl = program->declarations;
    while (decl) {
        if (decl->type == DECLARATION_PREFERENCE &&
            (decl->preference->type == HARD_PREFERENCE ||
             (decl->preference->type == SOFT_PREFERENCE && decl->preference->details->hasTime && decl->preference->details->hasDay))) {
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
            entry.isHard = (pref->type == HARD_PREFERENCE);

            logDebugging(logger, "Processing %s preference for professor %s, course %s, classroom %s",
                         entry.isHard ? "hard" : "soft",
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

            // Validar que el profesor pueda enseñar la materia
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
                int startMinutes = entry.startTime.hour * 60 + entry.startTime.minute;
                int endMinutes = entry.endTime.hour * 60 + entry.endTime.minute;
                int openFromMinutes = config->openFrom.hour * 60 + config->openFrom.minute;
                int openToMinutes = config->openTo.hour * 60 + config->openTo.minute;

                if (startMinutes < openFromMinutes || endMinutes > openToMinutes) {
                    logError(logger, "Schedule outside university open hours");
                    free(entry.professorId);
                    free(entry.courseId);
                    free(entry.classroomId);
                    return false;
                }

                // Validar duración de la clase
                int duration = endMinutes - startMinutes;
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
                    int availStart = avail.start.hour * 60 + avail.start.minute;
                    int availEnd = avail.end.hour * 60 + avail.end.minute;
                    if ((avail.dayOfWeek == entry.day || avail.dayOfWeek == DAY_EVERYDAY) &&
                        startMinutes >= availStart && endMinutes <= availEnd) {
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
                int startMinutes = entry.startTime.hour * 60 + entry.startTime.minute;
                int endMinutes = entry.endTime.hour * 60 + entry.endTime.minute;
                for (int i = 0; i < *scheduleCount; i++) {
                    if ((*schedule)[i].classroomId && strcmp((*schedule)[i].classroomId, entry.classroomId) == 0 &&
                        (*schedule)[i].day == entry.day) {
                        int existingStart = (*schedule)[i].startTime.hour * 60 + (*schedule)[i].startTime.minute;
                        int existingEnd = (*schedule)[i].endTime.hour * 60 + (*schedule)[i].endTime.minute;
                        if (!(existingEnd <= startMinutes || existingStart >= endMinutes)) {
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

            // Validar conflicto de profesor con otras entradas
            if (entry.professorId && pref->details->hasTime && pref->details->hasDay) {
                int startMinutes = entry.startTime.hour * 60 + entry.startTime.minute;
                int endMinutes = entry.endTime.hour * 60 + entry.endTime.minute;
                for (int i = 0; i < *scheduleCount; i++) {
                    if ((*schedule)[i].professorId && strcmp((*schedule)[i].professorId, entry.professorId) == 0 &&
                        (*schedule)[i].day == entry.day) {
                        int existingStart = (*schedule)[i].startTime.hour * 60 + (*schedule)[i].startTime.minute;
                        int existingEnd = (*schedule)[i].endTime.hour * 60 + (*schedule)[i].endTime.minute;
                        if (!(existingEnd <= startMinutes || existingStart >= endMinutes)) {
                            logError(logger, "Professor %s is already assigned at overlapping time on %s",
                                     entry.professorId, entry.day == DAY_WEDNESDAY ? "Wednesday" : "Unknown");
                            free(entry.professorId);
                            free(entry.courseId);
                            free(entry.classroomId);
                            return false;
                        }
                    }
                }
            }

            // Buscar aula disponible si no se especificó
            if (!entry.classroomId && pref->details->hasTime && pref->details->hasDay) {
                CourseData *course = (CourseData *)findSymbol(table, entry.courseId)->data;
                for (int r = 0; r < table->count; r++) {
                    if (table->symbols[r].type == ENTITY_CLASSROOM) {
                        ClassroomData *room = (ClassroomData *)table->symbols[r].data;
                        boolean validRoom = true;

                        // Verificar requisitos del curso
                        if (course->requires) {
                            boolean hasRequirement = false;
                            for (int h = 0; h < room->hasCount; h++) {
                                if (room->has[h] && strcmp(room->has[h], course->requires) == 0) {
                                    hasRequirement = true;
                                    break;
                                }
                            }
                            if (!hasRequirement) {
                                validRoom = false;
                            }
                        }

                        // Verificar conflicto de aula
                        if (validRoom && pref->details->hasTime && pref->details->hasDay) {
                            int startMinutes = entry.startTime.hour * 60 + entry.startTime.minute;
                            int endMinutes = entry.endTime.hour * 60 + entry.endTime.minute;
                            for (int k = 0; k < *scheduleCount; k++) {
                                if ((*schedule)[k].classroomId &&
                                    strcmp((*schedule)[k].classroomId, table->symbols[r].id) == 0 &&
                                    (*schedule)[k].day == entry.day) {
                                    int existingStart = (*schedule)[k].startTime.hour * 60 + (*schedule)[k].startTime.minute;
                                    int existingEnd = (*schedule)[k].endTime.hour * 60 + (*schedule)[k].endTime.minute;
                                    if (!(existingEnd <= startMinutes || existingStart >= endMinutes)) {
                                        validRoom = false;
                                        break;
                                    }
                                }
                            }
                        }

                        if (validRoom) {
                            entry.classroomId = strdup(table->symbols[r].id);
                            break;
                        }
                    }
                }
                if (!entry.classroomId) {
                    logError(logger, "No suitable classroom found for %s preference for course %s",
                             entry.isHard ? "hard" : "soft", entry.courseId);
                    free(entry.professorId);
                    free(entry.courseId);
                    return false;
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
            logDebugging(logger, "Added %s schedule entry %d", entry.isHard ? "hard" : "soft", *scheduleCount);
            (*scheduleCount)++;
        }
        decl = decl->next;
    }

    // Validar y asignar horas faltantes de los cursos
    for (int i = 0; i < table->count; i++) {
        if (table->symbols[i].type == ENTITY_COURSE) {
            CourseData *course = (CourseData *)table->symbols[i].data;
            int totalMinutes = 0;
            for (int j = 0; j < *scheduleCount; j++) {
                if (strcmp((*schedule)[j].courseId, table->symbols[i].id) == 0) {
                    int duration = ((*schedule)[j].endTime.hour * 60 + (*schedule)[j].endTime.minute) -
                                  ((*schedule)[j].startTime.hour * 60 + (*schedule)[j].startTime.minute);
                    totalMinutes += duration;
                }
            }
            int requiredMinutes = course->hours * 60;
            int missingMinutes = requiredMinutes - totalMinutes;

            if (missingMinutes > 0) {
                logDebugging(logger, "Course %s is missing %d minutes", course->name, missingMinutes);

                // Intentar asignar las horas faltantes en múltiples bloques si es necesario
                while (missingMinutes > 0) {
                    boolean assigned = false;
                    for (int j = 0; j < table->count && !assigned; j++) {
                        if (table->symbols[j].type == ENTITY_PROFESSOR) {
                            ProfessorData *prof = (ProfessorData *)table->symbols[j].data;
                            boolean canTeach = false;
                            for (int k = 0; k < prof->canTeachCount; k++) {
                                if (prof->canTeach[k] && strcmp(prof->canTeach[k], table->symbols[i].id) == 0) {
                                    canTeach = true;
                                    break;
                                }
                            }
                            if (!canTeach) continue;

                            // Buscar preferencia blanda para el curso (solo día)
                            boolean hasPreferredDay = false;
                            DayOfWeek preferredDay = DAY_MONDAY; // Valor por defecto
                            decl = program->declarations;
                            while (decl) {
                                if (decl->type == DECLARATION_PREFERENCE && decl->preference->type == SOFT_PREFERENCE) {
                                    Preference *pref = decl->preference;
                                    if (pref->details->professorId && pref->details->courseId &&
                                        strcmp(pref->details->professorId, table->symbols[j].id) == 0 &&
                                        strcmp(pref->details->courseId, table->symbols[i].id) == 0 &&
                                        pref->details->hasDay && !pref->details->hasTime) {
                                        preferredDay = pref->details->day;
                                        hasPreferredDay = true;
                                        break;
                                    }
                                }
                                decl = decl->next;
                            }

                            // Días a probar
                            DayOfWeek daysToTry[] = { DAY_MONDAY, DAY_TUESDAY, DAY_WEDNESDAY, DAY_THURSDAY, DAY_FRIDAY };
                            int daysCount = 5;

                            // Si hay día preferido, probarlo primero
                            if (hasPreferredDay) {
                                for (int d = 0; d < daysCount; d++) {
                                    if (daysToTry[d] == preferredDay) {
                                        DayOfWeek temp = daysToTry[0];
                                        daysToTry[0] = preferredDay;
                                        daysToTry[d] = temp;
                                        break;
                                    }
                                }
                            }

                            for (int d = 0; d < daysCount && !assigned; d++) {
                                DayOfWeek day = daysToTry[d];

                                // Verificar disponibilidad del profesor en el día
                                boolean dayAvailable = false;
                                IntervalDayOfWeek avail;
                                int availStart = 0, availEnd = 0;
                                for (int k = 0; k < prof->availabilityCount; k++) {
                                    avail = prof->availability[k];
                                    if (avail.dayOfWeek == day || avail.dayOfWeek == DAY_EVERYDAY) {
                                        dayAvailable = true;
                                        availStart = avail.start.hour * 60 + avail.start.minute;
                                        availEnd = avail.end.hour * 60 + avail.end.minute;
                                        break;
                                    }
                                }
                                if (!dayAvailable) continue;

                                // Calcular duración para este bloque
                                int duration = missingMinutes;
                                if (duration < config->minHours * 60) {
                                    duration = config->minHours * 60;
                                }
                                if (duration > config->maxHours * 60) {
                                    duration = config->maxHours * 60;
                                }
                                if (duration > (availEnd - availStart)) {
                                    duration = availEnd - availStart;
                                }
                                if (duration > missingMinutes) {
                                    duration = missingMinutes;
                                }

                                // Buscar horario disponible
                                int openFrom = config->openFrom.hour * 60 + config->openFrom.minute;
                                int openTo = config->openTo.hour * 60 + config->openTo.minute;
                                for (int start = availStart; start + duration <= availEnd && start + duration <= openTo; start += 30) {
                                    int end = start + duration;
                                    if (start < openFrom) continue;

                                    boolean conflict = false;
                                    // Verificar conflicto con otras clases del profesor
                                    for (int k = 0; k < *scheduleCount; k++) {
                                        if ((*schedule)[k].professorId && strcmp((*schedule)[k].professorId, table->symbols[j].id) == 0 &&
                                            (*schedule)[k].day == day) {
                                            int existingStart = (*schedule)[k].startTime.hour * 60 + (*schedule)[k].startTime.minute;
                                            int existingEnd = (*schedule)[k].endTime.hour * 60 + (*schedule)[k].endTime.minute;
                                            if (!(existingEnd <= start || existingStart >= end)) {
                                                conflict = true;
                                                break;
                                            }
                                        }
                                    }
                                    if (conflict) continue;

                                    // Buscar aula disponible
                                    char *selectedClassroomId = NULL;
                                    for (int r = 0; r < table->count; r++) {
                                        if (table->symbols[r].type == ENTITY_CLASSROOM) {
                                            ClassroomData *room = (ClassroomData *)table->symbols[r].data;
                                            boolean validRoom = true;

                                            // Verificar requisitos de la materia
                                            if (course->requires) {
                                                boolean hasRequirement = false;
                                                for (int h = 0; h < room->hasCount; h++) {
                                                    if (room->has[h] && strcmp(room->has[h], course->requires) == 0) {
                                                        hasRequirement = true;
                                                        break;
                                                    }
                                                }
                                                if (!hasRequirement) {
                                                    validRoom = false;
                                                }
                                            }

                                            // Verificar conflicto de aula
                                            if (validRoom) {
                                                for (int k = 0; k < *scheduleCount; k++) {
                                                    if ((*schedule)[k].classroomId &&
                                                        strcmp((*schedule)[k].classroomId, table->symbols[r].id) == 0 &&
                                                        (*schedule)[k].day == day) {
                                                        int existingStart = (*schedule)[k].startTime.hour * 60 + (*schedule)[k].startTime.minute;
                                                        int existingEnd = (*schedule)[k].endTime.hour * 60 + (*schedule)[k].endTime.minute;
                                                        if (!(existingEnd <= start || existingStart >= end)) {
                                                            validRoom = false;
                                                            break;
                                                        }
                                                    }
                                                }
                                            }

                                            if (validRoom) {
                                                selectedClassroomId = strdup(table->symbols[r].id);
                                                break;
                                            }
                                        }
                                    }

                                    if (selectedClassroomId) {
                                        // Crear nueva entrada de horario
                                        ScheduleEntry entry = {0};
                                        entry.professorId = strdup(table->symbols[j].id);
                                        entry.courseId = strdup(table->symbols[i].id);
                                        entry.classroomId = selectedClassroomId;
                                        entry.day = day;
                                        entry.startTime.hour = start / 60;
                                        entry.startTime.minute = start % 60;
                                        entry.endTime.hour = end / 60;
                                        entry.endTime.minute = end % 60;
                                        entry.isHard = false; // Asignación automática

                                        *schedule = realloc(*schedule, (*scheduleCount + 1) * sizeof(ScheduleEntry));
                                        if (!*schedule) {
                                            logError(logger, "Failed to allocate memory for schedule");
                                            free(entry.professorId);
                                            free(entry.courseId);
                                            free(entry.classroomId);
                                            return false;
                                        }
                                        (*schedule)[*scheduleCount] = entry;
                                        logDebugging(logger, "Assigned %d minutes to course %s with professor %s on %s at %02d:%02d-%02d:%02d",
                                                     duration, course->name, prof->name,
                                                     day == DAY_MONDAY ? "Monday" : day == DAY_TUESDAY ? "Tuesday" :
                                                     day == DAY_WEDNESDAY ? "Wednesday" : day == DAY_THURSDAY ? "Thursday" :
                                                     day == DAY_FRIDAY ? "Friday" : "Unknown",
                                                     entry.startTime.hour, entry.startTime.minute,
                                                     entry.endTime.hour, entry.endTime.minute);
                                        (*scheduleCount)++;
                                        missingMinutes -= duration;
                                        assigned = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    if (!assigned) {
                        logError(logger, "Could not assign %d minutes for course %s", missingMinutes, course->name);
                        return false;
                    }
                }
            }
        }
    }

    // Validar preferencias blandas sin horario (solo día)
    decl = program->declarations;
    while (decl) {
        if (decl->type == DECLARATION_PREFERENCE && decl->preference->type == SOFT_PREFERENCE && !decl->preference->details->hasTime) {
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

            // Validar que el profesor pueda enseñar la materia
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
            boolean hasClass = false;
            ClassroomData *room = NULL;
            for (int i = 0; i < *scheduleCount; i++) {
                if (strcmp((*schedule)[i].courseId, demand->courseId) == 0) {
                    hasClass = true;
                    if ((*schedule)[i].classroomId) {
                        room = (ClassroomData *)findSymbol(table, (*schedule)[i].classroomId)->data;
                        if (room->capacity < demand->students) {
                            logError(logger, "Classroom %s has insufficient capacity for %d students",
                                     (*schedule)[i].classroomId, demand->students);
                            return false;
                        }
                    }
                }
            }
            if (!hasClass) {
                logError(logger, "No class assigned for course %s with demand of %d students",
                         demand->courseId, demand->students);
                return false;
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
    fprintf(filePointer, "th { background-color: #f2f2f2; cursor: pointer; }\n");
    fprintf(filePointer, "#searchInput { width: 100%%; padding: 8px; margin-bottom: 10px; font-size: 16px; }\n");
    fprintf(filePointer, ".sort-arrow { margin-left: 5px; }\n");
    fprintf(filePointer, "</style>\n");
    fprintf(filePointer, "</head>\n<body>\n");
    fprintf(filePointer, "<h2>Class Schedule</h2>\n");
    fprintf(filePointer, "<input type=\"text\" id=\"searchInput\" placeholder=\"Buscar por curso, profesor o aula...\">\n");
    fprintf(filePointer, "<table id=\"scheduleTable\">\n");
    fprintf(filePointer, "<tr><th>Course</th><th>Professor</th><th>Classroom</th><th>Building</th><th>Features</th><th id=\"dayHeader\">Day<span id=\"dayArrow\" class=\"sort-arrow\">↑</span></th><th>Time</th></tr>\n");

    char **usedClassrooms = calloc(scheduleCount, sizeof(char *));
    int usedClassroomCount = 0;

    for (int i = 0; i < scheduleCount; i++) {
        ScheduleEntry *entry = &schedule[i];
        CourseData *course = (CourseData *)findSymbol(table, entry->courseId)->data;
        ProfessorData *prof = (ProfessorData *)findSymbol(table, entry->professorId)->data;
        ClassroomData *classroom = entry->classroomId ? (ClassroomData *)findSymbol(table, entry->classroomId)->data : NULL;

        if (!entry->classroomId) {
            for (int j = 0; j < table->count; j++) {
                if (table->symbols[j].type == ENTITY_CLASSROOM) {
                    ClassroomData *candidate = (ClassroomData *)table->symbols[j].data;
                    boolean valid = true;
                    for (int k = 0; k < usedClassroomCount; k++) {
                        if (strcmp(usedClassrooms[k], table->symbols[j].id) == 0 &&
                            schedule[i].day == schedule[k].day) {
                            int startMinutes = schedule[i].startTime.hour * 60 + schedule[i].startTime.minute;
                            int endMinutes = schedule[i].endTime.hour * 60 + schedule[i].endTime.minute;
                            int existingStart = schedule[k].startTime.hour * 60 + schedule[k].startTime.minute;
                            int existingEnd = schedule[k].endTime.hour * 60 + schedule[k].endTime.minute;
                            if (!(existingEnd <= startMinutes || existingStart >= endMinutes)) {
                                valid = false;
                                break;
                            }
                        }
                    }
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
            case DAY_EVERYDAY: dayString = "Everyday"; break;
            default: dayString = "Unknown"; break;
        }

        char *cleanCourseName = course->name ? strdup(course->name) : strdup("Unknown");
        char *cleanProfName = prof->name ? strdup(prof->name) : strdup("Unknown");
        char *cleanClassroomName = classroom ? (classroom->name ? strdup(classroom->name) : strdup("Not assigned")) : strdup("Not assigned");
        char *cleanBuilding = classroom ? (classroom->building ? strdup(classroom->building) : strdup("Not assigned")) : strdup("Not assigned");
        if (cleanCourseName[0] == '"' && cleanCourseName[strlen(cleanCourseName) - 1] == '"') {
            cleanCourseName[strlen(cleanCourseName) - 1] = '\0';
            memmove(cleanCourseName, cleanCourseName + 1, strlen(cleanCourseName));
        }
        if (cleanProfName[0] == '"' && cleanProfName[strlen(cleanCourseName) - 1] == '"') {
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

        char *features = strdup("None");
        if (classroom && classroom->hasCount > 0) {
            free(features);
            size_t totalLen = 0;
            for (int j = 0; j < classroom->hasCount; j++) {
                if (classroom->has[j]) {
                    totalLen += strlen(classroom->has[j]) + 2;
                }
            }
            features = malloc(totalLen + 1);
            features[0] = '\0';
            for (int j = 0; j < classroom->hasCount; j++) {
                if (classroom->has[j]) {
                    if (j > 0) strcat(features, ", ");
                    strcat(features, classroom->has[j]);
                }
            }
        }

        fprintf(filePointer, "<tr>\n");
        fprintf(filePointer, "<td>%s</td>\n", cleanCourseName);
        fprintf(filePointer, "<td>%s</td>\n", cleanProfName);
        fprintf(filePointer, "<td>%s</td>\n", cleanClassroomName);
        fprintf(filePointer, "<td>%s</td>\n", cleanBuilding);
        fprintf(filePointer, "<td>%s</td>\n", features);
        fprintf(filePointer, "<td>%s</td>\n", dayString);
        fprintf(filePointer, "<td>%02d:%02d - %02d:%02d</td>\n", entry->startTime.hour, entry->startTime.minute,
                entry->endTime.hour, entry->endTime.minute);
        fprintf(filePointer, "</tr>\n");

        free(cleanCourseName);
        free(cleanProfName);
        free(cleanClassroomName);
        free(cleanBuilding);
        free(features);
    }

    for (int i = 0; i < usedClassroomCount; i++) {
        free(usedClassrooms[i]);
    }
    free(usedClassrooms);

    fprintf(filePointer, "</table>\n");
    fprintf(filePointer, "<script>\n");
    fprintf(filePointer, "// Filtro de búsqueda\n");
    fprintf(filePointer, "document.getElementById('searchInput').addEventListener('input', function() {\n");
    fprintf(filePointer, "    let filter = this.value.toLowerCase();\n");
    fprintf(filePointer, "    let table = document.getElementById('scheduleTable');\n");
    fprintf(filePointer, "    let rows = table.getElementsByTagName('tr');\n");
    fprintf(filePointer, "    for (let i = 1; i < rows.length; i++) { // Skip header row\n");
    fprintf(filePointer, "        let course = rows[i].getElementsByTagName('td')[0].textContent.toLowerCase();\n");
    fprintf(filePointer, "        let professor = rows[i].getElementsByTagName('td')[1].textContent.toLowerCase();\n");
    fprintf(filePointer, "        let classroom = rows[i].getElementsByTagName('td')[2].textContent.toLowerCase();\n");
    fprintf(filePointer, "        if (course.includes(filter) || professor.includes(filter) || classroom.includes(filter)) {\n");
    fprintf(filePointer, "            rows[i].style.display = '';\n");
    fprintf(filePointer, "        } else {\n");
    fprintf(filePointer, "            rows[i].style.display = 'none';\n");
    fprintf(filePointer, "        }\n");
    fprintf(filePointer, "    }\n");
    fprintf(filePointer, "});\n");
    fprintf(filePointer, "// Ordenamiento por día\n");
    fprintf(filePointer, "let sortAscending = true;\n");
    fprintf(filePointer, "document.getElementById('dayHeader').addEventListener('click', function() {\n");
    fprintf(filePointer, "    let table = document.getElementById('scheduleTable');\n");
    fprintf(filePointer, "    let tbody = table.getElementsByTagName('tbody')[0] || table;\n");
    fprintf(filePointer, "    let rows = Array.from(table.getElementsByTagName('tr')).slice(1); // Skip header\n");
    fprintf(filePointer, "    const dayOrder = { 'Monday': 1, 'Tuesday': 2, 'Wednesday': 3, 'Thursday': 4, 'Friday': 5, 'Everyday': 6, 'Unknown': 7 };\n");
    fprintf(filePointer, "    rows.sort((a, b) => {\n");
    fprintf(filePointer, "        let dayA = a.getElementsByTagName('td')[5].textContent;\n");
    fprintf(filePointer, "        let dayB = b.getElementsByTagName('td')[5].textContent;\n");
    fprintf(filePointer, "        let orderA = dayOrder[dayA] || 7;\n");
    fprintf(filePointer, "        let orderB = dayOrder[dayB] || 7;\n");
    fprintf(filePointer, "        return sortAscending ? orderA - orderB : orderB - orderA;\n");
    fprintf(filePointer, "    });\n");
    fprintf(filePointer, "    rows.forEach(row => tbody.appendChild(row));\n");
    fprintf(filePointer, "    sortAscending = !sortAscending;\n");
    fprintf(filePointer, "    document.getElementById('dayArrow').textContent = sortAscending ? '↑' : '↓';\n");
    fprintf(filePointer, "});\n");
    fprintf(filePointer, "</script>\n");
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

    logDebugging(logger, "Generation completed!");
    compilerState->succeed = true;
}