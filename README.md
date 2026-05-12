Esse é um projeto de uma mini pista de corrida usando um esp32 como controlador principal para as operações de contar voltas, calcular velocidade através de sensores, calcular maior velocidade e criar média das informações obtidas durante o percurso.

Para a realização desse projeto são necessários: LEDS, display LCD de 1.8 polegadas, botões, analógico, buzzer e sensor ultrassônico.
Recomendo copiar e colar esse código no próprio arquivo User_Setup.h quando for aplicar o projeto.

User_Setup.h:

// #######################################################
// TFT_eSPI - ESP32 + ST7735
// Mini Pista de Corrida
// #######################################################

#define USER_SETUP_INFO "MiniPista_ST7735"

// #############################
// DRIVER DA TELA
// #############################

#define ST7735_DRIVER

// resolução da tela
#define TFT_WIDTH  128
#define TFT_HEIGHT 160

#define ST7735_BLACKTAB

// #############################
// PINAGEM SPI
// #############################

#define TFT_MOSI 23
#define TFT_SCLK 18

#define TFT_MISO -1

#define TFT_CS   21
#define TFT_DC   22
#define TFT_RST  27

// #############################
// FONTS
// #############################

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

// #############################
// SPI CLOCK
// #############################

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
