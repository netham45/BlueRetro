#ifndef _GAMEPORT_SPI_H_
#define _GAMEPORT_SPI_H_

void gameport_initialize(void);
void gameport_process(uint32_t player, uint32_t controller);

#endif /* _GAMEPORT_SPI_H_ */
