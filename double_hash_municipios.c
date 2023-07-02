#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define SEED    0x12345678


typedef struct {
    char codigo_ibge[8];
    char nome[30];
    char latitude[10];
    char longitude[10];
    char capital;
    char codigo_uf[2];
    char siafi_id[5];
    char ddd[2];
    char fuso_horario[15];
} tcidade;

char *get_key(void *reg) {
    return (*((tcidade *)reg)).codigo_ibge;
}

void *aloca_cidade(char *codigo_ibge, char *nome, char *latitude, char *longitude, char capital, char *codigo_uf, char *siafi_id, char *ddd, char *fuso_horario) {
    tcidade *cidade = malloc(sizeof(tcidade));
    strcpy(cidade->codigo_ibge, codigo_ibge);
    strcpy(cidade->nome, nome);
    strcpy(cidade->latitude, latitude);
    strcpy(cidade->longitude, longitude);
    cidade->capital = capital;
    strcpy(cidade->codigo_uf, codigo_uf);
    strcpy(cidade->siafi_id, siafi_id);
    strcpy(cidade->ddd, ddd);
    strcpy(cidade->fuso_horario, fuso_horario);
    return cidade;
}

typedef struct {
    uintptr_t *table;
    int size;
    int max;
    uintptr_t deleted;
    char *(*get_key)(void *);
} thash;

uint32_t hashf(const char *str, uint32_t h) {
    /* One-byte-at-a-time Murmur hash 
    Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

int hash_insere(thash *h, void *bucket) {
    uint32_t hash = hashf(h->get_key(bucket), SEED);
    int pos = hash % h->max;  // posicao inicial
    int i = 0;  // contador de colisoes

    if ((h->size + 1) > (h->max * 0.7)) { // so insere se hash table estiver menos que 70% cheia
        free(bucket);
        return EXIT_FAILURE;
    } else {
        while (h->table[pos] != 0) {  // se nao eh posicao vazia, 
            if (h->table[pos] == h->deleted) // nem posicao marcada como deletada, entao eh colisao
                break;  

            i++;  // posicao nao estava vazia e nao estava marcada como deletada, logo foi colisao
            
            pos = (hash + i * hashf(h->get_key(bucket), i)) % h->max; // calcula nova posicao usando double hashing

        }
    }
    h->table[pos] = (uintptr_t)bucket;  // insere o bucket na posicao livre encontrada
    h->size += 1; // incrementa o tamanho atual da hash table
    return EXIT_SUCCESS;  
}


void *hash_busca(thash h, const char *key) {
    uint32_t hash = hashf(key, SEED);
    int pos = hash % h.max;
    int i = 0; // contador de colisoes
    // enquanto nao encontrar posicao vazia, compara chave da pos atual de h com chave da pesquisa, se forem iguais, retorna pos atual
    while (h.table[pos] != 0) {
        if (strcmp(h.get_key((void *)h.table[pos]), key) == 0)
            return (void *)h.table[pos];
        else { // se nao forem iguais, utiliza double hash para encontrar proxima posicao onde seria feita a insercao em caso de colisao
            i++;
            
            pos = (hash + i * hashf(key, i)) % h.max;
            
        }
    }
    return NULL;
}

int hash_constroi(thash * h,int nbuckets, char * (*get_key)(void *)) {
    h->table = calloc(sizeof(uintptr_t),nbuckets +1);
    if (h->table == NULL){
        return EXIT_FAILURE;
    }
    h->max = nbuckets +1;
    h->size = 0;
    h->deleted = (uintptr_t) & (h->size);
    h->get_key = get_key;
    return EXIT_SUCCESS;
}

int hash_remove(thash *h, const char *key) {
    uint32_t hash = hashf(key, SEED);
    int pos = hash % h->max;
    int i = 0; // contador de colisoes
    
    // enquanto posicao atual nao estiver vazia, compare chave da pos atual com chave da busca
    while (h->table[pos] != 0) {
        if (strcmp(h->get_key((void *)h->table[pos]), key) == 0) { // se forem iguais
            free((void *)h->table[pos]);  
            h->table[pos] = h->deleted; // marca como DELETED
            h->size -= 1; // decrementa o tamanho atual da tabela
            return EXIT_SUCCESS;
        } else { // se as chaves nao forem iguais, houve colisao e atualiza pos com double hash
            i++;
            pos = (hash + i * hashf(h->get_key(key), i)) % h->max;
        }
    }
    return EXIT_FAILURE;
}


void hash_apaga(thash *h) {
    int pos;
    for (pos = 0; pos < h->max; pos++) {
        if (h->table[pos] != 0) {
            if (h->table[pos] != h->deleted) {
                free((void *)h->table[pos]);
            }
        }
    }
    free(h->table);
}



// funcao para visualizar a tabela
void hash_visualize(thash h) {
    for (int i = 0; i < h.max; i++) {
        if (h.table[i] == 0) {
            printf("Position %d: Empty\n", i);
        } else if (h.table[i] == h.deleted) {
            printf("Position %d: DELETED\n", i);
        } else {
            tcidade *cidade = (tcidade *)h.table[i];
            printf("Position %d: codigo_ibge=%s, nome=%s, latitude=%s, longitude=%s, capital=%d, codigo_uf=%s, siafi_id=%s, ddd=%s, fuso_horario=%s\n",
                    i, cidade->codigo_ibge, cidade->nome, cidade->latitude, cidade->longitude, cidade->capital,
                    cidade->codigo_uf, cidade->siafi_id, cidade->ddd, cidade->fuso_horario);
            }
        
        }
    }



int main() {                                                                          // ***********************************************
    FILE *file = fopen("C:\\Users\\Lucas\\Documents\\UFMS\\ES\\municipios.csv", "r"); // ** SUBSTITUA CAMINHO DO CSV AQUI PARA TESTAR **
    if (file == NULL) {                                                               // ***********************************************
        return EXIT_FAILURE;
    }

    thash hash_table;
    hash_constroi(&hash_table, 100, get_key); // tabela de no maximo 100 posicoes

    char line[1024];
    fgets(line, sizeof(line), file); // pula a primeira linha
    while (fgets(line, sizeof(line), file)) {
        char *codigo_ibge = strtok(line, ",");
        char *nome = strtok(NULL, ",");
        char *latitude = strtok(NULL, ",");
        char *longitude = strtok(NULL, ",");
        char *capital = strtok(NULL, ",");
        char *codigo_uf = strtok(NULL, ",");
        char *siafi_id = strtok(NULL, ",");
        char *ddd = strtok(NULL, ",");
        char *fuso_horario = strtok(NULL, "\n");

        void *bucket = aloca_cidade(codigo_ibge, nome, latitude, longitude, capital[0], codigo_uf, siafi_id, ddd, fuso_horario);
        hash_insere(&hash_table, bucket);
    }

    fclose(file);

    // visualiza
    hash_visualize(hash_table);

    // recuperando dados da cidade pelo seu codigo ibge
    const char *key = "5200050";
    void *result = hash_busca(hash_table, key);
    if (result != NULL) { // se o codigo esta presente na tabela, printe os dados do registro
        tcidade *cidade = (tcidade *)result;
        printf("\nREGISTRO ENCONTRADO: codigo_ibge=%s, nome=%s, latitude=%s, longitude=%s, capital=%d, codigo_uf=%s, siafi_id=%s, ddd=%s, fuso_horario=%s\n",
               cidade->codigo_ibge, cidade->nome, cidade->latitude, cidade->longitude, cidade->capital,
               cidade->codigo_uf, cidade->siafi_id, cidade->ddd, cidade->fuso_horario);
        
    } else {
        printf("\nREGISTRO NAO ENCONTRADO%s\n", key);
    }
    
    // removendo dados com base no codigo ibge
    const char *remove_key = "5200050";
    int remove_result = hash_remove(&hash_table, remove_key);
    if (remove_result == EXIT_SUCCESS) {
        printf("\nRegistro com a chave %s foi removido com sucesso.\n", remove_key);
    } else {
        printf("\nFalha ao remover o registro com a chave %s.\n", remove_key);
    }
    
    // visualiza apos remocao
    printf("\nAPOS REMOCAO:\n");
    hash_visualize(hash_table);

    hash_apaga(&hash_table);

    return 0;
}