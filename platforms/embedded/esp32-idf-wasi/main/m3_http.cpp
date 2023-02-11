
#include "m3_http.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include <m3_env.h>

#define TAG "http"

esp_http_client_handle_t client;
char buff[1024];
char *ptr = buff;

esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
    // printf("%.*s", evt->data_len, (char *)evt->data);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (!esp_http_client_is_chunked_response(evt->client))
    {
      // printf("%.*s", evt->data_len, (char *)evt->data);
      strncpy(ptr, (const char*)evt->data, evt->data_len);
      ptr += evt->data_len;
    }

    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
    *ptr = 0;
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  }
  return ESP_OK;
}

void http_call(const char* url, uint16_t *status, esp_http_client_handle_t *handle)
{
  esp_http_client_config_t config = {
      .url = url,
      .event_handler = _http_event_handle,
      .crt_bundle_attach = esp_crt_bundle_attach,
  };
  client = esp_http_client_init(&config);
  *handle = client;
  esp_err_t err = esp_http_client_perform(client);

  if (err == ESP_OK)
  {
    *status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "Status = %d, content_length = %d",
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
  }
}

/*
 * Note: each RawFunction should complete with one of these calls:
 *   m3ApiReturn(val)   - Returns a value
 *   m3ApiSuccess()     - Returns void (and no traps)
 *   m3ApiTrap(trap)    - Returns a trap
 */

m3ApiRawFunction(m3_request)
{
  m3ApiReturnType(uint32_t);

  m3ApiGetArgMem(char *, url)
  m3ApiGetArgMem(uint32_t, url_len)
  m3ApiGetArgMem(char *, method)
  m3ApiGetArgMem(uint32_t, method_len)
  m3ApiGetArgMem(char *, headers)
  m3ApiGetArgMem(uint32_t, headers_len)
  m3ApiGetArgMem(char *, body)
  m3ApiGetArgMem(uint32_t, body_len)
  m3ApiGetArgMem(uint16_t *, statusCode)
  m3ApiGetArgMem(uint32_t *, handle)

  http_call(url, statusCode, (esp_http_client_handle_t*)handle);

  m3ApiReturn(0);
}

m3ApiRawFunction(m3_body_read)
{
  m3ApiReturnType(uint32_t);
  m3ApiGetArgMem(uint32_t *, handle);
  m3ApiGetArgMem(uint32_t *, buffer);
  m3ApiGetArgMem(uint32_t, buffer_len);
  m3ApiGetArgMem(uint32_t *, written);

  uint32_t content_len = esp_http_client_get_content_length(client);
  strncpy((char*)buffer, buff, content_len);
  *written = content_len;

  m3ApiReturn(0);
}

m3ApiRawFunction(m3_close)
{
  m3ApiReturnType(uint32_t);
  m3ApiGetArgMem(uint32_t *, handle)

  esp_http_client_cleanup(client);

  m3ApiReturn(0);
}

M3Result m3_LinkHttp(IM3Module module)
{
  const char *http = "wasi_experimental_http";

  m3_LinkRawFunction(module, http, "req", "i(*i*i*i*iii)", &m3_request);
  m3_LinkRawFunction(module, http, "body_read", "i(**i*)", &m3_body_read);
  m3_LinkRawFunction(module, http, "req", "i(*)", &m3_close);

  return m3Err_none;
}