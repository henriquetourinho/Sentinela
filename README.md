# Sentinela 🛡️🇧🇷

**Sentinela** é o seu sistema de alarme inteligente com ESP32, agora mais completo e robusto. Protege seu ambiente com detecção de movimento, sirene, e notificações em tempo real no Telegram. Agora com controle triplo (Telegram, RF 433MHz e **botão físico**), combinando tecnologia e praticidade com aquele toque brasileiro.

Use o Sentinela para:

* Proteger sua casa, empresa ou qualquer espaço com um sistema versátil.
* Monitorar movimento com um sensor PIR de alta sensibilidade.
* Receber alertas instantâneos e detalhados diretamente no seu Telegram.
* Controlar seu alarme de três formas diferentes: app Telegram, controle remoto RF ou um simples botão.

---

## 📦 O que o Sentinela oferece

* 👁️ **Detecção de Movimento Precisa:** Utiliza um sensor PIR para identificar qualquer movimento na área protegida.
* 🔔 **Alerta Sonoro Imediato:** Aciona uma sirene (ou qualquer dispositivo de alerta) através de um módulo relé.
* 🕹️ **Controle Triplo:** Arma e desarma o sistema via app Telegram, controle remoto RF 433MHz ou um botão físico dedicado.
* 📲 **Comunicação Segura e Fluida:** Envia notificações e recebe comandos via Telegram, utilizando uma conexão Wi-Fi segura.
* 💾 **Logs Persistentes com Horário Real:** Graças ao sistema de arquivos LittleFS e à sincronização com servidores de tempo (NTP), cada evento é registrado com data e hora exatas.
* 📤 **Acesso aos Logs via Telegram:** Peça um relatório completo de eventos com o novo comando `/logs`.
* 🔄 **Gerenciamento de Status Claro:** Saiba a qualquer momento se o sistema está armado ou desarmado e se a sirene foi disparada.
* 🛡️ **Sistema Anti-Flood:** Evita o envio excessivo de mensagens repetidas no Telegram, garantindo uma comunicação limpa.
* 🔧 **Reconexão Automática:** O sistema monitora constantemente a conexão Wi-Fi e se reconecta automaticamente em caso de falha.

---

## 🚀 Por que escolher o Sentinela?

* **Fácil Instalação e Configuração:** Com instruções claras e um hardware simples, você monta seu sistema rapidamente.
* **Controle Centralizado e Remoto:** Gerencie tudo pelo seu celular ou pelos métodos de controle físico.
* **Feedback Instantâneo:** Alertas e confirmações de comando chegam em tempo real no seu Telegram.
* **Sistema Robusto e Confiável:** Ideal para qualquer ambiente, com código aberto e pronto para ser adaptado.
* **Código Aberto e Personalizável:** Sinta-se livre para adaptar e expandir o projeto para suas necessidades.

---

## ⚙️ O que você vai precisar

* **ESP32:** O cérebro do nosso sistema.
* **Sensor PIR:** Para detectar movimento.
* **Módulo Relé de 5V:** Para acionar a sirene com segurança.
* **Receptor RF 433MHz:** Para receber comandos do controle remoto.
* **Botão (Push Button / Táctil):** Para armar e desarmar o sistema fisicamente.
* **Rede Wi-Fi:** Com acesso à internet para o ESP32 se comunicar.
* **Bot Telegram e Chat ID:** Para receber as notificações e enviar comandos.

---

## 🛠️ Configuração rápida

No script `sentinela.cpp`, ajuste seus dados de Wi-Fi, Telegram e os códigos do seu controle RF na seção de configurações:

1.  **Credenciais de Rede e Telegram:**
    ```cpp
    const char* ssid = "SEU_SSID";
    const char* password = "SUA_SENHA_WIFI";

    #define BOT_TOKEN "SEU_BOT_TELEGRAM_TOKEN"
    #define CHAT_ID "SEU_CHAT_ID_TELEGRAM"
    ```

2.  **Códigos do Controle Remoto RF:**
    ```cpp
    const unsigned long CODE_ARM = 1234567;    // Código para ARMAR o sistema
    const unsigned long CODE_DISARM = 7654321; // Código para DESARMAR o sistema
    ```

---

## 🔌 Conexões de hardware

| Dispositivo | Pino ESP32 |
| :--- | :--- |
| Sensor PIR | 13 |
| Relé para Sirene | 12 |
| Receptor RF 433MHz | 14 |
| Botão Físico | 27 |

---

## 🎮 Comandos do Telegram disponíveis

| Comando | Descrição |
| :--- | :--- |
| `/armar` | Ativa a vigilância do sistema e o prepara para disparar. |
| `/desarmar` | Desativa o alarme e para a sirene, caso esteja tocando. |
| `/status` | Informa o estado atual: se o sistema está armado ou desarmado. |
| `/logs` | Envia o arquivo de log completo com os últimos eventos registrados. |

---

## 📋 Logs e alertas

* **Notificações Instantâneas:** Receba alertas no Telegram sempre que houver detecção de movimento ou quando o estado do sistema for alterado.
* **Origem do Comando Registrada:** O sistema informa se um comando veio do **Telegram**, do **Controle RF** ou do **Botão Físico**.
* **Timestamp Preciso:** Todos os logs são carimbados com data e hora exatas, graças à sincronização NTP, e salvos na memória interna do ESP32 (LittleFS).

---

## 🔮 Próximos passos para o Sentinela

* Integração com armazenamento em nuvem para logs históricos de longa duração.
* Painel web para monitoramento remoto e visualização em tempo real.
* Suporte multiusuário com permissões diferenciadas no Telegram.
* Autenticação avançada para controle RF (rolling code).
* Notificações via SMS e outros canais como fallback.

---

## ⚠️ Dicas importantes

* **Guarde seu token e chat ID do Telegram com segurança.** Eles são a chave para o seu sistema.
* **Garanta que o certificado root do Telegram no script esteja atualizado** para evitar falhas de conexão.
* **Teste o sistema exaustivamente** antes de implantá-lo em um ambiente real para garantir a estabilidade.

---

## 📞 Fale comigo!

Quer sugerir algo, reportar bugs ou só bater um papo sobre o projeto? Me chama no Telegram: [@henriquetourinho](https://t.me/henriquetourinho)