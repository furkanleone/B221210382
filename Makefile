# Derleyici ve bayraklar
CC = gcc
CFLAGS = -Wall -g

# Hedef dosya adı
TARGET = tarsau

# Varsayılan kural (Sadece 'make' yazinca calisir)
all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

# Derleme dosyalarini temizlemek icin (make clean)
clean:
	rm -f $(TARGET) *.sau
	rm -rf d1 cikti_klasoru