#include <jobs.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void init_jobs() {
    for (int i = 0; i < MAX_JOBS; ++i) {
        job_table[i].id = 0;
        job_table[i].pgid = 0;
        job_table[i].state = JOB_DONE;
        job_table[i].command = NULL;
    }
    next_job_id = 1;
    current_job = NULL;
    previous_job = NULL;
}

int add_job(pid_t pgid, char *command) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (job_table[i].id == 0) {
            job_table[i].id = next_job_id++;
            job_table[i].pgid = pgid;
            job_table[i].state = JOB_RUNNIG;
            if (command != NULL) {
                job_table[i].command = strdup(command);
            } else {
                job_table[i].command = "";
            }
            return job_table[i].id;
        }
    }
    return -1;
}

void remove_job_by_id(int id) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (job_table[i].id == id) {
            if (job_table[i].command != NULL) {
                free(job_table[i].command);
                job_table[i].command = NULL;
            }
            job_table[i].id = 0;
            job_table[i].pgid = 0;
            job_table[i].state = JOB_DONE;
            break;
        }
    }
    update_job_pointers();
}

void update_job_pointers(void) {
    Job *new_cur = NULL;
    Job *new_prev = NULL;
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (job_table[i].id != 0 && job_table[i].state != JOB_DONE) {
            if (new_cur == NULL) {
                new_cur = &job_table[i];
            } else if (new_prev == NULL) {
                new_prev = &job_table[i];
                break;
            }
        }
    }
    current_job = new_cur;
    previous_job = new_prev;
}

Job *find_job_by_id(int id) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (job_table[i].id == id) {
            return &job_table[i];
        }
    }
    return NULL;
}

Job *find_job_by_spec(char *spec) {
    if (spec == NULL) {
        return get_cur_job();
    }
    if (spec[0] == '%') {
        ++spec;
    }
    if (*spec == '+' || *spec == '\0') {
        return get_cur_job();
    } else if (*spec == '-') {
        return get_prev_job();
    }
    int id = atoi(spec);
    return find_job_by_id(id);
}

Job *get_cur_job(void) {
    if (current_job == NULL || current_job->state == JOB_DONE) {
        update_job_pointers();
    }
    return current_job;
}

Job *get_prev_job(void) {
    if (previous_job == NULL || previous_job->state == JOB_DONE) {
        update_job_pointers();
    }
    return previous_job;
}

Job *get_job_by_ind(int index) {
    if (index < 0 || index > MAX_JOBS
        || job_table[index].id == 0 || job_table[index].state == JOB_DONE) {
        return NULL;
    }
    return &job_table[index];
}

void update_jobs(void) {
    int status;
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (job_table[i].id != 0 && job_table[i].state != JOB_DONE) {
            pid_t pid = waitpid(-job_table[i].pgid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            if (pid > 0) {
                if (WIFSTOPPED(status)) {
                    job_table[i].state = JOB_STOPPED;
                } else if (WIFCONTINUED(status)) {
                    job_table[i].state = JOB_RUNNIG;
                } else {
                    job_table[i].state = JOB_DONE;
                    remove_job_by_id(job_table[i].id);
                }
            }
        }
    }
    update_job_pointers();
}

int has_active_jobs(void) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (job_table[i].id != 0 && job_table[i].state != JOB_DONE) {
            return 1;
        }
    }
    return 0;
}