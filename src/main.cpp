#include <Arduino.h>
#include <TFT_eSPI.h> // Substituído Adafruit_GFX e Adafruit_ST7735
#include <SPI.h>
#include "desenho.h"

// Define os botões
const int botaoA = 16;
const int botaoB = 17;

// Define portas das luzes
const int verde = 25;
const int amarelo = 26;
const int vermelho = 27;

// Define as portas do analogico

const int eixoY = 4;
const int eixoX = 15;
const int clickAnal = 5;

// Variáveis antibugs

bool moverJoy = false;
bool mudancaTela = false;

int opcaoSelecionada = 0;
const int totalItens = 3;

bool sequenciaAtiva = false;

// As definições de pinos agora são feitas no arquivo User_Setup.h da biblioteca TFT_eSPI

// Inicializa o objeto da tela com TFT_eSPI
TFT_eSPI tft = TFT_eSPI();

// Cria o objeto "Sprite" (nosso buffer de tela cheia na memória RAM)
TFT_eSprite telaBuffer = TFT_eSprite(&tft);

// Define os tempos para ser utilizado o Millis()
unsigned long tempoVermelho = 3000;
unsigned long tempoVerde = 4000;
unsigned long tempoAmarelo = 2000;

bool cliqueInicio = false;
unsigned long tempoDeInicio = 0;
int estadoAtual = 0;

void partidaContador()
{
  if (!sequenciaAtiva)
    return;

  switch (estadoAtual)
  {
  case 0:
    digitalWrite(vermelho, HIGH);
    digitalWrite(amarelo, LOW);
    digitalWrite(verde, LOW);
    tempoDeInicio = millis();
    estadoAtual = 1;
    if (digitalRead(botaoA) == HIGH)
    {
      cliqueInicio = false;
    }
    break;

  case 1:
    if (millis() - tempoDeInicio >= tempoVermelho)
    {
      digitalWrite(vermelho, HIGH);
      digitalWrite(amarelo, HIGH);
      digitalWrite(verde, LOW);
      tempoDeInicio = millis();
      estadoAtual = 2;
    }
    break;
  case 2:
    if (millis() - tempoDeInicio >= tempoAmarelo)
    {
      digitalWrite(vermelho, LOW);
      digitalWrite(amarelo, LOW);
      digitalWrite(verde, HIGH);
      tempoDeInicio = millis();
      estadoAtual = 3;
    }
    break;
  case 3:
    if (millis() - tempoDeInicio >= tempoVerde)
    {
      digitalWrite(vermelho, LOW);
      digitalWrite(amarelo, LOW);
      digitalWrite(verde, LOW);
      estadoAtual = 0;
      sequenciaAtiva = false;
    }
    break;
  }
}

void trocaDeItem()
{
  // Define as dimensões da tela/imagem
  int w = 160;
  int h = 128;
  const char *menuItens[] = {"Iniciar", "Mais procurado", "Config"};

  // Limpa o buffer com a cor preta. Todas as operações de desenho
  // a seguir ocorrerão na memória RAM, não no display físico.
  telaBuffer.fillSprite(TFT_BLACK);

  // Desenha a imagem de fundo no buffer
  // A função drawRGBBitmap existe na TFT_eSPI também.
  telaBuffer.pushImage(0, 0, w, h, desenho);

  // Desenha o texto no buffer
  telaBuffer.setCursor(5, 10);
  telaBuffer.setTextColor(TFT_WHITE);
  telaBuffer.setTextSize(2);
  telaBuffer.print("Menu");

  telaBuffer.setCursor(115, 40);
  telaBuffer.setTextSize(1);
  telaBuffer.print("DETRAN");

  int posY = 45;
  for (int i = 0; i < totalItens; i++)
  {
    if (i == opcaoSelecionada)
    {
      telaBuffer.fillTriangle(2, posY, 2, posY + 6, 5, posY + 3, TFT_WHITE);
      telaBuffer.setTextColor(TFT_BLACK, TFT_YELLOW);
      telaBuffer.setCursor(10, posY);
      telaBuffer.print(menuItens[i]);
    }
    else
    {
      telaBuffer.setTextColor(TFT_WHITE);
      telaBuffer.setCursor(10, posY);
      telaBuffer.print(menuItens[i]);
    }
    posY += 18;
  }

  // Agora que a imagem está completa no buffer, "empurra" tudo de uma só vez
  // para o display. Esta é a operação que atualiza a tela instantaneamente.
  telaBuffer.pushSprite(0, 0);
  mudancaTela = false;
}

void telaConfig(){
  int w = 160;
  int h = 128;

  telaBuffer.fillSprite(TFT_BLACK);

  telaBuffer.pushImage(0, 0, w, h, desenho);
  telaBuffer.pushSprite(0, 0);
}

void setup()
{
  Serial.begin(9600);

  // // Define os pinos das luzes

  pinMode(verde, OUTPUT);
  pinMode(amarelo, OUTPUT);
  pinMode(vermelho, OUTPUT);

  // // define pinos dos botões

  pinMode(botaoA, INPUT_PULLUP);
  pinMode(botaoB, INPUT_PULLUP);

  // Define pinos do analogico

  pinMode(eixoY, INPUT);
  pinMode(eixoX, INPUT);
  pinMode(clickAnal, INPUT);

  // // Inicializa a tela física

  tft.init();
  tft.setRotation(5);

  // Cria o espaço na memória RAM para o nosso buffer
  // Deve corresponder às dimensões da tela após a rotação
  telaBuffer.setColorDepth(8);      // 16-bit para cores (RGB565)
  telaBuffer.createSprite(160, 128); // Largura, Altura

  // Limpa a tela física uma vez no início para remover qualquer lixo da inicialização
  tft.fillScreen(TFT_BLACK);

  trocaDeItem();
}

void loop()
{
  int leitorEixoX = analogRead(eixoY);
  int leitorEixoY = analogRead(eixoX);
  int leituraBotaoA = digitalRead(botaoA);
  int leituraBotaoB = digitalRead(botaoB);

  if (leitorEixoX >= 4000)
  {
  }
  else if (leitorEixoX <= 500)
  {
  }
  if (!sequenciaAtiva)
  {
    
      if (leitorEixoY >= 4000 && !moverJoy)
      {
        opcaoSelecionada++;
        if (opcaoSelecionada >= totalItens)
        {
          opcaoSelecionada = totalItens - 1;
        }
        moverJoy = true;
        mudancaTela = true;
      }
      // Pra cima
      else if (leitorEixoY <= 500 && !moverJoy)
      {
        opcaoSelecionada = opcaoSelecionada - 1;
        if (opcaoSelecionada < 0)
        {
          opcaoSelecionada = 0;
        }
        mudancaTela = true;
        moverJoy = true;
      }
      else if(leitorEixoY > 1000 && leitorEixoY < 4000){
        moverJoy = false;
      }
  }

  if (opcaoSelecionada == 0 && leituraBotaoA == LOW && !sequenciaAtiva)
  {
    sequenciaAtiva = true;
    estadoAtual = 0;
  } else if(opcaoSelecionada ==2 && leituraBotaoA == LOW){
    telaConfig();
  }

  if (sequenciaAtiva)
  {
    partidaContador();
  }

  if (mudancaTela)
  {
    trocaDeItem();
  }

  Serial.println(moverJoy);
}