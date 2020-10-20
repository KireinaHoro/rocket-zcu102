#include "bits.h"

#define DEBUG_P(...) printf(__VA_ARGS__)
//#define DEBUG_P(...)

void main(int hartid, void *dtb) {
  int bitslip = 0;
  if (hartid == 0) {
    uart_init();
    // trap requires UART
    setup_trap();
    printf("\n>>> Init on hart 0\n");
    printf(">>> BootROM DTB at %p\n", dtb);
    printf(">>> SPI controller setup\n");
    spi_init();
    printf(">>> Device startup\n");

    printf(">>> Enabling ADC clock...\n");
    write_gpio_reg(0x10); // 0x10: EN_CLK_ADC

    for (int i = 7; i >= 0; --i) {
        uint8_t readback = 0;

        printf(">>> Bringing up ADC @ SC%c_%c...\n", 'A' + ((7 - i) / 2), 'A' + ((7 - i) % 2));

        spi_select_slave(16 + i);
        spi_send(0x00 | 0x00); // write 00h: reset
        spi_send(0x80);
        spi_deselect_slave();

        // 00h reset is write-only

        spi_select_slave(16 + i);
        spi_send(0x00 | 0x03); // write 03h: pattern MSB
        spi_send(0x80 | 0x19); // OUTTEST = 1
        spi_deselect_slave();

        spi_select_slave(16 + i);
        spi_send(0x80 | 0x03); // read 03h: pattern MSB
        spi_recv_multi(&readback, 1);
        spi_deselect_slave();
        DEBUG_P("03h = %#x\n", readback);

        spi_select_slave(16 + i);
        spi_send(0x00 | 0x04); // write 04h: pattern LSB
        spi_send(0x84);
        spi_deselect_slave();

        spi_select_slave(16 + i);
        spi_send(0x80 | 0x04); // read 04h: pattern LSB
        spi_recv_multi(&readback, 1);
        spi_deselect_slave();
        DEBUG_P("04h = %#x\n", readback);

        spi_select_slave(16 + i);
        spi_send(0x00 | 0x02); // write 02h: output mode
        spi_send(0x07);        // 3.5mA | no-termination | output-on | 1-lane 16bit
        spi_deselect_slave();

        spi_select_slave(16 + i);
        spi_send(0x80 | 0x02); // read 02h: output mode
        spi_recv_multi(&readback, 1);
        spi_deselect_slave();
        DEBUG_P("02h = %#x\n", readback);
    }

    printf(">>> Jobs done, spinning\n");
    goto hang;
  } else {
    goto hang;
  }
hang:
  while (true) {
      // GPIO LED counter
      printf("Current Bitslip: %d\n", bitslip);
      printf("Enter new bitslip: ");
      fflush(stdout);
      int temp;
      int ret = scanf("%d", &temp);
      if (ret <= 0) {
          printf("stdin got clogged up, flushing...\n");
          fflush(stdin);
          continue;
      }
      if (temp < 0 || temp >= 8) {
          printf("Invalid bitslip %d: must be in [0,8)\n", temp);
      } else {
          bitslip = temp;
          write_gpio_reg(bitslip << 5 | 0x10); // 0x10: EN_CLK_ADC
      }
  }
}
