#pragma once

#include <unistd.h>
#include <stdlib.h>

#define MAX_JOBS 64

typedef enum {
    JOB_RUNNIG,
    JOB_STOPPED,
    JOB_DONE
} JobState;

typedef struct {
    int id;
    pid_t pgid;
    JobState state;
    char *command
} Job;

static Job job_table[MAX_JOBS];
static int next_job_id = 1;
static Job *current_job = NULL;
static Job *previous_job = NULL;

void init_jobs(void);
int add_job(pid_t pgid, char *command);
void remove_job_by_id(int id);
Job *find_job_by_id(int id);
Job *find_job_by_spec(char *spec);
Job *get_cur_job(void);
Job *get_prev_job(void);
Job *get_job_by_ind(int index);
void update_jobs(void);
int has_active_jobs(void);
void update_job_pointers(void);
