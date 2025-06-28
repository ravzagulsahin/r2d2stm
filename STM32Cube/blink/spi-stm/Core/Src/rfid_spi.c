#include "rfid_spi.h"

extern SPI_HandleTypeDef hspi1;
#define CS_LOW HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)
#define CS_HIGH HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)

void write(uint8_t addr, uint8_t val)
{
    uint8_t data[2] = {(addr << 1) & 0x7E, val};
    CS_LOW;
    HAL_SPI_Transmit(&hspi1, data, 2, HAL_MAX_DELAY);
    CS_HIGH;
}

uint8_t read(uint8_t addr)
{
    uint8_t tx = ((addr << 1) & 0x7E) | 0x80;
    uint8_t rx;
    CS_LOW;
    HAL_SPI_Transmit(&hspi1, &tx, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi1, &rx, 1, HAL_MAX_DELAY);
    CS_HIGH;
    return rx;
}

void reset()
{
    write(CommandReg, 0x0F);
}

void antenac()
{
    uint8_t temp = read(TxControlReg);
    if (!(temp & 0x03))
        write(TxControlReg, temp | 0x03);
}

void antenkapa()
{
    uint8_t temp = read(TxControlReg);
    write(TxControlReg, temp & (~0x03));
}

void initial()
{
    reset();
    write(TModeReg, 0x8D);
    write(TPrescalerReg, 0x3E);
    write(TReloadRegL, 30);
    write(TReloadRegH, 0);
    write(0x15, 0x40);
    write(ModeReg, 0x3D);
    antenac();
}

uint8_t kart_var_mi()
{
    write(BitFramingReg, 0x07);
    write(CommandReg, PCD_IDLE);
    write(FIFOLevelReg, 0x80);
    write(FIFODataReg, PICC_REQIDL);
    write(CommandReg, PCD_TRANSCEIVE);
    write(BitFramingReg, 0x87);

    uint8_t irq = read(CommIrqReg);
    for (uint32_t i = 0; i < 2000 && !(irq & 0x30); i++)
    {
        irq = read(CommIrqReg);
    }

    uint8_t error = read(ErrorReg);
    return !(error & 0x1B);
}

uint8_t uid_oku(uint8_t *uid)
{
    write(BitFramingReg, 0x00);
    write(CommandReg, PCD_IDLE);
    write(FIFOLevelReg, 0x80);
    write(FIFODataReg, PICC_ANTICOLL);
    write(FIFODataReg, 0x20);
    write(CommandReg, PCD_TRANSCEIVE);

    // Burayı düzeltiyoruz:
    uint8_t irq;
    int i = 2000;
    do
    {
        irq = read(CommIrqReg);
        i--;
    } while (i && !(irq & 0x30)); // RxIRq | TimerIRq

    uint8_t error = read(ErrorReg);
    if (error & 0x1B)
        return 0;

    uint8_t n = read(FIFOLevelReg);
    for (int i = 0; i < n; i++)
    {
        uid[i] = read(FIFODataReg);
    }
    return 1;
}
