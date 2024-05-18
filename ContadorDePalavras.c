#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Armazena os dados da entidade Thread 
typedef struct {
    char *segment;
    int segment_size;
    int thread_id;
} ThreadData;

//Retorna o Tamanho total do arquivo index.txt
long fileSize(FILE *index){

    fseek(index, 0, SEEK_END);
    long size = ftell(index);
    rewind(index);
    return size;
}

void* process_segment(void *arg){
    ThreadData *data = (ThreadData*) arg;

    pthread_exit(NULL);
}

int main()
{

    const char *filename = "index.txt";
    FILE *file = fopen(filename, "r");

    if (!file) {
        perror("Não foi possivel abrir o arquivo");
        return EXIT_FAILURE;
    }

    long file_size = fileSize(file);
    printf("Tamanho do arquivo: %ld bytes\n", file_size);

    char *file_content = (char*) malloc(file_size + 1);
    if (!file_content) {
        perror("Erro ao alocar memória");
        fclose(file);

        return EXIT_FAILURE;
    }

    fread(file_content, 1, file_size, file);
    file_content[(file_size)] = '\0';

    fclose(file);


    int n_threads = 4;
    int segment_size = file_size/ n_threads;

    pthread_t threads[n_threads];
    ThreadData thread_data[n_threads];

    for (int i = 0; i < n_threads; i++) {
        int start = i * segment_size;
    }
    
    return 0;
}
