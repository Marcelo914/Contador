#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//Armazena os dados da entidade Thread 
typedef struct {
    char *segment;
    int segment_size;
    int thread_id;
} ThreadData;









//Retorna o Tamanho total do arquivo index.txt
long fileSize(FILE *index){
    //move o ponteiro do arquivo para o final
    fseek(index, 0, SEEK_END);
    //atribui a zise o valor atual do indicador de posição
    long size = ftell(index);
    //volta o ponteiro pro começo do arquivo
    rewind(index);
    return size;
}

//Define a função que será executada por cada thread 
void* process_segment(void *arg){
    ThreadData *data = (ThreadData*) arg;
    char *segment = data->segment;
    int segment_size = data->segment_size;
    


    pthread_exit(NULL);
}

int main()
{
    
    const char *filename = "index.txt";
    //abre o arquivo index.txt no modo de leitura 
    FILE *file = fopen(filename, "r");

    if (!file) {
        perror("Não foi possivel abrir o arquivo");
        return EXIT_FAILURE;
    }
    //Retorna o tamanho em bytes do arquivo
    long file_size = fileSize(file);
    printf("Tamanho do arquivo: %ld bytes\n", file_size);

    //file_content é um ponteiro para char onde será armazenado o conteudo,
    //Aloca memoria suficiente para o conteudo do arquivo + um byte extra para o caracter nulo
    char *file_content = (char*) malloc(file_size + 1);
    if (!file_content) {
        perror("Erro ao alocar memória");
        fclose(file);

        return EXIT_FAILURE;
    }

    fread(file_content, 1, file_size, file);
    //Colocando um caracter nulo no final da string para marcar o fim dela
    file_content[(file_size)] = '\0';

    fclose(file);

    //declaração do número de threads que será usado
    int n_threads = 4;
    //O tamanho de cada segmento do arquivo será seu tamanho em bytes/threads
    int segment_size = file_size/ n_threads;
     //array que armazena n_threads
    pthread_t threads[n_threads];
    //passando os dados do struct para dendo do array
    ThreadData thread_data[n_threads];

    for (int i = 0; i < n_threads; i++) {
        int start = i * segment_size;
        int size = (i == n_threads -1) ? (file_size - start) : segment_size;

        thread_data[i].segment = &file_content[start];
        thread_data[i].segment_size = size;
        thread_data[i].thread_id =1;

        int rc = pthread_create(&threads[i], NULL, process_segment,(void*)&thread_data[i]);
        if (rc) {
            fprintf(stderr,"Erro ao criar a thread %d: %d", i, rc);
            free(file_content);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < n_threads; i++) {
        int rc = pthread_join(threads[i],NULL);
        if (rc) {
            fprintf(stderr,"Erro ao esperar a thread %d terminar: %d\n", i, rc);
            free(file_content);
            return EXIT_FAILURE;
        }
    }



    
    return 0;
}
