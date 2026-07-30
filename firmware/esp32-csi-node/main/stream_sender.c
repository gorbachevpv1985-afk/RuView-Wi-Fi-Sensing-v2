#include "stream_sender.h"

#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "sdkconfig.h"

static const char *TAG = "stream_sender";

static int s_sock = -1;
static struct sockaddr_in s_dest_addr;

/**
 * ENOMEM backoff state.
 * When sendto fails with ENOMEM (errno 12), we suppress further sends for
 * a cooldown period to let lwIP reclaim packet buffers.  Without this,
 * rapid-fire CSI callbacks can exhaust the pbuf pool and crash the device.
 */
static int64_t s_backoff_until_us = 0;       /* esp_timer timestamp to resume */
#define ENOMEM_COOLDOWN_MS  100              /* base backoff; doubles per streak */
#define ENOMEM_COOLDOWN_MAX_MS 2000          /* cap on the exponential backoff */
#define ENOMEM_LOG_INTERVAL 50               /* log every Nth suppressed send */
static uint32_t s_enomem_suppressed = 0;
/* Consecutive ENOMEM episodes without an intervening successful send. A fixed
 * 100 ms backoff is too short to drain sustained lwIP/WiFi buffer pressure
 * (#1135 bug #1: tier-2 + concurrent TX keeps the node stuck), so the backoff
 * grows 100→200→400→…→2000 ms per streak and resets on the first send that
 * succeeds. */
static uint32_t s_enomem_streak = 0;

static int sender_init_internal(const char *ip, uint16_t port)
{
    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_sock < 0) {
        return -1;
    }

    memset(&s_dest_addr, 0, sizeof(s_dest_addr));
    s_dest_addr.sin_family = AF_INET;
    s_dest_addr.sin_port = htons(port);

    // Универсальный разрешитель: поддерживает и IP-адреса, и mDNS/DNS имена (.local)
    struct addrinfo hints;
    struct addrinfo *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int err = getaddrinfo(ip, NULL, &hints, &res);
    if (err != 0 || res == NULL) {
        close(s_sock);
        s_sock = -1;
        return -1;
    }

    // Копируем успешно разрешенный IP-адрес в структуру назначения
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
    s_dest_addr.sin_addr = ipv4->sin_addr;
    freeaddrinfo(res);

    return 0;
}

int stream_sender_init(void)
{
    return sender_init_internal(CONFIG_CSI_TARGET_IP, CONFIG_CSI_TARGET_PORT);
}

int stream_sender_init_with(const char *ip, uint16_t port)
{
    return sender_init_internal(ip, port);
}

int stream_sender_send(const uint8_t *data, size_t len)
{
    if (s_sock < 0) {
        return -1;
    }

    int64_t now = esp_timer_get_time();
    if (now < s_backoff_until_us) {
        s_enomem_suppressed++;
        return -2;
    }

    int err = sendto(s_sock, data, len, 0, (struct sockaddr *)&s_dest_addr, sizeof(s_dest_addr));
    if (err < 0) {
        if (errno == 12) { // ENOMEM
            uint32_t cooldown_ms = ENOMEM_COOLDOWN_MS << s_enomem_streak;
            if (cooldown_ms > ENOMEM_COOLDOWN_MAX_MS) {
                cooldown_ms = ENOMEM_COOLDOWN_MAX_MS;
            }
            s_backoff_until_us = now + (int64_t)cooldown_ms * 1000;
            s_enomem_streak++;
        }
        return -1;
    }

    s_enomem_streak = 0;
    return err;
}

// Восстанавливаем недостающую приоритетную функцию отправки
int stream_sender_send_priority(const uint8_t *data, size_t len)
{
    // Приоритетная отправка вызывает базовую функцию
    return stream_sender_send(data, len);
}
