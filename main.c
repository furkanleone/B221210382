#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_FILES 32
#define MAX_TOTAL_SIZE 200 * 1024 * 1024 // 200 MB

// Dosyanın ASCII olup olmadığını kontrol eder
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Kullanim: ./tarsau -b veya ./tarsau -a\n");
        return 1;
    }

    // --- ARŞİVLEME MODU (-b) ---
    if (strcmp(argv[1], "-b") == 0) {
        char *output_name = "a.sau";
        int file_indices[MAX_FILES];
        int file_count = 0;
        long total_size = 0;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) output_name = argv[++i];
                continue;
            }
            if (file_count >= MAX_FILES) {
                printf("Hata: En fazla 32 dosya!\n");
                return 1;
            }
            
            struct stat st;
            if (stat(argv[i], &st) != 0 || !is_text_file(argv[i])) {
                printf("%s giris dosyasinin formati uyumsuzdur!\n", argv[i]);
                return 1;
            }
            total_size += st.st_size;
            file_indices[file_count++] = i;
        }

        if (total_size > MAX_TOTAL_SIZE) {
            printf("Hata: Toplam boyut 200 MB'i gecemez!\n");
            return 1;
        }

        FILE *sau = fopen(output_name, "w");
        char header[10000] = "";
        for (int i = 0; i < file_count; i++) {
            struct stat st;
            stat(argv[file_indices[i]], &st);
            char temp[512];
            sprintf(temp, "|%s,%03o,%ld|", argv[file_indices[i]], st.st_mode & 0777, st.st_size);
            strcat(header, temp);
        }

        fprintf(sau, "%010ld", strlen(header) + 10);
        fputs(header, sau);

        for (int i = 0; i < file_count; i++) {
            FILE *in = fopen(argv[file_indices[i]], "r");
            int c;
            while ((c = fgetc(in)) != EOF) fputc(c, sau);
            fclose(in);
        }
        fclose(sau);
        printf("Dosyalar birlestirildi.\n");
    }

    // --- ÇIKARMA MODU (-a) ---
    else if (strcmp(argv[1], "-a") == 0) {
        if (argc < 3) return 1;
        char *archive_name = argv[2];
        char *target_dir = (argc > 3) ? argv[3] : ".";

        FILE *sau = fopen(archive_name, "r");
        if (!sau) {
            printf("Arşiv dosyası uygunsuz veya bozuk!\n");
            return 1;
        }

        char size_buf[11] = {0};
        fread(size_buf, 1, 10, sau);
        long header_size = atol(size_buf);
        char *header = malloc(header_size - 9);
        fread(header, 1, header_size - 10, sau);

        if (strcmp(target_dir, ".") != 0) {
            mkdir(target_dir, 0777);
            chdir(target_dir);
        }

        char *token = strtok(header, "|");
        while (token != NULL) {
            char name[256];
            int mode;
            long f_size;
            sscanf(token, "%[^,],%o,%ld", name, &mode, &f_size);

            FILE *out = fopen(name, "w");
            for (long i = 0; i < f_size; i++) fputc(fgetc(sau), out);
            fclose(out);
            chmod(name, mode); // Orijinal izinleri geri yükle

            token = strtok(NULL, "|");
        }
        free(header);
        fclose(sau);
        printf("Dosyalar acildi.\n");
    }

    return 0;
}