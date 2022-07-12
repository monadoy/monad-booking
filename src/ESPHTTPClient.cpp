#include "ESPHTTPClient.h"

#include "cert.h"
#include "esp_tls.h"

const char* USER_AGENT = "ESPClient";
const int BUFFER_SIZE = 1024;
const int BUFFER_SIZE_TX = 1024;
const int TIMEOUT_MS = 5000;

esp_err_t _http_event_handler(esp_http_client_event_t* evt) {
	switch (evt->event_id) {
		case HTTP_EVENT_ERROR:
			log_d("HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			log_d("HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			log_d("HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			log_d("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA: {
			log_d("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);

			ESPHTTPClient* myClient = static_cast<ESPHTTPClient*>(evt->user_data);
			if (myClient->_outputPrint) {
				myClient->_outputPrint->write((char*)evt->data, evt->data_len);
			} else if (myClient->_outputString) {
				*myClient->_outputString += String((char*)evt->data, evt->data_len);
			}

			break;
		}
		case HTTP_EVENT_ON_FINISH: {
			log_d("HTTP_EVENT_ON_FINISH");
			break;
		}
		case HTTP_EVENT_DISCONNECTED:
			log_d("HTTP_EVENT_DISCONNECTED");
			int mbedtls_err = 0;
			esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data,
			                                                 &mbedtls_err, NULL);
			if (err != 0) {
				log_e("Last esp error code: 0x%x", err);
				log_e("Last mbedtls failure: 0x%x", mbedtls_err);
			}
			break;
	}
	return ESP_OK;
}

void ESPHTTPClient::open(esp_http_client_method_t method, const char* url, const char* cert) {
	if (!_client || _cert != cert) {
		close();
		log_i("Creating new HTTP client");
		esp_http_client_config_t cfg = {
		    .url = url,
		    .cert_pem = GOOGLE_API_FULL_CHAIN_CERT,
		    .user_agent = USER_AGENT,
		    .method = method,
		    .timeout_ms = TIMEOUT_MS,
		    .event_handler = _http_event_handler,
		    .buffer_size = BUFFER_SIZE,
		    .buffer_size_tx = BUFFER_SIZE_TX,
		    .user_data = this,
		};

		_client = esp_http_client_init(&cfg);
		_cert = cert;
		assert(_client != nullptr);
		return;
	}

	log_i("Reusing old client");
	esp_http_client_set_method(_client, method);
	esp_http_client_set_post_field(_client, nullptr, 0);
	esp_http_client_set_url(_client, url);
}

ESPHTTPClient::~ESPHTTPClient() { close(); }

void ESPHTTPClient::close() {
	if (_client)
		esp_http_client_cleanup(_client);

	_client = nullptr;
}

void ESPHTTPClient::setHeader(const char* key, const char* value) {
	esp_http_client_set_header(_client, key, value);
}

void ESPHTTPClient::setPostData(const char* data, int length) {
	esp_http_client_set_post_field(_client, data, length);
}

/** Run and print the results into Print stream. Used for large requests. */
utils::Result<int> ESPHTTPClient::run(Print& output) {
	_outputPrint = &output;
	_outputString = nullptr;
	return _runInternal();
}

/** Run and print the results into String. Used for small requests. */
utils::Result<int> ESPHTTPClient::run(String& output) {
	_outputPrint = nullptr;
	_outputString = &output;
	return _runInternal();
}

utils::Result<int> ESPHTTPClient::_runInternal() {
	esp_err_t err = esp_http_client_perform(_client);
	if (err != ESP_OK) {
		close();

		const char* errName = "";
		if (err == ESP_ERR_HTTP_CONNECT) {
			// This happens pretty often, add a more user friendly error message
			errName = "connection refused";
		} else {
			errName = esp_err_to_name(err);
		}

		log_e("HTTP error: %s", errName);
		return utils::Result<int>::makeErr(new utils::Error{"HTTP: " + String(errName)});
	}

	int status = esp_http_client_get_status_code(_client);
	log_i("HTTP status = %d", status);
	return utils::Result<int>::makeOk(new int{status});
}