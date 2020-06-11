#include "qspi_wrapper.h"
#include "main.h"

uint8_t qspi_write_mode;
uint8_t qspi_wrote;
uint32_t qspi_last_access;

#define QSPI_READWRITE_WAIT 3000

void QSPI_init() {
  BSP_QSPI_DeInit();
  BSP_QSPI_Init();

  BSP_QSPI_MemoryMappedMode();
  qspi_last_access = HAL_GetTick();
  qspi_write_mode = 0;
  qspi_wrote = 0;
}

void QSPI_readwrite_mode() {
  if (qspi_write_mode == 0) {
    BSP_QSPI_Init();
  }
  qspi_write_mode = 1;
}

void QSPI_readonly_mode() {
  if (qspi_write_mode == 1) {
    BSP_QSPI_Init();
    BSP_QSPI_MemoryMappedMode();
  }
  qspi_write_mode = 0;
}

void QSPI_read_access() {
  qspi_last_access = HAL_GetTick();
}

void QSPI_write_access() {
  qspi_last_access = HAL_GetTick();
  qspi_wrote = 1;
}

void QSPI_wrote_clear() {
  qspi_wrote = 0;
}

uint8_t QSPI_wrote_get() {
  return qspi_wrote;
}

uint8_t QSPI_ready() {
  if (HAL_GetTick() > qspi_last_access + QSPI_READWRITE_WAIT)
    return 1;
  else
    return 0;
}

uint32_t QSPI_flash_size() {
  QSPI_Info pQSPI_Info;

  pQSPI_Info.FlashSize        = (uint32_t)0x00;
  pQSPI_Info.EraseSectorSize    = (uint32_t)0x00;
  pQSPI_Info.EraseSectorsNumber = (uint32_t)0x00;
  pQSPI_Info.ProgPageSize       = (uint32_t)0x00;
  pQSPI_Info.ProgPagesNumber    = (uint32_t)0x00;

  /* Read the QSPI memory info */
  BSP_QSPI_GetInfo(&pQSPI_Info);
  return pQSPI_Info.FlashSize;
}

// do not really open a file
QSPI_FILE *QSPI_fopen(const char *p, const char *omode) {
  QSPI_FILE *file = (QSPI_FILE *)malloc(sizeof(QSPI_FILE));
  file->pos = 0;
  file->size = QSPI_flash_size();
  return file;
}

inline int QSPI_fseek ( QSPI_FILE * f, size_t offset, int origin ) {
  switch (origin) {
  case SEEK_CUR:
    f->pos = f->pos + offset;
    break;
  case SEEK_SET:
    f->pos = offset;
    break;
  case SEEK_END:
    f->pos = f->size + offset;
    break;
  }

  return 0;
}

int QSPI_feof(QSPI_FILE * f) {
  if (f->pos == f->size) {
    return 1;
  } else {
    return 0;
  }
}

inline size_t QSPI_ftell(QSPI_FILE * f) {
  return f->pos;
}

void QSPI_fclose(QSPI_FILE *f) {
  free(f);
}


__IO uint8_t *qspi_addr = (__IO uint8_t *)(0x90000000);

uint8_t *QSPI_addr() {
  return (uint8_t* )qspi_addr;
}

uint8_t *QSPI_mmap(size_t pos, size_t size, QSPI_FILE * f) {
  return (uint8_t *)(qspi_addr + pos);
}

size_t QSPI_fread(void * ptr, size_t size, size_t count, QSPI_FILE * f ) {
  size_t max_size = count * size;
  uint8_t *buf = (uint8_t *)ptr;

  if (f->pos + count * size > f->size)
    max_size = f->size - f->pos;

  memcpy(buf, (uint8_t *)(qspi_addr + f->pos), max_size);

  f->pos += max_size;
  return count;
}
