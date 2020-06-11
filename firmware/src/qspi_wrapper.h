#ifndef _QSPI_WRAPPER_H
#define _QSPI_WRAPPER_H

#include "main.h"

typedef struct QSPI_FILE {
  size_t pos;
  size_t size;
} QSPI_FILE;

void QSPI_init(void);

void QSPI_write_access(void);
void QSPI_read_access(void);

void QSPI_readonly_mode(void);
void QSPI_readwrite_mode(void);

void QSPI_wrote_clear(void);
uint8_t QSPI_wrote_get(void);

uint8_t QSPI_ready(void);

uint32_t QSPI_flash_size(void);

QSPI_FILE *QSPI_fopen(const char *p, const char *omode);
int QSPI_fseek ( QSPI_FILE * f, size_t offset, int origin );
int QSPI_feof(QSPI_FILE * f);
size_t QSPI_ftell(QSPI_FILE * f);
size_t QSPI_fread(void * ptr, size_t size, size_t count, QSPI_FILE * f );
void QSPI_fclose(QSPI_FILE *);

uint8_t *QSPI_mmap(size_t pos, size_t size, QSPI_FILE *f);
uint8_t *QSPI_addr();
#endif