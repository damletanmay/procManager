#include <ftw.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

// this code can be found in /home/damlet/Desktop/asp_assignment/assignment_2/prc24s_tanmay_damle_SID.c
// there's a compile script written use bash compile_a1 and bash tests_a2 to compile and test the program
// executable prc24s is already put in the $PATH so that it can be run from anywhere

//  global constants
#define MIN_ARGS 3
#define MAX_ARGS 4
#define TOTAL_OPTIONS 14
#define FTW_DEPTH 8
#define FTW_PHYS 1

// for some reason FTW struct in ftw.h is not being imported so making this same struct from ftw.h
struct FTW
{
    int base;
    int level;
};

// options array index will refer to the options in code below wherever selected_option is referenced
const char *options[] = {"-dx", "-dt", "-dc", "-rp", "-nd", "-dd", "-sb", "-bz", "-zd", "-od", "-gc", "-sz", "-so", "-kz"};
bool is_option = false;
int selected_option = -1;

// proc directory path
const char *proc_path = "/proc/";
char *error = "Error";

// will hold root process and pid process
// will be initialized when no_option() function is called
struct process *root_process, *process_id;

// enum for status of a process, will be used in processes structure
enum state
{
    Sleeping,
    Zombie,
    Running,
    Idle,
    Orphan,
    Stopped,
    DiskSleep,
    Unknown
};

// sturcture for storing all processes in /proc
// null_process will hold an error state which will be returned every time a search fails..
struct process
{
    int pid;
    int ppid;
    int uid;
    int no_of_children;
    char *name;
    enum state state_;
    struct process *children[1000]; // a process can have max 1000 children
    struct process *parent;         // only one parent per process
} *all_processes, *null_process;

// this function will if a process is null process or not, returns -1 if null else 1
int check_if_null(struct process *process)
{
    if (strcmp(process->name, error) == 0)
    {
        return -1;
    }
    return 1;
}

// this function will parse data from file and then will "return" in the form of assigning pointer variables' value
// it'll return pid, ppid, uid & state of a process by reference
// it'll return process_name by reference as a return value

char *parse_data_from_file(const char *file_path, int *pid, int *ppid, int *uid, char *state_)
{
    // open file and read data
    int fd = open(file_path, O_RDONLY);
    char *process_name;

    // file open fail = process has been killed since the starting of this program
    if (fd < 0)
    {
        printf("Error for file %s\n", file_path);
        return NULL;
    }

    // read file
    char *buf = malloc(sizeof(char) * 10000);
    int bytes_read = read(fd, buf, 10000);

    // get first 9 new line indexes
    int *new_line_indexes = malloc(sizeof(int) * 9);

    // find first 9 \n characters
    for (int i = 0, j = 0; i < bytes_read, j < 9; i++)
    {
        if (buf[i] == '\n')
        {
            *(new_line_indexes + j) = i;
            j++;
        }
    }

    // traverse line by line
    for (int i = 0, j = 0; i < 9; i++)
    {
        // get the line
        char *substring = malloc(sizeof(char) * (new_line_indexes[i] - j + 1));
        strncpy(substring, buf + j, new_line_indexes[i] - j);
        j = new_line_indexes[i] + 1; // j will skip till next line's first character

        // get value of all attributes
        int skip_index = strcspn(substring, ":");
        char *sub_sub_string = malloc(sizeof(char) * 20);
        strncpy(sub_sub_string, substring + skip_index + 2, 20);
        sub_sub_string[20] = '\0';

        // fix to read zombie processes
        if (sub_sub_string[0] != 'Z' && i == 1)
        {
            continue;
        }

        switch (i)
        {
        case 0:
            // name
            process_name = malloc(sizeof(char) * strlen(sub_sub_string));
            strcpy(process_name, sub_sub_string);
            break;

        case 1:
            // fix to read zombie processes
            // State
            state_[0] = sub_sub_string[0];
            state_[1] = '\0';
            i = 3; // skip over index 2,3
            break;
        case 2:
            // State
            state_[0] = sub_sub_string[0];
            state_[1] = '\0';
            break;
        case 4:
            // pid for zombies
            if (state_[0] == 'Z')
            {
                *pid = atoi(sub_sub_string);
                i = 5;
            }
            break;
        case 5:
            // pid
            *pid = atoi(sub_sub_string);
            break;
        case 6:
            // ppid
            *ppid = atoi(sub_sub_string);
            break;
        case 8:
            // uid
            *uid = atoi(sub_sub_string);
            break;
        default:
            break;
        }
        // free(sub_sub_string);
        // free(substring);
    }
    // free(new_line_indexes);
    // free(buf);
    close(fd);
    return process_name;
}

// this function will get enum state from character value of state
void get_state(int ppid, char *state, enum state *state_)
{
    // if ppid = 1, orphan process
    if (ppid == 1)
    {
        *state_ = Orphan;
    }
    else
    {
        // if ppid not 1 then accroding to the first character in sub_sub_string
        switch (state[0])
        {
        case 'R':
            *state_ = Running;
            break;
        case 'I':
            *state_ = Idle;
            break;
        case 'S':
            *state_ = Sleeping;
            break;
        case 'Z':
            *state_ = Zombie;
            break;
        case 'T':
            *state_ = Stopped;
            break;
        case 'D':
            *state_ = DiskSleep;
            break;
        default:
            *state_ = Unknown;
            break;
        }
    }
}

// search process will return a process for a given pid
// search process implements dfs as a lot of processes will have ids 1 & 2
// bash is a child of 1 most of the times so while searching for parents we do not want to traverse into unwanted processes
struct process *search_process(struct process *current_process, int pid)
{
    int current_child = 0;

    // if process id is found, return
    if (pid == current_process->pid)
    {
        return current_process;
    }

    // if a process has no more descendants, return NULL
    if (current_child == current_process->no_of_children)
    {
        return null_process;
    }
    else
    {
        // search children
        while (current_child < current_process->no_of_children)
        {
            struct process *temp1 = search_process(current_process->children[current_child], pid);

            if (check_if_null(temp1) != -1)
            {
                return temp1;
            }
            current_child += 1;
        }
    }
    // if no processes are found after traversing all children return null process
    return null_process;
}

// insert a process
void insert_process(int pid, int ppid, int uid, char *name, enum state state_)
{
    // make a temp_process var
    struct process *temp_process = (struct process *)malloc(sizeof(struct process)); // to hold current process, which is being inserted
    temp_process->pid = pid;
    temp_process->ppid = ppid;
    temp_process->uid = uid;
    temp_process->no_of_children = 0;
    temp_process->name = malloc(sizeof(char) * (strlen(name)));
    strcpy(temp_process->name, name);
    temp_process->state_ = state_;

    // for the 0th custom process which will be the first process
    if (pid == 0)
    {
        all_processes = temp_process;
        all_processes->parent = null_process;
    }
    else
    {
        // for all the other processes find the parent,
        // increase it's child count and make the parent's child point to temp_process
        // make child's parent point to parent
        struct process *temp_process2; // to hold parent process

        temp_process2 = search_process(all_processes, ppid); // get the parent process

        // if null process is returned then point first children to null_process
        // this will only happen if a parent dies during making of all_processes struct pointer
        if (check_if_null(temp_process2) == -1)
        {
            // gets adopted by init
            temp_process->state_ = Orphan;
            temp_process->parent = all_processes->children[0];
        }
        else
        {
            temp_process2->children[temp_process2->no_of_children] = temp_process; // assign nth child to parent
            temp_process2->no_of_children += 1;                                    // increment no of children
            temp_process->parent = temp_process2;                                  // assign parent to temp_process
        }
    }
}

// callback function for nftw to traverse all files in proc_path
int callback_function(const char *file_path, const struct stat *file_info, int flag, struct FTW *ftw_info)
{
    // entity name is the file path
    const char *entity_name = file_path + ftw_info->base;

    // get folder name, every folder with numeric name in here is a process
    int pid = atoi(entity_name);

    // iterate over each directory, leave files in root and each directory
    if (flag == FTW_D && ftw_info->level == 1 && pid > 0)
    {
        // read the status file in each directory
        // make path for status file
        char *st = "/status";
        char *status_file_path = malloc(sizeof(char) * (strlen(st) + strlen(file_path)));
        strcpy(status_file_path, file_path);
        strcat(status_file_path, st);

        // get pid, ppid, uid, process name & status from /proc/process_id/status file
        int pid_;
        int ppid;
        int uid;
        char *process_name;
        char *state = malloc(sizeof(char) * 2);
        enum state state_;

        process_name = parse_data_from_file(status_file_path, &pid_, &ppid, &uid, state); // parse data from file and get data in variables
        get_state(ppid, state, &state_);                                                  // get state enum
        insert_process(pid_, ppid, uid, process_name, state_);                            // insert a process
    }
    return 0;
}

// return process_id as int
int return_process_id(char *process_id)
{
    int pid = atoi(process_id);
    if (pid == 0)
    {
        printf("%s is not numerical, Please Try Again!\n", process_id);
        exit(-1);
    }

    // if pid <0 then the process id is either negative or too long to store it in int so exit
    if (pid < 0)
    {
        printf("Process ID:%s is either negative or too long, Please Try Again!\n", process_id);
        exit(-1);
    }

    return pid;
}

// calls the nftw - file tree walk to get data from all current running processes and store in all_processes struct
// also initalizes null process which will act as an error state
void initialize_all_processes()
{
    // initalizing null process which will act as NULL state
    null_process = (struct process *)malloc(sizeof(struct process));
    null_process->name = malloc(sizeof(char) * 10);
    null_process->no_of_children = 0;
    strcpy(null_process->name, error);

    // // before getting all processes inserted into tree, making a 0th process as two processes 1 & 2 are born from 0th process
    insert_process(0, 0, 0, "0th Process", Running);

    nftw(proc_path, callback_function, getdtablesize(), FTW_PHYS); // recursively call the callback function
}

// this function will get invoked when user enters no option while running this program
// also will get invoked for every option to check if process_id is rooted at given root's process id
void no_option(char *root_, char *pid_, bool to_print)
{
    // if no option is provided by user
    // print pid,ppid of process id
    int root = return_process_id(root_);
    int pid = return_process_id(pid_);

    initialize_all_processes(); // initalize all processes & null process

    // find root and then if root does not exist show error
    root_process = search_process(all_processes, root);
    if (check_if_null(root_process) == -1)
    {
        printf("Root Process with PID: %d does not exist\n", root);
        exit(-1);
    }

    // if root is found, find process with pid passed by user in root_process' children
    process_id = search_process(root_process, pid);
    if (check_if_null(process_id) == -1 || pid == root)
    {

        printf("The process %d does not belong to the tree rooted at %d \n", pid, root);
        exit(-1);
    }
    // to_print boolean to determine if to print or not
    // it does not print if user enters valid option
    if (to_print)
    {
        printf("PID: %d, PPID:%d \n", process_id->pid, process_id->ppid);
    }
}

// this function will find selected option and verify that it exists
void find_selected_option(char *argv[])
{
    // lowercasing option part of argument
    for (int i = 0; i < strlen(argv[1]); i++)
    {
        argv[1][i] = tolower(argv[1][i]);
    }

    // find selected option
    for (int i = 0; i < TOTAL_OPTIONS; i++)
    {
        if (strcmp(options[i], argv[1]) == 0)
        {
            selected_option = i;
        }
    }

    // if selected option does not exist then just show all possible options
    if (selected_option == -1)
    {
        printf("No Such Option: %s\n", argv[1]);
        printf("Please select a option from below:\n");

        for (int i = 0; i < TOTAL_OPTIONS; i++)
        {
            printf("%s\n", options[i]);
        }

        exit(-1);
    }
}

// sends signals to all descendants of a process
// implements dfs similar to search_process() operation
void send_signal_to_descendants(struct process *current_process, int signal)
{
    int current_child = 0;

    // if a process has no more descendants, just kill the process
    // it won't kill root process as it'll have children

    if (current_process->no_of_children == 0 && current_process->pid != root_process->pid)
    {
        int kill_success;
        // send SIGCONT to only zombie processes
        if (signal == SIGCONT)
        {
            if (current_process->state_ == Stopped)
            {
                kill_success = kill(current_process->pid, signal);
            }
        }
        else
        {
            kill_success = kill(current_process->pid, signal);
        }
        if (kill_success == -1)
        {
            printf("You don't have permission to send signals to process with pid %d\n", current_process->pid);
        }
    }
    else
    {
        // kill children
        while (current_child < current_process->no_of_children)
        {
            send_signal_to_descendants(current_process->children[current_child], signal); // kill children
            current_child += 1;
        }

        // kill parent if not root
        if (current_process->pid != root_process->pid)
        {
            current_process->no_of_children = 0; // make parent children zero
            send_signal_to_descendants(current_process, signal);
        }
    }
}

// prints non direct descendants
// implements dfs similar to search_process() operation
void print_non_direct_descendants(struct process *current_process, int *nd_counter)
{
    int current_child = 0;

    // if a process has no more descendants and is not a child of pid
    if (current_process->no_of_children == 0 && current_process->parent->pid != process_id->pid && current_process->pid != process_id->pid)
    {
        *nd_counter += 1;
        printf("%d\n", current_process->pid);
    }
    else
    {
        // print parent if not direct child of process_id
        if (current_process->parent->pid != process_id->pid && current_process->pid != process_id->pid)
        {
            *nd_counter += 1;
            printf("%d\n", current_process->pid); // print parent id
        }

        // iterate over children & print their ids
        while (current_child < current_process->no_of_children)
        {
            print_non_direct_descendants(current_process->children[current_child], nd_counter); // print children
            current_child += 1;
        }
    }
}

// prints immediate descendants
void print_immediate_descendants(struct process *current_process, int *id_counter)
{
    int current_child = 0;

    // iterate over children
    while (current_child < current_process->no_of_children)
    {
        // print children's pid
        printf("%d\n", current_process->children[current_child]->pid);
        *id_counter += 1;
        current_child += 1;
    }
}

// prints siblings pid, boolean determines if to print only zombiefied siblings or all
// implements dfs similar to search_process() operation
void sibling_pid_print(struct process *current_process, int *sb_counter, bool zombies_only)
{
    struct process *parent_process = current_process->parent;
    int current_child = 0;

    // iterate over all children of parent
    while (current_child < parent_process->no_of_children)
    {
        // don't print current process's id
        if (parent_process->children[current_child]->pid != current_process->pid)
        {
            // if zombies only needed to be printed
            if (zombies_only)
            {
                if (parent_process->children[current_child]->state_ == Zombie)
                {
                    printf("%d\n", parent_process->children[current_child]->pid);
                    *sb_counter += 1;
                }
            }
            else
            {
                printf("%d\n", parent_process->children[current_child]->pid);
                *sb_counter += 1;
            }
        }
        current_child++;
    }
}

// prints pid, boolean determines if to print only zombiefied siblings or orphan siblings
// if is_zombies parameter is false, it'll print all orphan processes descendents of current_process
// if is_zombies parameter is true, it'll print all zombie process descendents of current_process
// implements dfs similar to search_process() operation
void print_all_descendents(struct process *current_process, int *counter, bool is_zombies)
{
    int current_child = 0;
    // if a process has no more descendants and is not a child of pid
    if (current_process->pid != process_id->pid)
    {
        if (is_zombies)
        {
            // if zombies needed to be printed
            if (current_process->state_ == Zombie)
            {
                printf("%d\n", current_process->pid);
                *counter += 1;
            }
        }
        else if (current_process->ppid == 1)
        {
            // if orphans needed to be printed
            // if current process is adopted by init i.e. 1, print
            printf("%d\n", current_process->pid);
            *counter += 1;
        }
    }

    // iterate over children & print their ids
    while (current_child < current_process->no_of_children)
    {
        print_all_descendents(current_process->children[current_child], counter, is_zombies); // print children
        current_child += 1;
    }
}

// goes 1 level down and prints grand children of given process_id
void print_grand_children(struct process *current_process, int *gc_counter)
{
    int current_child = 0;

    // print only if grand children, will be only true when depth = 2
    if (current_process->parent->parent->pid == process_id->pid)
    {
        printf("%d\n", current_process->pid);
        *gc_counter += 1;
    }
    else if (current_process->pid == process_id->pid)
    {
        // iterate over children & print their ids
        while (current_child < current_process->no_of_children)
        {
            int current_child_parent = 0;
            struct process *parent = current_process->children[current_child];
            while (current_child_parent < parent->no_of_children)
            {
                print_grand_children(parent->children[current_child_parent], gc_counter); // print grand children
                current_child_parent += 1;
            }

            current_child += 1;
        }
    }
}

// kills parents of all zombie processes including process_id if required
// implements dfs similar to search_process() operation
void kill_zombie_parents(struct process *current_process, int *kz_counter)
{
    int current_child = 0;
    // if not process id iter over children to find zombies and kill parents
    if (current_process->pid != process_id->pid && current_process->state_ == Zombie)
    {
        int kill_success = kill(current_process->parent->pid, SIGKILL);
        if (kill_success == -1)
        {
            printf("You don't have permission to send signals to process with pid %d\n", current_process->parent->pid);
        }
        else
        {
            *kz_counter += 1;
        }
    }
    else
    {
        // boolean to determine if to kill the process who called this function to kill or not
        bool zombie_children = false;
        // iterate over children & print their ids
        while (current_child < current_process->no_of_children)
        {
            if (current_process->children[current_child]->state_ == Zombie)
            {
                zombie_children = true;
            }
            kill_zombie_parents(current_process->children[current_child], kz_counter); // go in depth to find zombies
            current_child += 1;
        }
        if (zombie_children)
        {
            int kill_success = kill(current_process->pid, SIGKILL);
            if (kill_success == -1)
            {
                printf("You don't have permission to send signals to process with pid %d\n", current_process->parent->pid);
            }
            else
            {
                *kz_counter += 1;
            }
        }
    }
}

// this function will perform all the operations according to selected_option
void perform_operation()
{
    int counter = 0;
    int kill_success;
    switch (selected_option)
    {
    case 0:
        // for -dx option
        // send SIGKILL to root process's descendants
        send_signal_to_descendants(root_process, SIGKILL);
        break;

    case 1:
        // for -dt option
        // send SIGSTOP to root process's descendants
        send_signal_to_descendants(root_process, SIGSTOP);
        break;

    case 2:
        // for -dc option
        // send SIGCONT to root process's descendants
        send_signal_to_descendants(root_process, SIGCONT);
        break;

    case 3:
        // for -rp option
        // Root Process kills Pid Process

        kill_success = kill(process_id->pid, SIGKILL);
        if (kill_success == -1)
        {
            printf("You don't have permission to send signals to process with pid %d\n", process_id->pid);
        }
        break;

    case 4:
        // for -nd option
        // prints pid of all non direct descendants of p
        counter = 0;
        print_non_direct_descendants(process_id, &counter);
        if (counter == 0)
        {
            printf("No Non-Direct Descendants\n");
        }
        break;

    case 5:
        // for -dd option
        // prints pid of all immediate descendants of p
        counter = 0;

        print_immediate_descendants(process_id, &counter);
        if (counter == 0)
        {
            printf("No Direct Descendants\n");
        }
        break;

    case 6:
        // for -sb option
        // prints siblings process id
        counter = 0;
        sibling_pid_print(process_id, &counter, false);
        if (counter == 0)
        {
            printf("No Siblings\n");
        }
        break;

    case 7:
        // for -bz option
        // prints siblings process id only if they are zombies
        counter = 0;
        sibling_pid_print(process_id, &counter, true);
        if (counter == 0)
        {
            printf("No Defunct Siblings\n");
        }
        break;

    case 8:
        // for -zd option
        // prints all zombie desendents of process_id
        counter = 0;
        print_all_descendents(process_id, &counter, true);
        if (counter == 0)
        {
            printf("No Descendant Zombie Process\n");
        }
        break;

    case 9:
        // for -od option
        // prints all orphan desendents of process_id
        counter = 0;
        print_all_descendents(root_process, &counter, false);
        if (counter == 0)
        {
            printf("No Descendant Orphan Process\n");
        }
        break;

    case 10:
        // for -gc option
        // prints all grand children process_id
        counter = 0;
        print_grand_children(process_id, &counter);
        if (counter == 0)
        {
            printf("No Grandchildren\n");
        }
        break;

    case 11:
        // for -sz option
        // print status of process id Defunct / Not Defunct
        if (process_id->state_ == Zombie)
        {
            printf("Defunct\n");
        }
        else
        {
            printf("Not Defunct\n");
        }
        break;

    case 12:
        // for -so option
        // print status of process id Orphan / Not Orphan
        if (process_id->state_ == Orphan)
        {
            printf("Orphan\n");
        }
        else
        {
            printf("Not Orphan\n");
        }
        break;

    case 13:
        // for -kz option
        // kills parents of all zombie children of given process_id including process_id if it's children are zombies
        counter = 0;
        kill_zombie_parents(process_id, &counter);
        if (counter == 0)
        {
            printf("No Defunct Child Processes under process_id %d\n", process_id->pid);
        }
        break;

    default:
        exit(-1);
        break;
    }
}

// driver function
int main(int argc, char *argv[])
{
    printf("\n"); // easier display for the test cases TODO: remove later

    // if only 1 argument i.e. user only entered prc24s
    // show documentation
    if (argc == 1)
    {
        char *buffer = malloc(sizeof(char) * 2500);

        int man_fd = open("procManager_man_page.txt", O_RDONLY);

        if (man_fd == -1)
            exit(-1);

        read(man_fd, buffer, 2500);

        printf("%s\n", buffer);

        close(man_fd);
        // free(buffer);
        exit(0);
    }

    if (argc < MIN_ARGS)
    {
        printf("You Need to pass More Arguments!\n\n");
        exit(-1);
    }
    else if (argc > MAX_ARGS)
    {
        printf("You Need to pass Less Arguments!\n\n");
        exit(-1);
    }
    else
    {
        if (argc == 3)
        {
            no_option(argv[1], argv[2], true);
        }
        else
        {
            no_option(argv[2], argv[3], false); // will verify that process_id is rooted at root_process
            find_selected_option(argv);         // to find which option is selected by user and store it in selected_option
            perform_operation();                // perform operations according to selected_option
        }

        // free pointers
        // free(all_processes);
        // free(null_process);
    }
}