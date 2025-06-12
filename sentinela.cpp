/*********************************************************************************
 * Projeto: Sentinela - Sistema de Alarme Inteligente com ESP32
 * Autor:   Carlos Henrique Tourinho Santana
 * GitHub:  https://github.com/henriquetourinho/sentinela
 * Versão:  1.0
 * Data da Criação: 12 de junho de 2025
 *
 * Descrição:
 * Este firmware implementa um sistema de alarme completo e inteligente usando um
 * ESP32. Ele monitora um ambiente com um sensor de movimento (PIR), aciona uma
 * sirene via relé e envia notificações em tempo real para um chat do Telegram.
 * O controle do sistema (armar/desarmar) é versátil, podendo ser feito por:
 * 1. Comandos no aplicativo Telegram (`/armar`, `/desarmar`).
 * 2. Um controle remoto RF 433MHz.
 * 3. Um botão físico conectado diretamente ao ESP32.
 *
 * Funcionalidades Adicionais:
 * - Reconexão automática ao Wi-Fi com monitoramento de status.
 * - Sistema de logs de eventos persistente, salvo no sistema de arquivos LittleFS.
 * - Sincronização de horário com servidores NTP para timestamps precisos nos logs.
 * - Prevenção de "flood" (envio excessivo) de mensagens no Telegram.
 * - Lógica de "debounce" para o botão físico, evitando acionamentos múltiplos.
 *********************************************************************************/


// =================================================================================
// --- INCLUSÃO DE BIBLIOTECAS ---
// =================================================================================
#include <WiFi.h>                   // Para gerenciamento da conexão Wi-Fi.
#include <WiFiClientSecure.h>       // Para criar uma conexão segura (HTTPS) para o Telegram.
#include <UniversalTelegramBot.h>   // Biblioteca principal para interagir com a API do Telegram.
#include <RCSwitch.h>               // Para receber sinais de rádio frequência (RF 433MHz).
#include <LittleFS.h>               // Para criar um sistema de arquivos e salvar logs.
#include <time.h>                   // Para obter o tempo de servidores NTP e gerar timestamps.


// =================================================================================
// --- CONFIGURAÇÕES GERAIS E CREDENCIAIS (ALTERE AQUI) ---
// =================================================================================

// --- Configurações de Rede Wi-Fi ---
const char* ssid = "SEU_SSID";          // Nome da sua rede Wi-Fi.
const char* password = "SUA_SENHA_WIFI"; // Senha da sua rede Wi-Fi.

// --- Credenciais do Bot do Telegram ---
#define BOT_TOKEN "SEU_BOT_TELEGRAM_TOKEN" // Token do seu Bot, obtido com o @BotFather.
#define CHAT_ID "SEU_CHAT_ID_TELEGRAM"     // ID do chat para onde as mensagens serão enviadas.

// --- Mapeamento de Pinos do Hardware ---
const int PIR_PIN = 13;           // Pino onde o sensor de movimento PIR está conectado.
const int RELAY_PIN = 12;         // Pino conectado ao módulo relé que aciona a sirene.
const int RF_RECEIVER_PIN = 14;   // Pino de dados (DATA) do receptor RF 433MHz.
const int BUTTON_PIN = 27;        // Pino para o botão físico de armar/desarmar.

// --- Códigos do Controle Remoto RF ---
// Configure aqui os códigos que o seu controle remoto envia.
const unsigned long CODE_ARM = 1234567;    // Código para ARMAR o sistema.
const unsigned long CODE_DISARM = 7654321; // Código para DESARMAR o sistema.


// =================================================================================
// --- VARIÁVEIS GLOBAIS DE CONTROLE DO SISTEMA ---
// =================================================================================

// --- Instâncias de Objetos ---
WiFiClientSecure client;                      // Cliente seguro para a conexão HTTPS com o Telegram.
UniversalTelegramBot bot(BOT_TOKEN, client);  // Objeto do Bot, que gerencia a comunicação.
RCSwitch rfReceiver = RCSwitch();             // Objeto para gerenciar o receptor RF.

// --- Flags de Estado ---
bool sistemaAtivo = false;     // `true` se o alarme estiver armado, `false` se desarmado.
bool alarmeDisparado = false;  // `true` se a sirene estiver tocando.

// --- Controle de Tempo e Conexão ---
unsigned long lastMsgTime = 0;                  // Armazena o tempo do último polling de mensagens no Telegram.
const long msgInterval = 3000;                  // Intervalo (em ms) para checar novas mensagens (evita flood).
unsigned long lastWiFiCheck = 0;                // Armazena o tempo da última verificação de Wi-Fi.
const long wifiCheckInterval = 10000;           // Intervalo (em ms) para verificar se o Wi-Fi ainda está conectado.
bool wifiConectadoAnterior = false;             // Flag para detectar transições no status do Wi-Fi (queda/retorno).

// --- Arquivo de Log ---
const char* LOG_FILE = "/log_sentinela.txt";    // Nome do arquivo onde os logs serão salvos no LittleFS.


// =================================================================================
// --- CERTIFICADO DE SEGURANÇA DO TELEGRAM ---
// =================================================================================
// [IMPORTANTE!] O ESP32 precisa deste certificado para verificar a autenticidade
// do servidor do Telegram. Sem um certificado válido, a conexão segura (HTTPS) falhará.
// Este certificado pode expirar. Se a comunicação com o Telegram parar,
// obtenha um novo certificado para "api.telegram.org".
const char TELEGRAM_CERTIFICATE_ROOT[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIID... (COLE SEU CERTIFICADO ROOT CA VÁLIDO AQUI)
-----END CERTIFICATE-----
)EOF";


// =================================================================================
// --- FUNÇÃO SETUP: Executada uma vez na inicialização do ESP32 ---
// =================================================================================
void setup() {
  // Inicia a comunicação serial para debug via monitor serial.
  Serial.begin(115200);

  // Configura os pinos de hardware.
  pinMode(PIR_PIN, INPUT);          // Pino do sensor PIR como entrada.
  pinMode(RELAY_PIN, OUTPUT);       // Pino do relé como saída.
  digitalWrite(RELAY_PIN, LOW);     // Garante que a sirene comece desligada.
  pinMode(BUTTON_PIN, INPUT_PULLUP);// Pino do botão como entrada com resistor de pull-up interno.

  // Inicializa o sistema de arquivos LittleFS.
  if(!LittleFS.begin()){
    Serial.println("Erro crítico ao iniciar o LittleFS.");
  }

  // Ativa o receptor de RF no pino configurado.
  rfReceiver.enableReceive(RF_RECEIVER_PIN);

  // Tenta conectar ao Wi-Fi. Esta função é bloqueante na primeira execução.
  conectaWiFi();

  // Sincroniza o relógio interno do ESP32 com um servidor de tempo na internet (NTP).
  // GMT-3 (fuso de Brasília), 0 para horário de verão, "pool.ntp.org" é o servidor.
  // Essencial para que os timestamps nos logs estejam corretos.
  configTime(-3 * 3600, 0, "pool.ntp.org");

  // Associa o certificado de segurança ao cliente Wi-Fi.
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  // Registra o primeiro evento no log.
  logEvento("Sistema iniciado e configurado.");

  Serial.println("Setup concluído. Sentinela operacional.");
}


// =================================================================================
// --- FUNÇÃO LOOP: Executada repetidamente após o setup ---
// =================================================================================
void loop() {
  // Funções de verificação contínua (polling).
  checarWiFi();      // Monitora e tenta reconectar o Wi-Fi se necessário.
  checarTelegram();  // Verifica se há novos comandos via Telegram.
  checarRF();        // Verifica se há novos comandos via controle RF.
  checarBotao();     // Verifica se o botão físico foi pressionado.

  // Lógica principal de detecção de movimento.
  // Se o sistema está ativo (armado), o alarme ainda não foi disparado, e o sensor PIR detecta movimento...
  if (sistemaAtivo && !alarmeDisparado && digitalRead(PIR_PIN) == HIGH) {
    logEvento("Movimento detectado, disparando alarme");
    dispararAlarme(); // ...então, dispara o alarme.
  }
}


// =================================================================================
// --- FUNÇÕES DE VERIFICAÇÃO E CONTROLE ---
// =================================================================================

/**
 * @brief Verifica o status da conexão Wi-Fi em intervalos regulares.
 */
void checarWiFi(){
  if(millis() - lastWiFiCheck > wifiCheckInterval){
    lastWiFiCheck = millis(); // Reseta o contador de tempo.

    if(WiFi.status() != WL_CONNECTED){
      if(wifiConectadoAnterior){ // Se estava conectado e caiu...
        logEvento("Conexão Wi-Fi perdida.");
      }
      wifiConectadoAnterior = false;
      conectaWiFi(); // Tenta reconectar.
    } else {
      if(!wifiConectadoAnterior){ // Se estava desconectado e conseguiu reconectar...
        logEvento("Conexão Wi-Fi restabelecida.");
        bot.sendMessage(CHAT_ID, "✅ Sentinela: Conexão Wi-Fi restabelecida!", "");
      }
      wifiConectadoAnterior = true;
    }
  }
}

/**
 * @brief Tenta se conectar à rede Wi-Fi configurada.
 */
void conectaWiFi(){
  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    Serial.print("Conectando ao WiFi");
    unsigned long startAttemptTime = millis();
    // Tenta por 15 segundos antes de desistir temporariamente.
    while(WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000){
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if(WiFi.status() == WL_CONNECTED){
      Serial.println("WiFi conectado com sucesso.");
      logEvento("WiFi conectado.");
      wifiConectadoAnterior = true;
    } else {
      Serial.println("Falha ao conectar no WiFi.");
      logEvento("Falha na conexão WiFi.");
    }
  }
}

/**
 * @brief Verifica se há novas mensagens do Telegram.
 */
void checarTelegram(){
  // Só executa se o WiFi estiver conectado e o intervalo de tempo tiver passado.
  if (millis() - lastMsgTime > msgInterval && WiFi.status() == WL_CONNECTED) {
    // Pede ao bot por novas mensagens.
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    // Itera sobre cada nova mensagem recebida.
    for (int i = 0; i < numNewMessages; i++) {
      handleNewMessage(bot.messages[i]);
    }
    lastMsgTime = millis(); // Reseta o contador.
  }
}

/**
 * @brief Verifica se um sinal de RF foi recebido.
 */
void checarRF(){
  if(rfReceiver.available()){ // Se um código foi decodificado...
    unsigned long value = rfReceiver.getReceivedValue();
    Serial.printf("Sinal RF recebido: %lu\n", value);
    handleRF(value); // Processa o código.
    rfReceiver.resetAvailable(); // Prepara para receber o próximo.
  }
}

/**
 * @brief Verifica se o botão físico foi pressionado, com lógica de debounce.
 */
void checarBotao(){
  static bool ultimoEstado = HIGH;
  static unsigned long ultimoDebounce = 0;
  const unsigned long debounceDelay = 50; // Tempo (em ms) para ignorar ruídos no botão.

  int leitura = digitalRead(BUTTON_PIN);
  if(leitura != ultimoEstado){ // Se o estado mudou...
    ultimoDebounce = millis(); // ...reseta o contador do debounce.
  }

  // Se o estado permaneceu estável pelo tempo de debounce...
  if((millis() - ultimoDebounce) > debounceDelay){
    // E se a mudança foi de HIGH para LOW (botão foi pressionado)...
    if(leitura == LOW && ultimoEstado == HIGH){
      // Alterna o estado do sistema.
      if(sistemaAtivo){
        desarmarSistema("Botão físico");
      } else {
        armarSistema("Botão físico");
      }
    }
  }
  ultimoEstado = leitura; // Salva o estado atual para a próxima iteração.
}


// =================================================================================
// --- FUNÇÕES DE LÓGICA DO ALARME ---
// =================================================================================

/**
 * @brief Ativa a sirene e notifica o usuário via Telegram.
 */
void dispararAlarme(){
  digitalWrite(RELAY_PIN, HIGH); // Liga o relé, acionando a sirene.
  alarmeDisparado = true;        // Atualiza o status do sistema.
  bot.sendMessage(CHAT_ID, "⚠️ ALERTA! Movimento detectado! Sirene disparada!", "");
  logEvento("Alarme efetivamente disparado (sirene + notificação).");
}

/**
 * @brief Desarma o sistema, desliga a sirene e notifica o usuário.
 * @param origem A fonte do comando (ex: "Telegram", "Controle RF").
 */
void desarmarSistema(String origem){
  digitalWrite(RELAY_PIN, LOW); // Desliga o relé, parando a sirene.
  alarmeDisparado = false;
  sistemaAtivo = false;
  String msg = "✅ Sistema DESARMADO com sucesso pela origem: " + origem;
  bot.sendMessage(CHAT_ID, msg.c_str(), "");
  logEvento(msg);
}

/**
 * @brief Arma o sistema e notifica o usuário.
 * @param origem A fonte do comando (ex: "Telegram", "Controle RF").
 */
void armarSistema(String origem){
  if(!sistemaAtivo){ // Só arma se já não estiver armado.
    sistemaAtivo = true;
    alarmeDisparado = false; // Garante que o status de alarme seja resetado.
    String msg = "🔒 Sistema ARMADO com sucesso pela origem: " + origem;
    bot.sendMessage(CHAT_ID, msg.c_str(), "");
    logEvento(msg);
  } else {
    bot.sendMessage(CHAT_ID, "ℹ️ O sistema já se encontra armado.", "");
  }
}


// =================================================================================
// --- FUNÇÕES DE PROCESSAMENTO (HANDLERS) ---
// =================================================================================

/**
 * @brief Processa os comandos recebidos via Telegram.
 * @param msg O objeto da mensagem do Telegram.
 */
void handleNewMessage(TelegramMessage msg){
  String text = msg.text;
  Serial.printf("Comando recebido do Telegram: %s\n", text.c_str());

  if(text == "/armar"){
    armarSistema("Telegram");
  } else if(text == "/desarmar"){
    desarmarSistema("Telegram");
  } else if(text == "/status"){
    String status = sistemaAtivo ? "ARMADO" : "DESARMADO";
    String alarme = alarmeDisparado ? "SIM" : "NÃO";
    String resp = "📊 *Status do Sentinela*\n\n*Sistema:* " + status + "\n*Sirene Disparada:* " + alarme;
    bot.sendMessage(CHAT_ID, resp, "Markdown");
  } else if(text == "/logs"){
    enviarLogsTelegram();
  } else {
    bot.sendMessage(CHAT_ID, "Comando não reconhecido. Use /armar, /desarmar, /status ou /logs.", "");
  }
}

/**
 * @brief Processa os códigos recebidos via RF.
 * @param code O código numérico recebido.
 */
void handleRF(unsigned long code){
  if(code == CODE_ARM){
    armarSistema("Controle RF");
  } else if(code == CODE_DISARM){
    desarmarSistema("Controle RF");
  } else {
    String logMsg = "Código RF desconhecido recebido: " + String(code);
    logEvento(logMsg.c_str());
  }
}


// =================================================================================
// --- FUNÇÕES DE LOG E TIMESTAMP ---
// =================================================================================

/**
 * @brief Salva uma mensagem de evento no arquivo de log e no monitor serial.
 * @param msg A mensagem a ser registrada.
 */
void logEvento(const String& msg){
  String logMsg = "[" + timestamp() + "] " + msg + "\n";
  Serial.print(logMsg);

  // Abre o arquivo de log em modo "append" (adiciona ao final).
  File f = LittleFS.open(LOG_FILE, "a");
  if(!f){
    Serial.println("Erro ao abrir arquivo de log para escrita.");
    return;
  }
  f.print(logMsg);
  f.close(); // Fecha o arquivo para salvar as alterações.
}

/**
 * @brief Gera uma string de timestamp no formato AAAA-MM-DD HH:MM:SS.
 * @return O timestamp formatado ou uma mensagem de erro se o tempo não estiver sincronizado.
 */
String timestamp(){
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char buffer[30];
  // Checa se o ano é válido (maior que 2020), indicando que o NTP sincronizou.
  if(timeinfo->tm_year > (2020 - 1900)){
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(buffer);
  } else {
    return String("sincronizando relogio...");
  }
}

/**
 * @brief Envia o arquivo de log para o chat do Telegram.
 */
void enviarLogsTelegram(){
  File f = LittleFS.open(LOG_FILE, "r");
  if(!f){
    bot.sendMessage(CHAT_ID, "Erro: Não foi possível encontrar o arquivo de log.", "");
    return;
  }
  if(f.size() == 0){
    bot.sendMessage(CHAT_ID, "O arquivo de log está vazio.", "");
    f.close();
    return;
  }

  // Envia o arquivo como um documento. Isso evita o limite de caracteres
  // de uma mensagem normal e melhora a formatação.
  bot.sendDocument(CHAT_ID, f, "log_sentinela.txt");
  f.close();
}