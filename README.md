# vPlatform: I2C Device for QEMU

Данный проект представляет собой реализацию кастомного I2C-устройства для эмулятора QEMU в рамках технического задания.

## Состав проекта

*   **`vplatform_monitor.py`** — Python-скрипт для мониторинга регистра устройства в реальном времени через QMP (QEMU Machine Protocol).
*   **`REPORT.md`** — Подробный отчет о выполнении задания, включая ответы на теоретический тест.
*   **`solution/`** — Исходные коды реализации:
    *   `hw/misc/vplatform_i2c.c` — C-код I2C-устройства.
    *   `*.modified` — Файлы конфигурации QEMU (`Kconfig`, `meson.build`), модифицированные для поддержки устройства.
*   **`vPlatform.docx`** — Оригинальное техническое задание.

## Описание устройства

Устройство `vplatform-i2c` — это простое I2C-slave устройство с одним 8-битным регистром. 
Оно поддерживает:
1.  **I2C Read/Write**: Чтение и запись регистра из гостевой системы (Linux).
2.  **QOM Property**: Регистр доступен как свойство объекта QEMU, что позволяет считывать его через QMP без остановки эмуляции.

## Инструкция по сборке и запуску

### 1. Подготовка QEMU
Склонируйте исходники QEMU и перенесите файлы из папки `solution` в соответствующие директории `qemu/hw/misc/` и `qemu/configs/devices/arm-softmmu/`.

### 2. Сборка
```bash
mkdir build && cd build
../configure --target-list=arm-softmmu --disable-docs
make -j$(nproc) qemu-system-arm
```

### 3. Запуск эмуляции
```bash
./qemu-system-arm \
  -M versatilepb \
  -m 256M \
  -kernel path/to/vmlinuz \
  -initrd path/to/initrd.gz \
  -append "console=ttyAMA0" \
  -nographic \
  -device vplatform-i2c,bus=i2c,id=vdev0,address=0x50 \
  -qmp unix:qmp.sock,server,nowait
```

### 4. Мониторинг
В отдельном терминале запустите скрипт мониторинга:
```bash
python3 vplatform_monitor.py
```

При изменении регистра изнутри Linux (например, командой `i2cset`), скрипт мгновенно отобразит новое значение.
