#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "uthash.h"

// Armazena a contaem de palavras 
typedef struct {
    char *word;
    int count;
    UT_hash_handle hh; //Torna essa estrutura Hashable
} WordCount;

//Armazena os dados da entidade Thread 
typedef struct {
    char *segment;
    int segment_size;
    int thread_id;
    WordCount *word_counts; //ponteiro pro hash map
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

// Adiciona uma palavra ao hash map ou incrementa sua contagem
void add_word(WordCount **word_counts, char *word){
    WordCount *s;
    //Verifica se a palavra já foi adicionada ao hash map
    HASH_FIND_STR(*word_counts, word, s);
    // Se a palavra não estiver no hash aloca memoria para ela
    if (s == NULL) {
        s = (WordCount*) malloc(sizeof(WordCount));
        if (s == NULL) {
            perror("Erro ao alocar memória para WordCount");
            exit(EXIT_FAILURE);
        }
        //duplica a string 
        s->word = strdup(word);
        if (s->word == NULL) {
            perror("Erro ao duplicar a palavra");
            exit(EXIT_FAILURE);
        }
        // inicia a contagem da palavra em 1
        s->count = 1;
        //adiciona ao hashmap essa nova entrada
        HASH_ADD_KEYPTR(hh,*word_counts,s->word, strlen(s->word), s);
    }else {
        //incrementa na contagem
        s->count++;
    }
}

//Função executada por cada thread no seu segmento de arquivo
void* process_segment(void *arg){
    // converte o segmento para o tipo ThreadID
    ThreadData *data = (ThreadData*) arg;
    char *segment = data->segment;
    //tamanho do segmento
    int segment_size = data->segment_size;
    // buffer para a palavra atual
    int word_count[256] = {0};
    //comprimento para a palavra 
    char word[100];

    // variavel pro comprimento da palavra atual
    int word_length = 0;


    //percorre cada caracter
    for (int i = 0; i < segment_size; i++) {
        //se o caracter é alfanumerico 
        if (isalnum(segment[i])) {
            if (word_length < sizeof(word)-1) {
                //adiciona o caractere ao buffer
                word[word_length++] = tolower(segment[i]);
            }
            //Se encontar um separador e há uma palavra no buffer
        } else if (word_length > 0) {
            // termina a palavra com um caracter nulo
            word[word_length] = '\0';
            // adiciona ao hash map
            add_word(&data->word_counts, word);
            // reseta o comprimento da palavra
            word_length = 0;
        }
    }
    //Adiciona um caracter nulo se o segmento termina com uma palavra
    if (word_length > 0) {
        word[word_length] = '\0';
        add_word(&data->word_counts, word);
    }

    // encerra a thread
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
    
    //Cria as threads
    for (int i = 0; i < n_threads; i++) {
        // calcula o inicio do segmento
        int start = i * segment_size;
        // calcula o tamanho total do segmento
        int size = (i == n_threads -1) ? (file_size - start) : segment_size;


        // aponta o segmento para a posiçaõ certa
        thread_data[i].segment = &file_content[start];
        // define o tamanho do segmento 
        thread_data[i].segment_size = size;
        // define o ID da thread
        thread_data[i].thread_id =i;
        //inicializa o ponteiro do hash como NULL
        thread_data[i].word_counts = NULL;

        //cria a thread
        int rc = pthread_create(&threads[i], NULL, process_segment,(void*)&thread_data[i]);
        if (rc) {
            fprintf(stderr,"Erro ao criar a thread %d: %d", i, rc);
            free(file_content);
            return EXIT_FAILURE;
        }
    }

    //hash map para juntar os resultados de todas as threads
    WordCount *total_word_counts = NULL;

    //espera todas as threads terminarem e consolida os resultados
    for (int i = 0; i < n_threads; i++) {
        //espera as thread termina
        int rc = pthread_join(threads[i],NULL);
        if (rc) { 
            fprintf(stderr,"Erro ao esperar a thread %d terminar: %d\n", i, rc);
            free(file_content);
            return EXIT_FAILURE;
        }

        // consolida os resultados de contagem de palavras de cada thread
        WordCount *s, *tmp;
        HASH_ITER(hh, thread_data[i].word_counts, s, tmp){
            // adiciona ou incrementa a contagem
            add_word(&total_word_counts, s->word);
            // remove a entrada do hash da thread
            HASH_DEL(thread_data[i].word_counts, s);
            // libera o espaço da memoria
            free(s->word);
            // libera a memoria da estrutura WordCount
            free(s);
        }
    }

    WordCount *s, *tmp;
    HASH_ITER(hh, total_word_counts, s, tmp){
        // imprime a palavra e sua contagem
        printf("Palavra '%s': %d vezes\n", s->word, s->count);
        free(s->word);
        free(s);
    }

    // libera a memória do conteúdo do arquivo
    free(file_content);

    return 0;
}
