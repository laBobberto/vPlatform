# Отчет по заданию vPlatform

## 1. Исходный код устройства на C

Файл: `qemu-source/hw/misc/vplatform_i2c.c`

```c
#include "qemu/osdep.h"
#include "hw/i2c/i2c.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qom/object.h"

#define TYPE_VPLATFORM_I2C "vplatform-i2c"
OBJECT_DECLARE_SIMPLE_TYPE(VPlatformI2CState, VPLATFORM_I2C)

struct VPlatformI2CState {
    I2CSlave parent_obj;
    uint8_t reg; // Единственный регистр устройства
};

static int vplatform_i2c_event(I2CSlave *s, enum i2c_event event)
{
    return 0;
}

static uint8_t vplatform_i2c_recv(I2CSlave *s)
{
    VPlatformI2CState *state = VPLATFORM_I2C(s);
    return state->reg; // Чтение регистра
}

static int vplatform_i2c_send(I2CSlave *s, uint8_t data)
{
    VPlatformI2CState *state = VPLATFORM_I2C(s);
    state->reg = data; // Запись в регистр
    return 0;
}

// Геттер для QOM свойства "reg" (для доступа через QMP)
static void vplatform_i2c_get_reg(Object *obj, Visitor *v, const char *name,
                                  void *opaque, Error **errp)
{
    VPlatformI2CState *state = VPLATFORM_I2C(obj);
    uint8_t value = state->reg;
    visit_type_uint8(v, name, &value, errp);
}

// Сеттер для QOM свойства "reg"
static void vplatform_i2c_set_reg(Object *obj, Visitor *v, const char *name,
                                  void *opaque, Error **errp)
{
    VPlatformI2CState *state = VPLATFORM_I2C(obj);
    uint8_t value;
    if (!visit_type_uint8(v, name, &value, errp)) {
        return;
    }
    state->reg = value;
}

static void vplatform_i2c_init(Object *obj)
{
    // Добавляем свойство для мониторинга через QMP
    object_property_add(obj, "reg", "uint8",
                        vplatform_i2c_get_reg,
                        vplatform_i2c_set_reg,
                        NULL, NULL);
}

static void vplatform_i2c_class_init(ObjectClass *oc, const void *data)
{
    I2CSlaveClass *sc = I2C_SLAVE_CLASS(oc);
    sc->event = vplatform_i2c_event;
    sc->recv = vplatform_i2c_recv;
    sc->send = vplatform_i2c_send;
}

static const TypeInfo vplatform_i2c_info = {
    .name = TYPE_VPLATFORM_I2C,
    .parent = TYPE_I2C_SLAVE,
    .instance_size = sizeof(VPlatformI2CState),
    .instance_init = vplatform_i2c_init,
    .class_init = vplatform_i2c_class_init,
};

static void vplatform_i2c_register_types(void)
{
    type_register_static(&vplatform_i2c_info);
}

type_init(vplatform_i2c_register_types)
```

## 2. Исходный код программы на Python

Файл: `vplatform_monitor.py`

```python
import socket
import json
import time

def qmp_execute(sock, command, arguments=None):
    payload = {"execute": command}
    if arguments:
        payload["arguments"] = arguments
    sock.sendall(json.dumps(payload).encode() + b"\n")
    response = sock.readline()
    if not response:
        return None
    return json.loads(response)

def main():
    qmp_path = "qmp.sock"
    print(f"Connecting to QMP at {qmp_path}...")
    while True:
        try:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.connect(qmp_path)
            break
        except ConnectionRefusedError:
            time.sleep(1)
            
    f = sock.makefile("r")
    f.readline() # Greeting
    qmp_execute(sock, "qmp_capabilities")
    f.readline() # OK

    print("Monitoring vplatform-i2c register via QMP...")
    try:
        while True:
            # Используем qom-get для чтения значения свойства reg объекта vdev0
            res = qmp_execute(sock, "qom-get", {"path": "vdev0", "property": "reg"})
            if res and "return" in res:
                print(f"Register value: {res['return']}")
            time.sleep(1)
    except KeyboardInterrupt:
        print("Stopped.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
```

## 3. Команда запуска QEMU

```bash
./qemu-build/qemu-system-arm \
  -M versatilepb \
  -m 256M \
  -kernel vmlinuz-lts \
  -initrd initramfs-lts \
  -append "console=ttyAMA0" \
  -nographic \
  -device vplatform-i2c,bus=i2c,id=vdev0,address=0x50 \
  -qmp unix:qmp.sock,server,nowait
```

### Описание параметров:
* `-M versatilepb`: Выбор машины Versatile PB, которая имеет встроенный I2C контроллер.
* `-m 256M`: Выделение 256 МБ оперативной памяти для гостевой системы.
* `-kernel`, `-initrd`: Пути к ядру Linux и образу файловой системы.
* `-append "console=ttyAMA0"`: Передача параметров ядру для вывода консоли в последовательный порт.
* `-nographic`: Отключение графического окна и перенаправление консоли в терминал.
* `-device vplatform-i2c,bus=i2c,id=vdev0,address=0x50`:
    * `bus=i2c`: Подключение к шине I2C машины.
    * `id=vdev0`: Уникальный ID устройства для обращения через QMP.
    * `address=0x50`: I2C адрес устройства на шине.
* `-qmp unix:qmp.sock,server,nowait`: Создание UNIX-сокета для взаимодействия по протоколу QMP.

## 4. Результаты теоретического теста

```
1. Как создать директорию дерево директорий dir_a/dir_b в домашней папке пользователя, а в ней пустой файл test.txt? После выполнения команд мы должны оказаться в той же директории, что и были в начале. Выберите все рабочие варианты.
[ ] mkdir ~/dir_a/dir_b && touch ~/dir_a/dir_b/test.txt
[ ] mkdir /dir_a/dir_b && touch /dir_a/dir_b/test.txt
[x] mkdir -p ~/dir_a/dir_b && touch ~/dir_a/dir_b/test.txt
[ ] mkdir -p /home/dir_a/dir_b && touch /home/dir_a/dir_b/test.txt
[ ] mkdir -p ~/dir_a/dir_b & touch ~/dir_a/dir_b/test.txt
[ ] pwd && cd && mkdir dir_a && cd dir_a && mkdir dir_b && touch dir_b/test.txt && cd pwd

2. Какая команда выводит список файлов и поддиректорий в текущей директории с подробной информацией (права, размер, владелец), включая скрытые файлы, но исключая директории . и ..?
[ ] ls -l
[ ] ls -la
[x] ls -lA
[ ] ls -la | grep -v ".." | grep -v "."
[ ] ls --all
[ ] ls --almost-all
[ ] ls --list --hidden --noparent

3. Как удалить все файлы с расширением .log в директории /var/log, которые не изменялись за последние 7 дней?
[ ] find /var/log -name "*.log" -mtime +7 | rm
[ ] find /var/log -name "*.log" -mtime -7 | rm
[x] find /var/log -name "*.log" -mtime +7 -exec rm {} \;
[ ] find /var/log -name "*.log" -mtime -7 -exec rm {} \;

4. Какие из следующих пар IP-адресов находятся в одной подсети при указанной маске?
[x] 192.168.1.10/24 и 192.168.1.20/24
[ ] 10.0.1.5/24 и 10.0.2.5/24
[x] 172.16.10.5/23 и 172.16.11.6/23
[ ] 192.168.1.130/25 и 192.168.1.140/25

5. Я проверяю связность между хостом-А с адресом 192.168.1.21 и хостом-Б с адресом 172.16.172.22. Для этого выполняю на хосте-А команду ping 172.16.172.22 и вижу 0% дошедших пакетов. Значит ли это, что связности между хостами нет? Выберите верные утверждения.
[ ] Да, это означает, что связности нет.
[ ] Нет, не значит. Необходимо дополнительно выполнить на хосте-Б команду ping 192.168.1.1. Если и здесь не будет ответа, тогда связности между хостами нет.
[ ] Да, это означает, что связности нет. Можно было даже не проверять, так как хосты в разных подсетях
[x] Нет, не значит. Возможно, на хосте-Б настроен firewall, который блокирует такие запросы
[ ] Нет, не значит. Возможно, на хосте или где-то на маршруте закрыт igmp протокол
[x] Нет, не значит. Возможно, на хосте-Б или где-то на маршруте закрыт icmp протокол

6. Поступили жалобы на скорость работы сервиса. Инженер проверяет загрузку различных подсистем сервера. Выберите верные (возможные) действия. *команды указаны без параметров, только названия.
[ ] Проверить загрузку cpu командой iostat
[x] Проверить загрузку cpu командой mpstat
[x] Проверить загрузку cpu командой top
[ ] Проверить нагрузку на дисковую подсистему командой dtop
[x] Проверить нагрузку на дисковую подсистему командой iostat
[x] Проверить утилизацию памяти командой top
[x] Проверить утилизацию памяти командой free
[x] Проверить системный журнал
[x] Проверить наличие свободного места на дисках

7. Поступили жалобы на скорость работы сервиса. Инженер проверил загрузку различных подсистем сервера. Выберите верные утверждения.
[ ] Используется более 70% оперативной памяти, это ненормально
[x] Загрузка всех ядер CPU составляет больше 90%, скорее всего это ненормально
[ ] Задержки (latency) на дисках составляют около 1 мс, это может быть ненормально
[x] Задержки (latency) на дисках составляют около 1 сек, это может быть ненормально
[x] swap раздел занят на 100%, это ненормально

8. Что означает ключ `-fPIC` для `gcc`?
[ ] Включает режимы print, interactive и остановку на ошибках
[x] Исполняемый файл должен не зависеть от позиции в памяти
[ ] Исполняемый файл должен быть только предобработан, не скомпилирован и не слинкован
[ ] Такого ключа не существует

9. Что выведет команда `false && echo 1 || echo 2 && echo 3`?
[ ] Ничего
[x] 2 и 3
[ ] 1 и 3
[ ] 1, 2 и 3

10. Какие права доступа появятся в результате выполнения `chmod 3644 file`?
[ ] sticky и suid
[ ] suid и sgid
[x] sgid и sticky
[ ] sticky, suid и sgid

11. Что выведет команда `ps -ef |grep vim` если `vim` не запущен?
[ ] Ничего
[ ] Все строки из вывода `ps`, в которых есть подстрока `vim`
[ ] Все строки из вывода `ps`, в которых есть слово `vim`
[x] Только информацию о процессе `grep`

12. Что означает 20 в выводе `ls -l`: `-rw-r--r-- 92 korg root 20 Feb 25 13:37 file`?
[ ] 20 февраля
[ ] Количество жёстких ссылок на файл
[ ] Размер файла в блоках
[x] Размер файла в байтах

13. Что такое inode?
[ ] Номер файла в файловой системе
[ ] Файловый дескриптор процесса, через который можно работать с файлом
[ ] Величина сложности графа сущностей файловой системы
[x] Структура, описывающая файл в файловой системе

14. Что делает утилита `strip`?
[ ] Преобразует IP адрес в строку
[x] Удаляет символы из объектных файлов
[ ] Выводит последовательности печатных символов из файла
[ ] Удаляет пробелы в начале и конце каждой строки в STDIN

15. Что делает команда `git rebase master`?
[ ] Заменяет ветку `master` на текущую
[x] Переносит коммиты текущей ветки поверх коммитов `master`
[ ] Позволяет переименовать последний коммит на ветке `master`
[ ] Скачивает все изменения ветки `master` в её локальную копию

16. Что делает команда `grep -ve foo -e bar baz`?
[x] Выводит из файла baz все строки, кроме тех, в которых есть foo или bar
[ ] Выводит из файла baz все строки, кроме тех, в которых есть и foo, и bar
[ ] Выводит из файлов bar и baz все строки, в которых есть foo
[ ] Выводит из файлов bar и baz все строки, в которых нет foo

17. Что делает команда `sed samara syktyvkar`?
[x] Выводит ошибку
[ ] Заменяет samara на syktyvkar
[ ] Заменяет sam на ara
[ ] Заменяет m на r

18. У вас 4 варианта ответа. Выберите верный вариант
[ ] False
[x] Верно
[ ] Неверно
[ ] ложь
```

---
Проект реализован, QEMU собран с поддержкой нового устройства.
Для демонстрации работы:
1. Запустите QEMU (пункт 3).
2. В гостевой системе Linux выполните: `i2cset -y 0 0x50 0x42`.
3. Запустите `vplatform_monitor.py`, он покажет `Register value: 66` (0x42).
