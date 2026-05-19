#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_FILES 32
#define MAX_TOTAL_SIZE 200 * 1024 * 1024 // 200 MB 

// Dosyanın yalnızca ASCII metin olup olmadığını kontrol eder
int is_text_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return 0;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (!isascii(c)) {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

int has_sau_extension(const char *filename) {
    size_t len = strlen(filename);
    return len >= 4 && strcmp(filename + len - 4, ".sau") == 0;
}

const char *get_basename(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanim: ./tarsau -b veya ./tarsau -a\n");
        return 1;
    }

    // --- ARŞİVLEME MODU (-b) --- 
    if (strcmp(argv[1], "-b") == 0) {
        char *output_name = "a.sau"; // Varsayılan isim 
        int file_indices[MAX_FILES];
        int file_count = 0;
        long total_size = 0;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) output_name = argv[++i]; 
                continue;
            }
            if (file_count >= MAX_FILES) { 
                printf("Hata: En fazla 32 dosya girebilirsiniz!\n");
                return 1;
            }
            
            struct stat st;
            if (stat(argv[i], &st) != 0 || !is_text_file(argv[i])) {
                printf("%s giriş dosyasının formatı uyumsuzdur!\n", argv[i]); 
                return 1;
            }
            total_size += st.st_size;
            file_indices[file_count++] = i;
        }

        if (total_size > MAX_TOTAL_SIZE) { 
            printf("Hata: Giriş dosyalarının toplam boyutu 200 MB'ı geçemez!\n");
            return 1;
        }

        FILE *sau = fopen(output_name, "w");
        if (!sau) return 1;

        char header[10000] = "";
        for (int i = 0; i < file_count; i++) {
            struct stat st;
            stat(argv[file_indices[i]], &st);
            char temp[512];
            // Format: |Dosya adı, izinler, boyut| 
            sprintf(temp, "|%s,%o,%ld|", get_basename(argv[file_indices[i]]), st.st_mode & 0777, st.st_size);
            strcat(header, temp);
        }

        // İlk 10 bayt header boyutu 
        fprintf(sau, "%010ld", (long)strlen(header) + 10);
        fputs(header, sau); 

        for (int i = 0; i < file_count; i++) {
            FILE *in = fopen(argv[file_indices[i]], "r");
            if (in) {
                int c;
                while ((c = fgetc(in)) != EOF) fputc(c, sau); 
                fclose(in);
            }
        }
        fclose(sau);
        printf("Dosyalar birleştirildi.\n"); 
    }

    // --- ÇIKARMA MODU (-a) --- 
    else if (strcmp(argv[1], "-a") == 0) {
        if (argc < 3 || argc > 4) {
            printf("Hatalı parametre! Kullanım: ./tarsau -a arsiv.sau [dizin]\n");
            return 1;
        }
        char *archive_name = argv[2]; 
        char *target_dir = (argc > 3) ? argv[3] : "."; 

        if (!has_sau_extension(archive_name)) {
            printf("Ar\305\237iv dosyas\304\261 uygunsuz veya bozuk!\n");
            return 1;
        }

        FILE *sau = fopen(archive_name, "r");
        if (!sau) {
            printf("Arşiv dosyası uygunsuz veya bozuk!\n"); 
            return 1;
        }

        char size_buf[11] = {0};
        if (fread(size_buf, 1, 10, sau) < 10) { 
            printf("Arşiv dosyası uygunsuz veya bozuk!\n"); 
            fclose(sau);
            return 1;
        }
        
        long header_total_size = atol(size_buf); 
        int header_content_len = header_total_size - 10;
        
        if (header_content_len <= 0) {
            printf("Arşiv dosyası uygunsuz veya bozuk!\n");
            fclose(sau);
            return 1;
        }

        char *header = malloc(header_content_len + 1);
        fread(header, 1, header_content_len, sau);
        header[header_content_len] = '\0';

        // Dizin oluşturma ve dizine geçme 
        if (strcmp(target_dir, ".") != 0) {
            mkdir(target_dir, 0777);
            if (chdir(target_dir) != 0) {
                perror("Dizin hatası");
                free(header);
                fclose(sau);
                return 1;
            }
        }

        char *token = strtok(header, "|"); 
        while (token != NULL) {
            char name[256];
            unsigned int mode;
            long f_size;
            
            // Format: Dosya adı, izinler, boyut 
            if (sscanf(token, "%[^,],%o,%ld", name, &mode, &f_size) == 3) {
                FILE *out = fopen(name, "w");
                if (!out) {
                    perror("Dosya olusturma hatasi");
                    free(header);
                    fclose(sau);
                    return 1;
                }
                if (out) {
                    for (long i = 0; i < f_size; i++) {
                        int ch = fgetc(sau); 
                        if (ch != EOF) fputc(ch, out);
                    }
                    fclose(out);
                    chmod(name, mode); // İzinleri geri yükle 
                    printf("%s dosyası açıldı.\n", name); 
                }
            }
            token = strtok(NULL, "|");
        }
        free(header);
        fclose(sau);
    } 
    else {
        printf("Hatalı parametre! -b veya -a kullanın.\n");
        return 1;
    }

    return 0;
}
