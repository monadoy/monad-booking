#ifndef ESP_HTTP_CLIENT_H
#define ESP_HTTP_CLIENT_H

#include <vector>

#include "Arduino.h"
#include "esp_http_client.h"
#include "utils.h"

#define MAX_HTTP_RECV_BUFFER 2048  // 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

/**
 * A reusable wrapper for esp_http_client.
 * open() should be called every time before a request is made.
 * The connection closes when the class destructor is called.
 * The connection should be closed manually before the device goes to sleep.
 */
class ESPHTTPClient {
  public:
	~ESPHTTPClient();

	/**
	 * Opens a client.
	 * Reuses old client if cert is the same and the client is not already closed.
	 * */
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