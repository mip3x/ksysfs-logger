# ksysfs-logger

## Описание

Проект состоит из двух частей:

1. **Модуль ядра Linux `ksysfs-logger`**

После загрузки в ядро периодически (по таймеру) дописывает строки вида:

```
Hello from kernel module (N)
```

в файл:

```
/var/tmp/test_module/<filename>
```

где `N` - счётчик записей (1, 2, 3, ...).

2. **Пользовательская программа `sysfs-editor`**

Задаёт параметры модуля через **sysfs**:

- `filename` - имя файла (без `/`)
- `period_ms` - период записи в миллисекундах (программа принимает секунды и переводит в миллисекунды)

Также программа создаёт директорию `/var/tmp/test_module`, если её нет. Создавать директорию внутри модуля ядра не рекомендуется, поэтому директория создаётся из `userspace`-программы

## Структура проекта

```
├── kernel-module
│   ├── ksysfs-logger.c
│   └── Makefile
└── sysfs-editor
    ├── Makefile
    └── sysfs-editor.c
```

## Требования

- Linux kernel 6.12+
- Установлены заголовки ядра (kernel headers) для текущего ядра:
  - Debian/Ubuntu: `sudo apt install linux-headers-$(uname -r) build-essential`

- Права root для:
  - загрузки/выгрузки модуля (`insmod`, `rmmod`)
  - записи в sysfs (`/sys/kernel/...`)
  - создания `/var/tmp/test_module`

## Сборка модуля ядра

Перейти в папку модуля:

```sh
cd kernel_module
make
```

Удаление файлов компиляции и самого объектного файла:

```sh
make clean
```

## Загрузка и выгрузка модуля

Загрузка:

```sh
cd kernel_module
make load
```

Выгрузка:

```sh
cd kernel_module
make unload
```

Проверить, что модуль загружен:

```sh
lsmod | grep ksysfs_logger
```

Посмотреть сообщения ядра:

```sh
dmesg
```

## `sysfs` интерфейс

После загрузки модуля появляются файлы:

```
/sys/kernel/ksysfs_logger/filename
/sys/kernel/ksysfs_logger/period_ms
```

### Посмотреть текущие значения

```bash
cat /sys/kernel/ksysfs_logger/filename
cat /sys/kernel/ksysfs_logger/period_ms
```

### Изменить вручную

```bash
echo "log.txt" | sudo tee /sys/kernel/ksysfs_logger/filename
echo "2000" | sudo tee /sys/kernel/ksysfs_logger/period_ms
```

Ограничения:

- `filename` - **только имя файла**, без `/`
- `period_ms` - должен быть **не меньше 100 ms**

## Сборка пользовательской программы

Перейти в папку программы:

```sh
cd sysfs-editor
make
```

Очистка:

```sh
make clean
```

## Запуск `sysfs-editor`

Формат запуска:

```sh
sudo ./sysfs-editor <filename> <period_seconds>
```

Пример:

```sh
sudo ./sysfs-editor log.txt 5
```

Что делает эта команда:

- создаёт директорию `/var/tmp/test_module` (если отсутствует)
- записывает `filename=log.txt` в `/sys/kernel/ksysfs_logger/filename`
- записывает `period_ms=5000` в `/sys/kernel/ksysfs_logger/period_ms`

## Проверка результата

Просмотреть лог-файл:

```sh
tail -f /var/tmp/test_module/log.txt
```

Через каждые `period_seconds` секунд должны появляться новые строки:

```
Hello from kernel module (1)
Hello from kernel module (2)
Hello from kernel module (3)
...
```
