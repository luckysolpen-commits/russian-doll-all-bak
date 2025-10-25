
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for usleep
#include <pthread.h>  // for threading support
#include <sys/stat.h>  // for file operations
#include <string.h>  // for string operations

#define MAX_MODULES 10

// Forward declarations for functions in other files
void init_controller();
void init_view(const char* filename);
void init_model(const char* module_path);
int update_model();
void* ipc_thread_func(void* arg);  // New thread function for IPC
void get_window_properties(int* x, int* y, int* width, int* height, char* title, int title_size);

void display();
void reshape(int, int);
void mouse(int, int, int, int);
void keyboard(unsigned char, int, int);
void special_keys(int, int, int);
void mouse_motion(int, int);

char* module_executable = NULL;

// Forward declaration for controller function to update UI with model variables
void update_ui_with_model_variables();

// Forward declaration for getting modules from view
typedef struct {
    char path[512];
    int active;
} ModulePath;

ModulePath* get_modules(int* count);

// Function declarations for features
void check_navigation_requests();
void reload_view_with_file(const char* filename);
void init_multiple_modules(const char** module_paths, int count);

// Additional globals for the navigation system
char current_chtml_file[512];
int reload_requested = 0;
char next_chtml_file[512];

// Global variables for thread management
pthread_t ipc_thread;
int ipc_thread_running = 0;
int model_updated = 0;  // Flag to indicate if model was updated

void idle_func() {
    int updated = update_model();
    
    if (updated) {
        update_ui_with_model_variables();  // Update UI with new model data
        glutPostRedisplay();
    }
    
    // Check for navigation requests
    check_navigation_requests();
    
    // Limit to ~60 FPS by sleeping about 16ms between cycles
    usleep(16000); // Sleep for approximately 16 milliseconds
}

// Forward declarations for functions in view.c that need to be accessible
extern void cleanup_view(); // We'll implement this to reset the view state

// Function to reload the view with a new CHTML file
void reload_view_with_file(const char* filename) {
    printf("Reloading view with file: %s\n", filename);
    
    // Cleanup current view state
    cleanup_view();
    
    // Initialize MVC components - init_view first to parse modules
    init_view(filename);
    
    // Get modules from the parsed CHTML file
    int num_modules = 0;
    ModulePath* modules = get_modules(&num_modules);
    
    if (num_modules > 0) {
        printf("Found %d module(s) in CHTML file:\n", num_modules);
        
        // Create array of module paths
        const char* module_paths[MAX_MODULES];
        for (int i = 0; i < num_modules && i < MAX_MODULES; i++) {
            module_paths[i] = modules[i].path;
            printf("  Module %d: %s\n", i+1, modules[i].path);
        }
        
        // Initialize model with multiple modules
        init_multiple_modules(module_paths, num_modules);
    } else {
        // Initialize model without module if none specified in CHTML
        init_model(NULL);
    }
    
    init_controller();
    
    // Redraw the scene
    glutPostRedisplay();
}

// Function to check for navigation requests
void check_navigation_requests() {
    if (reload_requested) {
        reload_requested = 0;
        printf("Processing navigation request to: %s\n", next_chtml_file);
        reload_view_with_file(next_chtml_file);
        
        // Update current file path to reflect new page
        strncpy(current_chtml_file, next_chtml_file, sizeof(current_chtml_file) - 1);
        current_chtml_file[sizeof(current_chtml_file) - 1] = '\0';
    }
    
    // Check if there's a navigation request file
    FILE* nav_file = fopen("nav_request.tmp", "r");
    if (nav_file) {
        char new_file[512];
        if (fgets(new_file, sizeof(new_file), nav_file)) {
            // Remove newline if present
            new_file[strcspn(new_file, "\n")] = 0;
            printf("Navigation requested to: %s\n", new_file);
            
            // Verify the file exists
            struct stat buffer;
            if (stat(new_file, &buffer) == 0) {
                strncpy(next_chtml_file, new_file, sizeof(next_chtml_file) - 1);
                next_chtml_file[sizeof(next_chtml_file) - 1] = '\0';
                reload_requested = 1;
            } else {
                printf("Navigation target file does not exist: %s\n", new_file);
            }
        }
        fclose(nav_file);
        // Remove the navigation request file
        remove("nav_request.tmp");
    }
}

// Function called at application exit to clean up threads
void cleanup_ipc_thread() {
    if (ipc_thread_running) {
        ipc_thread_running = 0;
        void* status;
        pthread_join(ipc_thread, &status);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <markup_file.chtml>\n", argv[0]);
        exit(1);
    }
    
    strncpy(current_chtml_file, argv[1], sizeof(current_chtml_file) - 1);
    current_chtml_file[sizeof(current_chtml_file) - 1] = '\0';
    
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    
    // Initialize MVC components - init_view first to parse modules
    init_view(current_chtml_file); // Parse the CHTML file provided as argument
    
    // Get window properties from the parsed CHTML file
    int window_x, window_y, window_width, window_height;
    char window_title[256];
    get_window_properties(&window_x, &window_y, &window_width, &window_height, window_title, sizeof(window_title));
    
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition(window_x, window_y);
    glutCreateWindow(window_title);
    
    // Get modules from the parsed CHTML file
    int num_modules = 0;
    ModulePath* modules = get_modules(&num_modules);
    
    if (num_modules > 0) {
        printf("Found %d module(s) in CHTML file:\n", num_modules);
        
        // Create array of module paths
        const char* module_paths[MAX_MODULES];
        for (int i = 0; i < num_modules && i < MAX_MODULES; i++) {
            module_paths[i] = modules[i].path;
            printf("  Module %d: %s\n", i+1, modules[i].path);
        }
        
        // Initialize model with multiple modules
        init_multiple_modules(module_paths, num_modules);
    } else {
        // Initialize model without module if none specified in CHTML
        init_model(NULL);
    }
    
    init_controller();

    // Register GLUT callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special_keys);
    glutMotionFunc(mouse_motion);

    // Set the idle function for optimized rendering
    glutIdleFunc(idle_func);

    // Register cleanup function for when the window is closed
    atexit(cleanup_ipc_thread);

    glutMainLoop();

    return 0;
}
