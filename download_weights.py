import os
import socket
import ssl

IP = "185.199.108.133"
PORT = 443  # Переходим на защищенный HTTPS-порт

models = {
    "count": {
        "path": "/ruview/wifi-sensing/main/v2/crates/cog-person-count/cog/artifacts/count_v1.onnx.data",
        "dest": r"C:\esp-projects\v2\crates\cog-person-count\cog\artifacts\count_v1.onnx.data"
    },
    "pose": {
        "path": "/ruview/wifi-sensing/main/v2/crates/cog-pose-estimation/cog/artifacts/pose_v1.onnx.data",
        "dest": r"C:\esp-projects\v2\crates\cog-pose-estimation\cog\artifacts\pose_v1.onnx.data"
    }
}

# Отключаем валидацию сертификатов для обхода привязки к IP
ssl_context = ssl._create_unverified_context()

for name, info in models.items():
    os.makedirs(os.path.dirname(info["dest"]), exist_ok=True)
    print(f"Скачивание {name} через защищенный сокет SSL...")
    
    try:
        # Создаем базовый TCP сокет
        raw_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        raw_socket.settimeout(20)
        
        # Оборачиваем его в SSL контекст БЕЗ проверки hostname
        s = ssl_context.wrap_socket(raw_socket, server_hostname=None)
        s.connect((IP, PORT))
        
        # Формируем HTTPS запрос
        request = (
            f"GET {info['path']} HTTP/1.1\r\n"
            f"Host: ://githubusercontent.com\r\n"
            f"User-Agent: Mozilla/5.0\r\n"
            f"Connection: close\r\n\r\n"
        )
        s.sendall(request.encode('utf-8'))
        
        # Собираем бинарный ответ чанками
        response = b""
        while True:
            chunk = s.recv(16384)
            if not chunk:
                break
            response += chunk
        s.close()
        
        # Разделяем заголовки и бинарное тело
        header_end = response.find(b"\r\n\r\n")
        if header_end != -1:
            headers = response[:header_end].decode('utf-8', errors='ignore')
            body = response[header_end+4:]
            
            if "200 OK" in headers:
                with open(info["dest"], "wb") as f:
                    f.write(body)
                print(f"✅ {name} успешно скачан ({len(body)} байт)!")
            else:
                print(f"❌ Сервер вернул ошибку:\n{headers[:120]}")
        else:
            print("❌ Неверная структура ответа")
            
    except Exception as e:
        print(f"❌ Ошибка сокета: {e}")
