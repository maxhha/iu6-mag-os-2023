# Лабораторная работа по курсу Операционных Систем
## Задание

Распределенная система сортировки. Запускается на нескольких машинах программа-вычислитель, затем на одной из них запускается программа-терминал. Программа терминал и вычислители находят друг друга с помощью broadcast udp запроса. Затем вычислители подключаются к терминалу по tcp, получают свою часть для вычисления и возвращают результат терминалу.

Требования:
- массив входных данных нужно передать терминалу через стандартный поток ввода (stdin);
- параметры для всех программ задавать через аргументы командной строки;
- для сборки использовать Makefile;
- обработать ситуацию остановки вычислителя;
- какую функцию делает вычислитель взять на свое усмотрение (mergesort, вычисление интеграла и т.п.)

## Функция вычислителя

Была выбрана сортировка части массива. Вычислителю отдается массив, он сортирует его, отсортированный отдает терминалу, который с использовнием merge-сорта сливает все части в один массив и выводит в stdout.

## Как запустить

Перед запуском можно сгененрировать данные с использованием простого скрипта [generate_input.py](./generate_input.py):
```
$ python generate_input.py 100000000
..........
Saved in data/sample100M.txt
```

Проект делится на 2 подпроекта:
- `queen` (Королева) - программа-терминал
- `emmet` (муравей) - программа-вычислитель

В каждом имеется по `Makefile` и есть еще корневой `Makefile`, который позволяет собрать обе программы. Таким образом, чтобы скомпилировать обе программы достаточно вызвать `make` в корне проекта:

```
$ make
make -C emmet emmet.exe
make[1]: вход в каталог «/home/maxhha/Projects/os-task/emmet»
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic  -o out/main.o -c src/main.c
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic  -o out/receive_queen_addr.o -c src/receive_queen_addr.c
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic -o emmet.exe out/main.o out/receive_queen_addr.o
make[1]: выход из каталога «/home/maxhha/Projects/os-task/emmet»
cp ./emmet/emmet.exe emmet.exe
make -C queen queen.exe
make[1]: вход в каталог «/home/maxhha/Projects/os-task/queen»
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic  -o out/append_duties_from_stdin.o -c src/append_duties_from_stdin.c
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic  -o out/broadcast_queen_port.o -c src/broadcast_queen_port.c
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic  -o out/create_queen_socket.o -c src/create_queen_socket.c
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic  -o out/main.o -c src/main.c
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic  -o out/queen_state.o -c src/queen_state.c
gcc --std=gnu17 -Wall -Werror -Wfloat-equal -Wfloat-conversion -Wpedantic -o queen.exe out/append_duties_from_stdin.o out/broadcast_queen_port.o out/create_queen_socket.o out/main.o out/queen_state.o
make[1]: выход из каталога «/home/maxhha/Projects/os-task/queen»
cp ./queen/queen.exe queen.exe
```

В результате создано 2 файла в корне проекта: `queen.exe`, `emmet.exe`.

Запускать следует в следующем порядке:
- `emmet.exe` - следует запустить по 1 процессу на каждой машине-вычислителе;
- `queen.exe` - на вход принимает массив данных, выводит отсортированный;

Передав флаг `--help` в каждую из программ можно увидеть возможные аргументы к запуску:
```
$ ./emmet.exe --help
Usage: emmet.exe [OPTION...] [-p PORT] [-w SECONDS]
Worker of the colony

  -a, --arms=THREADS         How many threads emmet can handle simultaneously
                             (default: 6)
  -p, --port=PORT            Port to listen for the queen (default: 12345)
  -w, --wait=SECONDS         How long should wait for the queen (default: 30)
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

$ ./queen.exe --help
Usage: queen.exe [OPTION...] [-a ADDR] [-p PORT] [-w SECONDS]
Queen of the colony

  -a, --address=ADDR         Broadcast address of the colony (default:
                             255.255.255.255)
  -d, --duty_max_size=NUMBER Max amount of work for one emmet duty (default:
                             2048)
  -p, --port=PORT            Port of the colony (default: 12345)
  -w, --wait=SECONDS         How long should wait for the emmets (default: 10)
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```

## Пример локального запуска

В корне проекта:
```bash
make
python generate_input.py 100000
./emmet.exe &  # запустили процесс в фоне
cat data/sample100K.txt | ./queen.exe > ./out/result.txt
```

## Запуск в docker

В корне проекта:
```bash
python generate_input.py 100_000_000
INPUT_FILE=sample100M.txt docker compose up --build --scale emmet=3
# Результат будет сохранен в ./out/result.txt
```
