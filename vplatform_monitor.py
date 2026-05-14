import socket
import json
import time
import sys

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
    
    print(f"Подключение к QMP через {qmp_path}...")
    while True:
        try:
            sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            sock.connect(qmp_path)
            break
        except ConnectionRefusedError:
            print("Ожидание запуска QEMU...")
            time.sleep(1)
            
    f = sock.makefile("r")
    
    # Чтение приветственного сообщения
    greeting = f.readline()
    # Согласование возможностей (Capabilities negotiation)
    qmp_execute(sock, "qmp_capabilities")
    f.readline() # Ответ OK

    print("Мониторинг регистра vplatform-i2c через QMP...")
    try:
        while True:
            # Читаем значение свойства 'reg' у объекта 'vdev0'
            res = qmp_execute(sock, "qom-get", {"path": "vdev0", "property": "reg"})
            if res and "return" in res:
                print(f"Значение регистра: {res['return']}")
            else:
                print(f"Ошибка чтения регистра: {res}")
            time.sleep(1)
    except KeyboardInterrupt:
        print("Мониторинг остановлен.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
