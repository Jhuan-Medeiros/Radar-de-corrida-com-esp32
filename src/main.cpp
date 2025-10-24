#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Preferences.h>
#include "desenho.h"
#include "desenho2.h"

Preferences preferences;

// Define os botões
const int botaoA = 16;
const int botaoB = 17;

// Simulação sensores
const int sensor1 = 26;
const int sensor2 = 25;

// Define portas das luzes
const int verde = 14;
const int amarelo = 12;
const int vermelho = 13;

// Define as portas do analogico
const int eixoY = 4;
const int eixoX = 15;
const int clickAnal = 5;

// Define a porta do buzzer
const int buzzer = 32;

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
bool bloqueiaRedesenho = false;

// Ponteiro para o array de imagem ativa (desenho.h ou desenho2.h)
const uint16_t *imagemAtual = desenho;

// NOVO: Controle não-bloqueante do buzzer
unsigned long buzzerTempoInicio = 0;
unsigned long buzzerDuracao = 0;
bool buzzerAtivo = false;

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

// Variáveis do controle de informações das voltas
int voltas = 0;

unsigned long tempoUltimaVolta = 0;
unsigned long tempoVoltaAtual = 0;
unsigned long tempoMelhorVolta = 0;
float velocidadeMedia = 0;

// Estrutura para armazenar recordes
struct Recorde {
  String nome;
  unsigned long tempo;
};

// Array com os 3 melhores tempos
Recorde ranking[3] = {
  {"", 0},
  {"", 0},
  {"", 0}
};

// Variáveis para entrada de nome
String nomeEmEdicao = "";
int letraAtual = 0;
const char alfabeto[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
int totalLetras = 27;
int posicaoNovoRecorde = -1;
const int maxCaracteres = 10;

enum telaEstados
{
  tela_Menu,
  tela_Ajustes,
  tela_Procurado,
  tela_Corrida,
  tela_EntradaNome
};

telaEstados telaAtual = tela_Menu;

// Debounce do botão
bool botaoADeb = HIGH;
bool botaoBDeb = HIGH;

// Debounce do sensor
bool carroPassa1 = HIGH;
bool carroPassa2 = HIGH;

// ========== FUNÇÕES DE GERENCIAMENTO DE RANKING ==========

void salvarRanking() {
  preferences.begin("corrida", false);
  
  for (int i = 0; i < 3; i++) {
    String keyNome = "nome" + String(i);
    String keyTempo = "tempo" + String(i);
    
    preferences.putString(keyNome.c_str(), ranking[i].nome);
    preferences.putULong(keyTempo.c_str(), ranking[i].tempo);
  }
  
  preferences.end();
  Serial.println("Ranking salvo!");
}

void carregarRanking() {
  preferences.begin("corrida", true);
  
  for (int i = 0; i < 3; i++) {
    String keyNome = "nome" + String(i);
    String keyTempo = "tempo" + String(i);
    
    ranking[i].nome = preferences.getString(keyNome.c_str(), "");
    ranking[i].tempo = preferences.getULong(keyTempo.c_str(), 0);
  }
  
  preferences.end();
  Serial.println("Ranking carregado!");
}

void limparRanking() {
  preferences.begin("corrida", false);
  preferences.clear();
  preferences.end();
  
  for (int i = 0; i < 3; i++) {
    ranking[i].nome = "";
    ranking[i].tempo = 0;
  }
  
  Serial.println("Ranking limpo!");
}

int verificaNovoRecorde(unsigned long novoTempo) {
  if (novoTempo == 0) return -1;
  
  for (int i = 0; i < 3; i++) {
    if (ranking[i].tempo == 0 || novoTempo < ranking[i].tempo) {
      return i;
    }
  }
  
  return -1;
}

void inserirNoRanking(int posicao, String nome, unsigned long tempo) {
  if (posicao < 0 || posicao > 2) return;
  
  for (int i = 2; i > posicao; i--) {
    ranking[i] = ranking[i - 1];
  }
  
  ranking[posicao].nome = nome;
  ranking[posicao].tempo = tempo;
  
  salvarRanking();
}

// ========== FUNÇÕES DE TELA ==========

void corrida()
{
  int w = 160;
  int h = 128;

  int leitorSensor1 = digitalRead(sensor1);
  int leitorSensor2 = digitalRead(sensor2);

  if (leitorSensor1 == LOW && carroPassa1 == HIGH)
  {
    tempoVoltaAtual = millis();
  }
  else if (leitorSensor2 == LOW && carroPassa2 == HIGH)
  {
    voltas++;
    mudancaTela = true;
    tempoUltimaVolta = millis() - tempoVoltaAtual;
    velocidadeMedia = 30 / (tempoUltimaVolta / 1000.0);
    
    if (tempoMelhorVolta == 0)
    {
      tempoMelhorVolta = tempoUltimaVolta;
    }
    else if (tempoMelhorVolta > tempoUltimaVolta)
    {
      tempoMelhorVolta = tempoUltimaVolta;
    }
    
    if (tempoUltimaVolta > 0)
    {
      velocidadeMedia = 30 / (tempoUltimaVolta / 1000.0);
    }
    else
    {
      velocidadeMedia = 0.0f;
    }
    
    // Verifica se entra no ranking
    posicaoNovoRecorde = verificaNovoRecorde(tempoMelhorVolta);
  }

  telaBuffer.fillSprite(TFT_BLACK);
  telaBuffer.setCursor(5, 10);
  telaBuffer.setTextSize(2);
  telaBuffer.print("VOLTA ");
  telaBuffer.print(voltas);
  telaBuffer.setCursor(5, 70);
  telaBuffer.setTextSize(1);
  telaBuffer.print("Volta atual: ");
  telaBuffer.print(tempoUltimaVolta / 1000.0);
  telaBuffer.setCursor(5, 80);
  telaBuffer.print("Melhor volta: ");
  telaBuffer.print(tempoMelhorVolta / 1000.0);
  telaBuffer.setCursor(5, 90);
  telaBuffer.print("Velocidade: ");
  telaBuffer.print(velocidadeMedia);
  telaBuffer.print("cm/s");
  telaBuffer.pushSprite(0, 0);

  carroPassa1 = leitorSensor1;
  carroPassa2 = leitorSensor2;
}

void procurado() {
  int w = 160;
  int h = 128;
  
  telaBuffer.fillSprite(TFT_BLACK);
  telaBuffer.pushImage(0, 0, w, h, imagemAtual);
  
  telaBuffer.setCursor(5, 5);
  telaBuffer.setTextColor(TFT_WHITE);
  telaBuffer.setTextSize(1);
  telaBuffer.print("MAIS PROCURADOS");
  
  int posY = 30;
  uint16_t cores[3] = {TFT_YELLOW, 0xC618, TFT_ORANGE};
  const char* medalhas[3] = {"1o", "2o", "3o"};
  
  for (int i = 0; i < 3; i++) {
    telaBuffer.setTextColor(cores[i]);
    telaBuffer.setTextSize(1);
    telaBuffer.setCursor(5, posY);
    telaBuffer.print(medalhas[i]);
    
    if (ranking[i].tempo > 0 && ranking[i].nome.length() > 0) {
      telaBuffer.setCursor(25, posY);
      telaBuffer.setTextColor(TFT_WHITE);
      telaBuffer.print(ranking[i].nome);
      
      telaBuffer.setCursor(90, posY);
      telaBuffer.setTextColor(cores[i]);
      telaBuffer.print(ranking[i].tempo / 1000.0, 2);
      telaBuffer.print("s");
    } else {
      telaBuffer.setCursor(25, posY);
      telaBuffer.setTextColor(TFT_WHITE);
      telaBuffer.print("-------");
    }
    
    posY += 20;
  }
  
  telaBuffer.setCursor(5, 110);
  telaBuffer.setTextSize(1);
  telaBuffer.setTextColor(TFT_WHITE);
  telaBuffer.print("B:Voltar A:Limpar");
  
  telaBuffer.pushSprite(0, 0);
  mudancaTela = false;
}

void telaEntradaNome() {
  int w = 160;
  int h = 128;
  
  telaBuffer.fillSprite(TFT_BLACK);
  
  telaBuffer.setCursor(5, 5);
  telaBuffer.setTextColor(TFT_YELLOW);
  telaBuffer.setTextSize(2);
  
  if (posicaoNovoRecorde == 0) {
    telaBuffer.print("1o LUGAR!");
  } else if (posicaoNovoRecorde == 1) {
    telaBuffer.print("2o LUGAR!");
  } else {
    telaBuffer.print("3o LUGAR!");
  }
  
  telaBuffer.setCursor(5, 30);
  telaBuffer.setTextSize(1);
  telaBuffer.setTextColor(TFT_WHITE);
  telaBuffer.print("Digite seu nome:");
  
  telaBuffer.setCursor(5, 50);
  telaBuffer.setTextSize(2);
  telaBuffer.setTextColor(TFT_YELLOW);
  telaBuffer.print(nomeEmEdicao);
  
  if (millis() % 1000 < 500) {
    telaBuffer.print(alfabeto[letraAtual]);
  } else {
    telaBuffer.print("_");
  }
  
  telaBuffer.setCursor(5, 80);
  telaBuffer.setTextSize(1);
  telaBuffer.setTextColor(TFT_WHITE);
  telaBuffer.print("Joy: Trocar letra");
  telaBuffer.setCursor(5, 92);
  telaBuffer.print("A: Adicionar letra");
  telaBuffer.setCursor(5, 104);
  telaBuffer.print("B: Finalizar");
  
  telaBuffer.pushSprite(0, 0);
}

void atualizaBuzzer()
{
  if (buzzerAtivo && (millis() - buzzerTempoInicio >= buzzerDuracao))
  {
    noTone(buzzer);
    buzzerAtivo = false;
  }
}

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

void somNavegacao()
{
  tocarSom(1000, 50);
}

void somSelecao()
{
  tocarSom(1500, 100);
}

void somTrocaCor()
{
  tocarSom(2000, 200);
}

void somLargada()
{
  tocarSom(2500, 300);
}

void trocaDeItem()
{
  int w = 160;
  int h = 128;
  const char *menuItens[] = {"Iniciar", "Mais procurado", "Ajustes"};

  telaBuffer.fillSprite(TFT_BLACK);
  telaBuffer.pushImage(0, 0, w, h, imagemAtual);

  telaBuffer.setCursor(5, 10);
  telaBuffer.setTextColor(TFT_WHITE);
  telaBuffer.setTextSize(2);
  telaBuffer.print("Menu");

  if (imagemAtual == desenho)
  {
    telaBuffer.setCursor(115, 40);
    telaBuffer.setTextSize(1);
    telaBuffer.print("DETRAN");
  }

  int posY = 45;
  for (int i = 0; i < totalItens; i++)
  {
    if (i == opcaoSelecionada)
    {
      telaBuffer.setTextSize(1);
      telaBuffer.fillTriangle(2, posY, 2, posY + 6, 5, posY + 3, TFT_WHITE);
      telaBuffer.setTextColor(TFT_BLACK, TFT_YELLOW);
      telaBuffer.setCursor(10, posY);
      telaBuffer.print(menuItens[i]);
    }
    else
    {
      telaBuffer.setTextSize(1);
      telaBuffer.setTextColor(TFT_WHITE);
      telaBuffer.setCursor(10, posY);
      telaBuffer.print(menuItens[i]);
    }
    posY += 18;
  }

  telaBuffer.pushSprite(0, 0);
  mudancaTela = false;
}

void partidaContador()
{
  if (!sequenciaAtiva)
    return;

  switch (estadoAtual)
  {
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
    somTrocaCor();
    tempoDeInicio = millis();
    estadoAtual = 1;
    break;

  case 1:
    if (millis() - tempoDeInicio >= tempoVermelho)
    {
      digitalWrite(vermelho, HIGH);
      digitalWrite(amarelo, HIGH);
      digitalWrite(verde, LOW);
      telaBuffer.fillSprite(TFT_CYAN);
      telaBuffer.pushSprite(0, 0);
      somTrocaCor();
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
      telaBuffer.fillSprite(TFT_GREEN);
      telaBuffer.pushSprite(0, 0);
      somLargada();
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
      if (inversaoCor)
      {
        tft.invertDisplay(true);
      }
      sequenciaAtiva = false;

      voltas = 0;
      tempoMelhorVolta = 0;
      tempoVoltaAtual = 0;
      tempoUltimaVolta = 0;

      telaAtual = tela_Corrida;
      mudancaTela = true;
      bloqueiaRedesenho = false;

      return;
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

  telaBuffer.pushImage(0, 0, w, h, imagemAtual);

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
      if (i == 0)
        opcaoSelec = inversaoCor;
      if (i == 1)
        opcaoSelec = somSilenciado;
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

  pinMode(verde, OUTPUT);
  pinMode(amarelo, OUTPUT);
  pinMode(vermelho, OUTPUT);

  pinMode(sensor1, INPUT_PULLUP);
  pinMode(sensor2, INPUT_PULLUP);

  pinMode(botaoA, INPUT_PULLUP);
  pinMode(botaoB, INPUT_PULLUP);

  pinMode(eixoY, INPUT);
  pinMode(eixoX, INPUT);
  pinMode(clickAnal, INPUT_PULLUP);

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  noTone(buzzer);

  tft.init();
  tft.setRotation(5);

  telaBuffer.setColorDepth(8);
  telaBuffer.createSprite(160, 128);

  tft.fillScreen(TFT_BLACK);

  carregarRanking();

  trocaDeItem();

  delay(100);
  primeiraLeitura = false;
}

void loop()
{
  atualizaBuzzer();

  int leitorEixoX = analogRead(eixoY);
  int leitorEixoY = analogRead(eixoX);
  int leituraBotaoA = digitalRead(botaoA);
  int leituraBotaoB = digitalRead(botaoB);

  switch (telaAtual)
  {
  case tela_Menu:
  {
    if (!sequenciaAtiva)
    {
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
      else if (leitorEixoY > 1000 && leitorEixoY < 3800)
      {
        moverJoy = false;
      }
    }

    if (leituraBotaoA == LOW && botaoADeb == HIGH)
    {
      if (opcaoSelecionada == 0)
      {
        sequenciaAtiva = true;
        bloqueiaRedesenho = true;
        estadoAtual = 0;
        somSelecao();
        mudancaTela = false;
        break;
      }
      else if (opcaoSelecionada == 1)
      {
        telaAtual = tela_Procurado;
        mudancaTela = true;
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

    if (sequenciaAtiva)
    {
      partidaContador();

      if (telaAtual == tela_Corrida)
        break;
    }

    if (mudancaTela && telaAtual == tela_Menu && !sequenciaAtiva && !bloqueiaRedesenho)
    {
      trocaDeItem();
    }
    break;
  }
  
  case tela_Ajustes:
  {
    if (mudancaTela)
    {
      telaAjustes();
    }

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

        if (imagemAtual == desenho)
        {
          imagemAtual = desenho2;
        }
        else
        {
          imagemAtual = desenho;
        }

        Serial.println("Alterando icone do menu");
        mudancaTela = true;
      }
    }

    if (leituraBotaoB == LOW && botaoBDeb == HIGH)
    {
      somSelecao();
      telaAtual = tela_Menu;
      mudancaTela = true;
      Serial.println("Voltando para Menu");
    }

    botaoADeb = leituraBotaoA;
    botaoBDeb = leituraBotaoB;

    if (mudancaTela)
    {
      telaAjustes();
    }
    break;
  }
  
  case tela_Corrida:
  {
    corrida();
    
    if (leituraBotaoB == LOW && botaoBDeb == HIGH)
    {
      somSelecao();
      
      if (posicaoNovoRecorde >= 0) {
        telaAtual = tela_EntradaNome;
        nomeEmEdicao = "";
        letraAtual = 0;
      } else {
        telaAtual = tela_Menu;
      }
      
      mudancaTela = true;
      estadoAtual = 0;
    }
    
    botaoADeb = leituraBotaoA;
    botaoBDeb = leituraBotaoB;
    break;
  }
  
  case tela_Procurado:
  {
    if (mudancaTela) {
      procurado();
    }
    
    if (leituraBotaoB == LOW && botaoBDeb == HIGH) {
      somSelecao();
      telaAtual = tela_Menu;
      mudancaTela = true;
    }
    
    static unsigned long tempoAperto = 0;
    if (leituraBotaoA == LOW) {
      if (botaoADeb == HIGH) {
        tempoAperto = millis();
      }
      if (millis() - tempoAperto > 3000) {
        limparRanking();
        somSelecao();
        mudancaTela = true;
        tempoAperto = millis() + 10000;
      }
    }
    
    botaoADeb = leituraBotaoA;
    botaoBDeb = leituraBotaoB;
    break;
  }
  
  case tela_EntradaNome:
  {
    telaEntradaNome();
    
    if (leitorEixoY >= 4000 && !moverJoy) {
      letraAtual++;
      if (letraAtual >= totalLetras) letraAtual = 0;
      somNavegacao();
      moverJoy = true;
    }
    else if (leitorEixoY <= 500 && !moverJoy) {
      letraAtual--;
      if (letraAtual < 0) letraAtual = totalLetras - 1;
      somNavegacao();
      moverJoy = true;
    }
    else if (leitorEixoY > 1000 && leitorEixoY < 3800) {
      moverJoy = false;
    }
    
    if (leituraBotaoA == LOW && botaoADeb == HIGH) {
      if (nomeEmEdicao.length() < maxCaracteres) {
        nomeEmEdicao += alfabeto[letraAtual];
        letraAtual = 0;
        somSelecao();
      }
    }
    
    if (leituraBotaoB == LOW && botaoBDeb == HIGH) {
      if (nomeEmEdicao.length() > 0) {
        inserirNoRanking(posicaoNovoRecorde, nomeEmEdicao, tempoMelhorVolta);
        nomeEmEdicao = "";
        letraAtual = 0;
        posicaoNovoRecorde = -1;
        telaAtual = tela_Procurado;
        mudancaTela = true;
        somSelecao();
      }
    }
    
    botaoADeb = leituraBotaoA;
    botaoBDeb = leituraBotaoB;
    break;
  }
  }
}
