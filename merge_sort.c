#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <sys/time.h> // Include for gettimeofday

#define ARRAY_SIZE 80000000 
#define NUM_THREADS 6

typedef struct {
    double real;
    double imag;
} Complex;

struct thread_data {
    Complex *arr;
    int left;
    int right;
};

Complex generate_complex() {
    Complex c;
    c.real = (double)rand() / RAND_MAX * 200 - 100;
    c.imag = (double)rand() / RAND_MAX * 200 - 100;
    return c;
}

double complex_magnitude(Complex c) {
    return sqrt(c.real * c.real + c.imag * c.imag);
}

void merge(Complex arr[], int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    Complex *L = (Complex *)malloc(n1 * sizeof(Complex));
    Complex *R = (Complex *)malloc(n2 * sizeof(Complex));

    if (L == NULL || R == NULL) {
        fprintf(stderr, "Memory allocation failed in merge\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];

    i = 0;
    j = 0;
    k = left;

    while (i < n1 && j < n2) {
        if (complex_magnitude(L[i]) <= complex_magnitude(R[j])) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);
}

void mergeSort(Complex arr[], int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        mergeSort(arr, left, mid);
        mergeSort(arr, mid + 1, right);
        merge(arr, left, mid, right);
    }
}

void singleThread(Complex arr[], int size) {
    struct timeval start, end;
    gettimeofday(&start, NULL);
    mergeSort(arr, 0, size - 1);
    gettimeofday(&end, NULL);
    double cpu_time_used = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Single thread merge sort time: %f seconds\n", cpu_time_used);
}

void *multiThread(void *arg) {
    struct thread_data *data = (struct thread_data *)arg;
    mergeSort(data->arr, data->left, data->right);
    pthread_exit(NULL);
}

void mergeSortedArrays(Complex arr[], int left, int mid, int right) {
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    Complex *L = (Complex *)malloc(n1 * sizeof(Complex));
    Complex *R = (Complex *)malloc(n2 * sizeof(Complex));

    if (L == NULL || R == NULL) {
        fprintf(stderr, "Memory allocation failed in mergeSortedArrays\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];

    i = 0;
    j = 0;
    k = left;

    while (i < n1 && j < n2) {
        if (complex_magnitude(L[i]) <= complex_magnitude(R[j])) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);
}

int main() {
    Complex *arr = (Complex *)malloc(ARRAY_SIZE * sizeof(Complex));
    if (arr == NULL) {
        fprintf(stderr, "Memory allocation failed for arr\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        arr[i] = generate_complex();
    }

    Complex *arr_copy = (Complex *)malloc(ARRAY_SIZE * sizeof(Complex));
    if (arr_copy == NULL) {
        fprintf(stderr, "Memory allocation failed for arr_copy\n");
        free(arr);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < ARRAY_SIZE; i++) {
        arr_copy[i] = arr[i];
    }

    printf("Debug: Starting multi-threaded sort\n");
    struct timeval start, end;

    // Multi-threaded sort
    pthread_t threads[NUM_THREADS];
    struct thread_data thread_data_array[NUM_THREADS];
    int chunk_size = ARRAY_SIZE / NUM_THREADS;

    gettimeofday(&start, NULL);
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data_array[i].arr = arr;
        thread_data_array[i].left = i * chunk_size;
        thread_data_array[i].right = (i == NUM_THREADS - 1) ? ARRAY_SIZE - 1 : (i + 1) * chunk_size - 1;

        if (pthread_create(&threads[i], NULL, multiThread, (void *)&thread_data_array[i]) != 0) {
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Failed to join thread\n");
            exit(EXIT_FAILURE);
        }
    }

    // Merge the sorted chunks
    for (int step = 1; step < NUM_THREADS; step *= 2) {
        for (int i = 0; i < NUM_THREADS; i += 2 * step) {
            int left = i * chunk_size;
            int mid = left + step * chunk_size - 1;
            int right = left + (2 * step) * chunk_size - 1;

            // Ensure indices are within bounds
            if (left >= ARRAY_SIZE) break;
            if (mid >= ARRAY_SIZE) mid = ARRAY_SIZE - 1;
            if (right >= ARRAY_SIZE) right = ARRAY_SIZE - 1;

            mergeSortedArrays(arr, left, mid, right);
        }
    }

    gettimeofday(&end, NULL);
    double multi_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Multi-threaded merge sort time: %f seconds\n", multi_time);

    printf("Debug: Finished multi-threaded sort\n");

    printf("Debug: Starting single-threaded sort\n");
    // Single-threaded sort
    singleThread(arr_copy, ARRAY_SIZE);
    printf("Debug: Finished single-threaded sort\n");

    // Verify that both sorts produced the same result
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (complex_magnitude(arr[i]) != complex_magnitude(arr_copy[i])) {
            printf("Error: Sorts produced different results at index %d\n", i);
            break;
        }
    }

    free(arr);
    free(arr_copy);
    return 0;
}