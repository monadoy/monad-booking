#ifndef ESP_HTTP_CLIENT_H
#define ESP_HTTP_CLIENT_H

#include <vector>

#include "Arduino.h"
#include "esp_http_client.h"
#include "utils.h"

#define MAX_HTTP_RECV_BUFFER 2048  // 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

class ESPHTTPClient {
  public:
	~ESPHTTPClient();

	/** Start a new request. Reuses old client if possible. */
	void open(esp_http_client_method_t method, const char* url, const char* cert);

	void setHeader(const char* key, const char* value);

	/** Doesn't copy data, keep it in memory during the request */
	void setPostData(const char* data, int length);

	/** Run the request and print the results into Print stream. Used for large requests. */
	utils::Result<int> run(Print& output);

	/** Run the request and append the results into String. Used for small requests. */
	utils::Result<int> run(String& output);

	/** Closes the reusable connection. Must be called before sleeping. */
	void close();

	Print* _outputPrint = nullptr;
	String* _outputString = nullptr;

  private:
	utils::Result<int> _runInternal();

	esp_http_client* _client = nullptr;
	const char* _cert = nullptr;
};

#endif