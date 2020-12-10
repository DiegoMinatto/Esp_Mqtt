# Leitura do sensor DHT11 a partir do esp8266 e utilizando o protocolo MQTT para a troca de dados e com uma interface em react requisitando dados de um backend NodeJS para exibir os resultados.

## Funções implementadas no esp8266

- Conexão com Wi-Fi
- Botão para reconectar com Wi-Fi caso a conexão seja perdida.
- Utilização da led na placa para informar o status da conexão Wi-Fi (Acesa: Conectado, Piscando Lento: Conectando, Piscando Rápido: Falha ao conectar)
- Comunicação utilizando o protocolo SSL juntamente com o MQTT para a publicação do valor da temperatura, implementando dois usuarios um para o ESP e outro para o seridor.
- Leitura da temperatura a partir do sensor DHT11

## Funções do servidor NodeJS utilizado como backend.

- Armazenar dados historicos referentes a temperatura.
- Efetuar a comunicação em tempo real utilizando WebSocket.
- Subscrição no tópico MQTT utilizando SSL para criptografação com o intuito de efetuar a leitura da temperatura.

## Funções da interface web

- Informar a temperatura em tempo real (atraso de 2 segundos) para o usuario juntamente com o momento da ultima utilização.
- Criação de um grafico com a média de temperatura das últimas 24 horas.

Bibilioteca utilizada para leitura do sensor: https://github.com/UncleRus/esp-idf-lib
