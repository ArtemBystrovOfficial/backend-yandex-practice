<!-- ABOUT THE PROJECT -->
## О проекте
Игровой веб сервер, представленный как дипломный проект после курса Yandex Practicum Backed.
### Создано с помощью
- C++20
- Boost (Beast, Asio, Log, Serialization, JSON)
- Catch2
- pqxx
## Способы сборки
`Docker`
`Из исходников`

### `Docker`
#### Установка и запуск
```bash
sudo docker build -t your_container .
```
Либо скачать и запустить с Docker
```bash
sudo docker run -e GAME_DB_URL=<pqxx:url> -d --network <your_db_network> -p 80:8080 7moon120/yandex_test_server_artem_a7b4
```
------
### `Из исходников`
#### Предварительные требования
`conan 1.*`
`cmake`
`c++20+`
#### Установка и запуск
```bash
mkdir build; cd build
conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11
cmake ..
cmake --build .
cd ..
export GAME_DB_URL=<pqxx:url>
./build/bin/game_server --help
```
### Конфигурация
./buid/bin/game_server
```bash
Allowed options:
  -h [ --help ]                     produce help message
  --state-file file                 make save/load after crash
  --save-state-period milliseconds  make save/load after selected milliseconds
  -t [ --tick-period ] milliseconds set tick period
  -c [ --config-file ] file         set config file path
  -w [ --www-root ] dir             set static files root
  --randomize-spawn-points          spawn dogs at random positions
```
data/config.json
```JSON
{
  "defaultDogSpeed": 3.0,
  "lootGeneratorConfig": {
    "period": 5.0,
    "probability": 0.5
  },
  "dogRetirementTime": 15.0,
  "maps": [
    {
      "dogSpeed": 4.0,
      "id": "map1",
      "name": "Map 1",
      "lootTypes": [
        {
          "name": "key",
          "file": "assets/key.obj",
          "type": "obj",
          "rotation": 90,
          "color" : "#338844",
          "scale": 0.03,
          "value": 10
        }
      ],
      "roads": [
        {
          "x0": 0, "y0": 0, "x1": 40
        }
      ],
      "buildings": [
        {
          "x": 5, "y": 5, "w": 30, "h": 20
        }
      ]
    }
  ]
}
```
```diff
! Опционально
```
Возможность настроить мониторинг ресурсов с помощью grafana
Запустить два сервиса через systemctl либо локально
```bash
ExecStart=/home/user/node_exporter
ExecStart=/bin/bash -c 'docker logs -f <server_container> | python3 /home/user/web_exporter.py'
```
`NodeExporter` можно скачать с официального репозитория, `WebExporter` в папке проекта
`Prometheus` из стандартного docker репозитория, `Grafana` также. Можно посмотреть схему взаимодействия контйнеров и сервисов внутри ВМ
![1](https://github.com/ArtemBystrovOfficial/backend-yandex-practice/assets/92841151/00ab00cc-0cfc-452e-8bec-7ec9f69fae22)
