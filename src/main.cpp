#include <Arduino.h>
#include <TFT_eSPI.h>
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

// Define a porta do buzzer
const int buzzer = 1;

// Variáveis antibugs e de controle de listas
bool moverJoy = false;
bool mudancaTela = false;
bool primeiraLeitura = true;

int opcaoSelecionada = 0;
const int totalItens = 3;

int selecaoAjuste = 0;
int totalAjustes = 3;

bool sequenciaAtiva = false;

bool inversaoCor = false;

bool somSilenciado = false;

// NOVO: Controle não-bloqueante do buzzer
unsigned long buzzerTempoInicio = 0;
unsigned long buzzerDuracao = 0;
bool buzzerAtivo = false;

// As definições de pinos do display agora são feitas no arquivo User_Setup.h da biblioteca TFT_eSPI

// Inicializa o objeto da tela com TFT_eSPI
TFT_eSPI tft = TFT_eSPI();

// Cria o objeto "Sprite"
TFT_eSprite telaBuffer = TFT_eSprite(&tft);

// Define os tempos para ser utilizado o Millis()
unsigned long tempoVermelho = 3000;
unsigned long tempoVerde = 4000;
unsigned long tempoAmarelo = 2000;

bool cliqueInicio = false;
unsigned long tempoDeInicio = 0;
int estadoAtual = 0;

enum telaEstados
{
  tela_Menu,
  tela_Ajustes,
  tela_Procurado
};

telaEstados telaAtual = tela_Menu;

// Debounce do botão
bool botaoADeb = HIGH;
bool botaoBDeb = HIGH;

// NOVO: Função para gerenciar o buzzer de forma não-bloqueante
void atualizaBuzzer()
{
  if (buzzerAtivo && (millis() - buzzerTempoInicio >= buzzerDuracao))
  {
    noTone(buzzer);
    buzzerAtivo = false;
  }
}

// NOVO: Função para tocar som sem bloquear
void tocarSom(int frequencia, unsigned long duracao)
{
  if (!somSilenciado)
  {
    tone(buzzer, frequencia);
    buzzerTempoInicio = millis();
    buzzerDuracao = duracao;
    buzzerAtivo = true;
  }
}

// Som curto ao navegar
void somNavegacao()
{
  tocarSom(1000, 50); // 1000Hz, 50ms
}

// Som médio ao selecionar
void somSelecao()
{
  tocarSom(1500, 100); // 1500Hz, 100ms
}

// Som ao trocar cor na largada
void somTrocaCor()
{
  tocarSom(2000, 200); // 2000Hz, 200ms
}

// Som especial na largada (verde)
void somLargada()
{
  tocarSom(2500, 300); // 2500Hz, 300ms
}

void trocaDeItem()
{
  // Define as dimensões da tela/imagem
  int w = 160;
  int h = 128;
  // Cria a lista de itens da tela de menu
  const char *menuItens[] = {"Iniciar", "Mais procurado", "Ajustes"};

  // Limpa o buffer com a cor preta. Todas as operações de desenho
  // a seguir ocorrerão na memória RAM, não no display físico.
  telaBuffer.fillSprite(TFT_BLACK);

  // Desenha a imagem de fundo no buffer
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
  // Adiciona os itens da tela de menu e adiciona o highlight quando ele é selecionado
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

void partidaContador()
{
  if (!sequenciaAtiva)
    return;
    
  switch (estadoAtual)
  {
    // Inicia na cor vemelha e faz a checagem pra ver se o operador deu a partida
  case 0:
    if (inversaoCor)
    {
      tft.invertDisplay(false);
    }
    digitalWrite(vermelho, HIGH);
    digitalWrite(amarelo, LOW);
    digitalWrite(verde, LOW);
    telaBuffer.fillSprite(TFT_BLUE);
    telaBuffer.pushSprite(0, 0);
    somTrocaCor();  // Som toca apenas UMA vez aqui
    tempoDeInicio = millis();
    estadoAtual = 1;
    break;
    
    // Inicia a segunda sequencia da largada
  case 1:
    if (millis() - tempoDeInicio >= tempoVermelho)
    {
      digitalWrite(vermelho, HIGH);
      digitalWrite(amarelo, HIGH);
      digitalWrite(verde, LOW);
      telaBuffer.fillSprite(TFT_CYAN);
      telaBuffer.pushSprite(0, 0);
      somTrocaCor();  // Som DENTRO do if - toca apenas quando muda
      tempoDeInicio = millis();
      estadoAtual = 2;
    }
    break;
    
    // Da o inicio da corrida
  case 2:
    if (millis() - tempoDeInicio >= tempoAmarelo)
    {
      digitalWrite(vermelho, LOW);
      digitalWrite(amarelo, LOW);
      digitalWrite(verde, HIGH);
      telaBuffer.fillSprite(TFT_GREEN);
      telaBuffer.pushSprite(0, 0);
      somLargada();  // Som DENTRO do if - toca apenas quando muda
      tempoDeInicio = millis();
      estadoAtual = 3;
    }
    break;
    
    // Reseta as luzes e o estado da corrida
  case 3:
    if (millis() - tempoDeInicio >= tempoVerde)
    {
      digitalWrite(vermelho, LOW);
      digitalWrite(amarelo, LOW);
      digitalWrite(verde, LOW);
      if (inversaoCor)
      {
        tft.invertDisplay(true);
      }
      trocaDeItem();
      estadoAtual = 0;
      sequenciaAtiva = false;
    }
    break;
  }
}

void telaAjustes()
{
  int w = 160;
  int h = 128;

  const char *opcoesAjustes[] = {"Mudar tema", "Silenciar sons", "Mudar icone"};

  telaBuffer.fillSprite(TFT_BLACK);

  telaBuffer.pushImage(0, 0, w, h, desenho);

  telaBuffer.setCursor(5, 10);
  telaBuffer.setTextColor(TFT_WHITE);
  telaBuffer.setTextSize(2);
  telaBuffer.print("Ajustes");

  telaBuffer.setTextSize(1);

  int posY = 45;
  for (int i = 0; i < totalAjustes; i++)
  {
    if (i == selecaoAjuste)
    {
      telaBuffer.fillTriangle(2, posY, 2, posY + 6, 5, posY + 3, TFT_WHITE);
      telaBuffer.setTextColor(TFT_BLACK, TFT_YELLOW);
      telaBuffer.setCursor(10, posY);
      telaBuffer.print(opcoesAjustes[i]);
    }
    else
    {
      telaBuffer.setTextColor(TFT_WHITE);
      telaBuffer.setCursor(10, posY);
      telaBuffer.print(opcoesAjustes[i]);
    }

    if (i != totalAjustes - 1)
    {
      telaBuffer.drawRect(130, posY, 10, 10, TFT_WHITE);
      bool opcaoSelec = false;
      if (i == 0) opcaoSelec = inversaoCor;
      if (i == 1) opcaoSelec = somSilenciado;
      if (opcaoSelec)
        telaBuffer.fillRect(132, posY + 2, 6, 6, TFT_WHITE);
    }
    posY += 18;
  }

  telaBuffer.pushSprite(0, 0);
  mudancaTela = false;
}

void setup()
{
  Serial.begin(9600);

  // Define os pinos das luzes
  pinMode(verde, OUTPUT);
  pinMode(amarelo, OUTPUT);
  pinMode(vermelho, OUTPUT);

  // define pinos dos botões
  pinMode(botaoA, INPUT_PULLUP);
  pinMode(botaoB, INPUT_PULLUP);

  // Define pinos do analogico
  pinMode(eixoY, INPUT);
  pinMode(eixoX, INPUT);
  pinMode(clickAnal, INPUT_PULLUP);

  // Configura o buzzer e garante que esteja desligado
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  noTone(buzzer);

  // Inicializa a tela física
  tft.init();
  tft.setRotation(5);

  // Cria o espaço na memória RAM para o nosso buffer
  // Deve corresponder às dimensões da tela após a rotação
  telaBuffer.setColorDepth(8);
  telaBuffer.createSprite(160, 128);

  // Limpa a tela física uma vez no início para remover qualquer lixo da inicialização
  tft.fillScreen(TFT_BLACK);

  trocaDeItem();
  
  // Aguarda estabilização das leituras analógicas
  delay(100);
}

void loop()
{
  // IMPORTANTE: Atualiza o buzzer a cada loop (não-bloqueante)
  atualizaBuzzer();

  // Leitura do analógicos e botões
  int leitorEixoX = analogRead(eixoY);
  int leitorEixoY = analogRead(eixoX);
  int leituraBotaoA = digitalRead(botaoA);
  int leituraBotaoB = digitalRead(botaoB);

  // Ignora primeira leitura para evitar ruído ao ligar
  if (primeiraLeitura)
  {
    primeiraLeitura = false;
    return;
  }

  switch (telaAtual)
  {
    case tela_Menu:
    {
      // Trava a navegação se a sequência de LEDs estiver ativa
      if (!sequenciaAtiva)
      {
        // Pra baixo
        if (leitorEixoY >= 4000 && !moverJoy)
        {
          opcaoSelecionada++;
          if (opcaoSelecionada >= totalItens)
          {
            opcaoSelecionada = totalItens - 1;
          }
          somNavegacao();
          moverJoy = true;
          mudancaTela = true;
        }
        // Pra cima
        else if (leitorEixoY <= 500 && !moverJoy)
        {
          opcaoSelecionada--;
          if (opcaoSelecionada < 0)
          {
            opcaoSelecionada = 0;
          }
          somNavegacao();
          mudancaTela = true;
          moverJoy = true;
        }
        // Trava da deadzone
        else if (leitorEixoY > 1000 && leitorEixoY < 3800)
        {
          moverJoy = false;
        }
      }

      // Detecção de clique no botão A
      if (leituraBotaoA == LOW && botaoADeb == HIGH)
      {
        if (opcaoSelecionada == 0)
        {
          sequenciaAtiva = true;
          estadoAtual = 0;
          somSelecao();
          break;
        }
        else if (opcaoSelecionada == 1)
        {
          Serial.println("procurados");
          somSelecao();
          break;
        }
        else if (opcaoSelecionada == 2)
        {
          telaAtual = tela_Ajustes;
          selecaoAjuste = 0;
          mudancaTela = true;
          botaoADeb = leituraBotaoA;
          botaoBDeb = leituraBotaoB;
          somSelecao();
          break;
        }
      }
      
      botaoADeb = leituraBotaoA;
      
      // Se começar a corrida dá inicio nos LEDs
      if (sequenciaAtiva)
      {
        partidaContador();
      }

      // Chama a troca de item na tela toda vez que necessário
      if (mudancaTela)
      {
        trocaDeItem();
      }
      break;
    }

    case tela_Ajustes:
    {
      // Renderiza ao entrar
      if (mudancaTela)
      {
        telaAjustes();
      }

      // Navegação
      if (leitorEixoY >= 4000 && !moverJoy)
      {
        selecaoAjuste++;
        if (selecaoAjuste >= totalAjustes)
        {
          selecaoAjuste = totalAjustes - 1;
        }
        somNavegacao();
        moverJoy = true;
        mudancaTela = true;
      }
      else if (leitorEixoY <= 500 && !moverJoy)
      {
        selecaoAjuste--;
        if (selecaoAjuste < 0)
        {
          selecaoAjuste = 0;
        }
        somNavegacao();
        mudancaTela = true;
        moverJoy = true;
      }
      else if (leitorEixoY > 1000 && leitorEixoY < 3800)
      {
        moverJoy = false;
      }

      // Seleção de opção (Botão A)
      if (leituraBotaoA == LOW && botaoADeb == HIGH)
      {
        if (selecaoAjuste == 0)
        {
          inversaoCor = !inversaoCor;
          tft.invertDisplay(inversaoCor);
          somSelecao();
          mudancaTela = true;
          Serial.print("Tema invertido: ");
          Serial.println(inversaoCor ? "SIM" : "NAO");
        }
        else if (selecaoAjuste == 1)
        {
          somSilenciado = !somSilenciado;
          
          // Som específico ao silenciar/dessilenciar
          if (!somSilenciado)
          {
            tocarSom(1500, 100);
          }
          else
          {
            tocarSom(800, 100);
          }
          
          mudancaTela = true;
          Serial.print("Som silenciado: ");
          Serial.println(somSilenciado ? "SIM" : "NAO");
        }
        else if (selecaoAjuste == 2)
        {
          somSelecao();
          Serial.println("Mudar icone (nao implementado)");
        }
      }

      // Voltar ao menu (Botão B)
      if (leituraBotaoB == LOW && botaoBDeb == HIGH)
      {
        somSelecao();
        telaAtual = tela_Menu;
        mudancaTela = true;
        Serial.println("Voltando para Menu");
      }

      // Atualiza debounce
      botaoADeb = leituraBotaoA;
      botaoBDeb = leituraBotaoB;

      // Redesenha se necessário
      if (mudancaTela)
      {
        telaAjustes();
      }
      break;
    }
  }
}
