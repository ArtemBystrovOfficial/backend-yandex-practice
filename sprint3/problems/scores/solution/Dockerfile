# Создать образ на основе базового слоя gcc (там будет ОС и сам компилятор).
# 11.3 — используемая версия gcc.
FROM gcc:11.3 as build

# Выполнить установку зависимостей внутри контейнера.
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.* 

# Скопировать файлы проекта внутрь контейнера

RUN mkdir /app
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11

COPY ./tests /app/tests 
COPY ./src /app/src
COPY ./data /app/data
COPY CMakeLists.txt /app/
COPY ./static /app/static


RUN cd /app/build && cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . -j4 

#####################

FROM ubuntu:22.04 as run

RUN groupadd -r www && useradd -r -g www www 
USER www

COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

ENTRYPOINT ["/app/game_server", "-c", "/app/data/config.json" , "-w", "/app/static", "-t", "50"] 